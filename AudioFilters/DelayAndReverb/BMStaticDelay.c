//
//  BMStaticDelay.c
//  BMAudioFilters
//
//  Created by Blue Mangoo on 5/7/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif
    
#include <Accelerate/Accelerate.h>
#include "BMStaticDelay.h"
#include <stdlib.h>
    
#define BMSD_INIT_COMPLETED 9913156
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
        int bytesAvailableL, bytesAvailableR;
        TPCircularBufferHead(&This->bufferL, &bytesAvailableL);
        TPCircularBufferHead(&This->bufferR, &bytesAvailableR);
        return (bytesAvailableL > 0 && bytesAvailableR > 0);
    }
    
    
    void BMStaticDelay_init(BMStaticDelay* This,
                            size_t numChannelsIn, size_t numChannelsOut,
                            float delayTime,
                            float decayTime,
                            float wetAmount,
                            float crossMixAmount,
                            float lowpassFC,
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
        int32_t bytesAvailableForWrite;
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
        
        // set wet mix
        BMStaticDelay_setWetMix(This, wetAmount);
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
                                1,
                                sampleRate,
                                numChannelsOut == 2,
                                false,false);
        BMStaticDelay_setLowpassFc(This, lowpassFC);
        
        
        /*
         * set a code to indicate that the input has completed
         */
        This->initComplete = BMSD_INIT_COMPLETED;
    }
    
    
    
    void BMStaticDelay_setDelayTime(BMStaticDelay* This, float timeInSeconds){
        This->delayTimeInSeconds = timeInSeconds;
        This->delayTimeNeedsUpdate = true;
    }
    
    
    void BMStaticDelay_setFeedbackMatrix(BMStaticDelay* This){
        
        // set the matrix according to the crossMix angle
        This->feedbackMatrix = BM2x2Matrix_rotationMatrix(This->crossMixAngle);
        
        // scale the matrix to give the desired rt60 decay time
        This->feedbackMatrix = BM2x2Matrix_smmul(This->feedbackGain, This->feedbackMatrix);
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
    }
    
    
    
    
    
    
    void BMStaticDelay_setWetMix(BMStaticDelay* This, float amount){
        assert(amount >= 0.0f && amount <= 1.0f);
        This->wetMixAmount = amount;
        
        // when amount is 1.0 we have equal amounts of dry and wet.
        float mixAngle = M_PI_2 * 0.5 * amount;
        This->wetGain = sinf(mixAngle);
        This->dryGain = cosf(mixAngle);
    }
    
    
    
    
    
    void BMStaticDelay_setLowpassFc(BMStaticDelay* This, float fc){
        assert(fc > 0.0f);
        This->lowpassFC = fc;
        BMMultiLevelBiquad_setLowPass12db(&This->filter, fc, 0);
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
    static inline void processOneChannel(TPCircularBuffer* buffer, void* input, void* output, int numFrames){
        
        int bytesToProcess = numFrames * sizeof(float);
        
        while(bytesToProcess > 0){
            /*
             * Write to the circular buffer
             */
            
            // get a pointer to the head of the buffer
            // and how find out many bytes are available to write
            SInt32 bytesAvailableForWrite;
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
            SInt32 bytesAvailableForRead;
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
    
    
    
    
    
    void BMStaticDelay_processBufferMono(BMStaticDelay* This,
                                         float* input,
                                         float* output,
                                         int numFrames){
        // confirm that the delay is initialised
        assert(This->initComplete == BMSD_INIT_COMPLETED);
        
        // confirm that the delay is configured correctly
        assert(This->numChannelsIn == 1 && This->numChannelsOut == 1);
        
        // don't process anything if there are nan values in the input
        if (isnan(input[0])) {
            memset(output, 0, sizeof(float)*numFrames);
            return;
        }
        
        while(numFrames > 0){
            // what is the largest chunk we can process at one time?
            int framesProcessing = min(numFrames, This->delayTimeSamples);
            
            // if we are running in place, save a copy of the input
            if(input == output)
                memcpy(This->tempBufferL,
                       input,
                       sizeof(float)*framesProcessing);
            
            // We will use the feedback buffer as a cache for mixing. We
            // rename the feedback buffer to make the code easier to read.
            float* mixBuffer = This->feedbackBufferL;
            
            // mix the feedback buffer with the input, save to feedbackBufferL
            vDSP_vadd(input, 1, This->feedbackBufferL, 1,
                      mixBuffer, 1, framesProcessing);
            
            // process the delay
            processOneChannel(&This->bufferL, mixBuffer,
                              output, framesProcessing);
            
            // attenuate the feedback gain using the a coefficient in the matrix
            // and save to feedbackBufferL
            vDSP_vsmul(output, 1,
                       &This->feedbackMatrix.m11,
                       This->feedbackBufferL, 1, framesProcessing);
            
            // process the filter
            BMMultiLevelBiquad_processBufferMono(&This->filter,
                                                 output, output,
                                                 framesProcessing);
            
            
            // if we are running in place, take dry mix from the copy we
            // saved earlier
            float *dry;
            if (input == output)
                dry = This->tempBufferL;
            // otherwise take dry signal from the input
            else
                dry = input;
            
            
            
            // mix dry and wet signals
            vDSP_vsmsma(output, 1, &This->wetGain,
                        dry, 1, &This->dryGain,
                        output, 1, framesProcessing);
            
            // advance pointers
            input += framesProcessing;
            output += framesProcessing;
            
            numFrames -= framesProcessing;
        }
        
        if(This->delayTimeNeedsUpdate){
            BMStaticDelay_destroy(This);
            BMStaticDelay_init(This,
                               1, 1,
                               This->delayTimeInSeconds,
                               This->decayTime,
                               This->wetMixAmount,
                               This->crossMixAmount,
                               This->lowpassFC,
                               This->sampleRate);
        }
    }
    
    void static inline stereoBufferHelper(BMStaticDelay* This,
                                          float* inL, float* inR,
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
        SInt32 available;
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
        vDSP_vsmul(outL, 1, &This->feedbackMatrix.m11,
                   This->feedbackBufferL, 1,
                   framesProcessing);
        vDSP_vsmul(outR, 1, &This->feedbackMatrix.m22,
                   This->feedbackBufferR, 1,
                   framesProcessing);
        
        // cross mix (L=>R and R=>L)
        vDSP_vsma(outL, 1, &This->feedbackMatrix.m12,
                  This->feedbackBufferR, 1,
                  This->feedbackBufferR, 1,
                  framesProcessing);
        vDSP_vsma(outR, 1, &This->feedbackMatrix.m21,
                  This->feedbackBufferL, 1,
                  This->feedbackBufferL, 1,
                  framesProcessing);
        
    }
    
    
    
    
    void BMStaticDelay_processBufferStereo(BMStaticDelay* This,
                                           float* inL, float* inR,
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
        
        while(numFrames > 0){
            
            // what is the largest chunk we can process at one time?
            int framesProcessing = min(numFrames, This->delayTimeSamples);
            
            // if we are running in place, save a copy of the input
            if(inL == outL || inR == outR || inL == outR || inR == outL){
                memcpy(This->tempBufferL, inL, sizeof(float)*framesProcessing);
                memcpy(This->tempBufferR, inR, sizeof(float)*framesProcessing);
            }
            
            // mix both channels to the left side
            vDSP_vadd(inL, 1, inR, 1, inL, 1, framesProcessing);
            
            // scale down so that the energy per channel after mixing is equal
            float scale = M_SQRT1_2;
            vDSP_vsmul(inL, 1, &scale, inL, 1, framesProcessing);
            
            /*
             * multiplex the mono input to stereo, using a quadrature LFO
             * to rotate between the two channels.
             */
            // generate the quadrature LFO output into the temp buffers
            BMQuadratureOscillator_processQuad(&This->qosc,
                                               This->LFOBufferL,
                                               This->LFOBufferR,
                                               framesProcessing);
            
            // multiplex the mixed input to the left channel buffer
            vDSP_vmul(This->LFOBufferL, 1, inL,1,
                      This->LFOBufferL, 1,
                      framesProcessing);
            // multiplex the mixed input to the right channel buffer
            vDSP_vmul(This->LFOBufferR, 1, inL,1,
                      This->LFOBufferR, 1,
                      framesProcessing);
            
            // process the delay
            stereoBufferHelper(This,
                               This->LFOBufferL, This->LFOBufferR,
                               outL, outR,
                               framesProcessing);
            
            // filter the output
            BMMultiLevelBiquad_processBufferStereo(&This->filter,
                                                   outL, outR, outL, outR,
                                                   framesProcessing);
            
            // if we are running in place, take dry mix from the copy we
            // saved earlier
            float *dryL, *dryR;
            if(inL == outL || inR == outR || inL == outR || inR == outL){
                dryL = This->tempBufferL;
                dryR = This->tempBufferR;
            }
            // otherwise take dry signal from the input
            else {
                dryL = inL;
                dryR = inR;
            }
            
            // mix dry and wet signals
            vDSP_vsmsma(outL, 1, &This->wetGain,
                        dryL, 1, &This->dryGain,
                        outL, 1, framesProcessing);
            vDSP_vsmsma(outR, 1, &This->wetGain,
                        dryR, 1, &This->dryGain,
                        outR, 1, framesProcessing);
            
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
                               This->wetMixAmount,
                               This->crossMixAmount,
                               This->lowpassFC,
                               This->sampleRate);
        }
    }
    
    
    
    
    
    
    void BMStaticDelay_processBufferMonoToStereo(BMStaticDelay* This, float* input, float* outL, float* outR, int numFrames){
        // confirm that the delay is initialised
        assert(This->initComplete == BMSD_INIT_COMPLETED);
        
        
        // confirm that the delay is configured correctly
        assert(This->numChannelsIn == 1 && This->numChannelsOut == 2);
        
        
        // don't process anything if there are nan values in the input
        if (isnan(input[0])) {
            memset(outL, 0, sizeof(float)*numFrames);
            memset(outR, 0, sizeof(float)*numFrames);
            return;
        }
        
        
        while(numFrames > 0){
            
            // what is the largest chunk we can process at one time?
            int framesProcessing = min(numFrames, This->delayTimeSamples);
            
            // if we are running in place, save a copy of the input
            if(input == outL || input == outR)
                memcpy(This->tempBufferL,
                       input,
                       sizeof(float)*framesProcessing);
            
            /*
             * multiplex the mono input to stereo, using a quadrature LFO
             * to rotate between the two channels.
             */
            // generate the quadrature LFO output into the temp buffers
            BMQuadratureOscillator_processQuad(&This->qosc,
                                               This->LFOBufferL,
                                               This->LFOBufferR,
                                               framesProcessing);
            
            // multiplex the mono input to the left channel input
            vDSP_vmul(This->LFOBufferL, 1, input,1,
                      This->LFOBufferL, 1,
                      framesProcessing);
            // multiplex the mono input to the right channel input
            vDSP_vmul(This->LFOBufferR, 1, input,1,
                      This->LFOBufferR, 1,
                      framesProcessing);
            
            // process the delay
            stereoBufferHelper(This,
                               This->LFOBufferL, This->LFOBufferR,
                               outL, outR,
                               framesProcessing);
            
            // filter the output
            BMMultiLevelBiquad_processBufferStereo(&This->filter,
                                                   outL, outR, outL, outR,
                                                   framesProcessing);
            
            
            // if we are running in place, take dry mix from the copy we
            // saved earlier
            float *dry;
            if(input == outL || input == outR){
                dry = This->tempBufferL;
            }
            // otherwise take dry signal from the input
            else {
                dry = input;
            }
            
            // mix dry and wet signals
            vDSP_vsmsma(outL, 1, &This->wetGain,
                        dry, 1, &This->dryGain,
                        outL, 1, framesProcessing);
            vDSP_vsmsma(outR, 1, &This->wetGain,
                        dry, 1, &This->dryGain,
                        outR, 1, framesProcessing);
            
            // advance pointers
            input += framesProcessing;
            outL += framesProcessing;
            outR += framesProcessing;
            
            numFrames -= framesProcessing;
        }
        
        if(This->delayTimeNeedsUpdate){
            BMStaticDelay_destroy(This);
            BMStaticDelay_init(This,
                               1, 2,
                               This->delayTimeInSeconds,
                               This->decayTime,
                               This->wetMixAmount,
                               This->crossMixAmount,
                               This->lowpassFC,
                               This->sampleRate);
        }
    }
    
    
    
    
#ifdef __cplusplus
}
#endif
