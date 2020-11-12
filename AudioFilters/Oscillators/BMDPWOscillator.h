//
//  BMDPWOscillator.h
//  AudioFiltersXcodeProject
//
//  This class implements an antialiased differentiated polynomial waveform
//  oscillator. The basic idea is to express a basic synth waveform as a
//  polynomial. We symbolically integrate the polynomial several times,
//  producing a waveform that has very little high frequency content. We then
//  generate a sampled digital signal fromt that integrated formula and produce
//  a signal with very little aliasing. Finally, we use a differentiation filter
//  kernel to restore the spectrum of the original waveform.
//
//  This method is described in the following paper:
//
//  Alias-Suppressed Oscillators Based on DifferentiatedPolynomial Waveforms
//  by Valimaki, et. al.
//  IEEE TRANSACTIONS ON AUDIO, SPEECH, AND LANGUAGE PROCESSING,
//  VOL. 18, NO. 4, MAY 2010
//
//  Created by hans anderson on 11/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this code into the public
//  domain with no restrictions.
//

#ifndef BMDPWOscillator_h
#define BMDPWOscillator_h

#include <stdio.h>
#include "BMFIRFilter.h"
#include "BMDownsampler.h"
#include "BMMultiLevelBiquad.h"
#include "BMShortSimpleDelay.h"

enum BMDPWOscillatorType {BMDPWO_SAW};

typedef struct BMDPWOscillator {
	BMFIRFilter differentiator;
	BMDownsampler downsampler;
	BMMultiLevelBiquad smoothingFilter;
	BMShortSimpleDelay groupDelayCompensator;
	
	float outputSampleRate, oversampledSampleRate, nextStartPhase, rawPolyWavelength;
	size_t differentiationOrder, integrationOrder, oversampleFactor;
	float *b1, *b2;
} BMDPWOscillator;

void BMDPWOscillator_init(BMDPWOscillator *This,
						  enum BMDPWOscillatorType oscillatorType,
						  size_t integrationOrder,
						  size_t oversampleFactor,
						  float sampleRate);

void BMDPWOscillator_free(BMDPWOscillator *This);

#endif /* BMDPWOscillator_h */
