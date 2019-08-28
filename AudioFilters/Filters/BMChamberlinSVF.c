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
    float fPlusQ = This->f + This->q;
    
    for(size_t i=0; i<numSamples; i++){
        // calculate the lowpass SVF
        simd_float2 hp = simd_make_float2(inputL[i],inputR[i]) - (fPlusQ * This->i1 + This->i2);
        This->i1 = This->i1 + This->f * hp;
        This->i2 += This->f * This->i1;
        
        // copy output to the out buffers
        outputL[i] = This->i2.x;
        outputR[i] = This->i2.y;
    }
}



void BMChamberlinSVFStereo_setLowpassQ(BMChamberlinSVFStereo* This, float fc, float Q){
    // https://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
    This->q = 1.0f / Q;
    This->f = 2.0f * sin(M_PI * (fc / This->sampleRate));
}



void BMChamberlinSVFStereo_init(BMChamberlinSVFStereo* This, float sampleRate){
    This->sampleRate = sampleRate;
    This->i1 = 0.0f;
    This->i2 = 0.0f;
}
