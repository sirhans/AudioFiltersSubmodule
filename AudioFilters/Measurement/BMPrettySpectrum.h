//
//  BMPrettySpectrum.h
//  OscilloScope
//
//  This is a wrapper class for BMSpectrum that manages interpolation,
//  upsampling and downsampling the spectral output to display a smooth line in
//  Log scale or Bark Scale. It also manages animating the rise and fall of the
//  peaks over time.
//
//  Created by hans anderson on 8/19/20.
//  This file is public domain with no restrictions
//

#ifndef BMPrettySpectrum_h
#define BMPrettySpectrum_h

#include <stdio.h>
#include "BMSpectrum.h"
#include "TPCircularBuffer.h"

enum BMPSScale {BMPSLINEAR, BMPSBARK, BMPSLOG};

typedef struct BMPrettySpectrum {
	BMSpectrum spectrum;
	TPCircularBuffer buffer;
	float sampleRate, decayRateDbPerSecond, minFreq, maxFreq, timeSinceLastUpdate;
	size_t fftInputLength, fftOutputLength, outputLength, bufferLength;
	enum BMPSScale scale;
	float *fftb1, *fftb2, *ob1, *ob2;
} BMPrettySpectrum;



/*!
 *BMPrettySpectrum_init
 */
void BMPrettySpectrum_init(BMPrettySpectrum *This, size_t maxOutputLength, float sampleRate);


/*!
 *BMPrettySpectrum_free
 */
void BMPrettySpectrum_free(BMPrettySpectrum *This);

/*!
 *BMPrettySpectrum_inputBuffer
 *
 * This inserts samples into the buffer. It does not trigger any processing at
 * all. All processing is done from the graphics thread through
 */
void BMPrettySpectrum_inputBuffer(BMPrettySpectrum *This, const float *input, size_t length);


/*!
 * BMPrettySpectrum_getOutput
 */
void BMPrettySpectrum_getOutput(BMPrettySpectrum *This,
								float *output,
								float minFreq, float maxFreq,
								enum BMPSScale scale,
								size_t outputLength);

#endif /* BMPrettySpectrum_h */
