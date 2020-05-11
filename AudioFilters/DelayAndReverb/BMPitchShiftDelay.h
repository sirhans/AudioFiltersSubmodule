//
//  BMPitchShiftDelay.h
//  BMAUDimensionChorus
//
//  Created by Nguyen Minh Tien on 1/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMPitchShiftDelay_h
#define BMPitchShiftDelay_h

#include <stdio.h>
#include "BMSmoothDelay.h"
#include "BMSmoothGain.h"
#include "BMMultiLevelBiquad.h"
#include "BMSmoothSwitch.h"

#define ReadyNo 98573

typedef struct{
    float startSamples;
    float stopSamples;
    float sampleToConsume;
    float lastSTC;
    float lastIdx;
} DelayParam;

typedef struct BMPitchShiftDelay {
    float duration;
    size_t sampleRate;
    float delayRange;
    float maxDelayRange;
    float mixToOtherValue;
    
    float* wetBufferL;
    float* wetBufferR;
    float* tempBufferL;
    float* tempBufferR;
    //Smooth delay
    BMSmoothDelay delayLeft;
    BMSmoothDelay delayRight;
    float* strideBufferL;
    float* strideBufferR;
    size_t sampleToReachTarget;
    float currentSample;
    DelayParam delayParamL;
    DelayParam delayParamR;
    
    BMSmoothGain wetGain;
    BMSmoothGain dryGain;
    float wetDirectGain;
    
    BMMultiLevelBiquad midBandFilter;
    BMMultiLevelBiquad midBandAfterDelayFilter;
        
    BMSmoothSwitch offSwitchL;
    BMSmoothSwitch offSwitchR;
    bool resetFilter;
    int initNo;
} BMPitchShiftDelay;

/*
 * Simulate Dimension Chorus effect
 * speed : calculate in second
 * delayRange : max delay sample range
 */
void BMPitchShiftDelay_init(BMPitchShiftDelay* This,float duration,size_t delayRange,size_t maxDelayRange,size_t sampleRate,bool startAtMaxRange);
void BMPitchShiftDelay_destroy(BMPitchShiftDelay* This);

void BMPitchShiftDelay_processStereoBuffer(BMPitchShiftDelay* This,float* inL, float* inR, float* outL, float* outR,size_t numSamples);
void BMPitchShiftDelay_processMonoBuffer(BMPitchShiftDelay* This,float* input, float* output, size_t numSamples);

void BMPitchShiftDelay_setWetGain(BMPitchShiftDelay* This,float vol);
void BMPitchShiftDelay_setDelayRange(BMPitchShiftDelay* This,size_t newDelayRange);
void BMPitchShiftDelay_setBandLowpass(BMPitchShiftDelay* This,float fc);
void BMPitchShiftDelay_setBandHighpass(BMPitchShiftDelay* This,float fc);
void BMPitchShiftDelay_setMixOtherChannel(BMPitchShiftDelay* This,float value);
void BMPitchShiftDelay_setDelayDuration(BMPitchShiftDelay* This,float duration);
//void BMPitchShiftDelay_setSampleRate(BMPitchShiftDelay* This,size_t sr);
#endif /* BMPitchShiftDelay_h */
