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
#include "BMAttackShaper.h"
#include "BMSpectrum.h"
#include "BMMeasurementBuffer.h"
#include "BMSmoothFade.h"

typedef struct BMStereoBuffer{
    void* bufferL;
    void* bufferR;
} BMStereoBuffer;



typedef struct BMLongReverbUnit {
    BMMultiLevelBiquad biquadFilter;
    BMVelvetNoiseDecorrelator* vndArray;
    
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
    BMStereoBuffer lastWetBuffer;
    
    BMPanLFO inputPan;
    BMPanLFO outputPan;
    BMSmoothGain smoothGain;
    
    float diffusion;
    bool updateDiffusion;
    float decayTime;
    //Vnd lenght
    bool updateVND;
    bool vndDryTap;
    float vndLength;
    float desiredVNDLength;
    
    BMHarmonicityMeasure chordMeasure;
    
    size_t measureFillCount;
    
    //Attack softener
    BMMultibandAttackShaper attackSoftener;
    BMSpectrum measureSpectrum;
    size_t measureSamples;
    BMMeasurementBuffer measureDryInput;
    BMMeasurementBuffer measureWetOutput;
    float* measureSpectrumDryBuffer;
    float* measureSpectrumWetBuffer;
    float currentDecayTime;
    
    float* outputL;
    float* outputR;
} BMLongReverbUnit;

typedef struct{
    BMLongReverbUnit* reverb;
    
    float** vnd1BufferL;
    float** vnd1BufferR;
    float** vnd2BufferL;
    float** vnd2BufferR;
    float* dryInputL;
    float* dryInputR;
    
    size_t numVND;
    size_t numInput;
    float sampleRate;
    float maxTapsEachVND;
    
    float bellQ;
    float lsGain;
    
    size_t measureLength;
    int reverbActiveIdx;
    int updateReverbCount;
    int verifyReverbChangeCount;
    BMSmoothFade fadeIn;
    BMSmoothFade fadeOut;
    size_t fadeSamples;
    size_t changeReverbDelaySamples;
    size_t changeReverbCurrentSamples;
    
    float reverbMeasureThreshold;
    
    int initNo;
}BMLongReverb;

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

//Measurement
void BMLongReverb_setReverbMeasureThreshold(BMLongReverb* This,float threshold);
//Test
void BMLongReverb_impulseResponse(BMLongReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t length);
#endif /* BMLongReverb_h */
