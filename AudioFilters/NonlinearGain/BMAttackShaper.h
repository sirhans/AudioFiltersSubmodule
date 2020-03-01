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
#include "Constants.h"

#define BMAS_DELAY_AT_48KHZ_SAMPLES 20.0f
#define BMAS_ATTACK_FC 10.0f
// #define BMAS_LPF_NUMLEVELS 1
#define BMAS_RF_NUMLEVELS 3
#define BMAS_DSF_NUMLEVELS 2
#define BMAS_DSF_SENSITIVITY 2.0f
#define BMAS_DSF_FC 10.0f
#define BMAS_RF_FC 10.0f

typedef struct BMAttackShaper {
	float b1 [BM_BUFFER_CHUNK_SIZE];
    float b2 [BM_BUFFER_CHUNK_SIZE];
	BMReleaseFilter rf [BMAS_RF_NUMLEVELS];
	BMAttackFilter af;
	BMMultiLevelBiquad hpf;
    BMDynamicSmoothingFilter dsf[BMAS_DSF_NUMLEVELS];
    BMShortSimpleDelay dly;
	BMQuadraticLimiter ql;
	size_t delaySamples;
	float sampleRate;
} BMAttackShaper;




/*!
 *BMAttackShaper_init
 */
void BMAttackShaper_init(BMAttackShaper *This, float sampleRate);



/*!
*BMAttackShaper_setAttackTime
*
* @param attackTime in seconds
*/
void BMAttackShaper_setAttackTime(BMAttackShaper *This, float attackTime);




/*!
*BMAttackShaper_process
*/
void BMAttackShaper_process(BMAttackShaper *This,
							const float* input,
							float* output,
							size_t numSamples);


/*!
 *BMAttackShaper_free
 */
void BMAttackShaper_free(BMAttackShaper *This);


#endif /* BMAttackShaper_h */
