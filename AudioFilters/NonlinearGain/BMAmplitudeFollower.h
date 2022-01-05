//
//  BMAmplitudeFollower.h
//  BMAUAmpVelocityFilter
//
//  Created by Nguyen Minh Tien on 16/12/2021.
//

#ifndef BMAmplitudeFollower_h
#define BMAmplitudeFollower_h

#include <stdio.h>
#include "Constants.h"
#include "BMEnvelopeFollower.h"

typedef struct BMAmplitudeFollower {
    BMAttackFilter attackFilter;
    BMReleaseFilter releaseFilter;
    float** testBuffers;
    float* instantAttackEnvelope;
    float* slowAttackEnvelope;
    float noiseGateThreshold;
    bool noiseGateIsOpen;
} BMAmplitudeFollower;

void BMAmplitudeFollower_init(BMAmplitudeFollower* This,float sampleRate);
void BMAmplitudeFollower_destroy(BMAmplitudeFollower* This);

void BMAmplitudeFollower_processBuffer(BMAmplitudeFollower* This,float* input,float* envelope,size_t numSamples);

#endif /* BMAmplitudeFollower_h */
