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
#include "BMLongLoopFDN.h"
#include "BMPanLFO.h"
#include "BMAllpassNestedFilter.h"
#include "BMFIRFilter.h"
#include "BMSimpleDelay.h"
#include "BMSmoothGain.h"

typedef struct BMStereoBuffer{
    void* bufferL;
    void* bufferR;
} BMStereoBuffer;

typedef struct BMCloudReverb {
    BMMultiLevelBiquad biquadFilter;
    BMVelvetNoiseDecorrelator* vndArray;
    
    float** vnd1BufferL;
    float** vnd1BufferR;
    float** vnd2BufferL;
    float** vnd2BufferR;
    size_t numVND;
    size_t numInput;
    float fadeInS;
    
    BMPitchShiftDelay pitchShiftDelay;
    
    //Loopdelay
    BMLongLoopFDN loopFDN;
    
    BMWetDryMixer reverbMixer;
    
    BMStereoBuffer buffer;
    BMStereoBuffer loopInput;
    BMStereoBuffer lastLoopBuffer;
    BMStereoBuffer wetBuffer;
    BMStereoBuffer LFOBuffer;
    
    BMPanLFO inputPan;
    BMPanLFO outputPan;

    float sampleRate;
    float maxTapsEachVND;
    float diffusion;
    bool updateDiffusion;
    float decayTime;
    //Vnd lenght
    bool updateVND;
    bool vndDryTap;
    float vndLength;
    float desiredVNDLength;
    
    float bellQ;
    float lsGain;
    
    int initNo;
    
    BMSmoothGain smoothGain;
} BMCloudReverb;

void BMCloudReverb_init(BMCloudReverb* This,float sr);
void BMCloudReverb_destroy(BMCloudReverb* This);
void BMCloudReverb_processStereo(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples,bool offlineRendering);
//Set
void BMCloudReverb_setLoopDecayTime(BMCloudReverb* This,float decayTime);
void BMCloudReverb_setDelayPitchMixer(BMCloudReverb* This,float wetMix);
void BMCloudReverb_setOutputMixer(BMCloudReverb* This,float wetMix);
void BMCloudReverb_setDiffusion(BMCloudReverb* This,float diffusion);
void BMCloudReverb_setLSGain(BMCloudReverb* This,float gainDb);
void BMCloudReverb_setHighCutFreq(BMCloudReverb* This,float freq);
void BMCloudReverb_setFadeIn(BMCloudReverb* This,float timeInS);
//Test
void BMCloudReverb_impulseResponse(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length);
#endif /* BMCloudReverb_h */
