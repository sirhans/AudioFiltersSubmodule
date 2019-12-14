//
//  BMAudioStreamConverter.h
//  SaturatorAU
//
//  This is a very low-latency sample rate converter that works by upsampling
//  using a high-quality IIR allpass filter method that near-zero latency, then
//  downsampling using vector-optimised linear interpolation.
//  The latency from input to output is near zero.
//
//  This method is very accurate for INCREASING the sample rate by a factor of
//  two or more. It will result in aliasing if used for downsampling unless the
//  input signal has been lowpass fitlered prior to calling these functions.
//
//  Created by hans anderson on 12/13/19.
//  Anyone may use this file - no restrictions.
//

#ifndef BMAudioStreamConverter_h
#define BMAudioStreamConverter_h

#include <stdio.h>
#include "BMUpsampler.h"
#include <AudioToolbox/AudioToolbox.h>
#include "TPCircularBuffer.h"

#define BM_SRC_MAX_OUTPUT_LENGTH 2048

typedef struct BMSampleRateConverter {
	BMUpsampler upsampler;
	TPCircularBuffer outputBuffers [2];
	float *inputBuffers [2];
	float indexBuffer [BM_SRC_MAX_OUTPUT_LENGTH];
	AudioStreamBasicDescription input, output;
	double nextStartIndex, conversionRatio;
	bool stereoResampling;
} BMAudioStreamConverter;


void BMAudioStreamConverter_init(BMAudioStreamConverter *This,
								AudioStreamBasicDescription input,
								AudioStreamBasicDescription output);

void BMAudioStreamConverter_free(BMAudioStreamConverter *This);


/*!
 *BMAudioStreamConverter_convert
 *
 * @abstract converts two input buffers of length numSamplesIn and returns the length of the output buffers
 *
 * @returns length of output buffer, in samples
 */
size_t BMAudioStreamConverter_convert(BMAudioStreamConverter *This,
									  const void **input,
									  void **outputR,
									  size_t numSamplesIn);

#endif /* BMAudioStreamConverter_h */
