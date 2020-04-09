//
//  BMAttackShaper.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 18/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMAttackShaper_h
#define BMAttackShaper_h

#include <stdio.h>
#include "BMEnvelopeFollower.h"
#include "BMMultiLevelBiquad.h"
#include "BMDynamicSmoothingFilter.h"
#include "BMShortSimpleDelay.h"
#include "BMQuadraticLimiter.h"
#include "BMCrossover.h"
#include "Constants.h"

#define BMAS_DELAY_AT_48KHZ_SAMPLES 30.0f
#define BMAS_AF_NUMLEVELS 1
#define BMAS_RF_NUMLEVELS 3
#define BMAS_SF_NUMLEVELS 1
#define BMAS_DSF_NUMLEVELS 1
#define BMAS_DSF_SENSITIVITY 0.1f
#define BMAS_DSF_FC_MIN 10.0f
#define BMAS_RF_FC 10.0f
#define BMAS_NUM_SECTIONS 2
#define BMAS_SECTION_2_AF_MULTIPLIER 1.25f
#define BMAS_SECTION_2_RF_MULTIPLIER 2.0f
#define BMAS_SECTION_2_EXAGGERATION_MULTIPLIER 1.1f

#define BMAS_BAND_1_FC 300.0f
#define BMAS_BAND_2_FC 750.0f
#define BMAS_BAND_3_FC 1600.0f

typedef struct BMAttackShaperSection {
	float b0 [BM_BUFFER_CHUNK_SIZE];
	float b1 [BM_BUFFER_CHUNK_SIZE];
    float b2 [BM_BUFFER_CHUNK_SIZE];
	BMReleaseFilter rf [BMAS_RF_NUMLEVELS];
	BMAttackFilter af [BMAS_AF_NUMLEVELS];
    BMDynamicSmoothingFilter dsf[BMAS_DSF_NUMLEVELS];
    BMShortSimpleDelay dly;
	size_t delaySamples;
	float exaggeration, depth, sampleRate, noiseGateThreshold;
	bool isStereo;
	bool noiseGateIsOpen;
} BMAttackShaperSection;


typedef struct BMMultibandAttackShaper {
	BMAttackShaperSection asSections [BMAS_NUM_SECTIONS];
	BMCrossover crossover2;
	float *b1L, *b2L, *b1R, *b2R;
	bool isStereo;
} BMMultibandAttackShaper;


/*!
 *BMMultibandAttackShaper_init
 */
void BMMultibandAttackShaper_init(BMMultibandAttackShaper *This, bool isStereo, float sampleRate);


/*!
 *BMMultibandAttackShaper_free
 */
void BMMultibandAttackShaper_free(BMMultibandAttackShaper *This);


/*!
 *BMMultibandAttackShaper_processStereo
 */
void BMMultibandAttackShaper_processStereo(BMMultibandAttackShaper *This,
										   const float *inputL, const float *inputR,
										   float *outputL, float *outputR,
										   size_t numSamples);

/*!
 *BMMultibandAttackShaper_processMono
 */
void BMMultibandAttackShaper_processMono(BMMultibandAttackShaper *This,
										 const float *input,
										 float *output,
										 size_t numSamples);


/*!
 *BMMultibandAttackShaper_setAttackTime
 */
void BMMultibandAttackShaper_setAttackTime(BMMultibandAttackShaper *This, float attackTimeInSeconds);

/*!
 *BMMultibandAttackShaper_setAttackDepth
 *
 * The attack depth can be either positive or negative. With a depth of -1 the initial attack transient is totally silenced. With a depth of 0, no modification to the signal is made. With a depth of +1, the initial attack transient is increased by the same amount (in decibels) that it is decreased to silence it. Thus, a depth of +1 is excessive.
 *
 * There are no hard limits to the value of the depth parameter. Setting it lower than -1 will result in a longer period of silence and more extreme dips when there are partial transients in polyphonic material.
 *
 * @param This pointer to an initialised struct
 * @param depth -1 for silent attack. 0 for no change. small positive values for increased attack transient
 */
void BMMultibandAttackShaper_setAttackDepth(BMMultibandAttackShaper *This, float depth);


/*!
 *BMMultibandAttackShaper_setSidechainNoiseGateThreshold
 *
 * transients below the noise gate threshold level will be ignored.
 */
void BMMultibandAttackShaper_setSidechainNoiseGateThreshold(BMMultibandAttackShaper *This, float thresholdDb);

/*!
 *BMMultibandAttackShaper_sidechainNoiseGateIsOpen
 *
 * Use this to get the state of the sidechain noise gate.
 *
 * @returns true if the gate was open during any part of the previous audio buffer
 */
bool BMMultibandAttackShaper_sidechainNoiseGateIsOpen(BMMultibandAttackShaper *This);



/*!
 *BMMultibandAttackShaper_getLatencyInSeconds
 *
 * @param This pointer to a struct
 *
 * @returns the latency in seconds
 */
float BMMultibandAttackShaper_getLatencyInSeconds(BMMultibandAttackShaper *This);

#endif /* BMAttackShaper_h */
