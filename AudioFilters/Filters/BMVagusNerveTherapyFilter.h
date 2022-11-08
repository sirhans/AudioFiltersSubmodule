//
//  BMVagusNerveTherapyFilter.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/13/22.
//  Copyright © 2022 BlueMangoo. All rights reserved.
//

#ifndef BMVagusNerveTherapyFilter_h
#define BMVagusNerveTherapyFilter_h

#include <stdio.h>
#include "BMMultiLevelSVF.h"
#include "BMLFO.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BMVagusNerveTherapyFilter {
	BMMultiLevelSVF svf;
	BMMultiLevelBiquad fixedFilters;
	BMLFO lfo;
	size_t timeSamples, endTimeSamples;
	float sampleRate;
	bool getCoefficientsFromBiquadHelper, filterWithBiquad;
} BMVagusNerveTherapyFilter;


/*!
 *BMVagusNerveTherapyFilter_init
 */
void BMVagusNerveTherapyFilter_init(BMVagusNerveTherapyFilter *This, float sampleRate);


/*!
 *BMVagusNerveTherapyFilter_free
 */
void BMVagusNerveTherapyFilter_free(BMVagusNerveTherapyFilter *This);


/*!
 *BMVagusNerveTherapyFilter_setTimeHMS
 *
 * @abstract set time in hours, minutes, and seconds
 */
void BMVagusNerveTherapyFilter_setTimeHMS(BMVagusNerveTherapyFilter *This, size_t hours, size_t minutes, float seconds);


/*!
 *BMVagusNerveTherapyFilter_setTimeSamples
 *
 * @abstract set time in samples, counting from the beginning of the 5 hour course that resets every 30 days.
 */
void BMVagusNerveTherapyFilter_setTimeSamples(BMVagusNerveTherapyFilter *This, size_t timeInSamples);


/*!
 *BMVagusNerveTherapyFilter_process
 */
void BMVagusNerveTherapyFilter_process(BMVagusNerveTherapyFilter *This,
									   const float *inputL, const float *inputR,
									   float *outputL, float *outputR, size_t numSamples);


/*!
 *BMVagusNerveTherapyFilter_getCoefficientsFromBiquad
 *
 * @abstract
 * The SVF filter coefficients can be set either by using the SVF setBellWithSkirt
 * function or by calling the same function in the biquad helper and copying
 * the state from the biquad into the SVF. By default we use the SVF function
 * but by calling this function with enabled = TRUE you can switch to the biquad
 * helper method. This is useful for confirming that the formulae are correct
 * and the filter is correctly implemented.
 *
 * @param enabled set TRUE to use the biquad helper. False for direct SVF coefficient calculation
 */
void BMVagusNerveTherapyFilter_getCoefficientsFromBiquad(BMVagusNerveTherapyFilter *This, bool enabled);


/*!
 *BMVagusNerveTherapyFilter_filterWithBiquad
 *
 * @abstract
 * This class is designed to use a State Variable Filter for audio processing.
 * However, for the purpose of verifying that the SVF is working properly we
 * also include the option to process audio using a biquad filter. The biquad
 * filter does not handle filter coefficient changes as smoothly as the SVF so
 * it should not be used in a production enviornment.
 *
 * @param enabled set TRUE to use the biquad filter for both coefficient calculation and filtering
 */
void BMVagusNerveTherapyFilter_filterWithBiquad(BMVagusNerveTherapyFilter *This, bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* BMVagusNerveTherapyFilter_h */
