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
	float *b1, *b2;
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





#endif /* BMLFO_h */
