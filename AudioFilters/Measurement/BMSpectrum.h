//
//  BMSpectrum.h
//  Saturator
//
//  Created by Nguyen Minh Tien on 1/31/18.
//  This file is public domain. No restrictions.
//

#ifndef BMSpectrum_h
#define BMSpectrum_h

#include <stdio.h>
#import <Accelerate/Accelerate.h>
#include "BMFFT.h"

typedef struct BMSpectrum {
    BMFFT fft;
    float *buffer; 
    size_t maxInputLength;
} BMSpectrum;

/*!
 *BMSpectrum_init
 */
void BMSpectrum_init(BMSpectrum* This);


/*!
 *BMSpectrum_free
 */
void BMSpectrum_free(BMSpectrum* This);


/*!
 *BMSpectrum_initWithLength
 *
 * @param This pointer to an initialised struct
 * @param maxInputLength the input length may be shorter but not longer than This
 */
void BMSpectrum_initWithLength(BMSpectrum* This, size_t maxInputLength);

/*!
 *BMSpectrum_processData
 *
 *  calculates toDb(clip(abs(fft(input))))
 */
bool BMSpectrum_processData(BMSpectrum* This,
                            const float* inData,
                            float* outData,
                            size_t inSize,
                            size_t *outSize,
                            float* nyquist);


/*!
 *BMSpectrum_processDataBasic
 *
 * calculates abs(fft(input)) from bins DC up to Nyquist-1
 *
 * @param This pointer to an initialised struct
 * @param input array of real-valued time-series data of length inputLength
 * @param output real-valued output
 * @param applyWindow set true to apply a hamming window before doing the fft
 *
 * @returns abs(fft[nyquist])
 */float BMSpectrum_processDataBasic(BMSpectrum* This,
                                     const float* input,
                                     float* output,
                                     bool applyWindow,
                                     size_t inputLength);
#endif /* BMSpectrum_h */
