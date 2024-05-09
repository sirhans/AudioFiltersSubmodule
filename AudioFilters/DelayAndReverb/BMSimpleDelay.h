//
//  BMSimpleDelay.h
//  AudioFiltersXCodeProject
//
//  This class should be used when you need to implement a delay that does nothing other than
//  delay the signal as efficiently as possible (no feedback, no modulation, no multi-tap)
//
//
//  Created by hans on 31/1/19.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMSimpleDelay_h
#define BMSimpleDelay_h

#include <stdio.h>
#include "TPCircularBuffer+AudioBufferList.h"

typedef struct BMSimpleDelayMono {
    TPCircularBuffer buffer;
	size_t delayTimeSamples;
} BMSimpleDelayMono;

typedef struct BMSimpleDelayStereo {
    TPCircularBuffer bufferL, bufferR;
	size_t delayTimeSamples;
} BMSimpleDelayStereo;

/*!
*BMSimpleDelayMono_init
*/
void BMSimpleDelayMono_init(BMSimpleDelayMono *This, size_t delayTimeInSamples);

/*!
 *BMSimpleDelayStereo_init
 */
void BMSimpleDelayStereo_init(BMSimpleDelayStereo *This, size_t delayTimeInSamples);

/*!
*BMSimpleDelayMono_process
*/
void BMSimpleDelayMono_process(BMSimpleDelayMono *This,
                               const float* input,
                               float* output,
                               size_t numSamples);

/*!
*BMSimpleDelayStereo_process
*/
void BMSimpleDelayStereo_process(BMSimpleDelayStereo *This,
                                 const float* inputL,
                                 const float* inputR,
                                 float* outputL,
                                 float* outputR,
                                 size_t numSamples);
/*!
 *BMSimpleDelayStereo_getDelayLength
 *
 * @result returns the length of the delay in samples
 */
size_t BMSimpleDelayStereo_getDelayLength(BMSimpleDelayStereo *This);


/*!
 *BMSimpleDelayMono_getDelayLength
 *
 * @result returns the length of the delay in samples
 */
size_t BMSimpleDelayMono_getDelayLength(BMSimpleDelayMono *This);


/*!
 *BMSimpleDelayStereo_outputCapacity
 *
 * @abstract Returns the number of samples that can be output by calling BMSimpleDelayStereo_output() without inputting any new samples
 */
size_t BMSimpleDelayStereo_outputCapacity(BMSimpleDelayStereo *This);

/*!
 *BMSimpleDelayStereo_output
 *
 * @abstract Get output without inputting any new samples.
 *
 * @param This pointer to an initialised struct
 * @param outL pointer to an array for output with space for at least requestedOutputLength samples
 * @param outR pointer to an array for output with space for at least requestedOutputLength samples
 * @param requestedOutputLength maximum number of output samples requested. Actual output may be shorter than this.
 *
 * @result returns number of samples written
 */
size_t BMSimpleDelayStereo_output(BMSimpleDelayStereo *This, float *outL, float *outR, size_t requestedOutputLength);

/*!
 *BMSimpleDelayStereo_inputCapacity
 *
 * @abstract Returns the number of samples that can be written into the delay by calling BMSimpleDelayStereo_input() without taking any output.
 */
size_t BMSimpleDelayStereo_inputCapacity(BMSimpleDelayStereo *This);

/*!
 *BMSimpleDelayStereo_input
 *
 * @abstract input samples without reading output. Returns the number of samples accepted.
 *
 * @param This pointer to an initialised struct
 * @param inL input array Left containing inputLength samples of input
 * @param inR input array Rigt containing inputLength samples of input
 * @param inputLength length of input available in inL and inR
 *
 * @result returns the number of samples actually accepted from the input, which may be less than inputLength
 */
size_t BMSimpleDelayStereo_input(BMSimpleDelayStereo *This, float *inL, float *inR, size_t inputLength);

void BMSimpleDelayMono_free(BMSimpleDelayMono *This);
void BMSimpleDelayStereo_free(BMSimpleDelayStereo *This);

__attribute__((deprecated("please call BMSimpleDelayMono_free instead")))
void BMSimpleDelayMono_destroy(BMSimpleDelayMono *This);
__attribute__((deprecated("please call BMSimpleDelayStereo_free instead")))
void BMSimpleDelayStereo_destroy(BMSimpleDelayStereo *This);

#endif /* BMSimpleDelay_h */
