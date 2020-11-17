//
//  BMLerpUpsampler.c
//  AudioFiltersXcodeProject
//
//  Upsampling by linear interpolation between samples
//
//  Created by Hans on 17/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this file
//  into the public domain without restrictions.
//

#include "BMLerpUpsampler.h"
#include <Accelerate/Accelerate.h>

void BMLerpUpsampler_process(BMLerpUpsampler *This, const float *input, float *output, size_t upsampleFactor, size_t inputLength){
    size_t outputLength = inputLength*upsampleFactor;
    
    // set output to zero
    memset(output,0,sizeof(float)*outputLength);
    
    // copy the input samples to the output
    float scale = 1.0f / (float)upsampleFactor;
    vDSP_vsmul(input, 1, &scale, output, upsampleFactor, inputLength);
    
    vDSP_vswsum(output, 1, output, 1, outputLength, upsampleFactor*2);
}
