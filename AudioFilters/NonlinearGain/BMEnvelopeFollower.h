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
 * These smoothing filters are based on the dBMAttackFilter_setCutoffs://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
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
    float sampleRate, fc,fcMin,fcMax,fcCross;
    float minDb,maxDb;
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
    float localReleaseValue;
    float localPeakValue;
    bool attackMode;
} BMAttackFilter;




typedef struct BMEnvelopeFollower {
    // filters for standandard envelopes
    BMReleaseFilter* releaseFilters;
    BMAttackFilter*  attackFilters;
    size_t numReleaseStages, numAttackStages;
    bool processAttack;
} BMEnvelopeFollower;


/*!
 * BMEnvelopeFollower_processBuffer
 */
void BMEnvelopeFollower_processBuffer(BMEnvelopeFollower *This,
                                      const float* input,
                                      float* output,
                                      size_t numSamples);

/*!
 * BMEnvelopeFollower_init
 */
void BMEnvelopeFollower_init(BMEnvelopeFollower *This, float sampleRate);


/*!
 * BMEnvelopeFollower_free
 */
void BMEnvelopeFollower_free(BMEnvelopeFollower *This);



/*!
 * BMEnvelopeFollower_initWithCustomNumStages
 */
void BMEnvelopeFollower_initWithCustomNumStages(BMEnvelopeFollower *This, size_t numReleaseStages, size_t numAttackStages, float sampleRate);



/*!
 * BMEnvelopeFollower_setAttackTime
 */
void BMEnvelopeFollower_setAttackTime(BMEnvelopeFollower *This, float attackTimeSeconds);

/*!
 * BMEnvelopeFollower_setReleaseTime
 */
void BMEnvelopeFollower_setReleaseTime(BMEnvelopeFollower *This, float releaseTimeSeconds);

/*!
 * BMAttackFilter_init
 */
void BMAttackFilter_init(BMAttackFilter *This, float fc, float sampleRate);

/*!
 * BMReleaseFilter_init
 */
void BMReleaseFilter_init(BMReleaseFilter *This, float fc, float sampleRate);

/*!
 * BMAttackFilter_processBuffer
 */
void BMAttackFilter_processBuffer(BMAttackFilter *This,
                                  const float* input,
                                  float* output,
                                  size_t numSamples);

void BMAttackFilter_processBufferLP(BMAttackFilter *This,
                                  const float* input,
                                  float* output,
                                    size_t numSamples);
/*!
 * BMReleaseFilter_processBuffer
 */
void BMReleaseFilter_processBuffer(BMReleaseFilter *This,
                                   const float* input,
                                   float* output,
                                   size_t numSamples);

void BMReleaseFilter_processBufferDynamic(BMReleaseFilter *This,
                                   const float* input,
                                   float* output,
                                   float* standard,
                                   size_t numSamples);
/*!
 * BMReleaseFilter_processBufferNegative
 *
 * @abstract inverts the signal before and after processing
 */
void BMReleaseFilter_processBufferNegative(BMReleaseFilter *This,
                                   const float* input,
                                   float* output,
                                   size_t numSamples);


/*!
 * BMAttackFilter_setCutoff
 */
void BMAttackFilter_setCutoff(BMAttackFilter *This, float fc);


/*!
 * BMReleaseFilter_setCutoff
 */
void BMReleaseFilter_setCutoff(BMReleaseFilter *This, float fc);
void BMReleaseFilter_setCutoffRange(BMReleaseFilter *This, float min,float cross,float max);
void BMReleaseFilter_setDBRange(BMReleaseFilter *This, float min,float max);
/*!
 * BMReleaseFilter_updateSampleRate
 */
void BMReleaseFilter_updateSampleRate(BMReleaseFilter *This, float sampleRate);


/*!
 * BMAttackFilter_updateSampleRate
 */
void BMAttackFilter_updateSampleRate(BMAttackFilter *This, float sampleRate);


/*!
 * ARTimeToCutoffFrequency
 * @param time       time in seconds
 * @param numStages  number of stages in the filter cascade
 * @abstract The -3db point of the filters shifts to a lower frequency each time we add another filter to the cascade. This function converts from time to cutoff frequency, taking into account the number of filters in the cascade.
 */
float ARTimeToCutoffFrequency(float time, size_t numStages);

/*
 BMFirstOrderAttackFilter
 */

typedef struct BMFOAttackFilter {
    float z1_f, b0_f, b1_f, a1_f, az_f;
    float sampleRate, fc;
    float previousOutputValue;
    bool attackMode;
} BMFOAttackFilter;

void BMFOAttackFilter_init(BMFOAttackFilter* This,float fc, float sampleRate);
void BMFOAttackFilter_setCutoff(BMFOAttackFilter* This, float fc);
void BMFOAttackFilter_processBuffer(BMFOAttackFilter* This,
                                    const float* input,
                                    float* output,
                                    size_t numSamples);

/*
 BMFirstOrderReleaseFilter
 */

typedef struct BMFOReleaseFilter {
    float z1_f, b0_f, b1_f, a1_f, az_f;
    float sampleRate, fc;
    float previousOutputValue;
    bool attackMode;
} BMFOReleaseFilter;

void BMFOReleaseFilter_init(BMFOReleaseFilter* This,float fc, float sampleRate);
void BMFOReleaseFilter_setCutoff(BMFOReleaseFilter* This, float fc);
void BMFOReleaseFilter_processBuffer(BMFOReleaseFilter* This,
                                    const float* input,
                                    float* output,
                                    size_t numSamples);

/*
 BMFOAttackFilter
 */
void BMFOAttackFilter_init(BMFOAttackFilter* This,float fc, float sampleRate);
void BMFOAttackFilter_setLowpassFirstOrder(BMFOAttackFilter* This, float fc);
void BMFOAttackFilter_processBuffer(BMFOAttackFilter* This,
                                    const float* input,
                                    float* output,
                                    size_t numSamples);

/*
 BMMultiReleaseFilter
 */

typedef struct BMMultiReleaseFilter {
    BMReleaseFilter* filters;
    BMFOReleaseFilter* foFilters;
    int numLayers;
}BMMultiReleaseFilter;

void BMMultiReleaseFilter_init(BMMultiReleaseFilter *This, float fc,int numLayers, float sampleRate);
void BMMultiReleaseFilter_destroy(BMMultiReleaseFilter *This);

void BMMultiReleaseFilter_setCutoff(BMMultiReleaseFilter *This, float fc);
void BMMultiReleaseFilter_setCutoffRange(BMMultiReleaseFilter *This, float min,float cross,float max);
void BMMultiReleaseFilter_setDBRange(BMMultiReleaseFilter *This, float min,float max);

void BMMultiReleaseFilter_processBuffer(BMMultiReleaseFilter *This,
                                   const float* input,
                                   float* output,
                                        size_t numSamples);
void BMMultiReleaseFilter_processBufferFO(BMMultiReleaseFilter *This,
                                   const float* input,
                                   float* output,
                                        size_t numSamples);
void BMMultiReleaseFilter_processBufferDynamic(BMMultiReleaseFilter *This,
                                   const float* input,
                                   float* output,
                                   float* standard,
                                   size_t numSamples);


/*
 BMMultiAttackFilter
 */

typedef struct BMMultiAttackFilter {
    BMAttackFilter* filters;
    BMFOAttackFilter* foFilters;
    int numLayers;
}BMMultiAttackFilter;

void BMMultiAttackFilter_init(BMMultiAttackFilter *This, float fc,int numLayers, float sampleRate);
void BMMultiAttackFilter_destroy(BMMultiAttackFilter *This);

void BMMultiAttackFilter_setCutoff(BMMultiAttackFilter *This, float fc);

void BMMultiAttackFilter_processBuffer(BMMultiAttackFilter *This,
                                  const float* input,
                                  float* output,
                                       size_t numSamples);
void BMMultiAttackFilter_processBufferFO(BMMultiAttackFilter *This,
                                  const float* input,
                                  float* output,
                                         size_t numSamples);



#endif /* BMEnvelopeFollower_h */
