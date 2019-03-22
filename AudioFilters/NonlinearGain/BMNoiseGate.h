//
//  BMNoiseGate.h
//  AudioUnitTest
//
//  Created by TienNM on 9/25/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMNoiseGate_h
#define BMNoiseGate_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct BMNoiseGate {
    float threshold;
    float decayTime;
    float preVolume;
    float volume;
    float k;
    float fadeCount_Current;
    float fadeCount_Max;
} BMNoiseGate;

void BMNoiseGate_init(BMNoiseGate* this,float threshold,float decayTime,float sampleRate);
void BMNoiseGate_process(BMNoiseGate* this,float* input,float* output,size_t numSamplesIn);
void BMNoiseGate_free(BMNoiseGate* This);
void BMNoiseGate_setDecayTime(BMNoiseGate* this,float decayTime,float sampleRate);
void BMNoiseGate_setThreshold(BMNoiseGate* this,float thres);
    
#ifdef __cplusplus
}
#endif

#endif /* BMNoiseGate_h */
