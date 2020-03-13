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
//#define BMAS_ATTACK_FC 4.0f
#define BMAS_AF_NUMLEVELS 1
// #define BMAS_LPF_NUMLEVELS 1
#define BMAS_RF_NUMLEVELS 3
#define BMAS_SF_NUMLEVELS 1
#define BMAS_DSF_NUMLEVELS 1
#define BMAS_DSF_SENSITIVITY 0.1f
#define BMAS_DSF_FC_MIN 10.0f
#define BMAS_RF_FC 10.0f

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
	float sampleRate;
	bool isStereo;
} BMAttackShaperSection;


typedef struct BMMultibandAttackShaper {
	BMAttackShaperSection asSections [4];
	BMCrossover4way crossover4;
	BMCrossover crossover2;
	float b1L [BM_BUFFER_CHUNK_SIZE];
	float b2L [BM_BUFFER_CHUNK_SIZE];
	float b3L [BM_BUFFER_CHUNK_SIZE];
	float b4L [BM_BUFFER_CHUNK_SIZE];
	float b1R [BM_BUFFER_CHUNK_SIZE];
	float b2R [BM_BUFFER_CHUNK_SIZE];
	float b3R [BM_BUFFER_CHUNK_SIZE];
	float b4R [BM_BUFFER_CHUNK_SIZE];
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
 *BMAttackShaperSection_init
 */
void BMAttackShaperSection_init(BMAttackShaperSection *This,
								float releaseFilterFc,
								float attackFc,
								float dsfSensitivity,
								float dsfFcMin, float dsfFcMax,
								float sampleRate,
								bool isStereo);



/*!
*BMAttackShaperSection_setAttackTime
*
* @param attackTime in seconds
*/
void BMAttackShaperSection_setAttackTime(BMAttackShaperSection *This, float attackTime);




/*!
*BMAttackShaperSection_process
*/
void BMAttackShaperSection_processMono(BMAttackShaperSection *This,
							const float* input,
							float* output,
							size_t numSamples);


/*!
*BMAttackShaperSection_processStereo
*/
void BMAttackShaperSection_processStereo(BMAttackShaperSection *This,
										 const float* inputL, const float* inputR,
										 float* outputL, float* outputR,
										 size_t numSamples);


/*!
 *BMAttackShaperSection_free
 */
void BMAttackShaperSection_free(BMAttackShaperSection *This);


#endif /* BMAttackShaper_h */
