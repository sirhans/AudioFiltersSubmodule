//
//  BMSpectrogram.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/15/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMSpectrogram_h
#define BMSpectrogram_h

#include <stdio.h>
#include "BMSpectrum.h"
#include <simd/simd.h>

typedef struct BMSpectrogram {
	BMSpectrum spectrum;
	float *b1, *b2, *b3;
	float prevMinF, prevMaxF, sampleRate;
	size_t prevImageHeight, prevFFTSize;
} BMSpectrogram;



typedef simd_float3 BMHSBPixel;



/*!
 *BMSpectrogram_init
 */
void BMSpectrogram_init(BMSpectrogram *This,
						size_t maxFFTLength,
						size_t maxImageHeight,
						float sampleRate);



/*!
 *BMSpectrogram_process
 *
 * This function takes an audio array as input and converts it to a 2d image in
 * HSB colour format.
 *
 * It requires some additional samples to be available in the input array beyond
 * the region you actually plan to plot.
 *
 * @param This pointer to an initialised struct
 * @param inputAudio input array with 
 */
void BMSpectrogram_process(BMSpectrogram *This,
						   const float* inputAudio,
						   SInt32 inputLength,
						   SInt32 startSampleIndex,
						   SInt32 endSampleIndex,
						   SInt32 fftSize,
						   BMHSBPixel** imageOutput,
						   SInt32 pixelWidth,
						   SInt32 pixelHeight,
						   float minFrequency,
						   float maxFrequency);

#endif /* BMSpectrogram_h */
