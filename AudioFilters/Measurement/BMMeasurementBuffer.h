//
//  BMMeasurementBuffer.h
//  AudioFiltersXcodeProject
//
//  This is a buffer for taking spectral measurements on a live audio stream
//  where the normal buffer length is not guaranteed to be equal to the fft
//  length we want to use. It buffers input from short audio buffers into a
//  longer buffer using a function that should be called with every incoming
//  buffer and provides functions for extracting longer buffers of samples.
//
//  Created by Hans on 11/3/20.
//  This file is public domain. No restrictions.
//

#ifndef BMMeasurementBuffer_h
#define BMMeasurementBuffer_h

#include <stdio.h>
#include "TPCircularBuffer.h"

typedef struct BMMeasurmentBuffer {
    TPCircularBuffer buffer;
} BMMeasurementBuffer;

void BMMeasurementBuffer_init(BMMeasurementBuffer *This, size_t lengthInSamples);
void BMMeasurementBuffer_free(BMMeasurementBuffer *This);

void BMMeasurementBuffer_inputSamples(BMMeasurementBuffer *This,
                                      const float* input,
                                      size_t numSamples);
float* BMMeasurementBuffer_getCurrentPointer(BMMeasurementBuffer *This);
#endif /* BMMeasurementBuffer_h */
