//
//  BMAmplitudeFollower.c
//  BMAUAmpVelocityFilter
//
//  Created by Nguyen Minh Tien on 16/12/2021.
//

#include "BMAmplitudeFollower.h"
#include "Constants.h"

#define Test_MaxBuffers 5

void BMAmplitudeFollower_init(BMAmplitudeFollower* This,float sampleRate){
    BMAttackFilter_init(&This->attackFilter, 20.0f, sampleRate);
    BMReleaseFilter_init(&This->releaseFilter, 10.0f, sampleRate);
    
    This->instantAttackEnvelope = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->slowAttackEnvelope = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->testBuffers = malloc(sizeof(float*)*Test_MaxBuffers);
    for(int i=0;i<Test_MaxBuffers;i++){
        This->testBuffers[i] = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    }
}

void BMAmplitudeFollower_destroy(BMAmplitudeFollower* This){
    for(int i=0;i<Test_MaxBuffers;i++){
        free(This->testBuffers[i]);
    }
    free(This->testBuffers);
}

#define BMAEF_NOISE_GATE_CLOSED_LEVEL -100.0f
void simpleNoiseGate(const float* input,
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
}

void BMAmplitudeFollower_processBuffer(BMAmplitudeFollower* This,float* input,float* envelope,size_t numSamples){
    // absolute value
    vDSP_vabs(input, 1, envelope, 1, numSamples);
    
    // apply a simple per-sample noise gate
    float noiseGateClosedValue = BM_DB_TO_GAIN(BMAEF_NOISE_GATE_CLOSED_LEVEL);
    simpleNoiseGate(envelope, noiseGateClosedValue, noiseGateClosedValue, envelope, numSamples);
    
    // convert to decibels
    float one = 1.0f;
    vDSP_vdbcon(envelope, 1, &one, envelope, 1, numSamples, 0);
    
    size_t sampleProcessed = 0;
    size_t sampleProcessing = 0;
    while(sampleProcessed<numSamples){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples - sampleProcessed);
        
        BMReleaseFilter_processBuffer(&This->releaseFilter, input, This->instantAttackEnvelope, numSamples);
        BMAttackFilter_processBuffer(&This->attackFilter, This->instantAttackEnvelope, This->slowAttackEnvelope, numSamples);
        
        memcpy(This->testBuffers[0], This->instantAttackEnvelope, sizeof(float)*numSamples);
        
        vDSP_vsub(This->slowAttackEnvelope, 1,This->instantAttackEnvelope , 1, envelope, 1, numSamples);
        
        sampleProcessed += sampleProcessing;
    }
    
    
}


