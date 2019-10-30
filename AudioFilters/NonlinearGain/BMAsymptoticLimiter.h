//
//  BMAsymptoticLimiter.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 29/10/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMAsymptoticLimiter_h
#define BMAsymptoticLimiter_h

#include <simd/simd.h>

#include <stdio.h>

static inline void BMAsymptoticLimit(const float *input, float *output, size_t numSamples){
    simd_float4 *in4 = (simd_float4*)input;
    simd_float4 *out4 = (simd_float4*)output;
    size_t numSamples4 = numSamples/4;
    
    // main processing loop
    for(size_t i=0; i<numSamples4; i++)
        out4[i] = in4[i] / (1.0f + simd_abs(in4[i]));
    
    // if numSamples isn't divisible by 4, clean up the last few
    size_t cleanupStartIndex = numSamples - ((numSamples4 - 1) * 4);
    for(size_t i=cleanupStartIndex; i<numSamples; i++)
        output[i] = input[i] / (1.0f + fabsf(input[i]));
}

#endif /* BMAsymptoticLimiter_h */
