//
//  BMSimpleEnvelopeFollower.h
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 05/11/2021.
//  Copyright © 2021 BlueMangoo. All rights reserved.
//

#ifndef BMSimpleEnvelopeFollower_h
#define BMSimpleEnvelopeFollower_h

#include <stdio.h>

typedef struct BMSimpleEnvelopeFollower {
    float attack;
    float release;
    float lastOutputV;
}BMSimpleEnvelopeFollower;

void BMSimpleEnvelopeFollower_init(BMSimpleEnvelopeFollower* This);
void BMSimpleEnvelopeFollower_destroy(BMSimpleEnvelopeFollower* This);
void BMSimpleEnvelopeFollower_setAttack(BMSimpleEnvelopeFollower* This,float amount);
void BMSimpleEnvelopeFollower_setRelease(BMSimpleEnvelopeFollower* This,float amount);
void BMSimpleEnvelopeFollower_processBuffer(BMSimpleEnvelopeFollower* This,float* input,float* output,size_t numSamples);

#endif /* BMSimpleEnvelopeFollower_h */
