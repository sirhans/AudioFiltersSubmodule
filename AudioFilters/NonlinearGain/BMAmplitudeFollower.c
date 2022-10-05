//
//  BMAmplitudeFollower.c
//  BMAUAmpVelocityFilter
//
//  Created by Nguyen Minh Tien on 16/12/2021.
//

#include "BMAmplitudeFollower.h"
#include "Constants.h"

#define Test_MaxBuffers 5
#define BMAEF_NOISE_GATE_CLOSED_LEVEL -100.0f

void BMAmplitudeFollower_setSidechainNoiseGateThreshold(BMAmplitudeFollower *This, float thresholdDb);

void BMAmplitudeFollower_init(BMAmplitudeFollower* This,float sampleRate,int testLength){
    BMAttackFilter_init(&This->attackFilter, 20.0f, sampleRate);
    BMReleaseFilter_init(&This->releaseFilter, 1.0f, sampleRate);
    
    This->instantAttackEnvelope = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->slowAttackEnvelope = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    if(testLength>0){
        This->testBuffers = malloc(sizeof(float*)*Test_MaxBuffers);
        for(int i=0;i<Test_MaxBuffers;i++){
            This->testBuffers[i] = malloc(sizeof(float)*testLength);
        }
    }
    This->isTesting = testLength>0;
    
    BMAmplitudeFollower_setSidechainNoiseGateThreshold(This, BMAEF_NOISE_GATE_CLOSED_LEVEL);
}

void BMAmplitudeFollower_destroy(BMAmplitudeFollower* This){
    if(This->isTesting){
        for(int i=0;i<Test_MaxBuffers;i++){
            free(This->testBuffers[i]);
        }
        free(This->testBuffers);
    }
    free(This->instantAttackEnvelope);
    free(This->slowAttackEnvelope);
}

void BMAmplitudeFollower_simpleNoiseGate(BMAmplitudeFollower *This,
                                    const float* input,
                                    float threshold,
                                    float closedValue,
                                    float* output,
                                    size_t numSamples){
    
    // (input[i] < threshold) ? output[i] = 0;
    if(threshold > closedValue)
        vDSP_vthres(input, 1, &threshold, output, 1, numSamples);
    // (output[i] < closedValue) ? output[i] = closedValue;
    vDSP_vthr(output, 1, &closedValue, output, 1, numSamples);
    
    // get the max value to find out if the gate was open at any point during the buffer
    float maxValue;
    vDSP_maxv(output, 1, &maxValue, numSamples);
    This->noiseGateIsOpen = maxValue > This->noiseGateThreshold;
    if(This->noiseGateThreshold < closedValue) This->noiseGateIsOpen = false;
}

void BMAmplitudeFollower_setSidechainNoiseGateThreshold(BMAmplitudeFollower *This, float thresholdDb){
    // Don't allow the noise gate threshold to be set lower than the
    // gain setting the gate takes when it's closed
    assert(thresholdDb >= BMAEF_NOISE_GATE_CLOSED_LEVEL);
    This->noiseGateThreshold = BM_DB_TO_GAIN(thresholdDb);
}

void BMAmplitudeFollower_processBuffer(BMAmplitudeFollower* This,float* input,float* envelope,size_t numSamples){
    // absolute value
    vDSP_vabs(input, 1, input, 1, numSamples);
    
    // apply a simple per-sample noise gate
    float noiseGateClosedValue = BM_DB_TO_GAIN(BMAEF_NOISE_GATE_CLOSED_LEVEL);
    BMAmplitudeFollower_simpleNoiseGate(This,input, This->noiseGateThreshold, noiseGateClosedValue, input, numSamples);
    
    // convert to decibels
    float one = 1.0f;
    vDSP_vdbcon(input, 1, &one, input, 1, numSamples, 0);
    
    size_t sampleProcessed = 0;
    size_t sampleProcessing = 0;
    while(sampleProcessed<numSamples){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples - sampleProcessed);
        
        BMReleaseFilter_processBuffer(&This->releaseFilter, input+sampleProcessed, This->instantAttackEnvelope, sampleProcessing);
//        BMAttackFilter_processBuffer(&This->attackFilter, This->instantAttackEnvelope, This->slowAttackEnvelope, sampleProcessing);
        if(This->isTesting){
            memcpy(This->testBuffers[0]+sampleProcessed, This->instantAttackEnvelope, sizeof(float)*sampleProcessing);
//        memcpy(This->testBuffers[1]+sampleProcessed, This->slowAttackEnvelope, sizeof(float)*sampleProcessing);
        }
//        vDSP_vsub(This->slowAttackEnvelope, 1,This->instantAttackEnvelope , 1, envelope+sampleProcessed, 1, sampleProcessing);
        memcpy(envelope+sampleProcessed, This->instantAttackEnvelope, sizeof(float)*sampleProcessing);
        
        sampleProcessed += sampleProcessing;
    }
    
    
}


