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
    float f,fPlusq,sampleRate;
} BMChamberlinSVFStereo;

typedef struct BMChamberlinSVF4 {
    simd_float4 i1,i2;
    float f,fPlusq,sampleRate;
} BMChamberlinSVF4;


void BMChamberlinSVFStereo_lowpassBuffer(BMChamberlinSVFStereo* This,
                                        const float* inputL, const float* inputR,
                                        float* outputL, float* outputR,
                                        size_t numSamples);

/*!
 *BMChamberlinSVF4_lowpassBuffer
 *
 * @abstract process four channels simultaneously
 */
void BMChamberlinSVF4_lowpassBuffer(BMChamberlinSVF4* This,
                                    const float* input1, const float* input2,
                                    const float* input3, const float* input4,
                                    float* output1, float* output2,
                                    float* output3, float* output4,
                                    size_t numSamples);

/*!
 *BMChamberlinSVFStereo_setLowpassQ
 */
void BMChamberlinSVFStereo_setLowpassQ(BMChamberlinSVFStereo* This, float fc, float Q);

/*!
 *BMChamberlinSVF4_setLowpassQ
 */
void BMChamberlinSVF4_setLowpassQ(BMChamberlinSVF4* This, float fc, float Q);


void BMChamberlinSVFStereo_init(BMChamberlinSVFStereo* This, float sampleRate);

void BMChamberlinSVF4_init(BMChamberlinSVF4* This, float sampleRate);


#endif /* BMChamberlinSVF_h */
