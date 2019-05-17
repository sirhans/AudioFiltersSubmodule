//
//  BMEnvelopeFollower.h
//  AudioFiltersXCodeProject
//
//  Created by hans anderson on 2/26/19.
//
//  Anyone may use this file without restrictions.
//

#ifndef BMEnvelopeFollower_h
#define BMEnvelopeFollower_h

#include <stdio.h>
#include <stdbool.h>
#include "BMDynamicSmoothingFilter.h"

#define BMENV_NUM_STAGES 3


/*
 * These smoothing filters are based on the design by Andrew Simper:
 * https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
 *
 * We have modified his design so that the attack and release portions of
 * the smoothing operation are done with separate filters that can be configured
 * with differing cutoff frequencies and Q factors. When these filters switch
 * into operation, they set their internal state variables so that the function
 * value and gradient remain continuous.
 *
 * the struct below belongs to a smoothing filter that works only while the
 * signal amplitude is decreasing
 */
typedef struct BMReleaseFilter {
    float sampleRate, fc;
    float ic1, ic2;
    float g,k;
    float a1, a2, a3;
    float previousOutputValue;
    bool attackMode;
} BMReleaseFilter;


/*
 * A smoothing filter that only works while the signal amplitude is increasing
 */
typedef struct BMAttackFilter {
    float sampleRate, fc;
    float ic1, ic2;
    float g,gInv_2,k;
    float a1, a2, a3;
    float previousOutputValue, previousOutputGradient;
    bool attackMode;
} BMAttackFilter;


typedef struct BMEnvelopeFollower {
    // filters for standandard envelopes
    BMReleaseFilter releaseFilters [BMENV_NUM_STAGES];
    BMAttackFilter  attackFilters [BMENV_NUM_STAGES];
} BMEnvelopeFollower;


/*!
 * BMEnvelopeFollower_processBuffer
 */
void BMEnvelopeFollower_processBuffer(BMEnvelopeFollower* This,
                                      const float* input,
                                      float* output,
                                      size_t numSamples);

/*!
 * BMEnvelopeFollower_init
 */
void BMEnvelopeFollower_init(BMEnvelopeFollower* This, float sampleRate);

/*!
 * BMEnvelopeFollower_setAttackTime
 */
void BMEnvelopeFollower_setAttackTime(BMEnvelopeFollower* This, float attackTime);

/*!
 * BMEnvelopeFollower_setReleaseTime
 */
void BMEnvelopeFollower_setReleaseTime(BMEnvelopeFollower* This, float releaseTime);

/*!
 * BMAttackFilter_init
 */
void BMAttackFilter_init(BMAttackFilter* This, float fc, float sampleRate);

/*!
 * BMReleaseFilter_init
 */
void BMReleaseFilter_init(BMReleaseFilter* This, float fc, float sampleRate);

/*!
 * BMAttackFilter_processBuffer
 */
void BMAttackFilter_processBuffer(BMAttackFilter* This,
                                  const float* input,
                                  float* output,
                                  size_t numSamples);

/*!
 * BMReleaseFilter_processBuffer
 */
void BMReleaseFilter_processBuffer(BMReleaseFilter* This,
                                   const float* input,
                                   float* output,
                                   size_t numSamples);

/*!
 * BMAttackFilter_setCutoff
 */
void BMAttackFilter_setCutoff(BMAttackFilter* This, float fc);


/*!
 * BMReleaseFilter_setCutoff
 */
void BMReleaseFilter_setCutoff(BMReleaseFilter* This, float fc);


/*!
 * BMReleaseFilter_updateSampleRate
 */
void BMReleaseFilter_updateSampleRate(BMReleaseFilter* This, float sampleRate);


/*!
 * BMAttackFilter_updateSampleRate
 */
void BMAttackFilter_updateSampleRate(BMAttackFilter* This, float sampleRate);


/*!
 * ARTimeToCutoffFrequency
 * @param time       time in seconds
 * @param numStages  number of stages in the filter cascade
 * @abstract The -3db point of the filters shifts to a lower frequency each time we add another filter to the cascade. This function converts from time to cutoff frequency, taking into account the number of filters in the cascade.
 */
float ARTimeToCutoffFrequency(float time, size_t numStages);






#endif /* BMEnvelopeFollower_h */
