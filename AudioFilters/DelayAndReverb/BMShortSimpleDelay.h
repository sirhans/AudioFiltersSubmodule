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


typedef struct BMShortSimpleDelay{
    float **delayPtrs;
    float *delayMemory;
    size_t delayLength, numChannels;
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
 */
void BMShortSimpleDelay_init(BMShortSimpleDelay* This,
                             size_t numChannels,
                             size_t length);


/*!
 *BMShortSimpleDelay_free
 */
void BMShortSimpleDelay_free(BMShortSimpleDelay* This);


#endif /* BMShortSimpleDelay_h */
