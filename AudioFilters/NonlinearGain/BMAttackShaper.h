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
#include "Constants.h"

#define BMAS_DELAY_AT_48KHZ_SAMPLES 12.0f
#define BMAS_ATTACK_TIME_DEFAULT 0.050f
#define BMAS_LPF_NUMLEVELS 1
#define BMAS_RF2_NUMLEVELS 2
#define BMAS_DSF_SENSITIVITY 4.0f
#define BMAS_RF_FC 20.0f

typedef struct BMAttackShaper {
    float b1 [BM_BUFFER_CHUNK_SIZE];
    float b2 [BM_BUFFER_CHUNK_SIZE];
	BMReleaseFilter rf1;
	BMReleaseFilter rf2 [BMAS_RF2_NUMLEVELS];
	BMMultiLevelBiquad lpf;
    BMDynamicSmoothingFilter dsf;
    BMShortSimpleDelay dly;
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
