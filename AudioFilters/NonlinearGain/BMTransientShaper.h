//
//  BMTransientShaper.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/17/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMTransientShaper_h
#define BMTransientShaper_h

#include <stdio.h>
#include "BMEnvelopeFollower.h"
#include "Constants.h"


typedef struct BMTransientEnveloper {
    // filters for transient attack envelope
    BMReleaseFilter attackRF1 [BMENV_NUM_STAGES];
    BMReleaseFilter attackRF2 [BMENV_NUM_STAGES];
    BMAttackFilter  attackAF1 [BMENV_NUM_STAGES];
    BMAttackFilter  attackAF2 [BMENV_NUM_STAGES];
    BMDynamicSmoothingFilter attackDSF;
    bool filterAttackOnset;
    
    // filter for transient after-attack envelope
    BMReleaseFilter afterAttackRF;
    
    // filters for transient release envelope
    BMReleaseFilter releaseRF1 [BMENV_NUM_STAGES];
    BMReleaseFilter releaseRF2;
    BMDynamicSmoothingFilter releaseDSF;
    
    // filters for after-attack envelopes
} BMTransientEnveloper;



typedef struct BMTransientShaper {
    BMTransientEnveloper enveloper;
	float attackEnv [BM_BUFFER_CHUNK_SIZE];
	float postAttackEnv [BM_BUFFER_CHUNK_SIZE];
	float releaseEnv [BM_BUFFER_CHUNK_SIZE];
	float buffer1 [BM_BUFFER_CHUNK_SIZE];
	float attackStrength, postAttackStrength, releaseStrength;
} BMTransientShaper;



/*!
 *BMTransientShaper_init
 *
 * @abstract this function does not allocate heap memory
 */
void BMTransientShaper_init(BMTransientShaper *This, float sampleRate);


/*!
 *BMTransientShaper_processBufferStereo
 */
void BMTransientShaper_processBufferStereo(BMTransientShaper *This,
										   const float* inL, const float* inR,
										   float* outL, float* outR,
										   size_t numSamples);

/*!
 *BMTransientShaper_setAttackStrength
 *
 * @param This pointer to an initialised struct
 * @param strength in -1,1
 */
void BMTransientShaper_setAttackStrength(BMTransientShaper *This, float strength);


/*!
 *BMTransientShaper_setPostAttackStrength
 *
 * @param This pointer to an initialised struct
 * @param strength in -1,1
 */
void BMTransientShaper_setPostAttackStrength(BMTransientShaper *This, float strength);


/*!
 *BMTransientShaper_setReleaseStrength
 *
 * @param This pointer to an initialised struct
 * @param strength in -1,1
 */
void BMTransientShaper_setReleaseStrength(BMTransientShaper *This, float strength);


/*!
 *BMTransientShaper_setAttackTime
 */
void BMTransientShaper_setAttackTime(BMTransientShaper *This, float timeInSeconds);



/*!
 *BMTransientShaper_setPostAttackTime
 */
void BMTransientShaper_setPostAttackTime(BMTransientShaper *This, float timeInSeconds);




/*!
 *BMTransientShaper_setReleaseTime
 */
void BMTransientShaper_setReleaseTime(BMTransientShaper *This, float timeInSeconds);





#endif /* BMTransientShaper_h */
