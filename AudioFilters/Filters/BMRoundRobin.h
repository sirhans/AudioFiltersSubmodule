//
//  BMRoundRobin.h
//  ETSax
//
//  Created by Duc Anh on 6/22/17.
//  Anyone may use this file without restrictions
//

#ifndef BMRoundRobin_h
#define BMRoundRobin_h

#include <stdio.h>
#include "BMMultiTapDelay.h"
#include "Constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BMRoundRobinSetting {
    size_t* indices;
    float* gain;
    float* tempStorage;
    
    size_t numberOfChannel;
    size_t numberTaps;
    float sampleRate;
    float stableFrequencyWish;
    float bufferLengthInFrames;
    float bufferLengthInMs;

} BMRoundRobinSetting;


typedef struct BMRoundRobin {
    BMMultiTapDelay delay;
    
    BMRoundRobinSetting setting;
} BMRoundRobin;


void BMRoundRobin_init(BMRoundRobin* This,
                       float sampleRate,
                       float stableFrequencyWish,
                       size_t numTapsPerChannel);


/*
 * free memory used by the struct at *This
 */
void BMRoundRobin_destroy(BMRoundRobin* This);


void BMRoundRobin_processBufferStereo(BMRoundRobin* This,
                                      float* inputL, float* inputR,
                                      float* outputL, float* outputR,
                                      size_t numSamples);

void BMRoundRobin_processBufferMono(BMRoundRobin* This,
                                      float* input, float* output,
                                      size_t numSamples);

void BMRoundRobin_RegenIndicesAndGain(BMRoundRobin* This);

void BMRoundRobin_NewNote(BMRoundRobin* This);

void BMRoundRobin_testImpulseResponse(BMRoundRobin* This);
    
#ifdef __cplusplus
}
#endif

#endif /* BMRoundRobin_h */




