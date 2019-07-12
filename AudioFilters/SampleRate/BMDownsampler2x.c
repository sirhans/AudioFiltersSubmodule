//
//  BMDownsampler2x.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/10/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMDownsampler2x.h"
#include "BMHIIRStage.h"
#include <assert.h>

void BMDownsampler2x_clearBuffers(BMDownsampler2x* This);
void BMDownsampler2x_setCoefficients(BMDownsampler2x* This, const float* coefficientArray);

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
void BMDownsampler2x_init(BMDownsampler2x* This, const float* coefficientArray, size_t numCoefficients){
    This->numCoefficients = numCoefficients;
    This->numFilterStages = This->numCoefficients + 2;
    This->filterStages = malloc(sizeof(BMHIIRStage)*This->numFilterStages);
    
    // initialize all coefficients to zero
    for(size_t i=0; i < This->numFilterStages; i++)
        This->filterStages[i].coef = 0.0f;

    BMDownsampler2x_clearBuffers(This);
    BMDownsampler2x_setCoefficients(This, coefficientArray);
}


void BMDownsampler2x_free(BMDownsampler2x* This){
    free(This->filterStages);
    This->filterStages = NULL;
}


/*
 ==============================================================================
 Name: set_coefs
 Description:
 Sets filter coefficients. Generate them with the PolyphaseIir2Designer
 class.
 Call this function before doing any processing.
 Input parameters:
 - coef_arr: Array of coefficients. There should be as many coefficients as
 mentioned in the class template parameter.
 Throws: Nothing
 ==============================================================================
 */
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

/*!
 *BMDownsampler2x_setCoefficients
 *
 * @param coefficientArray Generate these using BMPolyphaseIIR2Designer
 */
void BMDownsampler2x_setCoefficients(BMDownsampler2x* This, const float* coefficientArray){
    assert(coefficientArray != 0);
    
    for(size_t i=0; i<This->numCoefficients; i++)
        This->filterStages[i+2].coef = coefficientArray[i]; // converting float to float4
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
inline simd_float4 BMDownsampler2x_processSampleFloat4(BMDownsampler2x* This, simd_float4 in0, simd_float4 in1){
    
    // switch the order of the input samples
    simd_float4 sample0 = in1;
    simd_float4 sample1 = in0;
    
    BMHIIRStage_processSamplePos(This->numCoefficients, This->numCoefficients, &sample0, &sample1, This->filterStages);
    
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
inline simd_float4 BMDownsampler2x_processSampleFloat8(BMDownsampler2x* This, simd_float8* input8){
    // split the float8 input into a pair of float4 vectors
    simd_float4 in0 = *((simd_float4*)input8);
    simd_float4 in1 = *(1 + (simd_float4*)input8);
    
    // and call the process function for the pair
    return BMDownsampler2x_processSampleFloat4(This,in0,in1);
}





/*
 ==============================================================================
 Name: process_block
 Description:
 Downsamples (x2) a block of vectors of 4 samples.
 Input and output blocks may overlap, see assert() for details.
 Input parameters:
 - in_ptr: Input array, containing nbr_spl * 2 vectors.
 No alignment constraint.
 - nbr_spl: Number of vectors to output, > 0
 Output parameters:
 - out_ptr: Array for the output vectors, capacity: nbr_spl vectors.
 No alignment constraint.
 Throws: Nothing
 ==============================================================================
 */
//
//template <int NC>
//void    Downsampler2x4Neon <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl)
//{
//    assert (in_ptr != 0);
//    assert (out_ptr != 0);
//    assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * 8);
//    assert (nbr_spl > 0);
//
//    long           pos = 0;
//    do
//    {
//        const float32x4_t val = process_sample (in_ptr + pos * 8);
//        vst1q_u8 (
//                  reinterpret_cast <uint8_t *> (out_ptr + pos * 4),
//                  vreinterpretq_u8_f32 (val)
//                  );
//        ++ pos;
//    }
//    while (pos < nbr_spl);
//}
void BMDownsampler2x_processBuffer(BMDownsampler2x* This, float* input, float* output, size_t numSamplesIn){
    // the pointers are not null
    assert (input  != 0);
    assert (output != 0);
    
    // input and output are not in an overlapping region of memory
    assert (output <= input || output >= input + numSamplesIn * 8);
    
    // cast the pointers to simd vector types and rename
    simd_float4* output4 = (simd_float4*)output;
    simd_float8* input8  = (simd_float8*)input;
    
    // process audio buffer
    while(numSamplesIn > 0){
        // process 8 floats of input to get 4 floats of output
        *output4 = BMDownsampler2x_processSampleFloat8(This, input8);
        
        // update pointers
        input++;
        output++;
        numSamplesIn -= 8;
    }
}




/*
 ==============================================================================
 Name: process_sample_split
 Description:
 Split (spectrum-wise) in half a pair of vector of 4 samples. The lower part
 of the spectrum is a classic downsampling, equivalent to the output of
 process_sample().
 The higher part is the complementary signal: original filter response
 is flipped from left to right, becoming a high-pass filter with the same
 cutoff frequency. This signal is then critically sampled (decimation by 2),
 flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
 Input parameters:
 - in_0: vector at t
 - in_1: vector at t + 1
 Output parameters:
 - low: output vector, lower part of the spectrum (downsampling)
 - high: output vector, higher part of the spectrum.
 Throws: Nothing
 ==============================================================================
 */
//template <int NC>
//void    Downsampler2x4Neon <NC>::process_sample_split (float32x4_t &low, float32x4_t &high, float32x4_t in_0, float32x4_t in_1)
//{
//    float32x4_t    spl_0 = in_1;
//    float32x4_t    spl_1 = in_0;
//
//    StageProc4Neon <NBR_COEFS>::process_sample_pos (
//                                                    NBR_COEFS, spl_0, spl_1, &_filter [0]
//                                                    );
//
//    const float32x4_t sum = spl_0 + spl_1;
//    low  = sum * vdupq_n_f32 (0.5f);
//    high = spl_0 - low;
//}
inline void BMDownsampler2x_processSampleSplit4(BMDownsampler2x* This, simd_float4* outLow, simd_float4* outHigh, const simd_float4* input0, const simd_float4* input1){
    // swap the order of the input samples
    simd_float4 sample0 = *input1;
    simd_float4 sample1 = *input0;
    
    // process one sample
    BMHIIRStage_processSamplePos(This->numCoefficients, This->numCoefficients, &sample0, &sample1, This->filterStages);
    
    const simd_float4 sum = sample0 + sample1;
    *outLow = sum * 0.5f;
    *outHigh = sample0 - *outLow;
}


/*
 ==============================================================================
 Name: process_sample_split
 Description:
 Split (spectrum-wise) in half a pair of vector of 4 samples. The lower part
 of the spectrum is a classic downsampling, equivalent to the output of
 process_sample().
 The higher part is the complementary signal: original filter response
 is flipped from left to right, becoming a high-pass filter with the same
 cutoff frequency. This signal is then critically sampled (decimation by 2),
 flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
 Input parameters:
 - in_ptr: pointer on the pair of input vectors. No alignment constraint.
 Output parameters:
 - low: output vector, lower part of the spectrum (downsampling)
 - high: output vector, higher part of the spectrum.
 Throws: Nothing
 ==============================================================================
 */
//template <int NC>
//void    Downsampler2x4Neon <NC>::process_sample_split (float32x4_t &low, float32x4_t &high, const float in_ptr [8])
//{
//    assert (in_ptr != 0);
//
//    const float32x4_t in_0 =
//    vreinterpretq_f32_u8 (*reinterpret_cast <const uint8x16_t *> (in_ptr    ));
//    const float32x4_t in_1 =
//    vreinterpretq_f32_u8 (*reinterpret_cast <const uint8x16_t *> (in_ptr + 4));
//
//    process_sample_split (low, high, in_0, in_1);
//}
void BMDownsampler2x_processSampleSplit8(BMDownsampler2x* This, simd_float4* outLow, simd_float4* outHigh, simd_float8* input){
    const simd_float4 in_0 = *((simd_float4*)input);
    const simd_float4 in_1 = *(1 + (simd_float4*)input);
    
    BMDownsampler2x_processSampleSplit4(This, outLow, outHigh, &in_0, &in_1);
}





/*
 ==============================================================================
 Name: process_block_split
 Description:
 Split (spectrum-wise) in half a pair of vector of 4 samples. The lower part
 of the spectrum is a classic downsampling, equivalent to the output of
 process_block().
 The higher part is the complementary signal: original filter response
 is flipped from left to right, becoming a high-pass filter with the same
 cutoff frequency. This signal is then critically sampled (decimation by 2),
 flipping the spectrum: Fs/4...Fs/2 becomes Fs/4...0.
 Input and output blocks may overlap, see assert() for details.
 Input parameters:
 - in_ptr: Input array, containing nbr_spl * 2 vectors.
 No alignment constraint.
 - nbr_spl: Number of vectors for each output, > 0
 Output parameters:
 - out_l_ptr: Array for the output vectors, lower part of the spectrum
 (downsampling). Capacity: nbr_spl vectors.
 No alignment constraint.
 - out_h_ptr: Array for the output vectors, higher part of the spectrum.
 Capacity: nbr_spl vectors.
 No alignment constraint.
 Throws: Nothing
 ==============================================================================
 */
//template <int NC>
//void    Downsampler2x4Neon <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl)
//{
//    assert (in_ptr != 0);
//    assert (out_l_ptr != 0);
//    assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * 8);
//    assert (out_h_ptr != 0);
//    assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * 8);
//    assert (out_h_ptr != out_l_ptr);
//    assert (nbr_spl > 0);
//
//    long           pos = 0;
//    do
//    {
//        float32x4_t    low;
//        float32x4_t    high;
//        process_sample_split (low, high, in_ptr + pos * 8);
//        *reinterpret_cast <uint8x16_t *> (out_l_ptr + pos * 4) =
//        vreinterpretq_u8_f32 (low);
//        *reinterpret_cast <uint8x16_t *> (out_h_ptr + pos * 4) =
//        vreinterpretq_u8_f32 (high);
//        ++ pos;
//    }
//    while (pos < nbr_spl);
//}



/*
 ==============================================================================
 Name: clear_buffers
 Description:
 Clears filter memory, as if it processed silence since an infinite amount
 of time.
 Throws: Nothing
 ==============================================================================
 */
//template <int NC>
//void    Downsampler2x4Neon <NC>::clear_buffers ()
//{
//    for (int i = 0; i < NBR_COEFS + 2; ++i)
//    {
//        _filter [i]._mem4 = vdupq_n_f32 (0);
//    }
//}
void BMDownsampler2x_clearBuffers(BMDownsampler2x* This){
    for(size_t i=0; i < This->numFilterStages; i++)
        This->filterStages[i].mem = 0.0f;
}
