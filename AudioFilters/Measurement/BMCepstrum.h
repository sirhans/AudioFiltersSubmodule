//
//  BMCepstrum.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/3/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMCepstrum_h
#define BMCepstrum_h

#include <stdio.h>
#include "BMSpectrum.h"

typedef struct BMCepstrum {
    BMSpectrum spectrum1, spectrum2;
    float *buffer;
    size_t maxInputLength;
} BMCepstrum;



/*!
 *BMCepstrum_init
 */
void BMCepstrum_init(BMCepstrum *This, size_t maxInputLength);



/*!
 *BMCepstrum_free
 */
void BMCepstrum_free(BMCepstrum *This);



/*!
 *BMCepstrum_getCepstrum
 *
 * Roughly, this computes abs(fft(abs(fft(input)))) or abs(fft(log(abs(fft(input))))).
 *
 * @param This pointer to an initialised struct
 * @param input array of input values of length inputLength
 * @param output array of length inputLength/4
 * @param logSpectrum set true to take the log of the first fft output before computing the second fft
 * @param inputLength a power of 2 such that 0 < inputLength <= This->maxInputLength
 */
void BMCepstrum_getCepstrum(BMCepstrum *This,
                            const float *input,
                            float *output,
                            bool logSpectrum,
                            size_t inputLength);

#endif /* BMCepstrum_h */
