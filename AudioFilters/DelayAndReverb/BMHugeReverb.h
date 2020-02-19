//
//  BMHugeReverb.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 12/6/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#ifndef BMHugeReverb_h
#define BMHugeReverb_h

#include <stdio.h>
#include "BMEarlyReflections.h"
#include "BMSimpleDelay.h"
#include "BMWetDryMixer.h"
#include "BMMultiLevelBiquad.h"
#include "Constants.h"

typedef struct BMHugeReverb {
	BMEarlyReflections diff1, diff2, diff3, tankDiff;
	BMSimpleDelayStereo delay, preDelay;
	BMWetDryMixer wetDryMix;
	BMMultiLevelBiquad toneFilter, decayFilter;
	size_t delayLengthSamples;
	
	float processBufferL [BM_BUFFER_CHUNK_SIZE];
	float processBufferR [BM_BUFFER_CHUNK_SIZE];
	float bypassBufferL [BM_BUFFER_CHUNK_SIZE];
	float bypassBufferR [BM_BUFFER_CHUNK_SIZE];
	float outputBufferL [BM_BUFFER_CHUNK_SIZE];
	float outputBufferR [BM_BUFFER_CHUNK_SIZE];
	float feedbackBufferL [BM_BUFFER_CHUNK_SIZE];
	float feedbackBufferR [BM_BUFFER_CHUNK_SIZE];
	
	float decayCoefficient, sampleRate, loopDiffuserLength;
	float preDelayValue, preDelayTarget;
	float diffusionValue, diffusionTarget;
} BMHugeReverb;

#endif /* BMHugeReverb_h */
