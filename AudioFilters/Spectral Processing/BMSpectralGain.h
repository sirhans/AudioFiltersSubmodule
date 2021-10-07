//
//  BMSpectralGain.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 27/08/2021.
//  Copyright © 2021 BlueMangoo. All rights reserved.
//

#ifndef BMSpectralGain_h
#define BMSpectralGain_h

#include <stdio.h>
#include "BMFFT.h"

typedef struct BMSpectralGain {
	BMFFT fft;
	float *b1r, *kaiserWindow, *kaiserToHann;
	float *outputRight, *outputCentre, *outputLeft;
	float *inputRight, *inputCentre, *inputLeft;
	DSPSplitComplex *b2c;
	size_t kaiserWindowLength, hannWindowLength;
} BMSpectralGain;



/*!
 * BMSpectralGain_processOneBuffer
 *
 * This function takes an input buffer and an array representing the gain to be applied to each FFT bin. It does the following operations:
 *
 *     spectralDomain = fft(KaiserWindow(input))
 *     output = windowFunction * ifft(gain * spectralDomain)
 *
 * Where,
 *
 *   windowFunction = HannWindow / KaiserWindow
 *
 * This window function removes the effect of the kaiser window applied in the first step and replaces it with a Hann window of smaller length, centred over the buffer.
 *
 * @param This pointer to an initialised struct
 * @param gain an array of values with length 1+inputLength/2 representing the gain to be applied to each fft bin
 * @param input an array of input of length inputLength
 * @param output an array of length outputLength
 * @param inputLength length of input
 * @param outputLength length of output. Must be in [1,inputLength)
 */
void BMSpectralGain_processOneBuffer(BMSpectralGain *This,
									 const float *gain,
									 const float *input,
									 float *output,
									 size_t inputLength,
									 size_t outputLength);

/*!
 *BMSpectralGain_processThreeBuffers
 *
 * @param This pointer to an initialised struct
 * @param gainLeft an array of length 1+fftSize/2 representing the gain change at each fft bin
 * @param inputLeft audio buffer containing fftSize/2 samples to the immediate left of inputCentre
 * @param gainCentre an array of length 1+fftSize/2 representing the gain change at each fft bin
 * @param inputCentre audio buffer containing the samples that correspond directly to the output
 * @param gainRight an array of length 1+fftSize/2 representing the gain change at each fft bin
 * @param inputRight audio buffer containing fftSize/2 samples to the immediate right of inputCentre
 * @param fftSize size of the fft used for spectral processing
 * @param sideBufferLength length of inputLeft and inputRight. must be equal to fftSize/2
 * @param centreBufferLength length of inputCentre and output. must be less than fftSize
 * @param output output buffer of length centreBufferLength
 */
void BMSpectralGain_processThreeBuffers(BMSpectralGain *This,
										const float *gainLeft, const float *inputLeft,
										const float *gainCentre, const float *inputCentre,
										const float *gainRight, const float *inputRight,
										size_t fftSize, size_t sideBufferLength, size_t centreBufferLength,
										float *output
										);

#endif /* BMSpectralGain_h */
