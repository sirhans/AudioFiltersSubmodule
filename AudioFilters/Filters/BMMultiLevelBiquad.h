//
//  BMMultiLevelBiquad.h
//  VelocityFilter
//
//  Created by Hans on 14/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMMultiLevelBiquad_h
#define BMMultiLevelBiquad_h

#include <stdio.h>
#include <Accelerate/Accelerate.h>
#include "BMSmoothGain.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BMMultiLevelBiquad {
    // dynamic memory
    vDSP_biquadm_Setup multiChannelFilterSetup;
    vDSP_biquad_Setup singleChannelFilterSetup;
    float* monoDelays;
    double* coefficients_d;
    float* coefficients_f;
    
    // static memory
    //float currentGain;
    //float desiredGain;
    size_t numLevels;
    size_t numChannels;
    double sampleRate;
    bool needsUpdate, useRealTimeUpdate, useBiquadm,useSmoothUpdate,needUpdateActiveLevels;
    bool *activeLevels;
    BMSmoothGain gain, gain2;
} BMMultiLevelBiquad;



/*!
 *BMMultiLevelBiquad_processBufferStereo
 */
void BMMultiLevelBiquad_processBufferStereo(BMMultiLevelBiquad* bqf, const float* inL, const float* inR, float* outL, float* outR, size_t numSamples);



/*!
 *BMMultiLevelBiquad_processBuffer4
 */
void BMMultiLevelBiquad_processBuffer4(BMMultiLevelBiquad* bqf,
                                       const float* in1, const float* in2, const float* in3, const float* in4,
                                       float* out1, float* out2, float* out3, float* out4,
                                       size_t numSamples);

/*!
 *BMMultiLevelBiquad_processBufferMono
 */
void BMMultiLevelBiquad_processBufferMono(BMMultiLevelBiquad* bqf, const float* input, float* output, size_t numSamples);

/*!
 *BMMultiLevelBiquad_init
 * @Abstract init must be called once before using the filter.  To change the number of levels in the fitler, call destroy first, then call this function with the new number of levels
 *
 * @param bqf pointer to an initialized filter struct
 * @param numLevels the number of biquad filters in the cascade
 * @param sampleRate audio sample rate
 * @param isStereo set true for stereo, false for mono
 * @param monoRealTimeUpdate If you are updating coefficients of a MONO filter in realtime, set this to true. Processing of audio is slightly slower, but updates can happen in realtime. This setting has no effect on stereo filters. This setting has no effect if the OS does not support realtime updates of vDSP_biquadm filter coefficients.
 *
 * @param smoothUpdate :    When BMMultilevelBiquad is init with smooth updates on, the update function will call setTargetsDouble to enable smooth update; and when it's off it will call setCoefficientsDouble.
 *
 */
void BMMultiLevelBiquad_init(BMMultiLevelBiquad* bqf,
                             size_t numLevels,
                             float sampleRate,
                             bool isStereo,
                             bool monoRealTimeUpdate,
                             bool smoothUpdate);


/*!
 *BMMultiLevelBiquad_init
 * @Abstract init must be called once before using the filter.  To change the number of levels in the fitler, call destroy first, then call this function with the new number of levels
 *
 * @param bqf           pointer to an initialized filter struct
 * @param numLevels     the number of biquad filters in the cascade
 * @param sampleRate    audio sample rate
 * @param smoothUpdate  When BMMultilevelBiquad is init with smooth updates on, the update function will call setTargetsDouble to enable smooth update; and when it's off it will call setCoefficientsDouble.
 *
 */
void BMMultiLevelBiquad_init4(BMMultiLevelBiquad* bqf,
                              size_t numLevels,
                              float sampleRate,
                              bool smoothUpdate);


// free up memory objects
void BMMultiLevelBiquad_destroy(BMMultiLevelBiquad* bqf);


// set a bell-shape filter at on the specified level in both channels
// and update filter settings
void BMMultiLevelBiquad_setBell(BMMultiLevelBiquad* bqf, float fc, float bandwidth, float gain_db, size_t level);

/*!
 * BMMultiLevelBiquad_setNormalizedBell
 *
 * @abstract This is based on the setBell function above. The difference is that it attempts to keep the overall gain at unity by adjusting the broadband gain to compensate for the boost or cut of the bell filter. This allows us to acheive extreme filter curves that approach the behavior of bandpas and notch filters without clipping or loosing too much signal gain.
 * @param bqf pointer to an initialized struct
 * @param fc bell filter cutoff frequency in Hz
 * @param bandwidth bell filter bandwidth in Hz
 * @param controlGainDb the actual post-compensation gain of the filter at fc
 * @param level the level number in the multilevel biquad struct
 */
void BMMultiLevelBiquad_normalizedBell(BMMultiLevelBiquad* bqf, float fc, float bandwidth, float controlGainDb, size_t level);


// set a high shelf filter at on the specified level in both
// channels and update filter settings
void BMMultiLevelBiquad_setHighShelf(BMMultiLevelBiquad* bqf, float fc, float gain_db, size_t level);

/*!
 *BMMultiLevelBiquad_setHighShelfAdjustableSlope
 *
 * @param bqf       pointer to initialized struct
 * @param fc        corner frequency
 * @param gain_db   gain cut or boost in decibels
 * @param slope     slope in [0,1].
 * @param level     biquad section number (counting from 0)
 */
void BMMultiLevelBiquad_setHighShelfAdjustableSlope(BMMultiLevelBiquad* bqf, float fc, float gain_db, float slope, size_t level);

/*!
 *BMMultiLevelBiquad_setHighShelfFirstOrder
 */
void BMMultiLevelBiquad_setHighShelfFirstOrder(BMMultiLevelBiquad* bqf, float fc, float gain_db, size_t level);

/*!
 *BMMultiLevelBiquad_setLowShelfFirstOrder
 */
void BMMultiLevelBiquad_setLowShelfFirstOrder(BMMultiLevelBiquad* bqf, float fc, float gain_db, size_t level);

// set a low shelf filter at on the specified level in both
// channels and update filter settings
void BMMultiLevelBiquad_setLowShelf(BMMultiLevelBiquad* bqf, float fc, float gain_db, size_t level);

void BMMultiLevelBiquad_setLowPass12db(BMMultiLevelBiquad* bqf, double fc, size_t level);

void BMMultiLevelBiquad_setLowPassQ12db(BMMultiLevelBiquad* bqf, double fc,double q, size_t level);

void BMMultiLevelBiquad_setHighPass12db(BMMultiLevelBiquad* bqf, double fc, size_t level);

/*!
 *BMMultiLevelBiquad_setHighPass12dbNeg
 *
 * @abstract negated second-order butterworth highpass for butterworth crossover
 */
void BMMultiLevelBiquad_setHighPass12dbNeg(BMMultiLevelBiquad* bqf, double fc, size_t level);

void BMMultiLevelBiquad_setHighPassQ12db(BMMultiLevelBiquad* bqf, double fc,double q,size_t level);

/*
 * sets up a butterworth filter of order 2*numLevels
 *
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use (order / 2)
 */
void BMMultiLevelBiquad_setHighOrderBWLP(BMMultiLevelBiquad* bqf, double fc, size_t firstLevel, size_t numLevels);






/*!
 * BMMultiLevelBiquad_setLegendreLP
 *
 * @abstract sets up a Legendre Lowpass filter of order 2*numLevels
 * @discussion Legendre lowpass fitlers have steeper cutoff than Butterworth, with nearly flat passband
 *
 * @param fc           cutoff frequency
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use. numLevels = (filterOrder / 2)
 */
void BMMultiLevelBiquad_setLegendreLP(BMMultiLevelBiquad* bqf, double fc, size_t firstLevel, size_t numLevels);






/*!
 * BMMultiLevelBiquad_setCriticallyDampedLP
 *
 * @abstract sets up a Critically-damped Lowpass filter of order 2*numLevels
 *
 * @param fc           cutoff frequency
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use. numLevels = (filterOrder / 2)
 */
void BMMultiLevelBiquad_setCriticallyDampedLP(BMMultiLevelBiquad* bqf, double fc, size_t firstLevel, size_t numLevels);





/*!
 * BMMultiLevelBiquad_setBesselLP
 *
 * @abstract sets up a Bessel Lowpass filter of order 2*numLevels
 * @discussion Bessel lowpass filters have less steep cutoff than Butterworth but they have nearly constant group delay in the passband and their step response is almost critically damped.
 *
 * @param fc           cutoff frequency
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use. numLevels = (filterOrder / 2)
 */
void BMMultiLevelBiquad_setBesselLP(BMMultiLevelBiquad* bqf, double fc, size_t firstLevel, size_t numLevels);

void BMMultiLevelBiquad_setLowPass6db(BMMultiLevelBiquad* bqf, double fc, size_t level);

void BMMultiLevelBiquad_setHighPass6db(BMMultiLevelBiquad* bqf, double fc, size_t level);


void BMMultiLevelBiquad_setLinkwitzRileyLP(BMMultiLevelBiquad* bqf, double fc, size_t level);

void BMMultiLevelBiquad_setLinkwitzRileyHP(BMMultiLevelBiquad* bqf, double fc, size_t level);

/*!
 *BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder
 *
 * @abstract this filter requires two consecutive levels
 *
 * @param fc          cutoff frequency
 * @param firstLevel  the first of two levels to use
 */
void BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(BMMultiLevelBiquad* bqf, double fc, size_t firstLevel);


/*!
 *BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder
 *
 * @abstract this filter requires two consecutive levels
 *
 * @param fc          cutoff frequency
 * @param firstLevel  the first of two levels to use
 */
void BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(BMMultiLevelBiquad* bqf, double fc, size_t firstLevel);


// c1, c2 are coefficients for a pair of first order allpass filters in series
void BMMultilevelBiquad_setAllpass2ndOrder(BMMultiLevelBiquad* bqf, double c1, double c2, size_t level);

void BMMultilevelBiquad_setAllpass1stOrder(BMMultiLevelBiquad* bqf, double c, size_t level);

/*!
 *BMMultilevelBiquad_setCriticallyDampedPhaseCompensator
 *
 * @abstract        This sets up an allpass filter that has a phase shift identical to the critically damped second order lowpass filter with cutoff at lowpassFC. Note that this is a first order allpass, so it wastes the last two filter coefficients in the biquad. If we need to compensate more than one level of critically damped lowpass filter, it may be better to use a second order version of this function, provided the filter cutoff is such that the allpass coefficients do not lead to a loss of precision.
 * @param bqf       pointer to an initialized filter struct
 * @param lowpassFC cutoff frequency of the critically damped lowpass filter whose phase response we want to match.
 * @param level     the filter level
 */
void BMMultilevelBiquad_setCriticallyDampedPhaseCompensator(BMMultiLevelBiquad * bqf, double lowpassFC, size_t level);


// Calling this sets the filter coefficients at 'level' to bypass.
// Note that the filter still processes through the bypassed section
// but the output is the same as the input.
void BMMultiLevelBiquad_setBypass(BMMultiLevelBiquad* bqf, size_t level);


// set gain in db
void BMMultiLevelBiquad_setGain(BMMultiLevelBiquad* bqf, float gain_db);


/*!
 * BMMultiLevelBiquad_tfMagVector
 *
 * @param frequency  an an array specifying frequencies at which we want to evaluate
 * the transfer function magnitude of the filter
 *
 * @param magnitude an array for storing the result
 * @param length  the number of elements in frequency and magnitude
 *
 */
void BMMultiLevelBiquad_tfMagVector(BMMultiLevelBiquad* bqf, const float *frequency, float *magnitude, size_t length);
void BMMultiLevelBiquad_tfMagVectorAtLevel(BMMultiLevelBiquad* bqf, const float *frequency, float *magnitude, size_t length,size_t level);

/*!
 * BMMultiLevelBiquad_groupDelay
 *
 * @abstract returns the total group delay of all levels of the filter at the specified frequency.
 *
 * @discussion uses a cookbook formula for group delay of biquad filters, based on the fft derivative method.
 *
 * @param freq the frequency at which you need to compute the group delay of the filter cascade
 * @return the group delay in samples at freq
 */
double BMMultiLevelBiquad_groupDelay(BMMultiLevelBiquad* bqf, double freq);

/*!
 * BMMiltiLevelBiquad_phaseResponse
 *
 * @abstract returns the phase response for all levels of the filter at the specified frequency.
 * @param bqf        pointer to an initialized struct
 * @param freq       the frequency at which we want to compute the phase response
 * @return           the phase shift, in radians, at freq
 */
double BMMiltiLevelBiquad_phaseResponse(BMMultiLevelBiquad* bqf, double freq);

//Call this function to manually disable desired filter level
void BMMultiLevelBiquad_setActiveOnLevel(BMMultiLevelBiquad* bqf,bool active,size_t level);

//Set coefficient z directly at level
void BMMultiLevelBiquad_setCoefficientZ(BMMultiLevelBiquad* bqf,size_t level,double* coeff);

#ifdef __cplusplus
}
#endif

#endif /* BMMultiLevelBiquad_h */
