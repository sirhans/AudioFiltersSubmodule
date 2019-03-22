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

#define LGI_Order 10
#define AudioBufferLength 2048

typedef struct BMStereoLagTime {
    TPCircularBuffer bufferL;
    TPCircularBuffer bufferR;
    bool isDelayLeftChannel;
    size_t sampleRate;
    uint32_t maxDelaySamples;
    float delaySamplesL;
    float delaySamplesR;
    int32_t desiredDSL;
    int32_t desiredDSR;
    int32_t targetDSL;
    int32_t targetDSR;
    
    bool shouldUpdateDS;
    
    float* lgiStrideIdx;
    float* lgiBufferL;
    float* lgiBufferR;
    float* lgiUpBuffer;
    float strideIdxL;
    float strideIdxR;
    BMLagrangeInterpolation lgInterpolation;
} BMStereoLagTime;

void BMStereoLagTime_init(BMStereoLagTime* This,size_t maxDelayTimeInMilSecond,size_t sampleRate);
void BMStereoLagTime_destroy(BMStereoLagTime* This);
void BMStereoLagTime_setDelayLeft(BMStereoLagTime* This,size_t delayInMilSecond);
void BMStereoLagTime_setDelayRight(BMStereoLagTime* This,size_t delayInMilSecond);

void BMStereoLagTime_process(BMStereoLagTime* This,float* inL, float* inR, float* outL, float* outR,size_t numSamples);

#endif /* BMStereoLagTime_h */
