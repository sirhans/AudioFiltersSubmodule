//
//  BMCloudReverb.h
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 2/27/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMLongReverb_h
#define BMLongReverb_h

#include <stdio.h>
#include "BMVelvetNoiseDecorrelator.h"
#include "BMMultiLevelBiquad.h"
#include "BMPitchShiftDelay.h"
#include "BMSimpleDelay.h"
#include "BMWetDryMixer.h"
#include "BMSmoothGain.h"
#include "BMLongLoopFDN.h"
#include "BMPanLFO.h"
#include "BMHarmonicityMeasure.h"
#include "BMFIRFilter.h"
#include "BMSimpleDelay.h"
#include "BMSmoothGain.h"

typedef struct BMStereoBuffer{
    void* bufferL;
    void* bufferR;
} BMStereoBuffer;

typedef struct BMLongReverb {
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
    
    BMHarmonicityMeasure chordMeasure;
    size_t measureLength;
    size_t measureFillCount;
    float* measureInput;
    float* sfmBuffer;
    size_t sfmIdx;
} BMLongReverb;

void BMLongReverb_init(BMLongReverb* This,float sr);
void BMLongReverb_destroy(BMLongReverb* This);
void BMLongReverb_processStereo(BMLongReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples,bool offlineRendering);
//Set
void BMLongReverb_setLoopDecayTime(BMLongReverb* This,float decayTime);
void BMLongReverb_setDelayPitchMixer(BMLongReverb* This,float wetMix);
void BMLongReverb_setOutputMixer(BMLongReverb* This,float wetMix);
void BMLongReverb_setDiffusion(BMLongReverb* This,float diffusion);
void BMLongReverb_setLSGain(BMLongReverb* This,float gainDb);
void BMLongReverb_setHighCutFreq(BMLongReverb* This,float freq);
void BMLongReverb_setFadeInVND(BMLongReverb* This,float timeInS);
//Test
void BMLongReverb_impulseResponse(BMLongReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length);
#endif /* BMLongReverb_h */
