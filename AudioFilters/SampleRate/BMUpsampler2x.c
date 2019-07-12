//
//  BMUpsampler2x.c
//  AudioFiltersXcodeProject
//
//  Ported from Upsampler2x.hpp from Laurent De Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMUpsampler2x.h"
#include <Accelerate/Accelerate.h>
#include <assert.h>

// from Upsampler2XNeon.hpp of Laurent de Soras HIIR library
//
//template <int NC>
//Upsampler2xNeon <NC>::Upsampler2xNeon ()
//:    _filter ()
//{
//    for (int i = 0; i < NBR_STAGES + 1; ++i)
//    {
//        _filter [i]._coef4 = vdupq_n_f32 (0);
//    }
//    if ((NBR_COEFS & 1) != 0)
//    {
//        const int      pos = (NBR_COEFS ^ 1) & (STAGE_WIDTH - 1);
//        _filter [NBR_STAGES]._coef [pos] = 1;
//    }
//
//    clear_buffers ();
//}
//
void BMUpsampler2x_init(BMUpsampler2x* This, size_t numCoefficients){
    This->numCoefficients = numCoefficients;
    This->numStages = (This->numCoefficients + BM_HIIR_STAGE_WIDTH - 1) / BM_HIIR_STAGE_WIDTH;
    This->numFilters = This->numCoefficients + 2;
    
    This->filters = malloc(sizeof(BMHIIRStageData)*This->numFilters);
    
    // set all the filter coefficients to zero
    for (size_t i = 0; i < This->numStages + 1; ++i)
    {
        This->filters[i].coef = 0.0f;
    }
    // if the number of coefficients is odd, set one of them equal to 1.0f
    if ((This->numCoefficients & 1) != 0)
    {
        size_t index = BM_HIIR_GET_INDEX(This->numCoefficients);
        This->filters[This->numStages].coef[index] = 1.0f;
    }
    
    BMUpsampler2x_clearBuffers(This);
}




void BMUpsampler2x_setCoefficients(BMUpsampler2x* This, const double* coefficientArray){
    assert(coefficientArray != NULL);
    
    for(size_t i=0; i<This->numCoefficients; i++){
        size_t stage = (i / BM_HIIR_STAGE_WIDTH) + 1;
        size_t index = BM_HIIR_GET_INDEX(i);
        This->filters[stage].coef[index] = (float)(coefficientArray[i]);
    }
    
}



static inline void BMUpsampler2x_processSample(BMUpsampler2x* This, float input, float* out0, float* out1){
    
    const simd_float2 sampleIn  = input;
    const simd_float2 sampleMid = simd_make_float2(This->filters[This->numStages].mem);
    simd_float4 y = simd_make_float4(sampleIn,sampleMid);
    simd_float4 mem = This->filters[0].mem;
    
    BMHIIRStage_processSample(
}



void BMUpsampler2x_processBuffer(BMUpsampler2x* This, const float* input, float* output, size_t numSamples){
    
}



void BMUpsampler2x_clearBuffers(BMUpsampler2x* This){
    
}
