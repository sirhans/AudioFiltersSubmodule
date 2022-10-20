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

typedef struct BMVagusNerveTherapyFilter {
	BMMultiLevelSVF svf;
	BMLFO lfo;
	size_t timeSamples, endTimeSamples;
	float sampleRate;
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

#endif /* BMVagusNerveTherapyFilter_h */
