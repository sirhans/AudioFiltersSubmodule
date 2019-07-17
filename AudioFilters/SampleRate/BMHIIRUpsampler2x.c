//
//  BMHIIRUpsampler2x.c
//  AudioFiltersXcodeProject
//
//  Ported from Upsampler2x4.hpp from Laurent De Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMHIIRUpsampler2x.h"
#include "BMPolyphaseIIR2Designer.h"
#include <Accelerate/Accelerate.h>
#include <assert.h>





///*
// ==============================================================================
// Name: set_coefs
// Description:
// Sets filter coefficients. Generate them with the PolyphaseIir2Designer
// class.
// Call this function before doing any processing.
// Input parameters:
// - coef_arr: Array of coefficients. There should be as many coefficients as
// mentioned in the class template parameter.
// Throws: Nothing
// ==============================================================================
// */
//
//template <int NC>
//void    Upsampler2x4Neon <NC>::set_coefs (const double coef_arr [NBR_COEFS])
//{
//    assert (coef_arr != 0);
//
//    for (int i = 0; i < NBR_COEFS; ++i)
//    {
//        _filter [i + 2]._coef4 = vdupq_n_f32 (float (coef_arr [i]));
//    }
//}
void BMHIIRUpsampler2x_setCoefficients(BMHIIRUpsampler2x* This, const double* coefficientArray){
    assert (coefficientArray != 0);
    
    for (int i = 0; i < This->numCoefficients; ++i)
        This->filterStages[i + 2].coef = (float)coefficientArray[i];
}





///*
// ==============================================================================
// Name: clear_buffers
// Description:
// Clears filter memory, as if it processed silence since an infinite amount
// of time.
// Throws: Nothing
// ==============================================================================
// */
//
//template <int NC>
//void    Upsampler2x4Neon <NC>::clear_buffers ()
//{
//    for (int i = 0; i < NBR_COEFS + 2; ++i)
//    {
//        _filter [i]._mem4 = vdupq_n_f32 (0);
//    }
//}
void BMHIIRUpsampler2x_clearBuffers(BMHIIRUpsampler2x* This){
    for(size_t i=0; i < This->numFilterStages; i++)
        This->filterStages[i].mem = 0.0f;
}




///*
// ==============================================================================
// Name: ctor
// Throws: Nothing
// ==============================================================================
// */
//
//template <int NC>
//Upsampler2x4Neon <NC>::Upsampler2x4Neon ()
//:    _filter ()
//{
//    for (int i = 0; i < NBR_COEFS + 2; ++i)
//    {
//        _filter [i]._coef4 = vdupq_n_f32 (0);
//    }
//
//    clear_buffers ();
//}
float BMHIIRUpsampler2x_init(BMHIIRUpsampler2x* This, size_t numCoefficients, float transitionBandwidth){
    assert(0.0f < transitionBandwidth && transitionBandwidth < 0.5f);
    
    // prepare the filter stages
    This->numCoefficients = numCoefficients;
    This->numFilterStages = This->numCoefficients + 2;
    This->filterStages = malloc(sizeof(BMHIIRStage)*This->numFilterStages);
    
    // generate filter coefficients
    double* coefficientArray = malloc(sizeof(double)*numCoefficients);
    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw(coefficientArray,
                                                     (int)numCoefficients,
                                                     transitionBandwidth);
    
    // initialise the filters
    BMHIIRUpsampler2x_setCoefficients(This, coefficientArray);
    BMHIIRUpsampler2x_clearBuffers(This);

    free(coefficientArray);
    
    // compute and return the stopband attenuation
    return (float)BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)numCoefficients, transitionBandwidth);
}




void BMHIIRUpsampler2x_free(BMHIIRUpsampler2x* This){
    free(This->filterStages);
    This->filterStages = NULL;
}



///*
// ==============================================================================
// Name: process_sample
// Description:
// Upsamples (x2) the input vector, generating two output vectors.
// Input parameters:
// - input: The input vector.
// Output parameters:
// - out_0: First output vector.
// - out_1: Second output vector.
// Throws: Nothing
// ==============================================================================
// */
//
//template <int NC>
//void    Upsampler2x4Neon <NC>::process_sample (float32x4_t &out_0, float32x4_t &out_1, float32x4_t input)
//{
//    float32x4_t    even = input;
//    float32x4_t    odd  = input;
//    StageProc4Neon <NBR_COEFS>::process_sample_pos (
//                                                    NBR_COEFS,
//                                                    even,
//                                                    odd,
//                                                    &_filter [0]
//                                                    );
//    out_0 = even;
//    out_1 = odd;
//}
static inline void BMHIIRUpsampler2x_processSampleFloat4(BMHIIRUpsampler2x* This, const simd_float4* input, simd_float4* out0, simd_float4* out1){

    // copy input to both inputs of BMHIIRStage_processSamplePos
    simd_float4 even, odd;
    even = odd = *input;
    
    // process the filters
    BMHIIRStage_processSamplePos(This->numCoefficients, This->numCoefficients, &even, &odd, This->filterStages);
    
    // copy the result to the output
    *out0 = even;
    *out1 = odd;
}





///*
// ==============================================================================
// Name: process_block
// Description:
// Upsamples (x2) the input vector block.
// Input and output blocks may overlap, see assert() for details.
// Input parameters:
// - in_ptr: Input array, containing nbr_spl vector.
// No alignment constraint.
// - nbr_spl: Number of input vectors to process, > 0
// Output parameters:
// - out_0_ptr: Output vector array, capacity: nbr_spl * 2 vectors.
// No alignment constraint.
// Throws: Nothing
// ==============================================================================
// */
//
//template <int NC>
//void    Upsampler2x4Neon <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl)
//{
//    assert (out_ptr != 0);
//    assert (in_ptr != 0);
//    assert (out_ptr >= in_ptr + nbr_spl * 4 || in_ptr >= out_ptr + nbr_spl * 4);
//    assert (nbr_spl > 0);
//
//    long           pos = 0;
//    do
//    {
//        const float32x4_t src = vreinterpretq_f32_u8 (
//                                                      vld1q_u8 (reinterpret_cast <const uint8_t *> (in_ptr + pos * 4))
//                                                      );
//        float32x4_t       dst_0;
//        float32x4_t       dst_1;
//        process_sample (dst_0, dst_1, src);
//        vst1q_u8 (
//                  reinterpret_cast <uint8_t *> (out_ptr + pos * 8    ),
//                  vreinterpretq_u8_f32 (dst_0)
//                  );
//        vst1q_u8 (
//                  reinterpret_cast <uint8_t *> (out_ptr + pos * 8 + 4),
//                  vreinterpretq_u8_f32 (dst_1)
//                  );
//        ++ pos;
//    }
//    while (pos < nbr_spl);
//}
void BMHIIRUpsampler2x_processBufferMono(BMHIIRUpsampler2x* This, const float* input, float* output, size_t numSamplesIn){
        assert (output != 0);
        assert (input != 0);
        assert (output >= input + numSamplesIn || input >= output + numSamplesIn );
    
        while(numSamplesIn > 0) {
            // push a pair of vectors on the stack to store the output
            simd_float4 dst0;
            simd_float4 dst1;
            
            // process 4 inputs; get 8 outputs.
            // BMHIIRUpsampler2x_processSampleFloat4(This, (simd_float4*)input, (simd_float4*)output, (simd_float4*)(output + 4));
            BMHIIRUpsampler2x_processSampleFloat4(This, (simd_float4*)input, &dst0, &dst1);
            
            // copy the output of the function call to the output buffer
            *((simd_float4*)output) = dst0;
            *(1 + (simd_float4*)output) = dst1;
            
            // increment the position
            input+=4;
            output+=8;
            numSamplesIn -= 4;
        }
}




void BMHIIRUpsampler2x_impulseResponse(BMHIIRUpsampler2x* This, float* IR, size_t numSamples){
    // prevent previous processing from affecting the IR output
    BMHIIRUpsampler2x_clearBuffers(This);
    
    // allocate memory for the impulse input
    float* impulseInput = malloc(sizeof(float)*(numSamples/2));
    
    // set the input to {1,0,0,0...}
    memset(impulseInput,0,sizeof(float)*numSamples);
    impulseInput[0] = 1.0f;
    
    // process the input into the IR output
    BMHIIRUpsampler2x_processBufferMono(This, impulseInput, IR, numSamples);
    
    free(impulseInput);
}
