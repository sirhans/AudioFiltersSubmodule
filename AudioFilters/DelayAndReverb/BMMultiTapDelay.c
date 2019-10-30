//
//  BMMultiTapDelay.c
//  ETSax
//
//  Created by Duc Anh on 5/29/17.
//  Copyright © 2017 BlueMangoo. All rights reserved.
//

#include "BMMultiTapDelay.h"
#include <Accelerate/Accelerate.h>
#include <stdlib.h>
#include "Constants.h"

void BMMultiTapDelay_initBuffer(BMMultiTapDelay* delay);
void BMMultiTapDelay_PerformReInitBuffer(BMMultiTapDelay* This);




void BMMultiTapDelay_Init(BMMultiTapDelay* This,
                          bool isStereo,
                          size_t* delayTimesL, size_t* delayTimesR,
                          size_t maxDelayTime,
                          float* gainL, float* gainR,
                          size_t numTaps, size_t maxTaps){
    
    BMMultiTapDelaySetting* setting = &This->setting;
    setting->isStereo = isStereo;
    
    This->_needUpdateGain = This->_needUpdateIndices = false;
    This->numberChannel = (isStereo? 2:1);
    This->maxTaps = maxTaps;
    This->numTaps = numTaps;
    This->maxDelayTime = maxDelayTime;
    
    This->buffer = malloc(sizeof(TPCircularBuffer) * This->numberChannel);
    This->tempBuffer = malloc(sizeof(float*) * This->numberChannel);
    
    This->input = malloc(sizeof(float*) * This->numberChannel);
    This->output = malloc(sizeof(float*) * This->numberChannel);
    setting->gains = malloc(sizeof(float*) * This->numberChannel);
    setting->indices = malloc(sizeof(size_t*) * This->numberChannel);
    setting->delayTimes = malloc(sizeof(size_t*) * This->numberChannel);
    This->tempGains = malloc(sizeof(float*) * This->numberChannel);
    This->tempIndices = malloc(sizeof(size_t*) * This->numberChannel);
    
    // alias the input arrays that have l and r channels into
    // multi-dimensional arrays so we can iterate the channels with a for
    // loop
    
    // malloc and set indices & gain
    for (int i=0; i < This->numberChannel; i++) {
        setting->indices[i] = malloc(sizeof(size_t) * maxTaps);
        setting->gains[i] = malloc(sizeof(float) * maxTaps);
        This->tempIndices[i] = malloc(sizeof(size_t) * maxTaps);
        This->tempGains[i] = malloc(sizeof(float) * maxTaps);
        This->tempBuffer[i] = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE);
    }
    
    BMMultiTapDelay_setDelayTimes(This, delayTimesL, delayTimesR);
    BMMultiTapDelay_setGains(This, gainL, gainR);
    
    BMMultiTapDelay_initBuffer(This);
}



void BMMultiTapDelay_initBypass(BMMultiTapDelay *This,
						   bool isStereo,
						   size_t maxDelayLength,
						   size_t maxTapsPerChannel){
	
	// allocate temporary memory and set everything to zero
	size_t *delayTimes = calloc(maxTapsPerChannel, sizeof(size_t));
	float       *gains = calloc(maxTapsPerChannel, sizeof(float));
	
	// set the gains of the first delay in each channel to one
	gains[0] = 1.0f;
	
	// init the struct
	BMMultiTapDelay_Init(This,
						 isStereo,
						 delayTimes, delayTimes,
						 maxDelayLength,
						 gains, gains,
						 maxTapsPerChannel,
						 maxTapsPerChannel);
	
	// free temporary memory
	free(delayTimes);
	free(gains);
}



void BMMultiTapDelay_initBuffer(BMMultiTapDelay* delay){
    //BMMultiTapDelaySetting* setting = &delay->setting;
    
    //init the rest
    size_t numberFrames = (delay->maxDelayTime + 1) + BM_BUFFER_CHUNK_SIZE;
    size_t numberBytes = numberFrames * sizeof(float);
    delay->zeroArray = malloc(numberBytes);
    memset(delay->zeroArray, 0, numberBytes);
    //[0][bufferChunkSize]
    
    //init circular buffer & tempbuffer
    for(size_t i=0; i<delay->numberChannel; i++){
        TPCircularBufferInit(&delay->buffer[i], (int32_t)numberBytes);
        TPCircularBufferProduceBytes(&delay->buffer[i], delay->zeroArray, (int32_t)numberBytes);
        TPCircularBufferConsume(&delay->buffer[i], (int32_t)(BM_BUFFER_CHUNK_SIZE+1) * sizeof(float));
    }
}

void BMMultiTapDelay_destroyBuffer(BMMultiTapDelay* delay){
    free(delay->zeroArray);
    delay->zeroArray = NULL;
    //Clear circular buffer
    for(size_t i=0; i<delay->numberChannel; i++){
        TPCircularBufferCleanup(&delay->buffer[i]);
    }
}

/*
 * works in place
 */
void BMMultiTapDelay_ProcessBufferMono(BMMultiTapDelay* delay,
                                       float* input, float* output,
                                       size_t frames){
    assert(delay->numberChannel == 1);
    
    if(delay->_needUpdateIndices)
        BMMultiTapDelay_PerformUpdateIndices(delay);
    if(delay->_needUpdateGain)
        BMMultiTapDelay_PerformUpdateGains(delay);
    
    // this function is stereo in, stereo out; fail if the delay was
    // not initialised for that configuration
    
    BMMultiTapDelaySetting* setting = &delay->setting;
    //this will bridge my code with Sir Hans's style ^_^
    delay->input[0] = input;
    delay->output[0] = output;

    size_t frameThisTime;
    
    size_t framesProcessed = 0;
    while(framesProcessed < frames){
        size_t framesProcessing = frames - framesProcessed;
        framesProcessing = (framesProcessing < BM_BUFFER_CHUNK_SIZE)? framesProcessing:BM_BUFFER_CHUNK_SIZE;
        
        frameThisTime = framesProcessing;
        
        //
        uint32_t availableBytes;
        uint32_t bytesThisTime;
        
        bytesThisTime = (int32_t)frameThisTime * sizeof(float);

        //set 0 to the tempbuffer
        memset(delay->tempBuffer[0], 0, BM_BUFFER_CHUNK_SIZE * sizeof(float));
        TPCircularBufferProduceBytes(&delay->buffer[0], delay->input[0]+framesProcessed, bytesThisTime);
        
        //from each read point, we read FrameThisTime frames to process
        //our aim is to get the buffer with FrameThisTime length
        //this buffer will add directly to the output
        float* buffer = TPCircularBufferTail(&delay->buffer[0], &availableBytes);
        
        for (int j=0; j<delay->numTaps; j++) {
            vDSP_vsma(buffer + setting->indices[0][j], 1, &setting->gains[0][j], delay->tempBuffer[0], 1, delay->tempBuffer[0], 1, frameThisTime);
        }
        TPCircularBufferConsume(&delay->buffer[0], bytesThisTime);
        
        //we overwrite data to the output
        memcpy(delay->output[0]+framesProcessed, delay->tempBuffer[0], bytesThisTime);

        framesProcessed += framesProcessing;
    }
}


/*
 * works in place
 */
void BMMultiTapDelay_processBufferStereo(BMMultiTapDelay* delay,
                                         float* inputL, float* inputR,
                                         float* outputL, float* outputR,
                                         size_t frames){
    assert(delay->numberChannel == 2);
    
    if(delay->_needUpdateIndices)
        BMMultiTapDelay_PerformUpdateIndices(delay);
    if(delay->_needUpdateGain)
        BMMultiTapDelay_PerformUpdateGains(delay);
	
    
    BMMultiTapDelaySetting* setting = &delay->setting;
    //this will bridge my code with Sir Hans's style ^_^
    delay->input[0] = inputL;
    delay->input[1] = inputR;
    delay->output[0] = outputL;
    delay->output[1] = outputR;
    
    size_t frameThisTime;
    
    size_t framesProcessed = 0;
    while(framesProcessed < frames){
        size_t framesProcessing = frames - framesProcessed;
        framesProcessing = (framesProcessing < BM_BUFFER_CHUNK_SIZE)? framesProcessing:BM_BUFFER_CHUNK_SIZE;
        
        frameThisTime = framesProcessing;
        
        //
        uint32_t availableBytes;
        uint32_t bytesThisTime;
        
        bytesThisTime = (int32_t)frameThisTime * sizeof(float);
        
        for(size_t i=0; i<delay->numberChannel; i++){
            //set 0 to the tempbuffer
            memset(delay->tempBuffer[i], 0, BM_BUFFER_CHUNK_SIZE * sizeof(float));
            TPCircularBufferProduceBytes(&delay->buffer[i], delay->input[i]+framesProcessed, bytesThisTime);
            
            //from each read point, we read FrameThisTime frames to process
            //our aim is to get the buffer with FrameThisTime length
            //this buffer will add directly to the output
            float* buffer = TPCircularBufferTail(&delay->buffer[i], &availableBytes);
            
            for (int j=0; j<delay->numTaps; j++) {
                vDSP_vsma(buffer + setting->indices[i][j], 1, &setting->gains[i][j], delay->tempBuffer[i], 1, delay->tempBuffer[i], 1, frameThisTime);
            }
            TPCircularBufferConsume(&delay->buffer[i], bytesThisTime);
            
            //we overwrite data to the output
            memcpy(delay->output[i]+framesProcessed, delay->tempBuffer[i], bytesThisTime);
        }
        
        //
        framesProcessed += framesProcessing;
    }
}


void BMMultiTapDelay_ProcessOneSampleStereo(BMMultiTapDelay* delay,
                                            float* inputL, float* inputR,
                                            float* outputL, float* outputR){
    if(delay->_needUpdateIndices)
        BMMultiTapDelay_PerformUpdateIndices(delay);
    if(delay->_needUpdateGain)
        BMMultiTapDelay_PerformUpdateGains(delay);
    
    // this function is stereo in, stereo out; fail if the delay was
    // not initialised for that configuration
    //sir Hans'style
    BMMultiTapDelaySetting* setting = &delay->setting;
    delay->input[0] = inputL;
    delay->input[1] = inputR;
    delay->output[0] = outputL;
    delay->output[1] = outputR;
    
    uint32_t availableBytes;
    
    for(size_t i=0; i<delay->numberChannel; i++){
        //set 0 to the tempbuffer
        //memset(delay->tempBuffer[i], 0, BMUS_CORE_BUFFER_LENGTH * sizeof(float));
        TPCircularBufferProduceBytes(&delay->buffer[i], delay->input[i], sizeof(float));
        
        //from each read point, we read FrameThisTime frames to process
        //our aim is to get the buffer with FrameThisTime length
        //this buffer will add directly to the output
        float* buffer = TPCircularBufferTail(&delay->buffer[i], &availableBytes);
        //
        vDSP_vgathr(buffer,
                    setting->indices[i], 1,
                    delay->tempBuffer[i], 1,
                    delay->numTaps);
        
        vDSP_dotpr(delay->tempBuffer[i], 1,
                   setting->gains[i], 1,
                   delay->output[i],
                   delay->numTaps);
        
        TPCircularBufferConsume(&delay->buffer[i], sizeof(float));
    }
}



/*
 *  Write zeros into the buffers to clear them
 */
void BMMultiTapDelay_clearBuffers(BMMultiTapDelay* delay){
    for(size_t i=0; i<delay->numberChannel; i++){
        memset(delay->buffer[i].buffer, 0, delay->buffer[i].length);
    }
}



/*
 *  Free memory of the struct at *This
 */
void BMMultiTapDelay_free(BMMultiTapDelay* This){
    BMMultiTapDelaySetting *setting = &This->setting;
    
    for (int i=0; i<This->numberChannel; i++) {
        free(This->tempBuffer[i]);
        This->tempBuffer[i] = NULL;
        
        This->input[i] = NULL;
        This->output[i] = NULL;
        
        TPCircularBufferCleanup(&This->buffer[i]);
        
        free(setting->indices[i]);
        setting->indices[i] = NULL;
        
        free(setting->gains[i]);
        setting->gains[i] = NULL;
        
        free(This->tempIndices[i]);
        This->tempIndices[i] = NULL;
        
        free(This->tempGains[i]);
        This->tempGains[i] = NULL;
    }
    free(This->buffer);
    This->buffer = NULL;
    
    free(This->tempBuffer);
    This->tempBuffer = NULL;
    
    free(This->input);
    This->input = NULL;
    
    free(This->output);
    This->output = NULL;
    
    free(This->zeroArray);
    This->zeroArray = NULL;
    
    //Memory leak
    free(setting->indices);
    setting->indices = NULL;
    
    free(setting->gains);
    setting->gains = NULL;
    
    free(setting->delayTimes);
    setting->delayTimes = NULL;
    
    free(This->tempGains);
    This->tempGains = NULL;
    
    free(This->tempIndices);
    This->tempIndices = NULL;
}

void BMMultiTapDelay_PerformUpdateIndices(BMMultiTapDelay* This){
    //write data from tempArray
    BMMultiTapDelaySetting* setting = &This->setting;
    for (int i=0; i<This->numberChannel; i++) {
        for (int j=0; j<This->numTaps; j++) {
            setting->indices[i][j] = This->tempIndices[i][j];
        }
    }
    This->_needUpdateIndices = false;
}



void BMMultiTapDelay_PerformUpdateGains(BMMultiTapDelay* This){
    //write data from tempArray
    BMMultiTapDelaySetting* setting = &This->setting;
    for (int i=0; i<This->numberChannel; i++) {
        for (int j=0; j<This->numTaps; j++) {
            setting->gains[i][j] = This->tempGains[i][j];
        }
    }
    This->_needUpdateGain = false;
}



void BMMultiTapDelay_setDelayTimes(BMMultiTapDelay* This,
                                   size_t* delayTimesL, size_t* delayTimesR){
    
    This->_needUpdateIndices = false;
    BMMultiTapDelaySetting* setting = &This->setting;
    setting->delayTimes[0] = delayTimesL;
    setting->delayTimes[1] = delayTimesR;
    for (int i=0; i< This->numberChannel; i++) {
        for (int j=0; j<This->numTaps; j++) {
            This->tempIndices[i][j] = This->maxDelayTime - setting->delayTimes[i][j];
        }
    }
    This->_needUpdateIndices = true;
}



void BMMultiTapDelay_setDelayTimeNumTap(BMMultiTapDelay* This,
                                        size_t* delayTimesL, size_t* delayTimesR,
                                        size_t numTaps){
    assert(numTaps <= This->maxTaps);
    BMMultiTapDelaySetting* setting = &This->setting;
    setting->delayTimes[0] = delayTimesL;
    setting->delayTimes[1] = delayTimesR;
    
    // confirm that none of the delay taps exceeds the max delay time
    for (int i=0; i< This->numberChannel; i++)
        for (int j=0; j<This->numTaps; j++)
            assert(setting->delayTimes[i][j] <= This->maxDelayTime);
    
    
    This->_needUpdateIndices = false;
    This->numTaps = numTaps;
    for (int i=0; i< This->numberChannel; i++) {
        for (int j=0; j<This->numTaps; j++) {
            This->tempIndices[i][j] = This->maxDelayTime - setting->delayTimes[i][j];
        }
    }
    This->_needUpdateIndices = true;
}



void BMMultiTapDelay_setGains(BMMultiTapDelay* This, float* gainL, float* gainR){
    This->_needUpdateGain = false;
    
    for (int i=0; i< This->numberChannel; i++) {
        for (int j=0; j < This->numTaps; j++) {
            This->tempGains[i][j] = (i == 0)? gainL[j]:gainR[j];
        }
    }
    This->_needUpdateGain = true;
}



/*
 *  Print the impulse response to standard output for testing
 */
void BMMultiTapDelay_impulseResponse(BMMultiTapDelay* This){
    // we include the right channel if the output is stereo
    bool includeRightChannel = This->setting.isStereo;
    
    // check that the indices are initialized
    assert(This->setting.indices[0] != NULL);
    if(includeRightChannel)
        assert(This->setting.indices[1] != NULL);
    
    // find the longest delay time
    size_t maxDelayTime = This->maxDelayTime;
    
    // allocate an array to store the output of the impulse response
    size_t IRlength = maxDelayTime + 1;
    float* leftIR = malloc(sizeof(float)*IRlength);
    float* rightIR = NULL;
    if(includeRightChannel)
        rightIR = malloc(sizeof(float)*IRlength);
    
    
    /************************************
     *   compute the impulse response   *
     ************************************/
    
    // clear any data that might be in the buffers
    //BMSimpleDelay_clearBuffers(This);
    
    // for the next few lines we assume stereo;
    // if mono, have to rewrite this section
    if(includeRightChannel){
        assert(includeRightChannel);
        
        // write the initial impulse into the delay,
        float one = 1.0f;
        BMMultiTapDelay_processBufferStereo(This,
                                          &one, &one,
                                          leftIR, rightIR,
                                          1);
        
        // process the remaining part of the impulse response
        //float zero = 0.0f;
        float* zero = malloc(sizeof(float) * (IRlength - 1));
        memset(zero, 0, (IRlength - 1) * sizeof(float));
        BMMultiTapDelay_processBufferStereo(This,
                                          zero, zero,
                                          leftIR + 1, rightIR + 1,
                                          IRlength - 1);
    }else{
        float one = 1.0f;
        BMMultiTapDelay_ProcessBufferMono(This, &one, leftIR, 1);
        
        // process the remaining part of the impulse response
        //float zero = 0.0f;
        float* zero = malloc(sizeof(float) * (IRlength - 1));
        memset(zero, 0, (IRlength - 1) * sizeof(float));
        BMMultiTapDelay_ProcessBufferMono(This, zero, leftIR + 1, IRlength - 1);
    }
    
	
    /************************************
     *    print the impulse response    *
     ************************************/
    
    // left channel
    printf("\n\nLeft channel impulse response: {\n");
    for(size_t i=0; i<(IRlength-1); i++)
        printf("%f\n", leftIR[i]);
    printf("%f}\n\n",leftIR[IRlength-1]);
    
    // right channel
    if(includeRightChannel){
        printf("\n\nRight channel impulse response: {\n");
        for(size_t i=0; i<(IRlength-1); i++)
            printf("%f\n", rightIR[i]);
        printf("%f}\n\n",rightIR[IRlength-1]);
    }
}



