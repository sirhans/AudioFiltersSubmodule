//
//  BMNoiseGate.c
//  AudioUnitTest
//
//  Created by TienNM on 9/25/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMNoiseGate.h"
#include "BMEnvelopeFollower.h"
#include <Accelerate/Accelerate.h>

    
    

    void BMNoiseGate_init(BMNoiseGate* This,float thresholdDb,float decayTimeSeconds,float sampleRate){
        BMEnvelopeFollower_init(&This->envFollower, sampleRate);
        BMNoiseGate_setThreshold(This, thresholdDb);
        BMNoiseGate_setDecayTime(This, decayTimeSeconds);
        This->lastState = 0.0f;
    }
    
    
    
    
    /*!
     *BMNoiseGate_threshold01
     *
     * @abstract convert values < threshold to 0; >= threshold to 1.
     */
    void BMNoiseGate_threshold01(BMNoiseGate* This, const float* input, float* output, size_t numSamples){
        // int version of numSamples for vForce functions
        int numSamplesI = (int)numSamples;
        
        // rectify
        vvfabsf(This->buffer, input, &numSamplesI);
        
        // shift so that threshold is at 0
        float negThreshold = -This->thresholdGain;
        vDSP_vsadd(This->buffer, 1, &negThreshold, This->buffer, 1, numSamples);
        
        // clip outside of [0,1]
        float zero = 0.0f; float one = 1.0f;
        vDSP_vclip(This->buffer, 1, &zero, &one, This->buffer, 1, numSamples);
        
        // ceiling to get only values in the set {0.0,1.0}
        vvceilf(This->buffer, This->buffer, &numSamplesI);
    }
    
    
    
    
    
    void BMNoiseGate_processStereo(BMNoiseGate* This,
                                 const float* inputL, const float* inputR,
                                 float* outputL, float* outputR,
                                 size_t numSamples){
        
        // begin chunked processing
        while (numSamples > 0){
            size_t samplesProcessing = BM_MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            
            // mix stereo input to mono and store into buffer
            float half = 0.5f;
            vDSP_vasm(inputL, 1, inputR, 1, &half, This->buffer, 1, samplesProcessing);
            
            // if abs(input) > threshold, buffer = 1, else buffer = 0
            BMNoiseGate_threshold01(This, This->buffer, This->buffer, samplesProcessing);
            
            // Filter the buffer, which now contains only 1 and 0 values, to
            // generate a gain control signal
            BMEnvelopeFollower_processBuffer(&This->envFollower, This->buffer, This->buffer, samplesProcessing);
            
            // Apply the gain control signal to the input and write to output
            vDSP_vmul(This->buffer, 1, inputL, 1, outputL, 1, samplesProcessing);
            vDSP_vmul(This->buffer, 1, inputR, 1, outputR, 1, samplesProcessing);
            
            // record the last gain value for use in apps that require a gain
            // indicator in the UI
            This->lastState = This->buffer[samplesProcessing-1];
            
            
            // advance pointers
            inputL += samplesProcessing;
            outputL += samplesProcessing;
            inputR += samplesProcessing;
            outputR += samplesProcessing;
            numSamples -= samplesProcessing;
        }
    }
    
    
    
    
    
    void BMNoiseGate_processMono(BMNoiseGate* This,
                                 const float* input,
                                 float* output,
                                 size_t numSamples){
        
        // begin chunked processing
        while (numSamples > 0){
            size_t samplesProcessing = BM_MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            
            // if abs(input) > threshold, buffer = 1, else buffer = 0
            BMNoiseGate_threshold01(This, input, This->buffer, samplesProcessing);
            
            //Filter the buffer, which now contains only 1 and 0 values, to
            //generate a gain control signal
            BMEnvelopeFollower_processBuffer(&This->envFollower, This->buffer, This->buffer, samplesProcessing);
            
            // Apply the gain control signal to the input and write to output
            vDSP_vmul(This->buffer, 1, input, 1, output, 1, samplesProcessing);
            
            // record the last gain value for use in apps that require a gain
            // indicator in the UI
            This->lastState = This->buffer[samplesProcessing-1];
            
            // advance pointers
            input += samplesProcessing;
            output += samplesProcessing;
            numSamples -= samplesProcessing;
        }
    }
    
    
    
    
    void BMNoiseGate_setAttackTime(BMNoiseGate* This,float attackTimeSeconds){
        BMEnvelopeFollower_setAttackTime(&This->envFollower, attackTimeSeconds);
    }
    
    
    
    
    
    void BMNoiseGate_setDecayTime(BMNoiseGate* This,float decayTimeSeconds){
        BMEnvelopeFollower_setReleaseTime(&This->envFollower, decayTimeSeconds);
    }
    
    
    
    
    void BMNoiseGate_setThreshold(BMNoiseGate* This,float thresholdDb){
        This->thresholdGain = BM_DB_TO_GAIN(thresholdDb);
    }
    
    
    
    float BMNoiseGate_getState(BMNoiseGate* This){
        return This->lastState;
    }
    
    
    
#ifdef __cplusplus
}
#endif
