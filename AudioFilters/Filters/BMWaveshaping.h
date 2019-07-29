//
//  BMWaveshaping.h
//  AudioFiltersXcodeProject
//
//  This class contains functions for polynomial waveshaping with hard limits
//
//  Created by Hans on 4/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMWaveshaper_h
#define BMWaveshaper_h

#include <stdio.h>
#include <simd/simd.h>

/*!
 *BMWaveshaper_processBufferBidirectional
 * @abstract clips the input to [-1,1] with a cubic soft clipping function
 */
void BMWaveshaper_processBufferBidirectional(const simd_float4* input, simd_float4* output, size_t numSamples);



/*!
 *BMWaveshaper_processBufferPositive
 * @abstract clips the input to [-inf,1] with a cubic soft clipping function
 * @input    an array of values in the range [0,inf]. Negative values are ok but the waveshaping will distort them in a way that may not be useful.
 * @output   if the input is in [0,inf] then the output will be in [0,1]
 */
void BMWaveshaper_processBufferPositive(const float* input, float* output, size_t numSamples);



/*!
 *BMWaveshaper_processBufferNegative
 * @abstract clips the input to [-1,inf] with a cubic soft clipping function
 * @input    an array of values in the range [-inf,0]. Positive values are ok but the waveshaping will distort them in a way that may not be useful.
 * @output   if the input is in [-inf,0] then the output will be in [-1,0]
 */
void BMWaveshaper_processBufferNegative(const float* input, float* output, size_t numSamples);

#endif /* BMWaveshaping_h */
