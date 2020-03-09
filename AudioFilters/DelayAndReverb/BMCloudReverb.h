//
//  BMCloudReverb.h
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 2/27/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMCloudReverb_h
#define BMCloudReverb_h

#include <stdio.h>
#include "BMVelvetNoiseDecorrelator.h"
#include "BMMultiLevelBiquad.h"
#include "BMPitchShiftDelay.h"
#include "BMSimpleDelay.h"
#include "BMWetDryMixer.h"
#include "BMSmoothGain.h"

typedef struct BMStereoBuffer{
    float* bufferL;
    float* bufferR;
} BMStereoBuffer;

typedef struct BMCloudReverb {
    BMMultiLevelBiquad biquadFilter;
    BMVelvetNoiseDecorrelator vnd1;
    BMVelvetNoiseDecorrelator vnd2;
    BMVelvetNoiseDecorrelator vnd3;
    
    BMPitchShiftDelay pitchDelay;
    BMSimpleDelayStereo simpleDelay1;
    BMSimpleDelayStereo simpleDelay2;
    BMSimpleDelayStereo simpleDelay3;
    BMSimpleDelayStereo simpleDelay4;
    float dl1TimeS;
    float dl2TimeS;
    float dl3TimeS;
    float dl4TimeS;
    BMSmoothGain simpleDelay1Gain;
    BMSmoothGain simpleDelay2Gain;
    BMSmoothGain simpleDelay3Gain;
    BMSmoothGain simpleDelay4Gain;
    
    BMWetDryMixer reverbMixer;
    
    BMStereoBuffer buffer;
    BMStereoBuffer loopInput;
    BMStereoBuffer lastLoopBuffer;
    BMStereoBuffer wetBuffer;

    float sampleRate;
    float maxTapsEachVND;
    float diffusion;
    float decayTime;
} BMCloudReverb;

void BMCloudReverb_init(BMCloudReverb* This,float sr);
void BMCloudReverb_destroy(BMCloudReverb* This);
void BMCloudReverb_processStereo(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples);
//Set
void BMCloudReverb_setLoopDecayTime(BMCloudReverb* This,float decayTime);
void BMCloudReverb_setDelayPitchMixer(BMCloudReverb* This,float wetMix);
void BMCloudReverb_setOutputMixer(BMCloudReverb* This,float wetMix);
//Test
void BMCloudReverb_impulseResponse(BMCloudReverb* This,float* outputL,float* outputR,size_t length);
#endif /* BMCloudReverb_h */
