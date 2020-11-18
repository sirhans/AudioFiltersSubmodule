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
	This->buffer = calloc(windowLength, sizeof(float));
	This->prevSum = 0.0f;
}

void BMSlidingWindowSum_free(BMSlidingWindowSum *This){
	free(This->buffer);
	This->buffer = NULL;
}

void BMSlidingWindowSum_processMono(BMSlidingWindowSum *This, const float *input, float *output, size_t numSamples){
	// finish up the last few samples of the previous buffer
	size_t sum = This->prevSum;
	for(size_t i=0; i<This->windowSize; i++){
		sum = sum + input[i] - This->buffer[i];
		output[i] = sum;
	}
	
	// process the sliding window sum for the new samples
	vDSP_vswsum(input, 1, output+This->windowSize, 1, numSamples-This->windowSize, This->windowSize);
	
	// copy the last few samples into the buffer for next time
	memcpy(This->buffer, input + numSamples - This->windowSize, This->windowSize);
	
	// save the last output
	This->prevSum = output[numSamples - 1];
}


