//
//  BMGaussianUpsampler.h
//  AudioFiltersXcodeProject
//
//  Upsampling by linear interpolation between samples
//
//  Created by Hans on 17/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this file
//  into the public domain without restrictions.

#ifndef BMGaussianUpsampler_h
#define BMGaussianUpsampler_h

#include <stdio.h>
#include "BMSlidingWindowSum.h"

typedef struct BMGaussianUpsampler {
	BMSlidingWindowSum *swSum;
	size_t upsampleFactor, numLevels;
} BMGaussianUpsampler;

/*!
 *BMGaussianUpsampler_init
 */
void BMGaussianUpsampler_init(BMGaussianUpsampler *This, size_t upsampleFactor, size_t numLevels);

/*!
 *BMGaussianUpsampler_free
 */
void BMGaussianUpsampler_free(BMGaussianUpsampler *This);

/*!
 *BMGaussianUpsampler_process
 */
void BMGaussianUpsampler_processMono(BMGaussianUpsampler *This, const float *input, float *output, size_t inputLength);

#endif /* BMGaussianUpsampler_h */
