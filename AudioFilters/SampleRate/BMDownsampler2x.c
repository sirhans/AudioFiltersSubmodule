//
//  BMDownsampler2x.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/10/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMDownsampler2x.h"
#include <assert.h>

void clearBuffers();

//template <int NC>
//Downsampler2x4Neon <NC>::Downsampler2x4Neon ()
//:    _filter ()
//{
//    for (int i = 0; i < NBR_COEFS + 2; ++i)
//    {
//        _filter [i]._coef4 = vdupq_n_f32 (0);
//    }
//
//    clear_buffers ();
//}
void BMDownsampler2x_init(BMDownsampler2x* This, size_t numCoefficients){
    This->numCoefficients = numCoefficients;
    This->numFilters = This->numCoefficients + 2;
    This->filters = malloc(sizeof(simd_float4)*This->numFilters);
    
    for(size_t i=0; i < This->numFilters; i++) This->filters[i] = 0.0f;

    clearBuffers();
}



//template <int NC>
//void    Downsampler2x4Neon <NC>::set_coefs (const double coef_arr [])
//{
//    assert (coef_arr != 0);
//
//    for (int i = 0; i < NBR_COEFS; ++i)
//    {
//        _filter [i + 2]._coef4 = vdupq_n_f32 (float (coef_arr [i]));
//    }
//}
void BMDownsampler2x_setCoefficients(BMDownsampler2x* This, const float* coefficientArray){
    assert(!isnull(coefficientArray));
    
    for(size_t i=0; i<This->numCoefficients; i++)
        This->filters[i+2] = coefficientArray[i];
}



//template <int NC>
//float32x4_t    Downsampler2x4Neon <NC>::process_sample (float32x4_t in_0, float32x4_t in_1)
//{
//    float32x4_t    spl_0 = in_1;
//    float32x4_t    spl_1 = in_0;
//
//    StageProc4Neon <NBR_COEFS>::process_sample_pos (
//                                                    NBR_COEFS, spl_0, spl_1, &_filter [0]
//                                                    );
//
//    const float32x4_t   sum = spl_0 + spl_1;
//    const float32x4_t   out = sum * vdupq_n_f32 (0.5f);
//
//    return out;
//}
simd_float4 BMDownsampler2x_processSample4(BMDownsampler2x* This, simd_float4 in0, simd_float4 in1){
    
    // switch the order because we process backwards
    simd_float4 sample0 = in1;
    simd_float4 sample1 = in0;
    
    stageProc4(This->stageProcessor, sample0, sample1);
    
    simd_float4 sum = sample0 + sample1;
    simd_float4 out = sum + 0.5f;
    
    return out;
}




//template <int NC>
//float32x4_t    Downsampler2x4Neon <NC>::process_sample (const float in_ptr [8])
//{
//    assert (in_ptr != 0);
//
//    const float32x4_t in_0 = vreinterpretq_f32_u8 (
//                                                   vld1q_u8 (reinterpret_cast <const uint8_t *> (in_ptr    ))
//                                                   );
//    const float32x4_t in_1 = vreinterpretq_f32_u8 (
//                                                   vld1q_u8 (reinterpret_cast <const uint8_t *> (in_ptr + 4))
//                                                   );
//
//    return process_sample (in_0, in_1);
//}
simd_float4 BMDownsampler2x_processSample(BMDownsampler2x* This, simd_float8* input8){
    simd_float4 in0 = *((simd_float4*)input8);
    simd_float4 in1 = *(1 + (simd_float4*)input8);
    return process_sample(This,in0,in1);
}
