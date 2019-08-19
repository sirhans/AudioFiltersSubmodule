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

    typedef struct BMNoiseGate {
        float thresholdGain, lastState;
        float buffer [BM_BUFFER_CHUNK_SIZE];
        BMEnvelopeFollower envFollower;
    } BMNoiseGate;

    /*!
     *BMNoiseGate_init
     *
     * @param thresholdDb       when the volume drops below this threshold the noise gate switches into release mode
     * @param decayTimeSeconds  time from when the volume drops below the threshold until the noiseGate is about 95% closed
     * @param sampleRate        sample rate
     */
    void BMNoiseGate_init(BMNoiseGate* this,float thresholdDb,float decayTimeSeconds,float sampleRate);
    
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
     *BMNoiseGate_setDecayTime
     */
    void BMNoiseGate_setDecayTime(BMNoiseGate* this,float decayTimeSeconds);
    
    /*!
     *BMNoiseGate_setAttackTime
     */
    void BMNoiseGate_setAttackTime(BMNoiseGate* this,float attackTimeSeconds);
    
    /*!
     *BMNoiseGate_setThreshold
     */
    void BMNoiseGate_setThreshold(BMNoiseGate* this,float thresholdDb);
    
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
