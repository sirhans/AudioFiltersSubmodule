//
//  BMSmoothValue.h
//  AudioFiltersXcodeProject
//
//  The purpose of this class is to fill an audio buffer with a numeric value
//  that responds smoothly to updates by smoothing with linear interpolation.
//
//  Created by hans anderson on 10/13/22.
//  I release this file into the public domain with no restrictions.
//

#ifndef BMSmoothValue_h
#define BMSmoothValue_h

#include <stdio.h>
#include <stdbool.h>
#include "Constants.h"

typedef struct BMSmoothValue {
	float currentValue, targetValue, pendingTargetValue, increment;
	float sampleRate;
	size_t updateTimeInSamples, pendingUpdateTimeInSamples;
	size_t updateProgress;
	bool incrementSet, immediateUpdate;
} BMSmoothValue;


/*!
 *BMSmoothValue_init
 *
 * @param updateTimeSeconds time from when a new value is set until the output reaches the new value
 * @param sampleRate audio system sample rate
 */
void BMSmoothValue_init(BMSmoothValue *This, float updateTimeSeconds, float sampleRate);


/*!
 *BMSmoothValue_setUpdateTime
 */
void BMSmoothValue_setUpdateTime(BMSmoothValue *This, float updateTimeSeconds);



/*!
 *BMSmoothValue_setValueSmoothly
 *
 * @param value the new target value that we will reach after updateTimeSeconds
 */
void BMSmoothValue_setValueSmoothly(BMSmoothValue *This, float value);


/*!
 *BMSmoothValue_setValueImmediately
 *
 * @param value the new value, set without interpolation
 */
void BMSmoothValue_setValueImmediately(BMSmoothValue *This, float value);



/*!
 *BMSmoothValue_processBuffer
 */
void BMSmoothValue_process(BMSmoothValue *This, float *output, size_t numSamples);

#endif /* BMSmoothValue_h */
