//
//  BMNoiseGate.h
//  AudioUnitTest
//
//  Created by TienNM on 9/25/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMNoiseGate_h
#define BMNoiseGate_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "Constants.h"
#include "BMEnvelopeFollower.h"
#include "BMMultiLevelBiquad.h"
#include "BMLevelMeter.h"

typedef struct BMNoiseGate {
    float thresholdGain, lastState, closedGain;
    float buffer [BM_BUFFER_CHUNK_SIZE];
    BMEnvelopeFollower envFollower;
    BMMultiLevelBiquad sidechainFilter;
    BMLevelMeter sidechainInputMeter, gateLevelMeter;
} BMNoiseGate;

/*!
 *BMNoiseGate_init
 *
 * @param thresholdDb       when the volume drops below this threshold the noise gate switches into release mode
 * @param sampleRate        sample rate
 */
void BMNoiseGate_init(BMNoiseGate* this, float thresholdDb, float sampleRate);



/*!
 *BMNoiseGate_processMono
 */
void BMNoiseGate_processMono(BMNoiseGate* this,
                             const float* input,
                             float* output,
                             size_t numSamplesIn);

/*!
 *BMNoiseGate_processStereo
 */
void BMNoiseGate_processStereo(BMNoiseGate* this,
                               const float* inputL, const float* inputR,
                               float* outputL, float* outputR,
                               size_t numSamplesIn);

/*!
 *BMNoiseGate_setReleaseTime
 */
void BMNoiseGate_setReleaseTime(BMNoiseGate* this,float releaseTimeSeconds);

/*!
 *BMNoiseGate_setAttackTime
 */
void BMNoiseGate_setAttackTime(BMNoiseGate* this,float attackTimeSeconds);

/*!
 *BMNoiseGate_setThreshold
 */
void BMNoiseGate_setThreshold(BMNoiseGate* this,float thresholdDb);

/*!
 *BMNoiseGate_setClosedGain
 *
 * @abstract sets the gain of the noise gate when it's in the closed state
 */
void BMNoiseGate_setClosedGain(BMNoiseGate* this, float gainDb);


/*!
 *BMNoiseGate_setSidechainLowpass
 *
 * @abstract set the lowpass filter cutoff frequency for the sidechain input
 *
 * @param this pointer to an initialised struct
 * @param fc   cutoff frequency or 0.0 for filter bypass
 */
void BMNoiseGate_setSidechainLowpass(BMNoiseGate* this, float fc);



/*!
 *BMNoiseGate_setSidechainHighpass
 *
 * @abstract set the highpass filter cutoff frequency for the sidechain input
 *
 * @param this pointer to an initialised struct
 * @param fc   cutoff frequency or 0.0 for filter bypass
 */
void BMNoiseGate_setSidechainHighpass(BMNoiseGate* this, float fc);


/*!
 * BMNoiseGate_getState
 *
 * @return   the last value of the gain control
 */
float BMNoiseGate_getState(BMNoiseGate* This);


#ifdef __cplusplus
}
#endif

#endif /* BMNoiseGate_h */
