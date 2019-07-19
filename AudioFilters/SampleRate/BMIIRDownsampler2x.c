//
//  BMIIRDownsampler2x.c
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
#include "BMIIRDownsampler2x.h"
#include "BMPolyphaseIIR2Designer.h"
#include "BMInterleaver.h"
#include "Constants.h"

// forward declaration of internal function
double* BMIIRDownsampler2x_genCoefficients(BMIIRDownsampler2x* This, float minStopbandAttenuationDb, float maxTransitionBandwidth, bool maximiseStopbandAttenuation);





size_t BMIIRDownsampler2x_init (BMIIRDownsampler2x* This,
                              float minStopbandAttenuationDb,
                              float maxTransitionBandwidth,
                              bool maximiseStopbandAttenuation,
                              bool stereo){
    This->stereo = stereo;
    
    // generate the filter coefficients. These will be first order allpass filter
    // coefficients.
    double* coefficientArray = BMIIRDownsampler2x_genCoefficients(This,
                                                                minStopbandAttenuationDb,
                                                                maxTransitionBandwidth,
                                                                maximiseStopbandAttenuation);
    
    // set up the filters
    float sampleRate = 48000.0f; // the filters will ignore this, but we have to set it to some dummy value.
    BMMultiLevelBiquad_init(&This->even, This->numBiquadStages, sampleRate, stereo, false, false);
    BMMultiLevelBiquad_init(&This->odd, This->numBiquadStages, sampleRate, stereo, false, false);
    BMIIRDownsampler2x_setCoefs(This, coefficientArray);
    
    free(coefficientArray);
    
    // allocate memory for buffers
    This->b1L = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b2L = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b1R = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->b2R = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    // return the number of coefficients used
    return This->numCoefficients;
}





/*!
 *BMIIRDownsampler2x_genCoefficients
 *
 * @abstract generates a list of first order allpass filter coefficients such that the filter cascade composed of all the even numbered coefficients generates signal A and the filter cascade composed of all the odd numbered coefficients generates signal B such that A and B are in quadrature phase across most of the frequency spectrum.
 * @param This                      pointer to a struct
 * @param minStopbandAttenuationDb     the AA filters will acheive at least this much stopband attenuation. specified in dB as a positive number.
 * @param maxTransitionBandwidth       the AA filters will not let the transition bandwidth exceed this value. In (0,0.5).
 * @param maximiseStopbandAttenuation  When this is set true, the transition bandwidth will be equal to the specified maximum value but the stopband attenuation will exceed the specified minimum value. When this is set false, the stopband attenuation will be equal to the specified minimum value but the transition bandwidth will be less than the specified maximum value.
 */
double* BMIIRDownsampler2x_genCoefficients(BMIIRDownsampler2x* This, float minStopbandAttenuationDb, float maxTransitionBandwidth, bool maximiseStopbandAttenuation){
    // find out how many allpass filter stages it will take to acheive the
    // required stopband attenuation and transition bandwidth
    This->numCoefficients = BMPolyphaseIIR2Designer_computeNbrCoefsFromProto(minStopbandAttenuationDb, maxTransitionBandwidth);
    
    // if numCoefficients is not divisible by four, increase to the nearest multiple of four
    if(This->numCoefficients % 4 != 0)
        This->numCoefficients += (4 - This->numCoefficients%4);
    
    // if we don't want to let the stopband attenuation exceed the minimum requirement
    // then we will narrow the transition bandwidth until the stopband attenuation
    // is only just barely over the specified minimum required value.
    if(!maximiseStopbandAttenuation){
        double actualAttenuation = BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)This->numCoefficients,maxTransitionBandwidth);
        while(actualAttenuation > minStopbandAttenuationDb){
            // make a small reduction in the transition bandwidth
            maxTransitionBandwidth *= 0.9995;
            // re-evaluate the transition bandwidth
            actualAttenuation = BMPolyphaseIIR2Designer_computeAttenFromOrderTbw((int)This->numCoefficients,maxTransitionBandwidth);
        }
        // we have now dropped below the required stopband attenuation, so take the
        // transition bandwidth back up one notch
        maxTransitionBandwidth /= 0.9995;
    }
    
    // Half of the biquad stages are used for even numbered samples and the
    // rest for odd. The total number of biquad filters is half the number of
    // coefficients. Therefore the number of biquad stages in each array (even,odd)
    // is numCoefficients / 4
    This->numBiquadStages = This->numCoefficients / 4;
    
    // generate filter coefficients
    double* coefficientArray = malloc(sizeof(double)*This->numCoefficients);
    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw(coefficientArray,
                                                     (int)This->numCoefficients,
                                                     maxTransitionBandwidth);
    
    return coefficientArray;
}






void BMIIRDownsampler2x_free (BMIIRDownsampler2x* This){
    
    BMMultiLevelBiquad_destroy(&This->even);
    BMMultiLevelBiquad_destroy(&This->odd);
    
    free(This->b1L);
    free(This->b2L);
    free(This->b1R);
    free(This->b2R);
    
    This->b1L = NULL;
    This->b2L = NULL;
    This->b1R = NULL;
    This->b2R = NULL;
}




void BMIIRDownsampler2x_setCoefs (BMIIRDownsampler2x* This, const double* coef_arr){
    assert (coef_arr != 0);
    
    size_t biquadSection = 0;
    for (size_t i = 0; i < This->numCoefficients; i+=4){
        BMMultilevelBiquad_setAllpass2ndOrder(&This->even,
                                              coef_arr[i],coef_arr[i+2],
                                              biquadSection);
        BMMultilevelBiquad_setAllpass2ndOrder(&This->odd,
                                              coef_arr[i+1],coef_arr[i+3],
                                              biquadSection);
        biquadSection++;
    }
}






void BMIIRDownsampler2x_processBufferMono (BMIIRDownsampler2x* This, float* input, float* output, size_t numSamplesIn){
    assert(!This->stereo);
    assert (output != 0);
    assert (input != 0);
    assert (output >= input + numSamplesIn || input >= output + numSamplesIn);
    assert (numSamplesIn > 0);
    
    float* even = This->b1L;
    float* odd = This->b2L;
    
    // chunk processing
    while(numSamplesIn > 0){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE*2, numSamplesIn);
        
        // copy even input samples to even, odd to odd
        BMDeInterleave(input, even, odd, samplesProcessing);
        
        // filter the odd-indexed inputs through the even-indexed filters
        BMMultiLevelBiquad_processBufferMono(&This->even, odd, odd, samplesProcessing/2);
    
        // filter the even-indexed inputs through the odd-indexed filters
        BMMultiLevelBiquad_processBufferMono(&This->odd, even, even, samplesProcessing/2);
        
        // sum the even and odd outputs into the main output and divide by two
        float half = 0.5;
        vDSP_vasm(even, 1, odd, 1, &half, output, 1, samplesProcessing/2);
        
        // advance pointers
        numSamplesIn -= samplesProcessing;
        input += samplesProcessing;
        output += samplesProcessing / 2;
    }
}
