//
//  BMTanhLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/31/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMTanhLimiter.h"
#include <Accelerate/Accelerate.h>
#include <assert.h>


void BMTanhLimiterUpper(const float* input, float* output, float* softLimit, float* hardLimit, size_t numSamples){
    assert(hardLimit > softLimit);
    
    // Mathematica prototype:
    //
    // TanhLimitUpper[x_,softLimit_,hardLimit_] :=
    //   Min[(hardLimit-softLimit)Tanh[(x/(hardLimit-softLimit)-softLimit/(hardLimit-softLimit))]+softLimit,x]

    //     (x/(hardLimit-softLimit)-softLimit/(hardLimit-softLimit))
    float scaleUp = (*hardLimit-*softLimit);
    float scaleDown = 1.0f / scaleUp;
    float shiftDown = -*softLimit * scaleDown;
    vDSP_vsmsa(input, 1, &scaleDown, &shiftDown, output, 1, numSamples);
    
    // apply the tanh function
    int numSamplesInt = (int)numSamples;
    vvtanhf(output,output,&numSamplesInt);
    
    // shift and scale back into place
    // (hardLimit-softLimit)output + softLimit
    vDSP_vsmsa(output, 1, &scaleUp, softLimit, output, 1, numSamples);
    
    // The positive side of the tanh function applies a gain reduction, so
    // wherever the output of the tanh function is greater than the input, we
    // are on the wrong side of the tanh, and should copy the input to the output
    // directly.
    vDSP_vmin(input, 1, output, 1, output, 1, numSamples);
}



void BMTanhLimiterLower(const float* input, float* output, float* softLimit, float* hardLimit, size_t numSamples){
    // Mathematica prototype:
    //
    // TanhLimitLower[x_,softLimit_,hardLimit_] :=
    //    Max[(hardLimit-softLimit)Tanh[(x/(hardLimit-softLimit)+softLimit/(hardLimit-softLimit))]-softLimit,x]

    //     (x/(hardLimit-softLimit)+softLimit/(hardLimit-softLimit))
    float scaleUp = (*hardLimit-*softLimit);
    float scaleDown = 1.0f / scaleUp;
    float shiftUp = *softLimit * scaleDown;
    vDSP_vsmsa(input, 1, &scaleDown, &shiftUp, output, 1, numSamples);
    
    // apply the tanh function
    int numSamplesInt = (int)numSamples;
    vvtanhf(output,output,&numSamplesInt);
    
    // shift and scale back into place
    // (hardLimit-softLimit)output - softLimit
    float shiftDown = -*softLimit;
    vDSP_vsmsa(output, 1, &scaleUp, &shiftDown, output, 1, numSamples);
    
    // The negative side of the tanh function takes the gain closer to zero, so
    // wherever the output of the tanh function is less than the input, we
    // are on the wrong side of the tanh, and should copy the input to the output
    // directly.
    vDSP_vmax(input, 1, output, 1, output, 1, numSamples);
}
