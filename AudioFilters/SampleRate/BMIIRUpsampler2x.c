//
//  BMIIRUpsampler2x.c
//  AudioFiltersXcodeProject
//
//  Based on Upsampler2xFpu.hpp from Laurent de Soras HIIR library
//  http://ldesoras.free.fr/prod.html
//
//  Created by Hans on 9/7/19.
//  Anyone may use this file without restrictions of any kind
//


#include <assert.h>
#include <string.h>
#include "BMIIRUpsampler2x.h"
#include "BMPolyphaseIIR2Designer.h"
#include "BMInterleaver.h"
#include "Constants.h"

#define BM_UPSAMPLER_CHUNK_SIZE BM_BUFFER_CHUNK_SIZE * 4



// forward declaration of internal function
double* BMIIRUpsampler2x_genCoefficients(BMIIRUpsampler2x *This, float minStopbandAttenuationDb, float maxTransitionBandwidth);





size_t BMIIRUpsampler2x_init (BMIIRUpsampler2x *This,
                              float minStopbandAttenuationDb,
                              float maxTransitionBandwidth,
                              bool stereo){
    This->stereo = stereo;
    
    // generate the filter coefficients. These will be first order allpass filter
    // coefficients.
    double* coefficientArray = BMIIRUpsampler2x_genCoefficients(This,
                                                                minStopbandAttenuationDb,
                                                                maxTransitionBandwidth);
    
    // Half of the biquad stages are used for even numbered samples and the
    // rest for odd. The total number of biquad filters is half the number of
    // coefficients. Therefore the number of biquad stages in each array (even,odd)
    // is numCoefficients / 4
    This->numBiquadStages = This->numCoefficients / 4;
    // if numCoefficients / 2 is odd then we need an extra first order filter section
    if(This->numCoefficients/2 % 2 == 1) This->numBiquadStages += 1;
    
    // set up the filters
    float sampleRate = 48000.0f; // the filters will ignore this, but we have to set it to some dummy value.
    BMMultiLevelBiquad_init(&This->even, This->numBiquadStages, sampleRate, stereo, true, false);
    BMMultiLevelBiquad_init(&This->odd, This->numBiquadStages, sampleRate, stereo, true, false);
    BMIIRUpsampler2x_setCoefs(This, coefficientArray);
    
    
    free(coefficientArray);
    
    // allocate memory for buffers
    This->b1L = malloc(sizeof(float)*BM_UPSAMPLER_CHUNK_SIZE);
    This->b2L = malloc(sizeof(float)*BM_UPSAMPLER_CHUNK_SIZE);
    if(stereo){
        This->b1R = malloc(sizeof(float)*BM_UPSAMPLER_CHUNK_SIZE);
        This->b2R = malloc(sizeof(float)*BM_UPSAMPLER_CHUNK_SIZE);
    } else {
        This->b1R = NULL;
        This->b2R = NULL;
    }
    
    // return the number of coefficients used
    return This->numCoefficients;
}




/*!
 *BMIIRUpsampler2x_genCoefficients
 *
 * @abstract generates a list of first order allpass filter coefficients such that the filter cascade composed of all the even numbered coefficients generates signal A and the filter cascade composed of all the odd numbered coefficients generates signal B such that A and B are in quadrature phase across most of the frequency spectrum.
 * @param This                      pointer to a struct
 * @param minStopbandAttenuationDb     the AA filters will acheive at least this much stopband attenuation. specified in dB as a positive number.
 * @param maxTransitionBandwidth       the AA filters will not let the transition bandwidth exceed this value. In (0,0.5).
 */
double* BMIIRUpsampler2x_genCoefficients(BMIIRUpsampler2x *This, float minStopbandAttenuationDb, float maxTransitionBandwidth){
    // find out how many allpass filter stages it will take to acheive the
    // required stopband attenuation and transition bandwidth
    This->numCoefficients = BMPolyphaseIIR2Designer_computeNbrCoefsFromProto(minStopbandAttenuationDb, maxTransitionBandwidth);
    
    printf("BMUpsampler: numCoefficients before rounding: %zu\n",This->numCoefficients);
    
    //    // if numCoefficients is not divisible by four, increase to the nearest multiple of four
    //    if(This->numCoefficients % 4 != 0)
    //        This->numCoefficients += (4 - This->numCoefficients%4);
    
    // if numCoefficients is not divisible by two, increase to the nearest multiple of two
    if(This->numCoefficients % 2 != 0)
        This->numCoefficients += 1;
    
    // generate filter coefficients
    double* coefficientArray = malloc(sizeof(double)*This->numCoefficients);
    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw(coefficientArray,
                                                     (int)This->numCoefficients,
                                                     maxTransitionBandwidth);
    
    return coefficientArray;
}






void BMIIRUpsampler2x_free (BMIIRUpsampler2x *This){
    
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




void BMIIRUpsampler2x_setCoefs (BMIIRUpsampler2x *This, const double* coef_arr){
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
    size_t i;
    for (i = 0; i < (This->numCoefficients-1)/2; i+=2){
        BMMultilevelBiquad_setAllpass2ndOrder(&This->even,
                                              coef_arr[i],coef_arr[This->numCoefficients - (i+2)],
                                              biquadSection);
        BMMultilevelBiquad_setAllpass2ndOrder(&This->odd,
                                              coef_arr[i+1],coef_arr[This->numCoefficients - (i+1)],
                                              biquadSection);
        biquadSection++;
    }
    // if numCoefficients/2 is odd, pick up the last coefficient with a first order section
    if(i<This->numCoefficients/2){
        BMMultilevelBiquad_setAllpass1stOrder(&This->even, coef_arr[i], biquadSection);
        BMMultilevelBiquad_setAllpass1stOrder(&This->odd, coef_arr[i+1], biquadSection);
    }
}






void BMIIRUpsampler2x_processBufferMono(BMIIRUpsampler2x *This, const float* input, float* output, size_t numSamplesIn){
    assert(!This->stereo);
    assert(input != output);
    
    float* even = This->b1L;
    float* odd = This->b2L;
    
    // chunk processing
    while(numSamplesIn > 0){
        size_t samplesProcessing = BM_MIN(BM_UPSAMPLER_CHUNK_SIZE, numSamplesIn);
        
        // filter the inputs through the even-indexed filters
        BMMultiLevelBiquad_processBufferMono(&This->even, input, even, samplesProcessing);
        
        // filter the inputs through the odd-indexed filters
        BMMultiLevelBiquad_processBufferMono(&This->odd, input, odd, samplesProcessing);
        
        // the even and odd signals are now in quadrature phase.
        // interleave the even and odd samples into one array
        BMInterleave(even, odd, output, samplesProcessing);
        
        // advance pointers
        numSamplesIn -= samplesProcessing;
        input += samplesProcessing;
        output += samplesProcessing * 2;
    }
}




void BMIIRUpsampler2x_processBufferStereo (BMIIRUpsampler2x *This, const float* inputL, const float* inputR, float* outputL, float* outputR, size_t numSamplesIn){
    assert(This->stereo);
    assert(inputL != outputL);
    assert(inputR != outputR);
    
    float* evenL = This->b1L;
    float* oddL = This->b2L;
    float* evenR = This->b1R;
    float* oddR = This->b2R;
    
    // chunk processing
    while(numSamplesIn > 0){
        size_t samplesProcessing = BM_MIN(BM_UPSAMPLER_CHUNK_SIZE, numSamplesIn);
        
        // filter the inputs through the even-indexed filters
        BMMultiLevelBiquad_processBufferStereo(&This->even, inputL, inputR, evenL, evenR, samplesProcessing);
        
        // filter the inputs through the odd-indexed filters
        BMMultiLevelBiquad_processBufferStereo(&This->odd, inputL, inputR, oddL, oddR, samplesProcessing);
        
        // the even and odd signals are now in quadrature phase.
        // interleave the even and odd samples into one array
        BMInterleave(evenL, oddL, outputL, samplesProcessing);
        BMInterleave(evenR, oddR, outputR, samplesProcessing);
        
        // advance pointers
        numSamplesIn -= samplesProcessing;
        inputL += samplesProcessing;
        outputL += samplesProcessing * 2;
        inputR += samplesProcessing;
        outputR += samplesProcessing * 2;
    }
}
