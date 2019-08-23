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

#define BM_DOWNSAMPLER_CHUNK_SIZE BM_BUFFER_CHUNK_SIZE * 4

// forward declaration of internal function
double* BMIIRDownsampler2x_genCoefficients(BMIIRDownsampler2x* This, float minStopbandAttenuationDb, float maxTransitionBandwidth);





size_t BMIIRDownsampler2x_init (BMIIRDownsampler2x* This,
                              float minStopbandAttenuationDb,
                              float maxTransitionBandwidth,
                              bool stereo){
    This->stereo = stereo;
    
    // generate the filter coefficients. These will be first order allpass filter
    // coefficients.
    double* coefficientArray = BMIIRDownsampler2x_genCoefficients(This,
                                                                minStopbandAttenuationDb,
                                                                maxTransitionBandwidth);
    
    // set up the filters
    float sampleRate = 48000.0f; // the filters will ignore this, but we have to set it to some dummy value.
    BMMultiLevelBiquad_init(&This->even, This->numBiquadStages, sampleRate, stereo, false, false);
    BMMultiLevelBiquad_init(&This->odd, This->numBiquadStages, sampleRate, stereo, false, false);
    BMIIRDownsampler2x_setCoefs(This, coefficientArray);
    
    free(coefficientArray);
    
    // allocate memory for buffers
    This->b1L = malloc(sizeof(float)*BM_DOWNSAMPLER_CHUNK_SIZE);
    This->b2L = malloc(sizeof(float)*BM_DOWNSAMPLER_CHUNK_SIZE);
    if(stereo){
        This->b1R = malloc(sizeof(float)*BM_DOWNSAMPLER_CHUNK_SIZE);
        This->b2R = malloc(sizeof(float)*BM_DOWNSAMPLER_CHUNK_SIZE);
    } else {
        This->b2R = NULL;
        This->b2R = NULL;
    }
    
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
 */
double* BMIIRDownsampler2x_genCoefficients(BMIIRDownsampler2x* This, float minStopbandAttenuationDb, float maxTransitionBandwidth){
    // find out how many allpass filter stages it will take to acheive the
    // required stopband attenuation and transition bandwidth
    This->numCoefficients = BMPolyphaseIIR2Designer_computeNbrCoefsFromProto(minStopbandAttenuationDb, maxTransitionBandwidth);
    
    printf("BMDownsampler: numCoefficients before rounding: %zu\n",This->numCoefficients);
    
    // if numCoefficients is not divisible by four, increase to the nearest multiple of four
    if(This->numCoefficients % 4 != 0)
        This->numCoefficients += (4 - This->numCoefficients%4);
    
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
    if(This->stereo){
        free(This->b1R);
        free(This->b2R);
    }
    
    This->b1L = NULL;
    This->b2L = NULL;
    This->b1R = NULL;
    This->b2R = NULL;
}




void BMIIRDownsampler2x_setCoefs (BMIIRDownsampler2x* This, const double* coef_arr){
    assert (coef_arr != 0);
    
    /*
     * In theory, the ordering of the filters is irrelevant. We simply need
     * to put all the allpass filters with even numbered coefficients into
     * one filter array and all the odd numbered filters in the other.
     * However, when we combine two first order filters into a single
     * second-order biquad section, if both coefficients are small, the
     * coefficients of the resulting biquad section that must be set equal
     * to the product of the two first order allpass coefficients will be
     * near zero and therefore lead to a significant increase in
     * quantisation noise. To prevent this, we note that the coefficients
     * in coef_arr are sorted from smallest to largest and therefore we
     * avoid producing excessively small numbers in the biquad filters by
     * combining filter coefficients from opposite ends of the array.
     *
     * For example, if coef_array has length 8 then the even biquad filters
     * will use coefficient pairs {0,6} and {2,4}, rather than {0,2} and {4,6}.
     * The resulting filter cascade has the same transfer function either
     * way but by ordering them thus we keep quantisation noise below the
     * noise floor.
     */
    size_t biquadSection = 0;
    for (size_t i = 0; i < This->numCoefficients/2; i+=2){
        BMMultilevelBiquad_setAllpass2ndOrder(&This->even,
                                              coef_arr[i],coef_arr[This->numCoefficients - (i+2)],
                                              biquadSection);
        BMMultilevelBiquad_setAllpass2ndOrder(&This->odd,
                                              coef_arr[i+1],coef_arr[This->numCoefficients - (i+1)],
                                              biquadSection);
        biquadSection++;
    }
}






void BMIIRDownsampler2x_processBufferMono (BMIIRDownsampler2x* This, const float* input, float* output, size_t numSamplesIn){
    assert(!This->stereo);
    assert (output != input);
    
    float* even = This->b1L;
    float* odd = This->b2L;
    
    // chunk processing
    while(numSamplesIn > 0){
        size_t samplesProcessing = BM_MIN(BM_DOWNSAMPLER_CHUNK_SIZE*2, numSamplesIn);
        
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



void BMIIRDownsampler2x_processBufferStereo (BMIIRDownsampler2x* This, const float* inputL, const float* inputR, float* outputL, float* outputR, size_t numSamplesIn){
    assert(This->stereo);
    assert (outputL != inputL);
    assert (outputR != inputR);
    
    float* evenL = This->b1L;
    float* oddL = This->b2L;
    float* evenR = This->b1R;
    float* oddR = This->b2R;
    
    // chunk processing
    while(numSamplesIn > 0){
        size_t samplesProcessing = BM_MIN(BM_DOWNSAMPLER_CHUNK_SIZE*2, numSamplesIn);
        
        // copy even input samples to even, odd to odd
        BMDeInterleave(inputL, evenL, oddL, samplesProcessing);
        BMDeInterleave(inputR, evenR, oddR, samplesProcessing);
        
        // filter the odd-indexed inputs through the even-indexed filters
        BMMultiLevelBiquad_processBufferStereo(&This->even, oddL, oddR, oddL, oddR, samplesProcessing/2);
        
        // filter the even-indexed inputs through the odd-indexed filters
        BMMultiLevelBiquad_processBufferStereo(&This->odd, evenL, evenR, evenL, evenR, samplesProcessing/2);
        
        // sum the even and odd outputs into the main output and divide by two
        float half = 0.5;
        vDSP_vasm(evenL, 1, oddL, 1, &half, outputL, 1, samplesProcessing/2);
        vDSP_vasm(evenR, 1, oddR, 1, &half, outputR, 1, samplesProcessing/2);
        
        // advance pointers
        numSamplesIn -= samplesProcessing;
        inputL += samplesProcessing;
        outputL += samplesProcessing / 2;
        inputR += samplesProcessing;
        outputR += samplesProcessing / 2;
    }
}
