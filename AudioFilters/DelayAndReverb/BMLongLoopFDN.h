//
//  BMLongLoopFDN.h
//  AUCloudReverb
//
//  Created by hans anderson on 3/20/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMLongLoopFDN_h
#define BMLongLoopFDN_h

#include <stdio.h>
#include "BMSimpleDelay.h"
#include "TPCircularBuffer.h"

typedef struct BMLongLoopFDN{
	BMSimpleDelayMono *delays;
	TPCircularBuffer *feedbackBuffers;
	float *inputBuffer;
	float inputAttenuation;
	size_t numDelays, samplesInFeedbackBuffer;
} BMLongLoopFDN;

/*!
 *BMLongLoopFDN_init
 */
void BMLongLoopFDN_init(BMLongLoopFDN *This, size_t numDelays, float minDelaySeconds, float maxDelaySeconds, float sampleRate);

/*!
 *BMLongLoopFDN_free
 */
void BMLongLoopFDN_free(BMLongLoopFDN *This);

/*!
 *BMLongLoopFDN_setRT60Decay
 */
void BMLongLoopFDN_setRT60Decay(BMLongLoopFDN *This, float timeSeconds);

/*!
 *BMLongLoopFDN_process
 */
void BMLongLoopFDN_process(BMLongLoopFDN *This,
						   const float* inputL, const float* inputR,
						   float *outputL, float *outputR,
						   size_t numSamples);

#endif /* BMLongLoopFDN_h */
