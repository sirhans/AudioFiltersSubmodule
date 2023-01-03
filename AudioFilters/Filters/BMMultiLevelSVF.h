//
//  BMMultiLevelSVF.h
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 1/9/19
//
//Source: https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf

#ifndef BMMultiLevelSVF_h
#define BMMultiLevelSVF_h

#include <stdio.h>
#include <Accelerate/Accelerate.h>
#import <os/lock.h>
#include "BMMultiLevelBiquad.h"

typedef struct BMMultiLevelSVF{
    float *g0;
    float *g1;
	float *g2;
	float *m0;
	float *m1;
	float *m2;
	float *k;
	// targets are the values that we are sweeping to in the current buffer
	float *g0_target;
	float *g1_target;
	float *g2_target;
	float *m0_target;
	float *m1_target;
	float *m2_target;
	float *k_target;
	// pending are the values we will be sweeping to in the next buffer
	float *g0_pending;
	float *g1_pending;
	float *g2_pending;
	float *m0_pending;
	float *m1_pending;
	float *m2_pending;
	float *k_pending;
	// interpolation arrays are used for sample-by-sample filter coefficient
	// update during filter sweeps
	float *g0_interp;
	float *g1_interp;
	float *g2_interp;
	float *m0_interp;
	float *m1_interp;
	float *m2_interp;
	float *k_interp;
	// these are the state variables
    float *ic1eq;
    float *ic2eq;
	
    size_t numLevels;
    size_t numChannels;
    double sampleRate;
    bool shouldUpdateParam, updateImmediately;
	bool filterSweep;
	os_unfair_lock lock;
	BMMultiLevelBiquad biquadHelper; // we have this so that we can reuse some functions such as the ones for plotting transfer functions
}BMMultiLevelSVF;

/*!
 *BMMultiLevelSVF_init
 */
void BMMultiLevelSVF_init(BMMultiLevelSVF *This, size_t numLevels,float sampleRate,
                          bool isStereo);
/*!
 *BMMultiLevelSVF_free
 */
void BMMultiLevelSVF_free(BMMultiLevelSVF *This);


/*!
 *BMMultiLevelSVF_processBufferMono
 */
void BMMultiLevelSVF_processBufferMono(BMMultiLevelSVF *This, const float* input, float* output, size_t numSamples);

/*!
 *BMMultiLevelSVF_processBufferStereo
 */
void BMMultiLevelSVF_processBufferStereo(BMMultiLevelSVF *This, const float* inputL,const float* inputR, float* outputL, float* outputR, size_t numSamples);



/**************************
   Filter setup functions
 **************************/

/*!
 *BMMultiLevelSVF_setLowpass
 */
void BMMultiLevelSVF_setLowpass(BMMultiLevelSVF *This, double fc, size_t level);

/*!
 *BMMultiLevelSVF_setLowpassQ
 */
void BMMultiLevelSVF_setLowpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level);

/*!
 *BMMultiLevelSVF_setBandpass
 */
void BMMultiLevelSVF_setBandpass(BMMultiLevelSVF *This, double fc, double q, size_t level);

/*!
 *BMMultiLevelSVF_setHighpass
 */
void BMMultiLevelSVF_setHighpass(BMMultiLevelSVF *This, double fc, size_t level);

/*!
 *BMMultiLevelSVF_setHighpassQ
 */
void BMMultiLevelSVF_setHighpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level);

/*!
 *BMMultiLevelSVF_setAllpass
 */
void BMMultiLevelSVF_setAllpass(BMMultiLevelSVF *This, double fc, double q, size_t level);

/*!
 *BMMultiLevelSVF_setBell
 */
void BMMultiLevelSVF_setBell(BMMultiLevelSVF *This, double fc, double gainDb, double q, size_t level);

/*!
 *BMMultiLevelSVF_setBellWithSkirt
 */
void BMMultiLevelSVF_setBellWithSkirt(BMMultiLevelSVF *This, double fc, double bellGainDb, double skirtGainDb, double q, size_t level);

/*!
 *BMMultiLevelSVF_setLowShelf
 *
 * @abstract set shelf with default slope = 1
 */
void BMMultiLevelSVF_setLowShelf(BMMultiLevelSVF *This, double fc, double gainDb, size_t level);

/*!
 *BMMultiLevelSVF_setLowShelfS
 *
 * @param fc cutoff frequency
 * @param gainDb shelf gain in decibels
 * @param S slope in [0.5,1]
 */
void BMMultiLevelSVF_setLowShelfS(BMMultiLevelSVF *This, double fc, double gainDb, double S, size_t level);

/*!
 *BMMultiLevelSVF_setHighShelf
 *
 * @abstract set shelf with default slope = 1
 */
void BMMultiLevelSVF_setHighShelf(BMMultiLevelSVF *This, double fc, double gainDb, size_t level);

/*!
 *BMMultiLevelSVF_setHighShelfS
 *
 * @param fc cutoff frequency
 * @param gainDb shelf gain in decibels
 * @param S slope in [0.5,1]
 */
void BMMultiLevelSVF_setHighShelfS(BMMultiLevelSVF *This, double fc, double gainDb, double S, size_t level);

/*!
 *BMMultiLevelSVF_impulseResponse
 */
void BMMultiLevelSVF_impulseResponse(BMMultiLevelSVF *This,size_t frameCount);


/*!
 *BMMultiLevelSVF_enableFilterSweep
 *
 * @abstract enable or disable smooth interpolation of parameters for filter sweeps. When sweeps are enabled, the filter interpolates the coefficient update so that it reaches the new filter coefficient values on the first sample of the following call to the process function. This means that if you want the filter to take more than one audio buffer to smoothly move toward a new configuration then you need to gradually update the filter parameters each time you call the BMMultiLevelSVF process function. This is a good way to do a filter sweep because the coefficient interpolation that happens automatically in this class is linear. The linear interpolation is efficient for sample by sample processing and for slow sweeps it is sufficiently accurate but if we were to use linear interpolation over longer sweeps it could lead to unexpected results midway through the sweep, because the filter coefficients generally don't have a liner relationship to parameters like cutoff frequency and Q.
 *
 * @param This    pointer to an initialised struct
 * @param sweepOn set true for filter sweep mode
 */
void BMMultiLevelSVF_enableFilterSweep(BMMultiLevelSVF *This, bool sweepOn);


/*!
 *BMMultiLevelSVF_copyStateFromBiquadHelper
 *
 * The biquadHelper is a BMMultiLevelBiquad struct that functions as a model
 * for this SVF to follow. First we set the biquad filter to the desired
 * configuration. Then we copy that configuration over to the SVF. This allows
 * us to use code written for the biquad filter in the SVF filter without
 * deriving new formulae for the filter coefficients.
 *
 * The key to this is the formula in _setFromBiquad() that converts biquad
 * filter coefficient values into SVF coefficients and results in an SVF filter
 * with the same transfer function.
 *
 */
void BMMultiLevelSVF_copyStateFromBiquadHelper(BMMultiLevelSVF *This);


/*!
 *BMMUltiLevelSVF_forceImmediateUpdate
 *
 * @abstract call this to force the filter to do the currently queued update immediately even when smooth update is on
 */
void BMMUltiLevelSVF_forceImmediateUpdate(BMMultiLevelSVF *This);


#endif /* BMMultiLevelSVF_h */
