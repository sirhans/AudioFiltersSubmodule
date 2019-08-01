//
//  BMVAStateVariableFilter.c
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 7/30/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMVAStateVariableFilter.h"
#include <math.h>
#include "DspUtilities.h"
#include "Constants.h"
#include <assert.h>
#include <Accelerate/Accelerate.h>

void BMVAStateVariableFilter_processBufferLPBPHP(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples);
void BMVAStateVariableFilter_processBufferUBP(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples);
void BMVAStateVariableFilter_processBufferBShelf(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples);
void BMVAStateVariableFilter_processBufferNotch(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples);
void BMVAStateVariableFilter_processBufferAP(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples);
void BMVAStateVariableFilter_processBufferPeak(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples);

void BMVAStateVariableFilter_init(BMVAStateVariableFilter* This,bool isStereo,size_t sRate,SVFType type){
    This->sampleRate = sRate;
    This->filterType = type;
    This->channelCount = isStereo ? 2 : 1;
    This->gCoeff = 1.0f;
    This->RCoeff = 1.0f;
    This->KCoeff = 0.0f;
    
    This->cutoffFreq = 1000.0f;
    This->Q = resonanceToQ(0.5);
    
    
    This->z1_A[0] = This->z2_A[0] = 0.0f;
    This->z1_A[1] = This->z2_A[1] = 0.0f;
    
    This->lpBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->hpBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->bpBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->ubpBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->apBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->notchBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->bshelfBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->peakBuffer = malloc(sizeof(simd_float2)*BM_BUFFER_CHUNK_SIZE);
    This->interleaveBuffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*2);
}

void BMVAStateVariableFilter_destroy(BMVAStateVariableFilter* This){
    free(This->lpBuffer);
    This->lpBuffer = NULL;
    
    free(This->hpBuffer);
    This->hpBuffer = NULL;
    
    free(This->bpBuffer);
    This->bpBuffer = NULL;
    
    free(This->ubpBuffer);
    This->ubpBuffer = NULL;
    
    free(This->apBuffer);
    This->apBuffer = NULL;
    
    free(This->notchBuffer);
    This->notchBuffer = NULL;
    
    free(This->bshelfBuffer);
    This->bshelfBuffer = NULL;
    
    free(This->peakBuffer);
    This->peakBuffer = NULL;
}

void BMVAStateVariableFilter_setFilterType(BMVAStateVariableFilter* This,SVFType type){
    This->filterType = type;
}

void BMVAStateVariableFilter_calcFilter(BMVAStateVariableFilter* This)
{
    if (This->active) {
        
        // prewarp the cutoff (for bilinear-transform filters)
        float wd = This->cutoffFreq * 2.0f * M_PI;
        float T = 1.0f / This->sampleRate;
        float wa = (2.0f / T) * tan(wd * T / 2.0f);
        
        // Calculate g (gain element of integrator)
        This->gCoeff = wa * T / 2.0f;            // Calculate g (gain element of integrator)
        
        // Calculate Zavalishin's R from Q (referred to as damping parameter)
        This->RCoeff = 1.0f / (2.0f * This->Q);
        
        // Gain for BandShelving filter
        This->KCoeff = This->shelfGain;
    }
}

void BMVAStateVariableFilter_setCutoffPitch(BMVAStateVariableFilter* This,const float newCutoffPitch)
{
    if (This->active) {
        This->cutoffFreq = pitchToFreq(newCutoffPitch);
        //cutoffLinSmooth.setValue(cutoffFreq);
        BMVAStateVariableFilter_calcFilter(This);
    }
}

void BMVAStateVariableFilter_setCutoffFreq(BMVAStateVariableFilter* This,const float newCutoffFreq)
{
    if (This->active) {
        This->cutoffFreq = newCutoffFreq;
        BMVAStateVariableFilter_calcFilter(This);
    }
}

void BMVAStateVariableFilter_setResonance(BMVAStateVariableFilter* This,const float newResonance)
{
    if (This->active) {
        This->Q = resonanceToQ(newResonance);
        BMVAStateVariableFilter_calcFilter(This);
    }
}

void BMVAStateVariableFilter_setQ(BMVAStateVariableFilter* This,const float newQ)
{
    if (This->active) {
        This->Q = newQ;
        BMVAStateVariableFilter_calcFilter(This);
    }
}

void BMVAStateVariableFilter_setShelfGain(BMVAStateVariableFilter* This,const float newGain)
{
    if (This->active) {
        This->shelfGain = newGain;
        BMVAStateVariableFilter_calcFilter(This);
    }
}

void BMVAStateVariableFilter_setFilter(BMVAStateVariableFilter* This,const int newType, const float newCutoffFreq,const float newResonance, const float newShelfGain)
{
    This->filterType = newType;
    This->cutoffFreq = newCutoffFreq;
    This->Q = resonanceToQ(newResonance);
    This->shelfGain = newShelfGain;
    BMVAStateVariableFilter_calcFilter(This);
}

void BMVAStateVariableFilter_setSampleRate(BMVAStateVariableFilter* This,const float newSampleRate)
{
    This->sampleRate = newSampleRate;
    //cutoffLinSmooth.reset(sampleRate, smoothTimeMs);
    BMVAStateVariableFilter_calcFilter(This);
}

/*void VAStateVariableFilter::setSmoothingTimeInMs(const float & newSmoothingTimeMs)
 {
 smoothTimeMs = newSmoothingTimeMs;
 }*/

void BMVAStateVariableFilter_setIsActive(BMVAStateVariableFilter* This,bool isActive)
{
    This->active = isActive;
}

//==============================================================================
static inline void copyOutput(simd_float2* input,float* outputL,float* outputR,size_t numSamples){
    float* interleaveInput = (float*)input;
    float zero = 0;
    vDSP_vsadd(interleaveInput, 2, &zero, outputL, 1, numSamples);
    vDSP_vsadd(interleaveInput+1, 2, &zero, outputR, 1, numSamples);
}

void BMVAStateVariableFilter_processBufferStereo(BMVAStateVariableFilter* This,float* const inputL, float* inputR,float* outputL,float* outputR, const int numSamples)
{
    // Test if filter is active. If not, bypass it
    if (This->active) {
        for(int channelIndex = 0;channelIndex<This->channelCount;channelIndex++){
            size_t sampleProcessed = 0;
            float zero = 0;
            while(sampleProcessed<numSamples){
                size_t sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples - sampleProcessed);
                //Copy input to interleave buffer
                vDSP_vsadd(inputL+sampleProcessed, 1, &zero, This->interleaveBuffer, 2, sampleProcessing);
                vDSP_vsadd(inputR+sampleProcessed, 1, &zero, This->interleaveBuffer+1, 2, sampleProcessing);
                //Cast interleave buffer to float2
                simd_float2* inputFloat2 = (simd_float2*)This->interleaveBuffer;
                
                if(This->filterType==SVFLowpass){
                    BMVAStateVariableFilter_processBufferLPBPHP(This, inputFloat2, sampleProcessing);
                    copyOutput(This->lpBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }else if(This->filterType==SVFBandpass){
                    BMVAStateVariableFilter_processBufferLPBPHP(This, inputFloat2, sampleProcessing);
                    copyOutput(This->bpBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }else if(This->filterType==SVFHighpass){
                    BMVAStateVariableFilter_processBufferLPBPHP(This, inputFloat2, sampleProcessing);
                    copyOutput(This->hpBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }else if(This->filterType==SVFUnitGainBandpass){
                    BMVAStateVariableFilter_processBufferUBP(This, inputFloat2, sampleProcessing);
                    copyOutput(This->ubpBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }else if(This->filterType==SVFBandShelving){
                    BMVAStateVariableFilter_processBufferBShelf(This, inputFloat2, sampleProcessing);
                    copyOutput(This->bshelfBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }else if(This->filterType==SVFNotch){
                    BMVAStateVariableFilter_processBufferNotch(This, inputFloat2, sampleProcessing);
                    copyOutput(This->notchBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }else if(This->filterType==SVFAllpass){
                    BMVAStateVariableFilter_processBufferAP(This, inputFloat2, sampleProcessing);
                    copyOutput(This->apBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }else if(This->filterType==SVFPeak){
                    BMVAStateVariableFilter_processBufferPeak(This, inputFloat2, sampleProcessing);
                    copyOutput(This->peakBuffer, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
                }
                
                sampleProcessed += sampleProcessing;
            }
        }
    }
}



inline void BMVAStateVariableFilter_processBufferLPBPHP(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples){
    
    // Filter processing:
    for (int i = 0; i < numSamples; i++) {
    
        // Filter processing:
        This->hpBuffer[i] = (input[i] - (2.0f * This->RCoeff + This->gCoeff) * This->z1_A - This->z2_A)
        / (1.0f + (2.0f * This->RCoeff * This->gCoeff) + This->gCoeff * This->gCoeff);
        
        This->bpBuffer[i] = This->hpBuffer[i] * This->gCoeff + This->z1_A;
        
        This->lpBuffer[i] = This->bpBuffer[i] * This->gCoeff + This->z2_A;
        
        This->z1_A = This->gCoeff * This->hpBuffer[i] + This->bpBuffer[i];        // unit delay (state variable)
        This->z2_A = This->gCoeff * This->bpBuffer[i] + This->lpBuffer[i];          // unit delay (state variable)
    }
}

inline void BMVAStateVariableFilter_processBufferUBP(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples){
    BMVAStateVariableFilter_processBufferLPBPHP(This, input, numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        This->ubpBuffer[i] = 2.0f * This->RCoeff * This->bpBuffer[i];
    }
}

inline void BMVAStateVariableFilter_processBufferBShelf(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples){
    BMVAStateVariableFilter_processBufferUBP(This, input, numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        This->bshelfBuffer[i] = input[i] + This->ubpBuffer[i] * This->KCoeff;
    }
}

inline void BMVAStateVariableFilter_processBufferNotch(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples){
    BMVAStateVariableFilter_processBufferUBP(This, input, numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        This->notchBuffer[i] = input[i] - This->ubpBuffer[i];
    }
}

inline void BMVAStateVariableFilter_processBufferAP(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples){
    BMVAStateVariableFilter_processBufferLPBPHP(This, input, numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        This->apBuffer[i] = input[i] - (4.0f * This->RCoeff * This->bpBuffer[i]);
    }
}

inline void BMVAStateVariableFilter_processBufferPeak(BMVAStateVariableFilter* This,simd_float2* input,  const size_t numSamples){
    BMVAStateVariableFilter_processBufferLPBPHP(This, input, numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        This->peakBuffer[i] = This->lpBuffer[i] - This->hpBuffer[i];
    }
}

