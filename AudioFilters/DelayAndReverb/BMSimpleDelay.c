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
    TPCircularBufferInit(&This->buffer, sizeof(float)*(dt + BM_BUFFER_CHUNK_SIZE));
    TPCircularBufferClear(&This->buffer);
    TPCircularBufferProduce(&This->buffer, sizeof(float)*dt);
	This->delayTimeSamples = delayTimeInSamples;
}


void BMSimpleDelayStereo_init(BMSimpleDelayStereo *This, size_t delayTimeInSamples){
    uint32_t dt = (uint32_t)delayTimeInSamples;
    TPCircularBufferInit(&This->bufferL, sizeof(float)*(dt + BM_BUFFER_CHUNK_SIZE));
    TPCircularBufferClear(&This->bufferL);
    TPCircularBufferProduce(&This->bufferL, sizeof(float)*dt);
    
    TPCircularBufferInit(&This->bufferR, sizeof(float)*(dt + BM_BUFFER_CHUNK_SIZE));
    TPCircularBufferClear(&This->bufferR);
    TPCircularBufferProduce(&This->bufferR, sizeof(float)*dt);
	
	This->delayTimeSamples = delayTimeInSamples;
}



size_t BMSimpleDelayStereo_getDelayLength(BMSimpleDelayStereo *This){
	return This->delayTimeSamples;
}



size_t BMSimpleDelayMono_getDelayLength(BMSimpleDelayMono *This){
	return This->delayTimeSamples;
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


size_t BMSimpleDelayStereo_outputCapacity(BMSimpleDelayStereo *This){
	uint32_t bytesAvailable;
	TPCircularBufferTail(&This->bufferL, &bytesAvailable);
	
	return bytesAvailable / sizeof(float);
}


size_t BMSimpleDelayStereo_output(BMSimpleDelayStereo *This, float *outL, float *outR, size_t requestedOutputLength){
	// how much output is ready to read out from the delay?
	size_t outputAvailable = BMSimpleDelayStereo_outputCapacity(This);
	
	// how much output are we actually going to write?
	size_t outputLength = BM_MIN(outputAvailable, requestedOutputLength);
	
	// get a read pointer
	uint32_t availableBytes;
	float* readPointerL = TPCircularBufferTail(&This->bufferL, &availableBytes);
	float* readPointerR = TPCircularBufferTail(&This->bufferR, &availableBytes);
	
	// if this isn't true, there is an error
	assert(availableBytes >= sizeof(float)*(size_t)outputLength);
	
	memcpy(outL, readPointerL, sizeof(float)*(size_t)outputLength);
	memcpy(outR, readPointerR, sizeof(float)*(size_t)outputLength);
	
	// mark the samples we just read as "read"
	TPCircularBufferConsume(&This->bufferL, (uint32_t)sizeof(float)*(uint32_t)outputLength);
	TPCircularBufferConsume(&This->bufferR, (uint32_t)sizeof(float)*(uint32_t)outputLength);
	
	return outputLength;
}


size_t BMSimpleDelayStereo_inputCapacity(BMSimpleDelayStereo *This){
	uint32_t bytesAvailable;
	TPCircularBufferHead(&This->bufferL, &bytesAvailable);
	
	return bytesAvailable / sizeof(float);
}


size_t BMSimpleDelayStereo_input(BMSimpleDelayStereo *This, float *inL, float *inR, size_t inputLength){
	// how much space is available to write into the delay?
	size_t spaceAvailable = BMSimpleDelayStereo_outputCapacity(This);
	
	// how much input are we actually going to write?
	size_t inputSamplesWriting = BM_MIN(spaceAvailable, inputLength);
	
	// get a write pointer
	uint32_t availableBytes;
	float* writePointerL = TPCircularBufferHead(&This->bufferL, &availableBytes);
	float* writePointerR = TPCircularBufferHead(&This->bufferR, &availableBytes);
	
	// if this isn't true, there is an error
	assert(availableBytes >= sizeof(float)*(size_t)inputSamplesWriting);
	
	// write to the buffer
	memcpy(writePointerL, inL, sizeof(float)*inputSamplesWriting);
	memcpy(writePointerR, inR, sizeof(float)*inputSamplesWriting);
	
	// mark the samples in the region we just filled as "written"
	TPCircularBufferProduce(&This->bufferL, (uint32_t)sizeof(float)*(uint32_t)inputSamplesWriting);
	TPCircularBufferProduce(&This->bufferR, (uint32_t)sizeof(float)*(uint32_t)inputSamplesWriting);
	
	return inputSamplesWriting;
}


void BMSimpleDelayMono_free(BMSimpleDelayMono *This){
    TPCircularBufferCleanup(&This->buffer);
}

void BMSimpleDelayStereo_free(BMSimpleDelayStereo *This){
    TPCircularBufferCleanup(&This->bufferL);
    TPCircularBufferCleanup(&This->bufferR);
}


void BMSimpleDelayMono_destroy(BMSimpleDelayMono *This){
	BMSimpleDelayMono_free(This);
}

void BMSimpleDelayStereo_destroy(BMSimpleDelayStereo *This){
	BMSimpleDelayStereo_free(This);
}
