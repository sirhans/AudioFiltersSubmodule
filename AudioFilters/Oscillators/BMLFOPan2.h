//
//  BMLFOPan2.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/8/24.
//
//  This is an auto-pan effect, controlled by an LFO
//

#ifndef BMLFOPan2_h
#define BMLFOPan2_h

#include <stdio.h>
#include "BMLFO.h"

typedef struct BMLFOPan2 {
	BMLFO lfo;
	float *mixControlSignalL, *mixControlSignalR, *buffer;
} BMLFOPan2;


/*!
 *BMLFOPan2_processStereo
 *
 * Input is mixed to mono before processing.
 */
void BMLFOPan2_processStereo(BMLFOPan2 *This,
								  float *inL, float *inR,
								  float *outL, float *outR,
								  size_t numSamples);

/*!
 *BMLFOPan2_init
 *
 * @param This pointer to a struct
 * @param LFOFreqHz the LFO frequency in Hz
 * @param depth in [0,1]. 0 means always panned center. 1 means full L-R panning
 * @param sampleRate sample rate in Hz
 */
void BMLFOPan2_init(BMLFOPan2 *This, float LFOFreqHz, float depth, float sampleRate);



/*!
 *BMLFOPan2_free
 */
void BMLFOPan2_free(BMLFOPan2 *This);

#endif /* BMLFOPan2_h */
