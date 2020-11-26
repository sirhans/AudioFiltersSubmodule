//
//  BMExtremeCompressor.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 18/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMExtremeCompressor_h
#define BMExtremeCompressor_h

#include <stdio.h>
#include "BMAttackShaper.h"
#include "BMAsymptoticLimiter.h"
#include "BMLowpassedLimiter.h"
#include "BMUpsampler.h"
#include "BMDownsampler.h"
#include "BMAttackShaper.h"

typedef struct BMExtremeCompressor {
	BMMultibandAttackShaper as;
	BMLowpassedLimiter llL, llR;
	BMUpsampler	 upsampler;
	BMDownsampler downsampler;
	float *b1L, *b1R, *b2L, *b2R;
	size_t osFactor;
	bool isStereo;
} BMExtremeCompressor;



void BMExtremeCompressor_init(BMExtremeCompressor *This, float sampleRate, bool isStereo, size_t oversampleFactor);



void BMExtremeCompressor_free(BMExtremeCompressor *This);




void BMExtremeCompressor_procesMono(BMExtremeCompressor *This,
                                    const float *in,
                                    float *out,
                                    size_t numSamples);






void BMExtremeCompressor_procesStereo(BMExtremeCompressor *This,
                                    const float *inL, const float *inR,
                                    float *outL, float *outR,
                                      size_t numSamples);



#endif /* BMExtremeCompressor_h */
