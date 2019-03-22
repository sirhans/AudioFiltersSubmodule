//
//  BMUpsampler.c
//  BMAudioFilters
//
//  This is a 2x upsampler using a polyphase FIR anti-aliasing filter.
//
//  Created by Hans on 10/9/17.
//  This file may be used by anyone without any restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMUpsampler.h"
#include <string.h>
#include <Accelerate/Accelerate.h>
#include "BMIntegerMath.h"
#include <stdlib.h>
    
#define BMUPSAMPLER_MAX_UPSAMPLE_FACTOR 8
    
    void BMUpsampler_init(BMUpsampler* This, size_t upsampleFactor){
        
        // currently only 2, 4, and 8 are supported
        assert(isPowerOfTwo(upsampleFactor) &&
               upsampleFactor <= BMUPSAMPLER_MAX_UPSAMPLE_FACTOR);
        
        // how many stages are we using?
        This->numStages = log2i((uint32_t)upsampleFactor);

        /*
         * This table of coefficients defines an FIR lowpass filter 
         * with 35 coefficients.
         * 
         * It is divided into two sections so that it can be used
         * as a polyphase filter for upsampling with 9.5 multiplications
         * per output sample.
         *
         * The filter coefficients were generated in Mathematica using the
         * following commands:
         * 
         * // Lowpass filter with cutoff at 72% of nyquist relative to
         * // the input sample rate.
         * n = 35;
         * h1 = LeastSquaresFilterKernel[{"Lowpass", 0.72 \[Pi]/2}, n];
         *
         * // apply a Kaiser window with Beta = 6.5
         * h1 = Array[KaiserWindow[#, 6.5] &, n, {-1/2, 1/2}] h1;
         *
         * // reshape into 2 sections of 24 coefficients each
         * Transpose[ArrayReshape[h1, {Ceiling[n/2], 2}]]
         */
        float stage0coefficients [BMUPSAMPLER_FLTR_SECTIONS][BMUPSAMPLER_FLTR_0_LENGTH] =
        {{0.000162747, -0.000944295, 0.00117317, 0.00272824, -0.0132964,
            0.024781, -0.0187135, -0.0369711, 0.291052,
            0.291052, -0.0369711, -0.0187135, 0.024781, -0.0132964, 0.00272824,
            0.00117317, -0.000944295,
            0.000162747},
            {0.0, -0.00130751,
                0.00478209, -0.00725887, 0.0, 0.0255661, -0.0672996,
                0.107966, 0.375, 0.107966, -0.0672996, 0.0255661,
                0.0, -0.00725887,
                0.00478209, -0.00130751, 0.0, 0.0}};
        
        /*
         * n = 24;
         * h2 = LeastSquaresFilterKernel[{"Lowpass", \[Pi]/4}, n];
         * h2 = Array[KaiserWindow[#, 6.5] &, n, {-1/2, 1/2}] h2;
         * Transpose[ArrayReshape[h2, {Ceiling[n/2], 2}]]
         */
        float stage1coefficients [BMUPSAMPLER_FLTR_SECTIONS][BMUPSAMPLER_FLTR_1_LENGTH] =
        {{0.0000996524, 0.00246622, -0.00390129, -0.025939, 0.0262281, 0.186298, 0.242251, 0.101981, -0.0168389, -0.0160465, 0.00211763, 0.000974242},
         {0.000974242, 0.00211763, -0.0160465, -0.0168389, 0.101981, 0.242251, 0.186298, 0.0262281, -0.025939, -0.00390129, 0.00246622, 0.0000996524}};
        
        /*
         * n = 12;
         * h3 = LeastSquaresFilterKernel[{"Lowpass", \[Pi]/8}, n];
         * h3 = Array[KaiserWindow[#, 6.3] &, n, {-1/2, 1/2}] h3;
         * Transpose[ArrayReshape[h3, {Ceiling[n/2], 2}]]
         */
        float stage2coefficients [BMUPSAMPLER_FLTR_SECTIONS][BMUPSAMPLER_FLTR_2_LENGTH] =
        {{0.000543975, 0.0242378, 0.0947517, 0.121264, 0.0565216, 0.00641545}, {0.00641545, 0.0565216, 0.121264, 0.0947517, 0.0242378, 0.000543975}};
        
        
        /*
         * Initialise the upsampling stages
         */
        // malloc the filterbank
        This->upsamplerBank = malloc(sizeof(BMUpsampler2x)*This->numStages);
        
        // init stage 0 for 2x upsampling
        BMUpsampler2x_init(&This->upsamplerBank[0],stage0coefficients[0],stage0coefficients[1],BMUPSAMPLER_FLTR_0_LENGTH,BMUPSAMPLER_FLTR_0_LENGTH-1);
        
        // init stage 1 for 4x upsampling (total)
        if(upsampleFactor >= 4)
            BMUpsampler2x_init(&This->upsamplerBank[1],stage1coefficients[0],stage1coefficients[1],BMUPSAMPLER_FLTR_1_LENGTH,BMUPSAMPLER_FLTR_1_LENGTH);
        
        // init stage 2 for 8x upsampling (total)
        if(upsampleFactor >= 8)
            BMUpsampler2x_init(&This->upsamplerBank[2],stage2coefficients[0],stage2coefficients[1],BMUPSAMPLER_FLTR_2_LENGTH,BMUPSAMPLER_FLTR_2_LENGTH);
        
        
        // calculate the length of the impulse response
        This->IRLength = This->upsamplerBank[0].IRLength;
        if(This->numStages >= 2)
            This->IRLength += This->upsamplerBank[1].IRLength;
        if(This->numStages >= 3)
            This->IRLength += This->upsamplerBank[2].IRLength;
    }
    
    
    
    
    
    /*
     * upsample a buffer that contains a single channel of audio samples
     *
     * @param input    length = numSamplesIn
     * @param output   length = numSamplesIn * upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMUpsampler_processBuffer(BMUpsampler* This, float* input, float* output, size_t numSamplesIn){
        
        BMUpsampler2x_process(&This->upsamplerBank[0], input, output, numSamplesIn);
        
        if(This->numStages >= 2)
            BMUpsampler2x_process(&This->upsamplerBank[1], output, output, 2*numSamplesIn);
        
        if(This->numStages >= 3)
            BMUpsampler2x_process(&This->upsamplerBank[2], output, output, 4*numSamplesIn);
    }

    
    
    
    
    void BMUpsampler_free(BMUpsampler* This){
        BMUpsampler2x_free(&This->upsamplerBank[0]);
        
        if(This->numStages >= 2)
            BMUpsampler2x_free(&This->upsamplerBank[1]);
        
        if(This->numStages >= 3)
            BMUpsampler2x_free(&This->upsamplerBank[2]);
    }
    
    
    
    
    /*
     * The calling function must ensure that the length of IR is
     * at least This->IRLength
     */
    void BMUpsampler_impulseResponse(BMUpsampler* This, float* IR){
        
        BMUpsampler2x_impulseResponse(&This->upsamplerBank[0], IR);
        
        if(This->numStages >= 2)
            BMUpsampler2x_process(&This->upsamplerBank[1], IR, IR, This->upsamplerBank[0].IRLength);
        
        if(This->numStages >= 3)
            BMUpsampler2x_process(&This->upsamplerBank[2], IR, IR, This->upsamplerBank[1].IRLength);
    }
    
    
#ifdef __cplusplus
}
#endif
