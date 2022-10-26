//
//  BMMultiLevelSVF.h
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 1/9/19.
//  Copyright © 2019 Hans. All rights reserved.
//

#ifndef BMMultiLevelSVF_h
#define BMMultiLevelSVF_h

#include <stdio.h>
#include <Accelerate/Accelerate.h>
#include "BMMultiLevelBiquad.h"

typedef struct BMMultiLevelSVF{
    float** a;
    float** m;
    float** target_a;
    float** target_m;
    float* ic1eq;
    float* ic2eq;
	float *a1interp;
	float *a2interp;
	float *a3interp;
	float *m0interp;
	float *m1interp;
	float *m2interp;
    size_t numLevels;
    size_t numChannels;
    double sampleRate;
    bool shouldUpdateParam;
	bool filterSweep;
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
 *BMMultiLevelSVF_setNotchpass
 */
void BMMultiLevelSVF_setNotchpass(BMMultiLevelSVF *This, double fc, double q, size_t level);

/*!
 *BMMultiLevelSVF_setPeak
 */
void BMMultiLevelSVF_setPeak(BMMultiLevelSVF *This, double fc, double q, size_t level);

/*!
 *BMMultiLevelSVF_setAllpass
 */
void BMMultiLevelSVF_setAllpass(BMMultiLevelSVF *This, double fc, double q, size_t level);

/*!
 *BMMultiLevelSVF_setBell
 */
void BMMultiLevelSVF_setBell(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level);

/*!
 *BMMultiLevelSVF_setLowShelf
 */
void BMMultiLevelSVF_setLowShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level);

/*!
 *BMMultiLevelSVF_setLowShelfQ
 */
void BMMultiLevelSVF_setLowShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level);

/*!
 *BMMultiLevelSVF_setHighShelf
 */
void BMMultiLevelSVF_setHighShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level);

/*!
 *BMMultiLevelSVF_setHighShelfQ
 */
void BMMultiLevelSVF_setHighShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level);

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



#endif /* BMMultiLevelSVF_h */
