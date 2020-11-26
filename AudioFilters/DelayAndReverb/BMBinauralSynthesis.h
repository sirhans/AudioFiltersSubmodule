//
//  BMBinauralSynthesis.h
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 5/6/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMBinauralSynthesis_h
#define BMBinauralSynthesis_h

#include <stdio.h>
#include "BMMultiLevelBiquad.h"
#include <Accelerate/Accelerate.h>

typedef struct BMBinauralSynthesis {
    BMMultiLevelBiquad filter;
    double* coefficient_d;
    double* coefficient_normD;
    float* coefficient_f;
    size_t numChannels;
    
    float sampleRate;
    
    float w0;
    float degree;
    
    float hsGain;
    float hsFreq;
    
    bool needUpdate;
} BMBinauralSynthesis;


void BMBinauralSynthesis_init(BMBinauralSynthesis *This,bool isStereo,float sampleRate);
void BMBinauralSynthesis_destroy(BMBinauralSynthesis *This);

void BMBinauralSynthesis_processMono(BMBinauralSynthesis *This,float* in,float* out, size_t numSamples);
void BMBinauralSynthesis_processStereo(BMBinauralSynthesis *This,float* inL,float* inR, float* outL, float* outR, size_t numSamples);

void BMBinauralSynthesis_setAngleLeft(BMBinauralSynthesis *This,float degree);
void BMBinauralSynthesis_setAngleRight(BMBinauralSynthesis *This,float degree);
void BMBinauralSynthesis_setHSGain(BMBinauralSynthesis *This,float gainDb);

//Test
void BMBinauralSynthesis_getTFMagVectorData(BMBinauralSynthesis *This,float* freq, float* magnitude, size_t length , size_t level);

#endif /* BMBinauralSynthesis_h */
