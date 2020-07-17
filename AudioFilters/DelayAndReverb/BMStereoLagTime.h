//
//  BMStereoLagTime.h
//  BMAUStereoLagTime
//
//  Created by Nguyen Minh Tien on 3/5/19.
//  Copyright © 2019 Nguyen Minh Tien. All rights reserved.
//

#ifndef BMStereoLagTime_h
#define BMStereoLagTime_h

#include <stdio.h>
#include "TPCircularBuffer.h"
#import "BMLagrangeInterpolation.h"
#import "BMSmoothDelay.h"

#define AudioBufferLength 2048

typedef struct{
    float startSamples;
    float stopSamples;
    float sampleToConsume;
    float lastSTC;
    float lastIdx;
    size_t sampleToReachTarget;
    float currentSample;
    float desiredDS;
} StereoLagDelayParam;

typedef struct BMStereoLagTime {
    size_t sampleRate;
    uint32_t maxDelayRange;
    
    //Smooth delay
    BMSmoothDelay delayLeft;
    BMSmoothDelay delayRight;
    float* strideBufferL;
    float* strideBufferR;
    float* strideBufferUnion;
    StereoLagDelayParam delayParamL;
    StereoLagDelayParam delayParamR;
} BMStereoLagTime;

void BMStereoLagTime_init(BMStereoLagTime* This,size_t maxDelaySamples,float speed,size_t sampleRate);
void BMStereoLagTime_destroy(BMStereoLagTime* This);
void BMStereoLagTime_setDelayLeft(BMStereoLagTime* This,size_t delaySample,bool now);
void BMStereoLagTime_setDelayRight(BMStereoLagTime* This,size_t delaySample,bool now);

void BMStereoLagTime_process(BMStereoLagTime* This,float* inL, float* inR, float* outL, float* outR,size_t numSamples);

#endif /* BMStereoLagTime_h */
