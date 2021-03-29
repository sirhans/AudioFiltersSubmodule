//
//  BMBlipOscillator.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 25/02/2021.
//  Copyright © 2021 BlueMangoo. All rights reserved.
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
	float sampleRate, nextPhase;
} BMBlipOscillator;


void BMBlipOscillator_init(BMBlipOscillator *This, float sampleRate, size_t oversampleFactor, size_t filterOrder, size_t numBlips);

void BMBlipOscillator_free(BMBlipOscillator *This);

void BMBlipOscilaltor_setLowpassFc(BMBlipOscillator *This, float fc);

void BMBlipOscillator_process(BMBlipOscillator *This, const float *frequencies, float* output, size_t length);

#endif /* BMBlipOscillator_h */
