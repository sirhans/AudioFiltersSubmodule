//
//  BMTanhLimiter.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/31/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMTanhLimiter_h
#define BMTanhLimiter_h

#include <stdio.h>


/*!
 *BMTanhLimiter
 *
 * @abstract  linear from x = y = -softLimit to +infinity, curved to clipping for x < -softLimit until y=-hardLimit
 * @param input      input array
 * @param output     output array
 * @param softLimit  this function is linear for -softLimit < input < softLimit
 * @param hardLimit  require hardLimit > softLimit. Limit abs(output) = hardLimit as input -> abs(infinity)
 * @param numSamples length of input and output arrays
 */
void BMTanhLimiter(const float* input, float* output, float* softLimit, float* hardLimit, size_t numSamples);


/*!
 *BMTanhLimiterUpper
 *
 * @abstract  linear from x = -inf to x=y=softLimit, then curved until y=hardLimit
 * @param input      input array
 * @param output     output array
 * @param softLimit  this function is linear for input < softLimit
 * @param hardLimit  require hardLimit > softLimit. Limit output = hardLimit as input -> +infinity
 * @param numSamples length of input and output arrays
 */
void BMTanhLimiterUpper(const float* input, float* output, float* softLimit, float* hardLimit, size_t numSamples);


/*!
 *BMTanhLimiterLower
 *
 * @abstract  linear from x = y = -softLimit to +infinity, curved to clipping for x < -softLimit until y=-hardLimit
 * @param input      input array
 * @param output     output array
 * @param softLimit  this function is linear for input > -softLimit. NOTE THE NEGATIVE SIGN HERE!!!
 * @param hardLimit  require -hardLimit < -softLimit. Limit output = -hardLimit as input -> -infinity
 * @param numSamples length of input and output arrays
 */
void BMTanhLimiterLower(const float* input, float* output, float* softLimit, float* hardLimit, size_t numSamples);

#endif /* BMTanhLimiter_h */
