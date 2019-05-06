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

    
    void BMNoiseGate_init(BMNoiseGate* this,float thresholdDb,float decayTime,float sampleRate);
    
    void BMNoiseGate_processMono(BMNoiseGate* this,
                                 const float* input,
                                 float* output,
                                 size_t numSamplesIn);
    
    void BMNoiseGate_processStereo(BMNoiseGate* this,
                                   const float* inputL, const float* inputR,
                                   float* outputL, float* outputR,
                                   size_t numSamplesIn);
    
    void BMNoiseGate_setDecayTime(BMNoiseGate* this,float decayTimeSeconds);
    
    void BMNoiseGate_setAttackTime(BMNoiseGate* this,float attackTimeSeconds);
    
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
