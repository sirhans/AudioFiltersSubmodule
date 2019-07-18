//
//  BMDownsampler2x.c
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
#include "BMDownsampler2x.h"
#include "BMHIIRStageFPU.h"
#include "BMPolyphaseIIR2Designer.h"



float BMDownsampler2x_init (BMDownsampler2x* This, float stopbandAttenuationDb, float transitionBandwidth){
    // find out how many allpass filter stages it will take to acheive the
    // required stopband attenuation and transition bandwidth
    This->numCoefficients = BMPolyphaseIIR2Designer_computeNbrCoefsFromProto(stopbandAttenuationDb, transitionBandwidth);
    
    // if numCoefficients is not divisible by four
    if(This->numCoefficients % 4 != 0) {
        // increase to make it divisible
        This->numCoefficients += (4 - This->numCoefficients%4);
    }
    
    
    // We usually overshoot the specified stopbandAttenuation, especially when
    // we increase the number of coefficients to get a multiple of four.
    // We want to use the extra precision we get from that to decrease the
    // transition bandwidth.
    // Below we iteratively test various transition bandwidth settings until we
    // find one that places the stopBand attenuation just above the required
    // value.
    double actualAttenuation = BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)This->numCoefficients,transitionBandwidth);
    while(actualAttenuation > stopbandAttenuationDb){
        // make a small reduction in the transition bandwidth
        transitionBandwidth *= 0.9995;
        // re-evaluate the transition bandwidth
        actualAttenuation = BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)This->numCoefficients,transitionBandwidth);
    }
    // we have now dropped below the required stopband attenuation, so take the
    // transition bandwidth back up one notch
    transitionBandwidth /= 0.9995;
    
    // Half of the biquad stages are used for even numbered samples and the
    // rest for odd. The total number of biquad filters is half the number of
    // coefficients. Therefore the number of biquad stages in each array (even,odd)
    // is numCoefficients / 4
    This->numBiquadStages = This->numCoefficients / 4;
    
    // allocate the biquad filters
    This->even = malloc(sizeof(BMMultiLevelBiquad)*This->numBiquadStages);
    This->odd = malloc(sizeof(BMMultiLevelBiquad)*This->numBiquadStages);
    
    // generate filter coefficients
    double* coefficientArray = malloc(sizeof(double)*This->numCoefficients);
    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw(coefficientArray,
                                                     (int)This->numCoefficients,
                                                     transitionBandwidth);
    
    // set up the filters
    BMDownsampler2x_setCoefs(This, coefficientArray);
    BMDownsampler2x_clearBuffers(This);
    
    free(coefficientArray);
    
    return BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)This->numCoefficients, transitionBandwidth);
}




void BMDownsampler2x_free (BMDownsampler2x* This){
    free(This->coef);
    free(This->y);
    free(This->x);
    
    This->coef = NULL;
    This->y = NULL;
    This->x = NULL;
}




void BMDownsampler2x_setCoefs (BMDownsampler2x* This, const double* coef_arr){
    assert (coef_arr != 0);
    
    for (size_t i = 0; i < This->numCoefficients; i+=4){
        BMMultilevelBiquad_setAllpass2ndOrder(&This->even[i],coef_arr[i],coef_arr[i+2]);
        BMMultilevelBiquad_setAllpass2ndOrder(&This->odd[i],coef_arr[i+1],coef_arr[i+3]);
    }
}




//void BMDownsampler2x_processSample(BMDownsampler2x* This, float* input2, float* output){
//
//    // process the sample
//    BMHIIRStageFPU_processSamplePos(This->numCoefficients, This->numCoefficients, input2+1, input2, This->coef, This->x, This->y);
//
//    // average the two results to get the output
//    *output = 0.5f * (*input2 + *(input2+1));
//}




void BMDownsampler2x_processBufferMono (BMDownsampler2x* This, float* input, float* output, size_t numSamplesIn){
    assert (output != 0);
    assert (input != 0);
    assert (output >= input + numSamplesIn || input >= output + numSamplesIn);
    assert (numSamplesIn > 0);
    
    while(numSamplesIn > 0)
    {
//        BMDownsampler2x_processSample (This,
                                              input,output);
        input += 2;
        output += 1;
        numSamplesIn -=2;
    }
}




void BMDownsampler2x_clearBuffers (BMDownsampler2x* This){
    memset(This->x,0,sizeof(float)*This->numCoefficients);
    memset(This->y,0,sizeof(float)*This->numCoefficients);
}


