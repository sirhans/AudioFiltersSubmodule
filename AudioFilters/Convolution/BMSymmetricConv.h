//
//  BMSymmetricConv.h
//  BMAudioFilters
//
//  This is a replacement for vDSP_conv that avoids multiplying twice by
//  the same value when the filter kernel is symmetric about its centre.
//  It also saves time by detecting zeros in the kernel and not multiplying
//  them.
//
//  The implementation uses direct time-domain convolution so it will only
//  be efficient for small filter kernel sizes.
//
//  Created by Hans on 15/9/17.
//  This file may be used by anyone without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif
    
#include <Accelerate/Accelerate.h>
    
#ifndef BMSymmetricConv_h
#define BMSymmetricConv_h
    
    
    /*
     * convolution with a symmetric filter kernel
     *
     * @param filterKernel   the complete set of filter coefficients
     * @param filterStride   used for polyphase filtering
     * @param audioInput     with length/inputStride >= numSamples + filterSamplesUsed - 1
     * @param inputStride    used for interleaved stereo audio input
     * @param audioOutput    with length >= numSamples * outputStride
     * @param outputStride   used for polyphase upsampling
     * @param buffer         calling function must provide this for temp storage
     * ****** bufferLength   must be >= numSamples + filterSamplesUsed - 1
     * @param filterSamplesUsed  length of filterKernel / filterStride
     * @param numSamples     does not include convolution margin
     */
    static __inline__ __attribute__((always_inline)) void BMSymmetricConv(
         float* filterKernel,
         size_t filterStride,
         float* audioInput,
         size_t inputStride,
         float* audioOutput,
         size_t outputStride,
         float* buffer,
         size_t filterSamplesUsed,
         size_t numSamples){
        
        
        // clear the output so we can sum into it
        vDSP_vclr(audioOutput, outputStride, numSamples);
        
        
        /*
         * main convolution processing loop
         */
        size_t maxI = filterSamplesUsed / 2;
        for(size_t i=0; i < maxI; i++){
            // multiply the ith value in the filter by
            // the audio input, buffer into the buffer
            size_t offset = i*inputStride;
            size_t numSamplesMul = numSamples + (filterSamplesUsed - 1) - (2*i);
            
            // read the ith value from the filter kernel
            float activeFilterKernelElement = filterKernel[i*filterStride];
            
            // if the ith value is nonzero, process it
            if(activeFilterKernelElement != 0.0){
                vDSP_vsmul(audioInput+offset, inputStride,
                           &activeFilterKernelElement,
                           buffer, 1, numSamplesMul);
                
                // add the result to the output
                vDSP_vadd(buffer, 1,
                          audioOutput, outputStride,
                          audioOutput, outputStride,
                          numSamples);
                
                // offset from the buffer and add to the output
                offset = (filterSamplesUsed - 1) - 2*i;
                vDSP_vadd(buffer+offset, 1,
                          audioOutput, outputStride,
                          audioOutput, outputStride,
                          numSamples);
            }
        }
        
        
        
        /*
         * in the odd length filter case the centre value of the kernel is
         * processed separately here.
         */
        if (filterSamplesUsed % 2 != 0){
            size_t offset = maxI*inputStride;
            
            float filterKernelCentre = filterKernel[maxI*filterStride];
            
            // if the filter kernel centre element is nonzero, process it
            if(filterKernelCentre != 0.0)
                // multiply the audio input by the centre value in the filter
                // and add directly to the output, shifted over by the offset
                vDSP_vsma(audioInput+offset, inputStride,
                          &filterKernelCentre,
                          audioOutput, outputStride,
                          audioOutput, outputStride,
                          numSamples);
        }
    }


    
    
#endif /* BMSymmetricConv_h */

#ifdef __cplusplus
}
#endif
