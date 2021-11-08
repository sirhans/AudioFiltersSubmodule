//
//  BMSimpleEnvelopeFollower.c
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 05/11/2021.
//  Copyright © 2021 BlueMangoo. All rights reserved.
//

#include "BMSimpleEnvelopeFollower.h"

void BMSimpleEnvelopeFollower_init(BMSimpleEnvelopeFollower* This){
    This->lastOutputV = 0;
}

void BMSimpleEnvelopeFollower_destroy(BMSimpleEnvelopeFollower* This){
    
}

void BMSimpleEnvelopeFollower_setAttack(BMSimpleEnvelopeFollower* This,float amount){
    This->attack = amount;
}

void BMSimpleEnvelopeFollower_setRelease(BMSimpleEnvelopeFollower* This,float amount){
    This->release = amount;
}

void BMSimpleEnvelopeFollower_processBuffer(BMSimpleEnvelopeFollower* This,float* input,float* output,size_t numSamples){
    for(int i=0;i<numSamples;i++){
        if(This->lastOutputV > input[i])
        {
          output[i] = (1 - This->release) * input[i] + This->release * output[i-1];//release phase
        }
        else
        {
          output[i] = (1 - This->attack) * input[i] + This->attack * output[i-1];//attack phase
        }
        This->lastOutputV = output[i];
    }
}
