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
    float sampleRate;
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
    float sampleRate;
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


typedef struct BMTransientEnveloper {
    // filters for transient attack envelopes
    BMReleaseFilter attackRF1 [BMENV_NUM_STAGES];
    BMReleaseFilter attackRF2 [BMENV_NUM_STAGES];
    BMAttackFilter  attackAF1 [BMENV_NUM_STAGES];
    BMAttackFilter  attackAF2 [BMENV_NUM_STAGES];
    BMDynamicSmoothingFilter attackDSF;
    bool filterAttackOnset;
    
    // filters for transient release envelopes
    BMReleaseFilter releaseRF1 [BMENV_NUM_STAGES];
    BMReleaseFilter releaseRF2;
    BMDynamicSmoothingFilter releaseDSF;
    
    // filters for after-attack envelopes
} BMTransientEnveloper;

void BMEnvelopeFollower_processBuffer(BMEnvelopeFollower* This,
                                      const float* input,
                                      float* output,
                                      size_t numSamples);

void BMEnvelopeFollower_init(BMEnvelopeFollower* This, float sampleRate);

void BMEnvelopeFollower_setAttackTime(BMEnvelopeFollower* This, float attackTime);

void BMEnvelopeFollower_setReleaseTime(BMEnvelopeFollower* This, float releaseTime);

/*!
 * BMTransientEnveloper_processBuffer
 * @abstract used as a component of a transient shaper
 *
 * @param This             pointer to an initialised BMEnvelopeFollower struct
 * @param input            single channel input array
 * @param attackEnvelope   positive values indicate that we are in the attack portion. output values in range: [0,input]
 * @param afterAttackEnvelope positive values indicate that we are in the portion immediately after the attack range: [0,+?]
 * @param releaseEnvelope  positive values indicate that we are in the release portion. output values in range: [0,input]
 */
void BMTransientEnveloper_processBuffer(BMTransientEnveloper* This,
                                           const float* input,
                                           float* attackEnvelope,
                                           float* afterAttackEnvelope,
                                           float* releaseEnvelope,
                                           size_t numSamples);

void BMTransientEnveloper_setAttackOnsetTime(BMTransientEnveloper* This, float seconds);

void BMTransientEnveloper_setAttackDuration(BMTransientEnveloper* This, float seconds);

void BMTransientEnveloper_setReleaseDuration(BMEnvelopeFollower* This, float seconds);

#endif /* BMEnvelopeFollower_h */
