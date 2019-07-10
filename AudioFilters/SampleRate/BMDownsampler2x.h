//
//  BMDownsampler2x.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/10/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMDownsampler2x_h
#define BMDownsampler2x_h

#include <simd/simd.h>
#include <MacTypes.h>

typedef struct BMDownsampler2x {
    simd_float4* filters;
    size_t numCoefficients, numFilters;
} BMDownsampler2x;

#endif /* BMDownsampler2x_h */
