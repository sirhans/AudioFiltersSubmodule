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
 * @param applyWindow set true to apply a blackman-harris window before doing the fft
 *
 * @returns abs(fft[nyquist])
 */float BMSpectrum_processDataBasic(BMSpectrum* This,
                                     const float* input,
                                     float* output,
                                     bool applyWindow,
                                     size_t inputLength);

/*!
 *BMSpectrum_barkScaleFFTBinIndices
 *
 * populate the array A with floating point FFT bin numbers for interpolated
 * read from fft output into a spectrogram display.
 *
 * @param A output array
 * @param fftSize length of FFT input (not output)
 * @param minHz the lowest frequency (in Hz) to be represented by the bin numbers in A
 * @param maxHz the highest frequency "
 * @param sampleRate audio buffer sample rate
 * @param length length of A
 */
void BMSpectrum_barkScaleFFTBinIndices(float *A, size_t fftSize, float minHz, float maxHz, float sampleRate, size_t length);


/*!
 *BMSpectrum_fftDownsamplingIndices
 *
 * When representing FFT output in log scale, mel scale, or bark scale, or even
 * just downsampling linear scale we may have several fft bins grouped together
 * into one bin of the output array. This function calculates the fft bin index
 * at which each output bin starts (startIndices) and the number of fft bins
 * that will be grouped together in that one output bin (groupLengths).
 *
 * @param startIndices output array to contain the first fft bin index used in each bin of the output
 * @param groupLengths the number of fft bins to group together into each output bin
 * @param interpolatedIndices the floating point fft index representing the centre point in the fft bins that will be represented in each element of the downsampled output
 * @param minHz minimum frequency to be represented in the output
 * @param maxHz maximum frequency "
 * @param sampleRate audio buffer sample rate
 * @param fftSize length of input to fft
 * @param length length of downsampled output array, length of startIndices, groupLengths, and interpolatedIndices
 */
void BMSpectrum_fftDownsamplingIndices(size_t *startIndices,
									   size_t *groupLengths,
									   const float *interpolatedIndices,
									   float minHz,
									   float maxHz,
									   float sampleRate,
									   size_t fftSize,
									   size_t length);


/*!
 *BMSpectrum_numUpsampledPixels
 *
 * When resampling fft output to bark scale, mel scale, or log scale there is
 * often an fft bin index below which we have to upsample and above which we
 * have to downsample. This function finds the number of pixels that should be
 * upsampled as a result of the resampling process.
 */
size_t BMSpectrum_numUpsampledPixels(size_t fftSize,
									 float minF,
									 float maxF,
									 float sampleRate,
									 size_t outputLength);


#endif /* BMSpectrum_h */
