//
//  Decimation.h
//  OscilloScope
//
//  Created by Anh Nguyen Duc on 5/26/20.
//  Copyright © 2020 Anh Nguyen Duc. All rights reserved.
//

#ifndef Decimation_h
#define Decimation_h

#include <stdio.h>

typedef struct BMDecimation {
    float* previousSourcePosition;
    size_t previousDownsampleGroupIndex;
} BMDecimation;


/*!
 *maxDecimation
 */
void maxDecimation(const float* input, float* output, size_t N, size_t outputLength);


/*!
*minDecimation
*/
void minDecimation(const float* input, float* output, size_t N, size_t outputLength);


/*!
 *minMaxDecimation
 *
 * This function is equivalent to calling minDecimation and maxDecimation. It gives both the min and max value from each group of N input samples.
 */
void minMaxDecimation(const float *input, float *outputMin, float *outputMax, size_t N, size_t outputLength);


/*!
 *minMaxDecimationInterleaved
 *
 * Decimates the input by a factor of N, leaving an interleaved {min,max} or {max,min} pair for each
 * group of N input samples. The interleaving order is {min,max} when the N samples in the input are
 * increasing and it is {max,min} when the input samples are decreasing.
 *
 * @param input input vector of length halfOutputLength * N
 * @param output output vector of length 2 * halfOutputLength
 * @param temp1 temporary memory of length halfOutputLength
 * @param temp2 temporary memory of length halfOutputLength
 * @param N decimation factor. The function writes one {min,max} pair for every N input samples
 * @param halfOutputLength half the length of the output array
 */
void minMaxDecimationInterleaved(const float *input, float *output, float *temp1, float *temp2, size_t N, size_t halfOutputLength);


/*!
*maxAbsDecimation
*/
void maxAbsDecimation(float* input, float* output, size_t N, size_t outputLength);


/*!
*minAbsDecimation
*/
void minAbsDecimation(float* input, float* output, size_t N, size_t outputLength);


/*!
*signedMaxAbsDecimationVDSP
*
* copy 1 in every N elements from input to output, taking the signed value that has the largest absolute value.
*
* @param input input array of length outputLength*N
* @param output length = outputLength
* @param temp temporary buffer memory length = outputLength
* @param N decimation factor
* @param outputLength length of output
*/
void signedMaxAbsDecimationVDSP(float *input, float *output, float *temp, size_t N, size_t outputLength);



/*!
*signedMaxAbsDecimationVDSPStable
*
* copy 1 in every N elements from input to output, taking the signed value that has the largest absolute value.
* This stable version keeps the locations of the boundaries between decimation groups at the same index (Modulo N)
* even when *input points to an arbitrary location in the source array.
*
* @param This pointer to a struct (does not need initialization)
* @param input input array of length outputLength*N
* @param output length = outputLength
* @param temp temporary buffer memory length = outputLength
* @param sourcePointer pointer to the start of the pre-upsampled source buffer
* @param upsampleFactor this is needed to calculate how many samples we advanced in the upsampled input buffer since last call
* @param N decimation factor
* @param outputLength length of output
*/
void signedMaxAbsDecimationVDSPStable(BMDecimation *This, float *input, float *output, float *temp, float *sourcePointer, size_t upsampleFactor, size_t N, size_t outputLength);



/*!
 *signedMinAbsDecimationVDSP
 */
void signedMinAbsDecimationVDSP(float* input, float* output, float* temp, size_t N, size_t outputLength);



/*!
*DecimationVDSP
*/
void DecimationVDSP(float* input, float* output, float* temp, size_t N, size_t outputLength);



/*!
*signedMaxAbsDecimation
*/
void signedMaxAbsDecimation(float* input, float* output, size_t N, size_t outputLength);



/*!
*getResampleFactors
*/
void getResampleFactors(size_t sampleWidth, size_t targetSampleWidth, size_t *upsampleFactor, size_t *downsampleFactor);


#endif /* Decimation_h */
