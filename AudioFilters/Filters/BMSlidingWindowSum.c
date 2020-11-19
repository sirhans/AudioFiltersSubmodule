//
//  BMSlidingWindowSum.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 17/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this file into the public
//  domain with no restrictions.
//

#include "BMSlidingWindowSum.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"


void BMSlidingWindowSum_init(BMSlidingWindowSum *This, size_t windowLength){
	This->windowSize = windowLength;
//	This->buffer = calloc(windowLength, sizeof(float));
//	This->prevSum = 0.0f;
	
	// init a buffer
	uint32_t bufferLength = (uint32_t)(sizeof(float)*(BM_BUFFER_CHUNK_SIZE + windowLength));
	TPCircularBufferInit(&This->buffer, bufferLength);
	
	// set the first few elements in the buffer to zero
	uint32_t availableBytes;
	float *writePointer = TPCircularBufferHead(&This->buffer, &availableBytes);
	uint32_t bytesWritten = (uint32_t)(sizeof(float)*(windowLength-1));
	memset(writePointer,0,bytesWritten);
	
	// advance the write pointer to account for the delay needed to do the sliding window
	TPCircularBufferProduce(&This->buffer, bytesWritten);
}





void BMSlidingWindowSum_free(BMSlidingWindowSum *This){
//	free(This->buffer);
//	This->buffer = NULL;
	TPCircularBufferCleanup(&This->buffer);
}





void BMSlidingWindowSum_processMono(BMSlidingWindowSum *This, const float *input, float *output, size_t numSamples){
	
	size_t samplesRemaining = numSamples;
	size_t i=0;
	
	while(samplesRemaining > 0){
		// get a write pointer and count available space
		uint32_t availableBytes;
		float *writePointer = TPCircularBufferHead(&This->buffer, &availableBytes);
		size_t samplesProcessing = BM_MIN(samplesRemaining,availableBytes/sizeof(float));
		size_t bytesProcessing = sizeof(float)*samplesProcessing;
		
		// write to the buffer
		memcpy(writePointer,input+i,bytesProcessing);
		TPCircularBufferProduce(&This->buffer, (uint32_t)bytesProcessing);
		
		// get read pointer
		float *readPointer = TPCircularBufferTail(&This->buffer, &availableBytes);
		
		// do the sliding window sum
		vDSP_vswsum(readPointer, 1, output+i, 1, samplesProcessing, This->windowSize);
		
		// mark the bytes read
		TPCircularBufferConsume(&This->buffer, (uint32_t)bytesProcessing);
		
		// advance pointers
		samplesRemaining -= samplesProcessing;
		i += samplesProcessing;
	}
	
//	// finish up the last few samples of the previous buffer
//	size_t sum = This->prevSum;
//	for(size_t i=0; i<This->windowSize-1; i++){
//		sum = sum + input[i] - This->buffer[i];
//		output[i] = sum;
//	}
//	
//	// process the sliding window sum for the new samples
//	vDSP_vswsum(input, 1, output+This->windowSize-1, 1, numSamples-(This->windowSize-1), This->windowSize);
//	
//	// copy the last few samples into the buffer for next time
//	memcpy(This->buffer, input + numSamples - (This->windowSize-1), sizeof(float)*(This->windowSize-1));
//	
//	// save the last output
//	This->prevSum = output[numSamples - 1];
}


