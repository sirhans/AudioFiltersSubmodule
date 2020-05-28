//
//  BMSincDownsampler.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 28/5/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMSincDownsampler_h
#define BMSincDownsampler_h

#include <stdio.h>

typedef struct BMSincDownsampler {
    size_t kernelLength, downsampleFactor, paddingLeft, paddingRight;
    float* kernel;
} BMSincDownsampler;

#endif /* BMSincDownsampler_h */
