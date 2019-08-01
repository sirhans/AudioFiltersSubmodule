//
//  BMVAStateVariableFilter.h
//  AudioFiltersXcodeProject
//
//  Created by Nguyen Minh Tien on 7/30/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMVAStateVariableFilter_h
#define BMVAStateVariableFilter_h

#include <stdio.h>
#include <simd/simd.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum  {
    SVFLowpass = 0,
    SVFBandpass,
    SVFHighpass,
    SVFUnitGainBandpass,
    SVFBandShelving,
    SVFNotch,
    SVFAllpass,
    SVFPeak
}SVFType;

typedef struct BMVAStateVariableFilter {
    //    Parameters:
    int filterType;
    int channelCount;
    float cutoffFreq;
    float Q;
    float shelfGain;
    
    float sampleRate;
    bool active;    // is the filter processing or not
    
    //    Coefficients:
    float gCoeff;        // gain element
    float RCoeff;        // feedback damping element
    float KCoeff;        // shelf gain element
    
    simd_float2 z1_A;
    simd_float2 z2_A;
    
    simd_float2* lpBuffer;
    simd_float2* bpBuffer;
    simd_float2* hpBuffer;
    simd_float2* ubpBuffer;
    simd_float2* bshelfBuffer;
    simd_float2* notchBuffer;
    simd_float2* apBuffer;
    simd_float2* peakBuffer;
    float* interleaveBuffer;
} BMVAStateVariableFilter;

    void BMVAStateVariableFilter_init(BMVAStateVariableFilter* This,bool isStereo,size_t sRate,SVFType type);
    
    void BMVAStateVariableFilter_processBufferStereo(BMVAStateVariableFilter* This,float* const inputL, float* inputR,float* outputL,float* outputR, const int numSamples);
    
    void BMVAStateVariableFilter_destroy(BMVAStateVariableFilter* This);
    
    
#ifdef __cplusplus
}
#endif

#endif /* BMVAStateVariableFilter_h */
