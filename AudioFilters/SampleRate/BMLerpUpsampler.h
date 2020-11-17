//
//  BMLerpUpsampler.h
//  AudioFiltersXcodeProject
//
//  Upsampling by linear interpolation between samples
//
//  Created by Hans on 17/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this file
//  into the public domain without restrictions.

#ifndef BMLerpUpsampler_h
#define BMLerpUpsampler_h

#include <stdio.h>

typedef struct BMLerpUpsampler {
    
} BMLerpUpsampler;

void BMLerpUpsampler_process(const float *input, float *output, size_t upsampleFactor, size_t inputLength);

#endif /* BMLerpUpsampler_h */
