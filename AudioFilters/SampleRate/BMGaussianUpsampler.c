//
//  BMGaussianUpsampler.c
//  AudioFiltersXcodeProject
//
//  Upsampling by gaussian kernel interpolation between samples. The gaussian
//  kernel antialiasing filter is implemented by a series of sliding window sum
//  operations.
//
//  Created by Hans on 17/11/20.
//  Copyright © 2020 BlueMangoo. We release this file
//  into the public domain without restrictions.
//

#include "BMGaussianUpsampler.h"
#include <Accelerate/Accelerate.h>

void BMGaussianUpsampler_init(BMGaussianUpsampler *This, size_t upsampleFactor, size_t lowpassNumPasses){
	This->upsampleFactor = upsampleFactor;
	This->numLevels = lowpassNumPasses;
	
	This->swSum = malloc(sizeof(BMSlidingWindowSum) * lowpassNumPasses);
	
	for(size_t i=0; i<This->numLevels; i++)
		BMSlidingWindowSum_init(&This->swSum[i], upsampleFactor);
}




void BMGaussianUpsampler_free(BMGaussianUpsampler *This){
	for(size_t i=0; i<This->numLevels; i++)
		BMSlidingWindowSum_free(&This->swSum[i]);
	free(This->swSum);
	This->swSum = NULL;
}




void BMGaussianUpsampler_processMono(BMGaussianUpsampler *This, const float *input, float *output, size_t inputLength){
	if(This->upsampleFactor > 1){
		size_t outputLength = inputLength*This->upsampleFactor;
		
		// set output to zero
		memset(output,0,sizeof(float)*outputLength);
		
		// copy the input samples to the output with scaling factor and stride for upsampling
		float scale = 1.0f / powf(This->upsampleFactor,This->numLevels-1);
		vDSP_vsmul(input, 1, &scale, output, This->upsampleFactor, inputLength);
		
		// gaussian kernel antialias filter implemented by a series of sliding
		// window sum operations
		for(size_t i=0; i<This->numLevels; i++)
			BMSlidingWindowSum_processMono(&This->swSum[i], output, output, inputLength*This->upsampleFactor);
	} else {
		if(input != output)
			memcpy(output,input,sizeof(float)*inputLength);
	}
}
