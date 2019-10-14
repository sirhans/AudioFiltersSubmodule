//
//  BMStaticDelay.c
//  BMAudioFilters
//
//  Created by Blue Mangoo on 5/7/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#include <Accelerate/Accelerate.h>
#include "BMStaticDelay.h"
#include <stdlib.h>

//#ifdef __cplusplus
//extern "C" {
//#endif




#define BMSD_BUFFER_PADDING 256

static inline int min(int a, int b) {
    return a > b ? b : a;
}

float rt60FeedbackGain(float delayTime, float rt60DecayTime){
    return powf(10.0f, (-3.0f * delayTime) / rt60DecayTime );
}


// returns true if there is space available to write into the
// circular buffers
bool hasWriteBytesAvailable(BMStaticDelay* This){
    uint32_t bytesAvailableL, bytesAvailableR;
    TPCircularBufferHead(&This->bufferL, &bytesAvailableL);
    TPCircularBufferHead(&This->bufferR, &bytesAvailableR);
    return (bytesAvailableL > 0 && bytesAvailableR > 0);
}


void BMStaticDelay_init(BMStaticDelay* This,
                        size_t numChannelsIn, size_t numChannelsOut,
                        float delayTime,
                        float decayTime,
                        float wetGainDb,
                        float crossMixAmount,
                        float lowpassFC,
                        float highpassFC,
                        float sampleRate){
    // You must call BMStaticDelay_destroy before re-initialising
    assert(This->initComplete != BMSD_INIT_COMPLETED);
    
    // set the sample rate (with a validity check)
    assert(sampleRate > 0);
    This->sampleRate = sampleRate;
    
    // do not support more than two channel stereo
    assert(numChannelsOut <= 2);
    This->numChannelsOut = numChannelsOut;
    
    This->delayTimeNeedsUpdate = false;
    
    // this delay does not support mixdown (but can spread mono to stereo)
    assert(numChannelsIn <= numChannelsOut);
    This->numChannelsIn = numChannelsIn;
    
    This->delayTimeInSeconds = delayTime;
    
    // get the length of the buffer in samples
    This->delayTimeSamples = (int)(round(delayTime*sampleRate));
    
    // the delay time must be non-zero
    assert(This->delayTimeSamples > 0);
    
    // allocate a buffer for the left side
    bool initSuccessful = TPCircularBufferInit(&This->bufferL,
                                               (This->delayTimeSamples+BMSD_BUFFER_PADDING)*sizeof(float));
    assert(initSuccessful);
    
    // since we are not accessing the buffer in a multi-threaded context
    // we disable atomicity to improve performance
    TPCircularBufferSetAtomic(&This->bufferL, false);
    
    // repeat the last three steps for the right channel if stereo out
    if(numChannelsOut==2){
        initSuccessful = TPCircularBufferInit(&This->bufferR,
                                              (This->delayTimeSamples+BMSD_BUFFER_PADDING)*sizeof(float));
        assert(initSuccessful);
        TPCircularBufferSetAtomic(&This->bufferR, false);
    }
    
    /*
     * write zeros into the buffer to put the write pointer ahead of the read pointer
     */
    
    // get a pointer to the buffer head and ensure that there is sufficient space available to write there
    uint32_t bytesAvailableForWrite;
    int32_t bytesWriting = This->delayTimeSamples*sizeof(float);
    float* head = TPCircularBufferHead(&This->bufferL, &bytesAvailableForWrite);
    assert(bytesAvailableForWrite > bytesWriting);
    
    // write zeros
    memset(head, 0, bytesWriting);
    
    // mark the space as written
    TPCircularBufferProduce(&This->bufferL, bytesWriting);
    
    
    // do the right channel if stereo
    if(This->numChannelsOut == 2){
        head = TPCircularBufferHead(&This->bufferR, &bytesAvailableForWrite);
        assert(bytesAvailableForWrite > bytesWriting);
        // write zeros
        memset(head, 0, bytesWriting);
        
        // mark the space as written
        TPCircularBufferProduce(&This->bufferR, bytesWriting);
    }
    
    
    /*
     * configure the feedback matrix
     */
    
    // don't allow stereo cross-mix for mono output
    if(numChannelsOut == 1)
        assert(crossMixAmount = 0.0f);
    
    // set cross mix for stereo output
    BMStaticDelay_setFeedbackCrossMix(This, crossMixAmount);
    
    // set RT60 decay time
    BMStaticDelay_setRT60DecayTime(This, decayTime);
    
    // set dry and wet gain
    BMSmoothGain_init(&This->wetGain, sampleRate);
    BMSmoothGain_init(&This->dryGain, sampleRate);
    BMSmoothGain_setGainDb(&This->wetGain, wetGainDb);
    BMSmoothGain_setGainDb(&This->dryGain, 0.0f);
    /*
     * allocate memory for the feedback buffers
     */
    This->feedbackBufferL = malloc(sizeof(float)*This->delayTimeSamples);
    This->tempBufferL = malloc(sizeof(float)*This->delayTimeSamples);
    memset(This->feedbackBufferL,0,sizeof(float)*This->delayTimeSamples);
    if(numChannelsOut == 2){
        This->feedbackBufferR = malloc(sizeof(float)*This->delayTimeSamples);
        This->LFOBufferL = malloc(sizeof(float)*This->delayTimeSamples);
        This->LFOBufferR = malloc(sizeof(float)*This->delayTimeSamples);
        memset(This->feedbackBufferR,0,sizeof(float)*This->delayTimeSamples);
        BMQuadratureOscillator_init(&This->qosc, 0.5, sampleRate);
        if(numChannelsIn == 2){
            This->tempBufferR = malloc(sizeof(float)*This->delayTimeSamples);
        }
    }
    
    /*
     * set up the lowpass filter to tone down the clarity of the delay signal
     */
    BMMultiLevelBiquad_init(&This->filter,
                            Filter_Count,
                            sampleRate,
                            true,
                            false,
                            false);
    BMStaticDelay_setLowpassFc(This, lowpassFC);
    BMStaticDelay_setHighpassFc(This, highpassFC);
    
    
    /*
     * set a code to indicate that the input has completed
     */
    This->initComplete = BMSD_INIT_COMPLETED;
}



void BMStaticDelay_setDelayTime(BMStaticDelay* This, float timeInSeconds){
    if(This->delayTimeInSeconds!=timeInSeconds){
        This->delayTimeInSeconds = timeInSeconds;
        This->delayTimeNeedsUpdate = true;
    }
}


void BMStaticDelay_setFeedbackMatrix(BMStaticDelay* This){
    
    // set the matrix according to the crossMix angle
    This->feedbackMatrix = BM2x2Matrix_rotationMatrix(This->crossMixAngle);
    
    // scale the matrix to give the desired rt60 decay time
    This->feedbackMatrix = simd_mul(This->feedbackGain, This->feedbackMatrix);
}






void BMStaticDelay_setRT60DecayTime(BMStaticDelay* This, float rt60DecayTime){
    This->decayTime = rt60DecayTime;
    This->feedbackGain = rt60FeedbackGain(This->delayTimeInSeconds, rt60DecayTime);
    BMStaticDelay_setFeedbackMatrix(This);
}





/*
 * @param amount in [0,1]. 1 is full crossover
 */
void BMStaticDelay_setFeedbackCrossMix(BMStaticDelay* This, float amount){
    This->crossMixAmount = amount;
    This->crossMixAngle = amount * M_PI_2;
    BMStaticDelay_setFeedbackMatrix(This);
    
    if(amount>0.01){
        This->lfoFreq = 0.1;//amount/(This->delayTimeInSeconds*4.);
    }else{
        This->lfoFreq = 0;
    }
    This->updateLFO = true;
    //        printf("lfo freq %f %f %f\n",This->lfoFreq,amount,This->delayTimeInSeconds);
}


void BMStaticDelay_updateLFO(BMStaticDelay* This){
    if(This->updateLFO){
        This->updateLFO = false;
        BMQuadratureOscillator_setFrequency(&This->qosc, This->lfoFreq);
    }
}



void BMStaticDelay_setWetGain(BMStaticDelay* This, float gainDb){
    BMSmoothGain_setGainDb(&This->wetGain, gainDb);
}


void BMStaticDelay_setDryGain(BMStaticDelay* This, float gainDb){
    BMSmoothGain_setGainDb(&This->dryGain, gainDb);
}



void BMStaticDelay_setLowpassFc(BMStaticDelay* This, float fc){
    assert(fc > 0.0f);
    This->lowpassFC = fc;
    BMMultiLevelBiquad_setLowPass12db(&This->filter, fc, Filter_LP_Level);
}

void BMStaticDelay_setHighpassFc(BMStaticDelay* This, float fc){
    assert(fc > 0.0f);
    This->highpassFC = fc;
    BMMultiLevelBiquad_setHighPass12db(&This->filter, fc, Filter_HP_Level);
}

void BMStaticDelay_setBypass(BMStaticDelay* This, size_t level){
    BMMultiLevelBiquad_setBypass(&This->filter, level);
}


void BMStaticDelay_destroy(BMStaticDelay* This){
    // mark the delay uninitialised to prevent usage after deletion
    This->initComplete = 0;
    
    // free resources used by all configurations
    TPCircularBufferCleanup(&This->bufferL);
    free(This->feedbackBufferL);
    free(This->tempBufferL);
    
    // free resources used by stereo out configurations
    if(This->numChannelsOut == 2){
        TPCircularBufferCleanup(&This->bufferR);
        free(This->feedbackBufferR);
        free(This->LFOBufferL);
        free(This->LFOBufferR);
        
        if(This->numChannelsIn == 2)
            free(This->tempBufferR);
    }
    
    // free the filter resources
    BMMultiLevelBiquad_destroy(&This->filter);
}






/*
 * Processing function used once for mono and twice for stereo
 */
static inline void processOneChannel(TPCircularBuffer* buffer, const void* input, void* output, int numFrames){
    
    int bytesToProcess = numFrames * sizeof(float);
    
    while(bytesToProcess > 0){
        /*
         * Write to the circular buffer
         */
        
        // get a pointer to the head of the buffer
        // and how find out many bytes are available to write
        uint32_t bytesAvailableForWrite;
        void* head = TPCircularBufferHead(buffer, &bytesAvailableForWrite);
        
        // how many bytes are we processing This time through the loop?
        SInt32 bytesProcessing = min(bytesToProcess,bytesAvailableForWrite);
        
        // copy from input to the buffer head
        memcpy(head, input, bytesProcessing);
        
        // mark the written region of the buffer as written
        TPCircularBufferProduce(buffer, bytesProcessing);
        
        // advance the input pointer
        input += bytesProcessing;
        
        
        /*
         * read from the circular buffer
         */
        
        // get a pointer to the tail of the buffer and
        // find out how many bytes are available for reading
        uint32_t bytesAvailableForRead;
        void* tail = TPCircularBufferTail(buffer, &bytesAvailableForRead);
        
        // if the bytes available for reading is less than what we just wrote,
        // something has gone wrong
        assert(bytesAvailableForRead >= bytesProcessing);
        
        // copy from buffer tail to output
        memcpy(output, tail, bytesProcessing);
        
        // mark the read bytes as used
        TPCircularBufferConsume(buffer, bytesProcessing);
        
        // advance the output pointer
        output += bytesProcessing;
        
        // how many bytes still left to process?
        bytesToProcess -= bytesProcessing;
    }
}





void static inline stereoBufferHelper(BMStaticDelay* This,
                                      const float* inL, const float* inR,
                                      float* outL, float* outR,
                                      int framesProcessing){
    
    // We will use the feedback buffer as a cache for mixing. We
    // rename the feedback buffer to make the code easier to read.
    float* mixBufferL = This->feedbackBufferL;
    float* mixBufferR = This->feedbackBufferR;
    
    // mix the feedback buffer with the input. Cache to the mix buffer.
    vDSP_vadd(inL, 1,
              This->feedbackBufferL, 1,
              mixBufferL, 1,
              framesProcessing);
    vDSP_vadd(inR, 1,
              This->feedbackBufferR, 1,
              mixBufferR, 1,
              framesProcessing);
    
    // process the delay to the outputs
    uint32_t available;
    TPCircularBufferHead(&This->bufferL, &available);
    TPCircularBufferHead(&This->bufferR, &available);
    
    processOneChannel(&This->bufferL,
                      mixBufferL,
                      outL,
                      framesProcessing);
    
    processOneChannel(&This->bufferR,
                      mixBufferR,
                      outR,
                      framesProcessing);
    
    /*
     * mix the feedback to the feedback buffer
     */
    // straight mix (L=>L and R=>R)
	float m00 = This->feedbackMatrix.columns[0][0];
	float m11 = This->feedbackMatrix.columns[1][1];
    vDSP_vsmul(outL, 1, &m00,
               This->feedbackBufferL, 1,
               framesProcessing);
    vDSP_vsmul(outR, 1, &m11,
               This->feedbackBufferR, 1,
               framesProcessing);
    
    // cross mix (L=>R and R=>L)
	float m01 = This->feedbackMatrix.columns[1][0];
	float m10 = This->feedbackMatrix.columns[0][1];
    vDSP_vsma(outL, 1, &m01,
              This->feedbackBufferR, 1,
              This->feedbackBufferR, 1,
              framesProcessing);
    vDSP_vsma(outR, 1, &m10,
              This->feedbackBufferL, 1,
              This->feedbackBufferL, 1,
              framesProcessing);
}






void BMStaticDelay_processBufferStereo(BMStaticDelay* This,
                                       const float* inL, const float* inR,
                                       float* outL, float* outR,
                                       int numFrames){
    // confirm that the delay is initialised
    assert(This->initComplete == BMSD_INIT_COMPLETED);
    
    // confirm that the delay is configured correctly
    assert(This->numChannelsIn == 2 && This->numChannelsOut == 2);
    
    // don't process anything if there are nan values in the input
    if (isnan(inL[0]) || isnan(inR[0])) {
        memset(outL, 0, sizeof(float)*numFrames);
        memset(outR, 0, sizeof(float)*numFrames);
        return;
    }
    
    BMStaticDelay_updateLFO(This);
    
    while(numFrames > 0){
        
        // what is the largest chunk we can process at one time?
        int framesProcessing = min(numFrames, This->delayTimeSamples);
        
        // mix both channels to one
        vDSP_vadd(inL, 1, inR, 1, This->tempBufferL, 1, framesProcessing);
        
        // scale down so that the energy per channel after mixing is equal
        float scale = M_SQRT1_2;
        vDSP_vsmul(This->tempBufferL, 1, &scale, This->tempBufferL, 1, framesProcessing);
        
        /*
         * multiplex the mono input to stereo, using a quadrature LFO
         * to rotate between the two channels.
         */
        // generate the quadrature LFO output into the temp buffers
        BMQuadratureOscillator_process(&This->qosc,
									   This->LFOBufferL,
									   This->LFOBufferR,
									   framesProcessing);
        
        // multiplex the mixed input to the left channel buffer
        vDSP_vmul(This->LFOBufferL, 1, This->tempBufferL,1,
                  This->LFOBufferL, 1,
                  framesProcessing);
        // multiplex the mixed input to the right channel buffer
        vDSP_vmul(This->LFOBufferR, 1, This->tempBufferL,1,
                  This->LFOBufferR, 1,
                  framesProcessing);
        
        // process the delay
        stereoBufferHelper(This,
                           This->LFOBufferL, This->LFOBufferR,
                           This->tempBufferL, This->tempBufferR,
                           framesProcessing);
        
        // filter the output
        BMMultiLevelBiquad_processBufferStereo(&This->filter,
                                               This->tempBufferL, This->tempBufferR,
                                               This->tempBufferL, This->tempBufferR,
                                               framesProcessing);
        
        // process gain of wet signal
        BMSmoothGain_processBuffer(&This->wetGain,
                                   This->tempBufferL, This->tempBufferR,
                                   This->tempBufferL, This->tempBufferR,
                                   framesProcessing);
        
        // process gain of dry signal, buffering into the output
        BMSmoothGain_processBuffer(&This->dryGain,
                                   inL, inR,
                                   outL, outR,
                                   framesProcessing);
        
        // mix wet and dry signals
        vDSP_vadd(This->tempBufferL,1,outL,1,outL,1,framesProcessing);
        vDSP_vadd(This->tempBufferR,1,outR,1,outR,1,framesProcessing);
        
        
        // advance pointers
        inL += framesProcessing;
        inR += framesProcessing;
        outL += framesProcessing;
        outR += framesProcessing;
        
        numFrames -= framesProcessing;
    }
    
    if(This->delayTimeNeedsUpdate){
        
        BMStaticDelay_destroy(This);
        
        BMStaticDelay_init(This,
                           2, 2,
                           This->delayTimeInSeconds,
                           This->decayTime,
                           BMSmoothGain_getGainDb(&This->wetGain),
                           This->crossMixAmount,
                           This->lowpassFC,
                           This->highpassFC,
                           This->sampleRate);
    }
}





//#ifdef __cplusplus
//}
//#endif
