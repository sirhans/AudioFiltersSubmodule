//
//  BMShortSimpleDelay.h
//  SaturatorAU
//
//  This is a static delay class for efficiently implementing delay times that
//  are short relative to the audio buffer length.
//
//  Created by hans anderson on 9/27/19.
//  Anyone may use this file without restrictions of any kind.
//

#ifndef BMShortSimpleDelay_h
#define BMShortSimpleDelay_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BMShortSimpleDelay{
    float **delayPtrs;
    float *delayMemory;
    size_t delayLength, numChannels, targetDelayLength;
} BMShortSimpleDelay;


/*!
 *BMShortSimpleDelay_process
 */
void BMShortSimpleDelay_process(BMShortSimpleDelay* This,
                                const float** inputs,
                                float** outputs,
                                size_t numChannels,
                                size_t numSamples);

/*!
 *BMShortSimpleDelay_init
 *
 * @param This  pointer to an uninitialised struct
 * @param numChannels number of audio channels the struct will be processing (2 for stereo)
 * @param lengthSamples length of the delay in samples
 */
void BMShortSimpleDelay_init(BMShortSimpleDelay* This,
                             size_t numChannels,
                             size_t lengthSamples);


/*!
 *BMShortSimpleDelay_free
 */
void BMShortSimpleDelay_free(BMShortSimpleDelay* This);



/*!
 *BMShortSimpleDelay_changeLength
 */
void BMShortSimpleDelay_changeLength(BMShortSimpleDelay* This, size_t lengthSamples);

#ifdef __cplusplus
}
#endif

#endif /* BMShortSimpleDelay_h */
