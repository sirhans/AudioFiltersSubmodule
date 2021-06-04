//
//  BMGaussianUpsampler.h
//  AudioFiltersXcodeProject
//
//  Upsampling by linear interpolation between samples
//
//  Created by Hans on 17/11/20.
//  Copyright © 2020 BlueMangoo. We release this file
//  into the public domain without restrictions.

#ifndef BMGaussianUpsampler_h
#define BMGaussianUpsampler_h

#include <stdio.h>
#include "BMSlidingWindowSum.h"
#include "BMFIRFilter.h"

typedef struct BMGaussianUpsampler {
	BMSlidingWindowSum *swSum;
	// BMFIRFilter *aaFilter;
	
	size_t upsampleFactor, numLevels;
} BMGaussianUpsampler;

/*!
 *BMGaussianUpsampler_init
 *
 * @param This pointer to an initialised struct
 * @param upsampleFactor upsample factor
 * @param lowpassNumPasses >= 1 this class implements a gaussian FIR lowpass filter by using multiple passes of a sliding rectangular window filter. When this value is 1, we get linear interpolation. Higher values approximate a gaussian lowpass filter.
 */
void BMGaussianUpsampler_init(BMGaussianUpsampler *This, size_t upsampleFactor, size_t lowpassNumPasses);

/*!
 *BMGaussianUpsampler_free
 */
void BMGaussianUpsampler_free(BMGaussianUpsampler *This);

/*!
 *BMGaussianUpsampler_process
 */
void BMGaussianUpsampler_processMono(BMGaussianUpsampler *This, const float *input, float *output, size_t inputLength);

#endif /* BMGaussianUpsampler_h */
