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


void BMWaveshaper_squareClipping(const simd_float4* input, simd_float4* output, size_t numSamples);


#endif /* BMWaveshaping_h */
