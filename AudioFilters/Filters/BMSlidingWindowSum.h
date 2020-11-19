//
//  BMSlidingWindowSum.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 17/11/20.
//  Copyright © 2020 BlueMangoo. We hereby release this file into the public
//  domain with no restrictions.
//

#ifndef BMSlidingWindowSum_h
#define BMSlidingWindowSum_h

#include <stdio.h>
#include "Constants.h"
#include "TPCircularBuffer.h"

typedef struct BMSlidingWindowSum {
	TPCircularBuffer buffer;
	size_t windowSize;
} BMSlidingWindowSum;

void BMSlidingWindowSum_init(BMSlidingWindowSum *This, size_t windowLength);

void BMSlidingWindowSum_free(BMSlidingWindowSum *This);

void BMSlidingWindowSum_processMono(BMSlidingWindowSum *This, const float *input, float *output, size_t numSamples);

#endif /* BMSlidingWindowSum_h */
