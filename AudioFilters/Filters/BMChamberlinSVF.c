//
//  BMChamberlinSVF.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 8/28/19.
//  License: anyone may freely use this file.
//

#include "BMChamberlinSVF.h"




void BMChamberlinSVFStereo_lowpassBuffer(BMChamberlinSVFStereo* This,
                                        const float* inputL, const float* inputR,
                                        float* outputL, float* outputR,
                                        size_t numSamples){
    
    for(size_t i=0; i<numSamples; i++){
        // calculate the lowpass SVF
        simd_float2 hp = simd_make_float2(inputL[i],inputR[i]) -
                                            (This->fPlusq * This->i1 + This->i2);
        This->i1 += This->f * hp;
        This->i2 += This->f * This->i1;
        
        // copy output to the out buffers
        outputL[i] = This->i2.x;
        outputR[i] = This->i2.y;
    }
}



void BMChamberlinSVF4_lowpassBuffer(BMChamberlinSVF4* This,
                                    const float* input1, const float* input2,
                                    const float* input3, const float* input4,
                                    float* output1, float* output2,
                                    float* output3, float* output4,
                                    size_t numSamples){
    
    for(size_t i=0; i<numSamples; i++){
        // calculate the lowpass SVF
        simd_float4 hp = simd_make_float4(input1[i],input2[i],input3[i],input4[i]) -
                                            (This->fPlusq * This->i1 + This->i2);
        This->i1 += This->f * hp;
        This->i2 += This->f * This->i1;
        
        // copy output to the out buffers
        output1[i] = This->i2.x;
        output2[i] = This->i2.y;
        output3[i] = This->i2.z;
        output4[i] = This->i2.w;
    }
}



void BMChamberlinSVFStereo_setLowpassQ(BMChamberlinSVFStereo* This, float fc, float Q){
    // reference:
    // https://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
    
    float q = 1.0f / Q;
    This->f = 2.0f * sin(M_PI * (fc / This->sampleRate));
    This->fPlusq = This->f + q;
}



void BMChamberlinSVF4_setLowpassQ(BMChamberlinSVF4* This, float fc, float Q){
    // reference:
    // https://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
    
    float q = 1.0f / Q;
    This->f = 2.0f * sin(M_PI * (fc / This->sampleRate));
    This->fPlusq = This->f + q;
}




void BMChamberlinSVFStereo_init(BMChamberlinSVFStereo* This, float sampleRate){
    This->sampleRate = sampleRate;
    This->i1 = 0.0f;
    This->i2 = 0.0f;
}



void BMChamberlinSVF4_init(BMChamberlinSVF4* This, float sampleRate){
    This->sampleRate = sampleRate;
    This->i1 = 0.0f;
    This->i2 = 0.0f;
}
