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

/*!
 *BMWaveshaper_processBufferBidirectional
 * @abstract clips the input to [-1,1] with a cubic soft clipping function
 */
void BMWaveshaper_processBufferBidirectional(const float* input, float* output, size_t numSamples);



/*!
 *BMWaveshaper_processBufferPositive
 * @abstract clips the input to [0,1] with a cubic soft clipping function
 * @input    an array of values in the range [0,inf].
 * @output   if the input is in [0,inf] then the output will be in [0,1]
 */
void BMWaveshaper_processBufferPositive(const float* input, float* output, size_t numSamples);



/*!
 *BMWaveshaper_processBufferNegative
 * @abstract clips the input to [-1,0] with a cubic soft clipping function
 * @input    an array of values in the range [-1,0].
 * @output   if the input is in [-inf,0] then the output will be in [-1,0]
 */
void BMWaveshaper_processBufferNegative(const float* input, float* output, size_t numSamples);

#endif /* BMWaveshaping_h */
