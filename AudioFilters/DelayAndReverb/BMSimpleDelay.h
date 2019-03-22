//
//  BMSimpleDelay.h
//  AudioFiltersXCodeProject
//
//  This class should be used when you need to implement a delay that does nothing other than
//  delay the signal as efficiently as possible (no feedback, no modulation, no multi-tap)
//
//
//  Created by hans on 31/1/19.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMSimpleDelay_h
#define BMSimpleDelay_h

#include <stdio.h>
#include "TPCircularBuffer+AudioBufferList.h"

typedef struct BMSimpleDelayMono {
    TPCircularBuffer buffer;
} BMSimpleDelayMono;

typedef struct BMSimpleDelayStereo {
    TPCircularBuffer bufferL, bufferR;
} BMSimpleDelayStereo;

void BMSimpleDelayMono_init(BMSimpleDelayMono* This, size_t delayTimeInSamples);
void BMSimpleDelayStereo_init(BMSimpleDelayStereo* This, size_t delayTimeInSamples);

void BMSimpleDelayMono_process(BMSimpleDelayMono* This,
                               const float* input,
                               float* output,
                               size_t numSamples);

void BMSimpleDelayStereo_process(BMSimpleDelayStereo* This,
                                 const float* inputL,
                                 const float* inputR,
                                 float* outputL,
                                 float* outputR,
                                 size_t numSamples);

void BMSimpleDelayMono_destroy(BMSimpleDelayMono* This);
void BMSimpleDelayStereo_destroy(BMSimpleDelayStereo* This);

#endif /* BMSimpleDelay_h */
