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
#include "BMSFM.h"
#include "BMShortSimpleDelay.h"


typedef struct BMNoiseGate {
    float buffer [BM_BUFFER_CHUNK_SIZE];
    BMEnvelopeFollower envFollower;
    BMMultiLevelBiquad sidechainFilter;
    BMLevelMeter sidechainInputMeter;
	BMShortSimpleDelay delay;
    float thresholdGain, ratio, sidechainInputLeveldB, controlSignalLeveldB, sidechainMinFreq, sidechainMaxFreq;
} BMNoiseGate;


/*!
 *BMNoiseGate_init
 *
 * @param thresholdDb       when the volume drops below this threshold the noise gate switches into release mode
 * @param sampleRate        sample rate
 */
void BMNoiseGate_init(BMNoiseGate *This, float thresholdDb, float sampleRate);



/*!
 *BMNoiseGate_free
 */
void BMNoiseGate_free(BMNoiseGate *This);





/*!
 *BMNoiseGate_processMono
 */
void BMNoiseGate_processMono(BMNoiseGate *This,
                             const float* input,
                             float* output,
                             size_t numSamplesIn);

/*!
 *BMNoiseGate_processStereo
 */
void BMNoiseGate_processStereo(BMNoiseGate *This,
                               const float* inputL, const float* inputR,
                               float* outputL, float* outputR,
                               size_t numSamplesIn);

/*!
 *BMNoiseGate_setReleaseTime
 */
void BMNoiseGate_setReleaseTime(BMNoiseGate *This,float releaseTimeSeconds);


/*!
 *BMNoiseGate_setAttackTime
 */
void BMNoiseGate_setAttackTime(BMNoiseGate *This,float attackTimeSeconds);


/*!
 *BMNoiseGate_setThreshold
 */
void BMNoiseGate_setThreshold(BMNoiseGate *This,float thresholdDb);


/*!
 *BMNoiseGate_setRatio
 *
 * @abstract sets the downward expansion ratio of the gate
 */
void BMNoiseGate_setRatio(BMNoiseGate *This, float ratio);


/*!
 *BMNoiseGate_setSidechainHighCut
 *
 * @abstract set the lowpass filter cutoff frequency for the sidechain input
 *
 * @param This pointer to an initialised struct
 * @param fc   cutoff frequency or 0.0 for filter bypass
 */
void BMNoiseGate_setSidechainHighCut(BMNoiseGate *This, float fc);



/*!
 *BMNoiseGate_setSidechainLowCut
 *
 * @abstract set the highpass filter cutoff frequency for the sidechain input
 *
 * @param This pointer to an initialised struct
 * @param fc   cutoff frequency or 0.0 for filter bypass
 */
void BMNoiseGate_setSidechainLowCut(BMNoiseGate *This, float fc);


/*!
 * BMNoiseGate_getGateVolumeDB
 *
 * @return   the most recent value of the gain control in decibels
 */
float BMNoiseGate_getGateVolumeDB(BMNoiseGate *This);


/*!
 * BMNoiseGate_getSidechainInputLevelDB
 *
 * @return   the most recent RMS power level of the sidechain input in decibels
 */
float BMNoiseGate_getSidechainInputLevelDB(BMNoiseGate *This);


#ifdef __cplusplus
}
#endif

#endif /* BMNoiseGate_h */
