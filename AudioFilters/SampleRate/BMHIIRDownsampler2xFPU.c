//
//  BMHIIRDownsampler2xFPU.c
//  AudioFiltersXcodeProject
//
//  Based on Downsampler2xFpu.hpp from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//



#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "BMHIIRDownsampler2xFPU.h"
#include "BMHIIRStageFPU.h"
#include "BMPolyphaseIIR2Designer.h"




float BMHIIRDownsampler2xFPU_init (BMHIIRDownsampler2xFPU* This, float stopbandAttenuationDb, float transitionBandwidth){
    // find out how many allpass filter stages it will take to acheive the
    // required stopband attenuation and transition bandwidth
    This->numCoefficients = BMPolyphaseIIR2Designer_computeNbrCoefsFromProto(stopbandAttenuationDb, transitionBandwidth);
    
    This->coef = malloc(sizeof(float)*This->numCoefficients);
    This->x = malloc(sizeof(float)*This->numCoefficients);
    This->y = malloc(sizeof(float)*This->numCoefficients);
    
    // generate filter coefficients
    double* coefficientArray = malloc(sizeof(double)*This->numCoefficients);
    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw(coefficientArray,
                                                     (int)This->numCoefficients,
                                                     transitionBandwidth);
    
    // set up the filters
    BMHIIRDownsampler2xFPU_setCoefs(This, coefficientArray);
    BMHIIRDownsampler2xFPU_clearBuffers(This);
    
    free(coefficientArray);
    
    return BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)This->numCoefficients, transitionBandwidth);
}




void BMHIIRDownsampler2xFPU_free (BMHIIRDownsampler2xFPU* This){
    free(This->coef);
    free(This->y);
    free(This->x);
    
    This->coef = NULL;
    This->y = NULL;
    This->x = NULL;
}




void BMHIIRDownsampler2xFPU_setCoefs (BMHIIRDownsampler2xFPU* This, const double* coef_arr){
    assert (coef_arr != 0);
    
    for (size_t i = 0; i < This->numCoefficients; ++i)
        This->coef[i] = (float)(coef_arr[i]);
}




void BMHIIRDownsampler2xFPU_processSample(BMHIIRDownsampler2xFPU* This, float* input2, float* output){
    
    // process the sample
    BMHIIRStageFPU_processSamplePos(This->numCoefficients, This->numCoefficients, input2+1, input2, This->coef, This->x, This->y);
    
    // average the two results to get the output
    *output = 0.5f * (*input2 + *(input2+1));
}




void BMHIIRDownsampler2xFPU_processBufferMono (BMHIIRDownsampler2xFPU* This, float* input, float* output, size_t numSamplesIn){
    assert (output != 0);
    assert (input != 0);
    assert (output >= input + numSamplesIn || input >= output + numSamplesIn);
    assert (numSamplesIn > 0);
    
    while(numSamplesIn > 0)
    {
        BMHIIRDownsampler2xFPU_processSample (This,
                                              input,output);
        input += 2;
        output += 1;
        numSamplesIn -=2;
    }
}




void BMHIIRDownsampler2xFPU_clearBuffers (BMHIIRDownsampler2xFPU* This){
    memset(This->x,0,sizeof(float)*This->numCoefficients);
    memset(This->y,0,sizeof(float)*This->numCoefficients);
}


