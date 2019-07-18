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
#include "Constants.h"

size_t BMDownsampler2x_init (BMDownsampler2x* This, float stopbandAttenuationDb, float transitionBandwidth){
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
    
    free(coefficientArray);
    
    // allocate memory for buffers
    This->b1L = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b2L = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b1R = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b2R = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    // return the number of coefficients used
    return This->numCoefficients;
}




void BMDownsampler2x_free (BMDownsampler2x* This){
    
    for(size_t i=0; i<This->numBiquadStages; i++){
        BMMultiLevelBiquad_destroy(&This->even[i]);
        BMMultiLevelBiquad_destroy(&This->odd[i]);
    }
    
    free(This->b1L);
    free(This->b2L);
    free(This->b1R);
    free(This->b2R);
    free(This->even);
    free(This->odd);
    
    This->b1L = NULL;
    This->b2L = NULL;
    This->b1R = NULL;
    This->b2R = NULL;
    This->even = NULL;
    This->odd = NULL;
}




void BMDownsampler2x_setCoefs (BMDownsampler2x* This, const double* coef_arr){
    assert (coef_arr != 0);
    
    size_t biquadSection = 0;
    for (size_t i = 0; i < This->numCoefficients; i+=4){
        BMMultilevelBiquad_setAllpass2ndOrder(&This->even[i],coef_arr[i],coef_arr[i+2],biquadSection);
        BMMultilevelBiquad_setAllpass2ndOrder(&This->odd[i],coef_arr[i+1],coef_arr[i+3],biquadSection);
        biquadSection++;
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
    
    float* even = This->b1L;
    float* odd = This->b2L;
    
    // chunk processing
    while(numSamplesIn > 0){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE*2, numSamplesIn);
        
        float* inputShifted = input+1;
        
        // copy even input samples to even, odd to odd
        for(size_t i=0; i<samplesProcessing/2; i++){
            // even and odd are opposite what we would expect here
            even[i] = inputShifted[2*i];
            odd[i] = input[2*i];
        }
        
        // filter the input through the even filters
        BMMultiLevelBiquad_processBufferMono(This->even, even, even, samplesProcessing);
    
        // filter the input through the odd filters
        BMMultiLevelBiquad_processBufferMono(This->odd, odd, odd, samplesProcessing);
        
        // sum the even and odd outputs into the main output
        float half = 0.5;
        vDSP_vasm(even, 1, odd, 1, &half, output, 1, samplesProcessing);
        
        // advance pointers
        numSamplesIn -= samplesProcessing;
        input += samplesProcessing;
        output += samplesProcessing / 2;
    }
}





