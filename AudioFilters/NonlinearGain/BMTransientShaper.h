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
} BMTransientShaper;



/*!
 *BMTransientShaper_init
 */
void BMTransientShaper_init(BMTransientShaper *This, float sampleRate);

/*!
 *BMTransientShaper_free
 */
void BMTransientShaper_free(BMTransientShaper *This);

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
void BMTransientShaper_setAttackTime(BMTransientShaper *This, float timeInMS);



/*!
 *BMTransientShaper_setPostAttackTime
 */
void BMTransientShaper_setPostAttackTime(BMTransientShaper *This, float timeInMS);




/*!
 *BMTransientShaper_setReleaseTime
 */
void BMTransientShaper_setReleaseTime(BMTransientShaper *This, float timeInMS);




/*!
 * BMTransientEnveloper_processBuffer
 * @abstract used as a component of a transient shaper
 *
 * @param This             pointer to an initialised BMEnvelopeFollower struct
 * @param input            single channel input array
 * @param attackEnvelope   positive values indicate that we are in the attack portion. output values in range: [0,input]
 * @param afterAttackEnvelope positive values indicate that we are in the portion immediately after the attack range: [0,+?]
 * @param releaseEnvelope  positive values indicate that we are in the release portion. output values in range: [0,input]
 */
void BMTransientEnveloper_processBuffer(BMTransientEnveloper *This,
                                        const float* input,
                                        float* attackEnvelope,
                                        float* afterAttackEnvelope,
                                        float* releaseEnvelope,
                                        size_t numSamples);

void BMTransientEnveloper_setAttackOnsetTime(BMTransientEnveloper *This, float seconds);

void BMTransientEnveloper_setAttackDuration(BMTransientEnveloper *This, float seconds);

void BMTransientEnveloper_setReleaseDuration(BMEnvelopeFollower *This, float seconds);

#endif /* BMTransientShaper_h */
