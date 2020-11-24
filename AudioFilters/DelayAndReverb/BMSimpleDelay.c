//
//  BMSimpleDelay.c
//  AudioFiltersXCodeProject
//
//  Created by hans on 31/1/19.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMSimpleDelay.h"
#include "Constants.h"

#ifndef MIN
#define MIN(X,Y) (X<Y)? X:Y
#endif

void BMSimpleDelayMono_init(BMSimpleDelayMono *This, size_t delayTimeInSamples){
    uint32_t dt = (uint32_t)delayTimeInSamples;
    TPCircularBufferInit(&This->buffer, dt + BM_BUFFER_CHUNK_SIZE);
    TPCircularBufferClear(&This->buffer);
    TPCircularBufferProduce(&This->buffer, sizeof(float)*dt);
}


void BMSimpleDelayStereo_init(BMSimpleDelayStereo *This, size_t delayTimeInSamples){
    uint32_t dt = (uint32_t)delayTimeInSamples;
    TPCircularBufferInit(&This->bufferL, sizeof(float)*(dt + BM_BUFFER_CHUNK_SIZE));
    TPCircularBufferClear(&This->bufferL);
    TPCircularBufferProduce(&This->bufferL, sizeof(float)*dt);
    
    TPCircularBufferInit(&This->bufferR, sizeof(float)*(dt + BM_BUFFER_CHUNK_SIZE));
    TPCircularBufferClear(&This->bufferR);
    TPCircularBufferProduce(&This->bufferR, sizeof(float)*dt);
}

void BMSimpleDelayMono_process(BMSimpleDelayMono *This,
                               const float* input,
                               float* output,
                               size_t numSamples){
    uint32_t n = (uint32_t)numSamples;
    while (n > 0){

        // get a write pointer and find out how many bytes are available for writing
        uint32_t availableBytes;
        float* writePointer = TPCircularBufferHead(&This->buffer, &availableBytes);
        
        // what's the maximum number of samples we can process this time through the loop?
        uint32_t samplesProcessing = MIN(n,availableBytes/sizeof(float));
        
        // write to the buffer
        memcpy(writePointer,input,sizeof(float)*samplesProcessing);
        
        // mark as written to the region we just filled with data
        TPCircularBufferProduce(&This->buffer, sizeof(float)*samplesProcessing);
        
        // get a read pointer
        float* readPointer = TPCircularBufferTail(&This->buffer, &availableBytes);
        
        // if this isn't true, there is an error
        assert(availableBytes >= sizeof(float)*samplesProcessing);
        
        memcpy(output, readPointer, sizeof(float)*samplesProcessing);
        
        // mark as read, the region we just read
        TPCircularBufferConsume(&This->buffer, sizeof(float)*samplesProcessing);
        
        n -= samplesProcessing;
        input += samplesProcessing;
        output += samplesProcessing;
    }
}

void BMSimpleDelayStereo_process(BMSimpleDelayStereo *This,
                                 const float* inputL,
                                 const float* inputR,
                                 float* outputL,
                                 float* outputR,
                                 size_t numSamples){
    uint32_t n = (uint32_t)numSamples;
    while (n > 0){
        
        // get a write pointer and find out how many bytes are available for writing
        uint32_t availableBytes;
        float* writePointerL = TPCircularBufferHead(&This->bufferL, &availableBytes);
        float* writePointerR = TPCircularBufferHead(&This->bufferR, &availableBytes);
        
        // what's the maximum number of samples we can process this time through the loop?
        uint32_t samplesProcessing = MIN(n,availableBytes/sizeof(float));
        
        // write to the buffer
        memcpy(writePointerL,inputL,sizeof(float)*samplesProcessing);
        memcpy(writePointerR,inputR,sizeof(float)*samplesProcessing);
        
        // mark as written to the region we just filled with data
        TPCircularBufferProduce(&This->bufferL, sizeof(float)*samplesProcessing);
        TPCircularBufferProduce(&This->bufferR, sizeof(float)*samplesProcessing);
        
        // get a read pointer
        float* readPointerL = TPCircularBufferTail(&This->bufferL, &availableBytes);
        float* readPointerR = TPCircularBufferTail(&This->bufferR, &availableBytes);
        
        // if this isn't true, there is an error
        assert(availableBytes >= sizeof(float)*samplesProcessing);
        
        memcpy(outputL, readPointerL, sizeof(float)*samplesProcessing);
        memcpy(outputR, readPointerR, sizeof(float)*samplesProcessing);
        
        // mark as read, the region we just read
        TPCircularBufferConsume(&This->bufferL, sizeof(float)*samplesProcessing);
        TPCircularBufferConsume(&This->bufferR, sizeof(float)*samplesProcessing);
        
        n -= samplesProcessing;
        inputL += samplesProcessing;
        inputR += samplesProcessing;
        outputL += samplesProcessing;
        outputR += samplesProcessing;
    }
}

void BMSimpleDelayMono_destroy(BMSimpleDelayMono *This){
    TPCircularBufferCleanup(&This->buffer);
}

void BMSimpleDelayStereo_destroy(BMSimpleDelayStereo *This){
    TPCircularBufferCleanup(&This->bufferL);
    TPCircularBufferCleanup(&This->bufferR);
}
