//
//  BMCompressor.h
//  CompressorApp
//
//  Created by Duc Anh on 7/17/18.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMCompressor_h
#define BMCompressor_h

#include <stdio.h>
#include <stdbool.h>
#include "BMMultiLevelBiquad.h"
#include "BMEnvelopeFollower.h"
#include "BMQuadraticThreshold.h"

typedef struct{
    float thresholdInDB, kneeWidthInDB, releaseTime, attackTime,slope;
    BMEnvelopeFollower envelopeFollower;
    BMQuadraticThreshold quadraticThreshold;
    float *buffer1, *buffer2;
} BMCompressor;

void BMCompressor_init(BMCompressor* compressor, float sampleRate);

void BMCompressor_initWithSettings(BMCompressor* compressor, float sampleRate, float thresholdInDB, float kneeWidthInDB, float ratio, float attackTime, float releaseTime);

/*!
 * BMCompressor_ProcessBufferMono
 * @param This         pointer to a compressor struct
 * @param input        input array of length "length"
 * @param output       output array of length "length"
 * @param minGainDb    the lowest gain setting the compressor reached while processing the buffer
 * @param numSamples   length of arrays
 * @brief apply dynamic range compression to input; result in output (MONO)
 * @notes result[i] is 1.0 where X[i] is within limits, 0.0 otherwise
 * @discussion returns floating point output for use in vectorised code without conditional branching
 * @code result[i] = -1.0f * (X[i] >= lowerLimit && X[i] <= upperLimit);
 * @warning no warnings
 */
void BMCompressor_ProcessBufferMono(BMCompressor *This, const float* input, float* output, float* minGainDb, size_t numSamples);


void BMCompressor_ProcessBufferMonoWithSideChain(BMCompressor *This,
												 const float *input,
												 const float *scInput,
												 float* output,
												 float* minGainDb,
												 size_t numSamples);



void BMCompressor_ProcessBufferStereo(BMCompressor* compressor, float* inputL, float* inputR, float* outputL, float* outputR, float* minGainDb, size_t frameCount);

void BMCompressor_SetThresholdInDB(BMCompressor* compressor, float threshold);
void BMCompressor_SetRatio(BMCompressor* compressor, float ratio);
void BMCompressor_SetAttackTime(BMCompressor* compressor, float attackTime);
void BMCompressor_SetReleaseTime(BMCompressor* compressor, float releaseTime);
void BMCompressor_SetSampleRate(BMCompressor* compressor, float sampleRate);
void BMCompressor_SetKneeWidthInDB(BMCompressor* compressor, float kneeWidth);

void BMCompressor_Free(BMCompressor *This);

#endif /* BMCompressor_h */
