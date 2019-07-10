//
//  BMUpsampler2x.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMUpsampler2x_h
#define BMUpsampler2x_h

#include <simd/simd.h>
#include <mactypes.h>

typedef struct BMUpsampler2x {
    size_t numCoefficients, numStages, stageWidth;
    simd_float4* filters;
} BMUpsampler2x;

#endif /* BMUpsampler2x_h */
