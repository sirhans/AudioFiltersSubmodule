//
//  BMLFO.h
//  BMAudioFilters
//
//  Created by Hans on 13/5/16.
//  Copyright © 2016 Hans. All rights reserved.
//

#ifndef BMLFO_h
#define BMLFO_h

#include <stdio.h>
#include <stdlib.h>
#include "BMQuadratureOscillator.h"
#include "BMSmoothValue.h"

typedef struct BMLFO {
	BMQuadratureOscillator osc;
	BMSmoothValue minValue, scale;
	float *b1, *b2, lastOutput;
} BMLFO;



/*!
 *BMLFO_init
 *
 * @param This pointer to an uninitialised struct
 * @param frequency LFO frequency
 * @param minVal lower limit of the oscillation
 * @param maxVal upper limit of the oscillation
 * @param sampleRate audio system sample rate
 */
void BMLFO_init(BMLFO *This, float frequency, float minVal, float maxVal, float sampleRate);


/*!
 *BMLFO_free
 */
void BMLFO_free(BMLFO *This);


/*!
 *BMLFO_process
 */
void BMLFO_process(BMLFO *This, float *output, size_t numSamples);



/*!
 *BMLFO_setFrequency
 *
 * @param frequency frequency in Hz
 */
void BMLFO_setFrequency(BMLFO *This, float frequency);



/*!
 *BMLFO_setTimeInSamples
 *
 * @param timeInSamples Time since the start sample. This assumes that the frequency hasn't changed since the start. The result is not exact. 
 */
void BMLFO_setTimeInSamples(BMLFO *This, size_t timeInSamples);



/*!
 *BMLFO_setUpdateTime
 *
 * @abstract sets the time it takes setMinMaxSmoothly() to complete a smooth parameter update
 */
void BMLFO_setUpdateTime(BMLFO *This, float timeSeconds);



/*!
 *BMLFO_setMinMaxSmoothly
 *
 * @abstract This sets the min and max values of the oscillator output with smoothing applied to prevent harsh clicks.
 *
 * @param minVal minimum value of the oscillator output
 * @param maxVal maximum value of the oscillator output
 */
void BMLFO_setMinMaxSmoothly(BMLFO *This, float minVal, float maxVal);




/*!
 *BMLFO_setMinMaxImmediately
 *
 * @abstract This sets the min and max values of the oscillator output immediately, without smoothing
 *
 * @param minVal minimum value of the oscillator output
 * @param maxVal maximum value of the oscillator output
 */
void BMLFO_setMinMaxImmediately(BMLFO *This, float minVal, float maxVal);




/*!
 *BMLFO_advance
 *
 * @abstract advance the position of the LFO by numSamples without actually processing the samples. Returns the value of the LFO before advancing. 
 *
 * @param This pointer to an initialised struct
 * @param numSamples number of samples to advance the LFO
 *
 */
float BMLFO_advance(BMLFO *This, size_t numSamples);


/*!
 *BMLFO_getLastValue
 *
 * @returns the last output from either BMLFO_advance or BMLFO_process. This is useful for updating indicators on the GUI that monitor the position of the LFO.
 */
float BMLFO_getLastValue(BMLFO *This);


/*!
 *BMLFO_getAngle
 *
 * @returns the current angle in radians of the internal oscillator
 */
float BMLFO_getAngle(BMLFO *This);



#endif /* BMLFO_h */
