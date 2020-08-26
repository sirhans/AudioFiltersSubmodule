//
//  BMCompressor.c
//  CompressorApp
//
//  Created by Duc Anh on 7/17/18.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#include "BMCompressor.h"
#include <stdlib.h>
#include <math.h>
#include "fastlog.h"
#include "fastpow.h"
#include "BMVectorOps.h"
#include "Constants.h"


void BMCompressor_init(BMCompressor *This, float sampleRate){
    float threshold = -10.0f;
    float kneeWidth = 25.0f;
    float ratio = 4.0f;
    float attackTime = 0.010;
    float releaseTime = 0.080;
    
    BMCompressor_initWithSettings(This,
                                  sampleRate,
                                  threshold,
                                  kneeWidth,
                                  ratio,
                                  attackTime,
                                  releaseTime);
}


/*!
 * BMCompressor_Init
 *
 * @param sampleRate       audio system sampling rate
 * @param thresholdInDB    above this threshold, we start compressing
 * @param kneeWidthInDB    soften the transition from compressing to not compressing in [threshold-knee,thresthold+knee]
 * @param ratio            compressor ratio
 * @param attackTime       time from onset of note to 90% compressor gain change
 * @param releaseTime      time from release of note to 90% compressor gain change
 */
void BMCompressor_initWithSettings(BMCompressor *This, float sampleRate, float thresholdInDB, float kneeWidthInDB, float ratio, float attackTime, float releaseTime){
    
    This->slope = 1.0f - (1.0f / ratio);
    This->thresholdInDB = thresholdInDB;
    This->kneeWidthInDB = kneeWidthInDB;
    
    BMEnvelopeFollower_init(&This->envelopeFollower, sampleRate);
    BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, attackTime);
    BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, releaseTime);
    BMQuadraticThreshold_initLower(&This->quadraticThreshold,
                                   This->thresholdInDB,
                                   This->kneeWidthInDB);
    
    
    //init buffers
    This->buffer1 = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE);
    This->buffer2 = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE);
}


/*
 * Free memory used by temp buffers
 */
void BMCompressor_Free(BMCompressor *This){
    free(This->buffer1);
    This->buffer1 = NULL;
    free(This->buffer2);
    This->buffer2 = NULL;
	BMEnvelopeFollower_free(&This->envelopeFollower);
}



void BMCompressor_ProcessBufferMonoWithSideChain(BMCompressor *This,
												 const float *input,
												 const float *scInput,
												 float* output,
												 float* minGainDb,
												 size_t numSamples){
	
    // prepare to track the minimum gain
    float minGainThisChunk;
    float minGainWholeBuffer = FLT_MAX;
    
    size_t samplesProcessed = 0;
    size_t samplesProcessing;
    while(samplesProcessed < numSamples){
        samplesProcessing = MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
        
        // get a shorter name for the buffer
        float* buffer1 = This->buffer1;
        
        // rectify the input signal
        vDSP_vabs(scInput+samplesProcessed, 1, buffer1, 1, samplesProcessing);
        
        // convert linear gain to decibel scale
        float one = 1.0f;
        vDSP_vdbcon(buffer1,1,&one,buffer1,1,samplesProcessing,0);
        
        // clip values below the threshold with a soft knee
        BMQuadraticThreshold_lowerBuffer(&This->quadraticThreshold,
                                         buffer1, This->buffer2,
                                         samplesProcessing);
        
        // shift the values up so that zero is the minimum
        float oppositeThreshold = -This->thresholdInDB;
        vDSP_vsadd(This->buffer2, 1, &oppositeThreshold, buffer1, 1, samplesProcessing);
        
        // apply the compression ratio
        vDSP_vsmul(buffer1,1,&This->slope,buffer1,1,samplesProcessing);
        
        // filter to get a smooth volume change envelope
        BMEnvelopeFollower_processBuffer(&This->envelopeFollower, buffer1, buffer1, samplesProcessing);
        
        // negate the signal to get the dB change required to apply the compression
        vDSP_vneg(buffer1, 1, buffer1, 1, samplesProcessing);
        
        // get the minimum gain set by the compressor in this chunk
        vDSP_minv(buffer1, 1, &minGainThisChunk, samplesProcessing);
        
        // if this chunk min gain is less than the min gain for the whole
        // buffer, update the min gain for the buffer
        minGainWholeBuffer = MIN(minGainThisChunk,minGainWholeBuffer);
        
        // convert to linear gain control signal
        vector_fastDbToGain(buffer1,buffer1,samplesProcessing);

        // apply the gain adjustment to the audio signal
        vDSP_vmul(buffer1,1,input+samplesProcessed,1,output+samplesProcessed,1,samplesProcessing);
        
        // advance pointers to the next chunk
        samplesProcessed += samplesProcessing;
    }
    
    *minGainDb = minGainWholeBuffer;
}


void BMCompressor_ProcessBufferMono(BMCompressor *This, const float* input, float* output, float* minGainDb, size_t numSamples){
	BMCompressor_ProcessBufferMonoWithSideChain(This, input, input, output, minGainDb, numSamples);
}




void BMCompressor_ProcessBufferStereoWithSideChain(BMCompressor *This,
                                      float* inputL, float* inputR,
                                       float* scInputL, float* scInputR,
                                      float* outputL, float* outputR,
                                      float* minGainDb, size_t numSamples){
    
    // prepare to track the minimum gain
    float minGainThisChunk;
    float minGainWholeBuffer = FLT_MAX;
    
    size_t samplesProcessed = 0;
    size_t samplesProcessing;
    
    while(samplesProcessed<numSamples){
        samplesProcessing = MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
        
        // get a shorter name for the buffer
        float* buffer1 = This->buffer1;
        
        // take the arithmetic mean of the left and right channels
        float half = 0.5;
        vDSP_vasm(scInputL+samplesProcessed, 1, scInputR+samplesProcessed, 1, &half, buffer1, 1, samplesProcessing);
        
        // rectify the input signal
        vDSP_vabs(buffer1, 1, buffer1, 1, samplesProcessing);
        
        // convert linear gain to decibel scale
        float one = 1.0f;
        vDSP_vdbcon(buffer1,1,&one,buffer1,1,samplesProcessing,0);
        
        // clip values below the threshold with a soft knee
        BMQuadraticThreshold_lowerBuffer(&This->quadraticThreshold, buffer1, This->buffer2, samplesProcessing);
        
        // shift the values up so that zero is the minimum
        float oppositeThreshold = -This->thresholdInDB;
        vDSP_vsadd(This->buffer2, 1, &oppositeThreshold, buffer1, 1, samplesProcessing);
        
        // apply the compression ratio
        vDSP_vsmul(buffer1,1,&This->slope,buffer1,1,samplesProcessing);
        
        // filter to get a smooth volume change envelope
        BMEnvelopeFollower_processBuffer(&This->envelopeFollower, buffer1, buffer1, samplesProcessing);
        
        // negate the signal to get the dB change required to apply the compression
        vDSP_vneg(buffer1, 1, buffer1, 1, samplesProcessing);
        
        // get the minimum gain set by the compressor in this chunk
        vDSP_minv(buffer1, 1, &minGainThisChunk, samplesProcessing);
        
        // if this chunk min gain is less than the min gain for the whole
        // buffer, update the min gain for the buffer
        minGainWholeBuffer = MIN(minGainThisChunk,minGainWholeBuffer);
        
        // convert to linear gain control signal
        vector_fastDbToGain(buffer1,buffer1,samplesProcessing);
        
        // apply the gain adjustment to the audio signal
        vDSP_vmul(buffer1,1,inputL+samplesProcessed,1,outputL+samplesProcessed,1,samplesProcessing);
        vDSP_vmul(buffer1,1,inputR+samplesProcessed,1,outputR+samplesProcessed,1,samplesProcessing);
        
        // advance pointers to the next chunk
        samplesProcessed += samplesProcessing;
    }
    
    *minGainDb = minGainWholeBuffer;
}

void BMCompressor_ProcessBufferStereo(BMCompressor *This,
                                        float* inputL, float* inputR,
                                        float* outputL, float* outputR,
                                        float* minGainDb, size_t numSamples){
    BMCompressor_ProcessBufferStereoWithSideChain(This, inputL, inputR, inputL, inputR, outputL, outputR, minGainDb, numSamples);
}

void updateThreshold(BMCompressor *This){
    BMQuadraticThreshold_initLower(&This->quadraticThreshold,
                                   This->thresholdInDB,
                                   This->kneeWidthInDB);
}

void BMCompressor_SetThresholdInDB(BMCompressor *This, float threshold){
    This->thresholdInDB = threshold;
    updateThreshold(This);
}

void BMCompressor_SetKneeWidthInDB(BMCompressor *This, float knee){
    assert(knee > 0.0f);
    
    This->kneeWidthInDB = knee;
    updateThreshold(This);
}

void BMCompressor_SetRatio(BMCompressor *This, float ratio){
    assert(ratio > 0.0);
    
    This->slope = 1.0f - (1.0f / ratio);
}

void BMCompressor_SetAttackTime(BMCompressor *This, float attackTime){
    assert(attackTime >= 0.0f);
    
    This->attackTime = attackTime;
    BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, attackTime);
}

void BMCompressor_SetReleaseTime(BMCompressor *This, float releaseTime){
    assert(releaseTime > 0.0f);
    
    This->releaseTime = releaseTime;
    BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, releaseTime);
}

void BMCompressor_SetSampleRate(BMCompressor *This, float sampleRate){
    assert(sampleRate > 0.0f);
    
	BMEnvelopeFollower_free(&This->envelopeFollower);
    BMEnvelopeFollower_init(&This->envelopeFollower, sampleRate);
    BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, This->attackTime);
    BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, This->releaseTime);
}


