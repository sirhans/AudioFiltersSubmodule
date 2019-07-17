//
//  BMHIIRUpsampler2xFPU.c
//  AudioFiltersXcodeProject
//
//  Based on Upsampler2xFpu.hpp from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "BMHIIRUpsampler2xFPU.h"
#include "BMHIIRStageFPU.h"
#include "BMPolyphaseIIR2Designer.h"




float BMHIIRUpsampler2xFPU_init (BMHIIRUpsampler2xFPU* This, size_t numCoefficients, float transitionBandwidth){
    This->numCoefficients = numCoefficients;
    
    This->coef = malloc(sizeof(float)*numCoefficients);
    This->x = malloc(sizeof(float)*numCoefficients);
    This->y = malloc(sizeof(float)*numCoefficients);
    
    // generate filter coefficients
    double* coefficientArray = malloc(sizeof(double)*numCoefficients);
    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw(coefficientArray,
                                                     (int)numCoefficients,
                                                     transitionBandwidth);
    
    // set up the filters
    BMHIIRUpsampler2xFPU_setCoefs(This, coefficientArray);
    BMHIIRUpsampler2xFPU_clearBuffers(This);
    
    free(coefficientArray);
    
    return BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)numCoefficients, transitionBandwidth);
}



void BMHIIRUpsampler2xFPU_free (BMHIIRUpsampler2xFPU* This){
    free(This->coef);
    free(This->y);
    free(This->x);
    
    This->coef = NULL;
    This->y = NULL;
    This->x = NULL;
}




void BMHIIRUpsampler2xFPU_setCoefs (BMHIIRUpsampler2xFPU* This, const double* coef_arr){
    assert (coef_arr != 0);
    
    for (size_t i = 0; i < This->numCoefficients; ++i)
        This->coef[i] = (float)(coef_arr[i]);
}




void BMHIIRUpsampler2xFPU_processSample(BMHIIRUpsampler2xFPU* This, float* out_0, float* out_1, float input){
    // copy input to both outputs
    *out_0 = *out_1 = input;
    
    // and process the sample
    BMHIIRStageFPU_processSamplePos(This->numCoefficients, This->numCoefficients, out_0, out_1, This->coef, This->x, This->y);
}




void BMHIIRUpsampler2xFPU_processBufferMono (BMHIIRUpsampler2xFPU* This, const float* in_ptr, float* out_ptr, size_t nbr_spl){
    assert (out_ptr != 0);
    assert (in_ptr != 0);
    assert (out_ptr >= in_ptr + nbr_spl || in_ptr >= out_ptr + nbr_spl);
    assert (nbr_spl > 0);
    
    long           pos = 0;
    do
    {
        BMHIIRUpsampler2xFPU_processSample (This,
                                             &out_ptr[pos * 2],
                                             &out_ptr[pos * 2 + 1],
                                             in_ptr[pos]);
        ++pos;
    }
    while (pos < nbr_spl);
}



void BMHIIRUpsampler2xFPU_clearBuffers (BMHIIRUpsampler2xFPU* This){
    memset(This->x,0,sizeof(float)*This->numCoefficients);
    memset(This->y,0,sizeof(float)*This->numCoefficients);
}


