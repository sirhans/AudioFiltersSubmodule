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
	float *outputOS;
	float *phaseIncrements;
	float *phaseIncrementsOS;
	float **fractionalOffsets;
	size_t **integerOffsets;
	size_t *numImpulsesForBlip;
	size_t numBlips;
	BMBlip *blips;
} BMBlipOscillator;

#endif /* BMBlipOscillator_h */
