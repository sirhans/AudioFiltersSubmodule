//
//  BMSincUpsampler.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/16/20.
//  This file is released to the public domain.
//

#include "BMSincUpsampler.h"
#include <Accelerate/Accelerate.h>
#include "BMFFT.h"


void BMSincUpsampler_initFilterKernels(BMSincUpsampler *This);






void BMSincUpsampler_init(BMSincUpsampler *This,
                          size_t interpolationPoints,
                          size_t upsampleFactor){
    
    // for non-trivial upsample factors
    if(upsampleFactor > 1){
        // even filter kernel lengths only
        assert(interpolationPoints % 2 == 0);
        
        This->upsampleFactor = upsampleFactor;
        This->kernelLength = interpolationPoints;
        This->inputPaddingLeft = (This->kernelLength / 2) - 1;
        This->inputPaddingRight = (This->kernelLength / 2) - 1;
        This->numKernels = This->upsampleFactor;
        
        This->filterKernels = malloc(sizeof(float*) * This->numKernels);
        for(size_t i=0; i<This->numKernels; i++)
            This->filterKernels[i] = malloc(sizeof(float*)*This->kernelLength);
        
        BMSincUpsampler_initFilterKernels(This);
    }
    
    // if upsample factor is 1 then this code will do nothing
    else {
        This->upsampleFactor = 1;
        This->inputPaddingLeft = 0;
        This->inputPaddingRight = 0;
    }
}





void BMSincUpsampler_free(BMSincUpsampler *This){
    if(This->upsampleFactor > 1){
        for(size_t i=0; i<This->numKernels; i++)
            free(This->filterKernels[i]);
        free(This->filterKernels);
        This->filterKernels = NULL;
    }
}





size_t BMSincUpsampler_process(BMSincUpsampler *This,
                             const float* input,
                             float* output,
                             size_t inputLength){
    // for trivial upsample factors, do nothing
    if(This->upsampleFactor == 1){
        memcpy(output, input, sizeof(float)*inputLength);
        return inputLength;
    }
    
    assert(input != output);
    
    // how many samples of the input must be left out from the output at the beginning and end?
    size_t inputLengthMinusPadding = inputLength - This->inputPaddingLeft - This->inputPaddingRight;
    
    // do the interpolation by convolution
    // the index starts at 1 because the first filter kernel will be
    // replaced by the vsmul command on the line below
    for(size_t i=1; i<This->upsampleFactor; i++)
        vDSP_conv(input, 1, This->filterKernels[This->numKernels - i], 1, output+i, This->upsampleFactor, inputLengthMinusPadding-1, This->kernelLength);
    
    // copy the original samples from input to output to fill in the remaining samples
    float zero = 0.0f;
    vDSP_vsadd(input+This->inputPaddingLeft, 1, &zero, output, This->upsampleFactor, inputLengthMinusPadding);
    
    return 1 + (inputLengthMinusPadding - 1)*This->upsampleFactor;
}





/*!
 *BMSincUpsampler_inputPaddingBefore
 *
 * @returns the number of samples at the beginning of the input array that are not present in the output
 */
size_t BMSincUpsampler_inputPaddingBefore(BMSincUpsampler *This){
    return This->inputPaddingLeft;
}




/*!
 *BMSincUpsampler_inputPaddingAfter
 *
 * @returns the number of samples at the end of the input that are not present in the output
 */
size_t BMSincUpsampler_inputPaddingAfter(BMSincUpsampler *This){
    return This->inputPaddingRight;
}




/*!
 *BMSincUpsampler_outputLength
 *
 * @returns the number of samples output for the given length of input
 */
size_t BMSincUpsampler_outputLength(BMSincUpsampler *This, size_t inputLength){
    // how many samples of the input must be left out from the output at the beginning and end?
    size_t inputLengthMinusPadding = inputLength - This->inputPaddingLeft - This->inputPaddingRight;
    return 1 + (inputLengthMinusPadding - 1)*This->upsampleFactor;
}








void BMSincUpsampler_initFilterKernels(BMSincUpsampler *This){
    // allocate an array for the complete discretised sinc interpolation function
    size_t completeKernelLength = This->kernelLength * This->upsampleFactor;
    float *interleavedKernel = malloc(sizeof(float) * completeKernelLength);
    
    // generate the sinc function
    // Mathematica prototype:
    //    Table[Sinc[(t - K/2) \[Pi] / us], {t, 0, K}]
    for(size_t t=0; t<completeKernelLength; t++){
        // rename and cast variables for easier readability
        double td = t;
        double K = completeKernelLength;
        double us = This->upsampleFactor;
        double phi = (td - (K / 2.0)) * M_PI / us;
        
        // calculate the sinc function
        if(fabs(phi) > FLT_EPSILON)
            interleavedKernel[t] = sin(phi)/phi;
        else
            interleavedKernel[t] = 1.0;
    }
    
    // generate window coefficients (hamming, blackman-harris, hann, etc.)
    //
    // note that the window has an extra coefficient at the end that is not used
    // on the kernel. This is because the complete interleaved kernel is not
    // perfectly symmetric. It is missing a zero at the end that would make it
    // symmetric. That zero is missing because it doesn't fit into any of the
    // polyphase kernels. Technically, that final zero belongs to the first
    // polyphase kernel, the one that is all zeros except its centre element.
    //
    size_t windowLength = completeKernelLength + 1;
    float* window = malloc(sizeof(float)*windowLength);
    // BMFFT_generateBlackmanHarrisCoefficients(window, windowLength);
    // BMFFT_generateKaiserCoefficients(window, windowLength);
    int useHalfWindow = false;
    //vDSP_hamm_window(window, windowLength, useHalfWindow);
    vDSP_blkman_window(window, windowLength, useHalfWindow);
    
    // apply the blackman-harris window to the interleaved kernel
    vDSP_vmul(window,1,interleavedKernel,1,interleavedKernel,1,completeKernelLength);
    
    // copy from the complete kernel to the polyphase kernels
    for(size_t i=0; i<This->numKernels; i++)
        for(size_t j=0; j<This->kernelLength; j++)
            This->filterKernels[i][j] = interleavedKernel[j*This->numKernels + i];
    
    free(interleavedKernel);
    free(window);
}
