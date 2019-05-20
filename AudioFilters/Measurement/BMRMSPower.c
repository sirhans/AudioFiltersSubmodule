//
//  BMRMSPower.c
//  AudioUnitTest
//
//  Created by TienNM on 10/2/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMRMSPower.h"
#include <Accelerate/Accelerate.h>

float BMRMSPower_process(const float* input,size_t processSample){
    float sumSquare = 0;
    vDSP_svesq(input, 1, &sumSquare, processSample);
    return sqrtf(sumSquare/processSample);
}
