//
//  BMOscillatorArray.c
//  BMAudioFilters
//
//  Created by Hans on 28/4/17.
//  Copyright © 2017 Hans. All rights reserved.
//

#include "BMOscillatorArray.h"
#include "BMQuadratureOscillator.h"
#include <Accelerate/Accelerate.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    
    /*
     * Initialize
     *
     * @param This       pointer to an oscillator array struct
     * @param magnitude  magnitude energy output of each oscillator
     * @param phase      initial phase of each oscillator
     * @param frequency  frequency of each oscillator
     * @param sampleRate sample rate in Hz
     * @param length     number of oscillators in the array
     *
     * ALL ARRAYS MUST HAVE AT LEAST length ELEMENTS
     */
    void BMOscillatorArray_init(BMOscillatorArray *This,
                                float* magnitude,
                                float* phase,
                                float* frequency,
                                float  sampleRate,
                                size_t length){
        
        // allocate memory
        This->m11 = malloc(sizeof(float)*length);
        This->m12 = malloc(sizeof(float)*length);
        This->m21 = malloc(sizeof(float)*length);
        This->m22 = malloc(sizeof(float)*length);
        This->temp = malloc(sizeof(float)*length);
        This->r = malloc(sizeof(float)*length);
        This->q = malloc(sizeof(float)*length);
        
        
        // initialize rotation matrices and energy store values
        for(size_t i=0; i < length; i++){
            // init a matrix that rotates at the specified frequency
            simd_float2x2 m;
            BMQuadratureOscillator_initMatrix(&m, frequency[i], sampleRate);
            
            // copy the matrix values into the arrays
			This->m11[i] = m.columns[0][0]; // m.m11;
			This->m12[i] = m.columns[1][0]; // m.m12;
			This->m21[i] = m.columns[0][1]; // m.m21;
			This->m22[i] = m.columns[1][1]; // m.m22;
    
            // set initial values at the specified magnitudes and phases
            This->r[i] = magnitude[i]*sinf(phase[i]);
            This->q[i] = magnitude[i]*cosf(phase[i]);
        }
    }
    
    
    
    
    void BMOscillatorArray_destroy(BMOscillatorArray *This){
        free(This->m11);
        free(This->m12);
        free(This->m21);
        free(This->m22);
        free(This->temp);
        free(This->r);
        free(This->q);
        
        This->m11 = NULL;
        This->m12 = NULL;
        This->m21 = NULL;
        This->m22 = NULL;
        This->temp = NULL;
        This->r = NULL;
        This->q = NULL;
    }
    
    
    
    
    
    
    /*
     * Generate one sample of audio, store it in output
     *
     * @param output     pointer to the output
     *
     */
    void BMOscillatorArray_processSample(BMOscillatorArray *This,
                                         float* output){
        // the following is a vectorised multiplication:
        //
        //     [r; q] = m.[r; q]
        //
        // where each element in the vector belongs to another oscillator.
        // each oscillator has its own r, q, and m.
        //
        // temp = r*11 + q*12
        vDSP_vmma(This->r, 1, This->m11, 1,
                  This->q, 1, This->m12, 1,
                  This->temp, 1, This->length);
        
        // q = r*21 + q*22
        vDSP_vmma(This->r, 1, This->m21, 1,
                  This->q, 1, This->m22, 1,
                  This->q, 1, This->length);
        
        
        // we stored the first row output to temp; now we restore it
        // to its final place in r
        //
        // pointerSwap(temp,r)
        float* k = This->r;
        This->r = This->temp;
        This->temp = k;
        
        
        // finally, mix the output of all the oscillators to one channel
        // output = sum(r);
        vDSP_sve(This->r, 1, output, This->length);
    }
    
    
    
#ifdef __cplusplus
}
#endif
