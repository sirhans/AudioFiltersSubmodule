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

typedef struct BMExtremeCompressor {
	BMAttackShaper asL, asR;
	BMLowpassedLimiter llL, llR;
	BMUpsampler	 upsampler;
	BMDownsampler downsampler;
	float *b1L, *b1R, *b2L, *b2R;
	size_t osFactor;
	bool isStereo;
} BMExtremeCompressor;

#endif /* BMExtremeCompressor_h */
