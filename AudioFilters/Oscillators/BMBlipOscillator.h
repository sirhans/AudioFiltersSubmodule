//
//  BMBlipOscillator.h
//  AudioFiltersXcodeProject
//
//  This oscillator produces a band-limited series of impulses with frequency as specified by the input array of frequencies.
//  The input array specifies the frequency of each individual sample. This is done so that audio-frequency filters and other audio effects can be applied directly to the frequency control signal. The frequency is specified by giving the log_2 of the desired frequency. This ensures that effects applied to the frequency control signal respond similarly regardless of the oscillator frequency.
//  The lowpass filter is applied in the analgog (s) domain before sampling the impulses. This ensures that we can more effectively reduce aliasing artifacts by oversampling.
//  When sweeping the lowpass filter frequency, the frequency is updated once for each impulse and it remains unchanged until the impulse response decays to a near-zero value. Since the impulse response may be longer than the wavelength it is possible to have two or more filter frequencies simultaneously, one for each impulse that is currently affecting the audio output.
//
//  Created by hans anderson on 25/02/2021.
//  We release this file into the public domain without restrictions
//

#ifndef BMBlipOscillator_h
#define BMBlipOscillator_h

#include <stdio.h>
#include "BMGaussianUpsampler.h"
#include "BMDownsampler.h"
#include "BMMultilevelBiquad.h"
#include "BMBlip.h"


typedef struct BMBlipOscillator{
	BMGaussianUpsampler upsampler;
	BMDownsampler downsampler;
	BMMultiLevelBiquad highpass;
	size_t numBlips, nextBlip, filterOrder;
	BMBlip *blips;
	float sampleRate, lastPhase;
    float *b1, *b2;
    size_t *b3i;
} BMBlipOscillator;


/*!
 *BMBlipOscillator_init
 */
void BMBlipOscillator_init(BMBlipOscillator *This, float sampleRate, size_t oversampleFactor, size_t filterOrder, size_t numBlips);


/*!
 *BMBlipOscillator_free
 */
void BMBlipOscillator_free(BMBlipOscillator *This);


/*!
 *BMBlipOscilaltor_setLowpassFc
 */
void BMBlipOscilaltor_setLowpassFc(BMBlipOscillator *This, float fc);


/*!
 *BMBlipOscillator_process
 *
 * @param This pointer to an initialised struct
 * @param log2Frequencies array of length length containing log_2 of the oscillator frequency at the corresponding sample
 * @param output array of length length
 * @param length length of log2Frequencies and output
 */
void BMBlipOscillator_process(BMBlipOscillator *This, const float *log2Frequencies, float* output, size_t length);

#endif /* BMBlipOscillator_h */
