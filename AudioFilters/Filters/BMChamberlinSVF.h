//
//  BMChamberlinSVF.h
//  AudioFiltersXcodeProject
//
//  The Chamberlin State Variable filter is a simple digital implementation
//  that works well only at low cutoff frequencies. Despite its shortcomings,
//  we use it in cases where efficiency is critical and our cutoff frequency is
//  below samplerate / 4.
//
//  reference: https://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
//
//  Created by hans anderson on 8/28/19.
//  License: anyone may freely use this file.
//

#ifndef BMChamberlinSVF_h
#define BMChamberlinSVF_h

#include <stdio.h>
#include <simd/simd.h>

typedef struct BMChamberlinSVFStereo {
    simd_float2 i1,i2;
    float f,q,sampleRate;
} BMChamberlinSVFStereo;


void BMChamberlinSVFStereo_lowpassBuffer(BMChamberlinSVFStereo* This,
                                        const float* inputL, const float* inputR,
                                        float* outputL, float* outputR,
                                        size_t numSamples);



void BMChamberlinSVFStereo_setLowpassQ(BMChamberlinSVFStereo* This, float fc, float Q);


void BMChamberlinSVFStereo_init(BMChamberlinSVFStereo* This, float sampleRate);


#endif /* BMChamberlinSVF_h */
