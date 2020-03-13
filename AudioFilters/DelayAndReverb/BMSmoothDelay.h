//
//  BMSmoothDelay.h
//  BMAUDimensionChorus
//
//  Created by Nguyen Minh Tien on 1/8/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMSmoothDelay_h
#define BMSmoothDelay_h

#include <stdio.h>
#import "BMLagrangeInterpolation.h"
#include "TPCircularBuffer.h"


#define LGI_Order 4
#define AudioBufferLength 2048

typedef struct BMSmoothDelay {
    TPCircularBuffer buffer;
    size_t sampleRate;
    uint32_t delaySampleRange;
    float delaySpeed;
    float delaySamples;
    float maxDelaySamples;
    int32_t desiredDS;
    int32_t targetDS;
    
    bool shouldUpdateDS;
    
    float* lgiBuffer;
    float* lgiUpBuffer;
    float strideIdx;
    float* strideBuffer;
    float baseIdx;
    size_t storeSamples;
    BMLagrangeInterpolation lgInterpolation;

    float speed;
    size_t sampleToReachTarget;
} BMSmoothDelay;

void BMSmoothDelay_init(BMSmoothDelay* This,size_t defaultDS,float speed,size_t delayRange,size_t sampleRate);
void BMSmoothDelay_destroy(BMSmoothDelay* This);
void BMSmoothDelay_resetToDelaySample(BMSmoothDelay* This,size_t defaultDS);

void BMSmoothDelay_processBuffer(BMSmoothDelay* This,float* inBuffer, float* outBuffer,size_t numSamples);
void BMSmoothDelay_processBufferByStride(BMSmoothDelay* This,float* inBuffer, float* outBuffer,float* strideBuffer,float sampleToConsume,size_t numSamples);
//Set
void BMSmoothDelay_setDelaySample(BMSmoothDelay* This,size_t delaySample);
void BMSmoothDelay_setDelaySpeed(BMSmoothDelay* This,float speed);

bool BMSmoothDelay_reachTarget(BMSmoothDelay* This);
#endif /* BMSmoothDelay_h */
