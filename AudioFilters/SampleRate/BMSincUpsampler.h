//
//  BMSincUpsampler.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/16/20.
//  This file is released to the public domain.
//

#ifndef BMSincUpsampler_h
#define BMSincUpsampler_h

#include <stdio.h>

#define BMSINC_MAX_KERNEL_LENGTH

typedef struct BMSincUpsampler {
    size_t upsampleFactor, kernelLength, inputPaddingLeft, inputPaddingRight, numKernels;
    float **filterKernels;
} BMSincUpsampler;



/*!
 *BMSincUpsampler_init
 *
 * When using this for drawing graphs of audio, we recommend setting interpolationPoints to at least 8.
 *
 * @param This pointer to an uninitialised struct
 * @param interpolationPoints number of points read for each new point generated. This MUST be an even number. Recommended minimum value = 8
 * @param upsampleFactor if upsampleFactor = N then you get N points out from every 1 point in, minus the padding at the left and right ends of the input
 */
void BMSincUpsampler_init(BMSincUpsampler *This,
                          size_t interpolationPoints,
                          size_t upsampleFactor);




/*!
 *BMSincUpsampler_free
 */
void BMSincUpsampler_free(BMSincUpsampler *This);




/*!
 *BMSincUpsampler_inputPaddingBefore
 *
 * @returns the number of input samples that are not present in the output
 */
size_t BMSincUpsampler_inputPaddingBefore(BMSincUpsampler *This);




/*!
 *BMSincUpsampler_inputPaddingAfter
 *
 * @returns the number of samples at the end of the input that are not present in the output
 */
size_t BMSincUpsampler_inputPaddingAfter(BMSincUpsampler *This);




/*!
 *BMSincUpsampler_outputLength
 *
 * @returns the number of samples output for the given length of input
 */
size_t BMSincUpsampler_outputLength(BMSincUpsampler *This, size_t inputLength);




/*!
 *BMSincUpsampler_process
 *
 * @param This pointer to an initialised struct
 * @param input input array with length inputLength
 * @param output output array with length (inputLength - interpolationPoints/2) * upsampleFactor
 *
 * @returns the number of output points
 */
size_t BMSincUpsampler_process(BMSincUpsampler *This,
                             const float* input,
                             float* output,
                             size_t inputLength);


#endif /* BMSincUpsampler_h */
