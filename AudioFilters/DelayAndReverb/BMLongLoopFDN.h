//
//  BMLongLoopFDN.h
//  AUCloudReverb
//
//  Created by hans anderson on 3/20/20.
//  This file is in the public domain.
//

#ifndef BMLongLoopFDN_h
#define BMLongLoopFDN_h

#include <stdio.h>
#include "BMSimpleDelay.h"
#include "TPCircularBuffer.h"


typedef struct BMLongLoopFDN{
	BMSimpleDelayMono *delays;
	TPCircularBuffer *feedbackBuffers;
	float **mixingBuffers;
	float *feedbackCoefficients, *delayTimes;
	float inputAttenuation;
	size_t numDelays, samplesInFeedbackBuffer;
	bool hasZeroTaps;
	bool *tapSigns;
} BMLongLoopFDN;



/*!
 *BMLongLoopFDN_init
 *
 * @param This pointer to an initialised struct
 * @param numDelays number of delays in the FDN. Must be an even number
 * @param minDelaySeconds the shortest delay time
 * @param maxDelaySeconds longest delay time
 * @param hasZeroTaps set this true to get an output with zero delay and gain adjusted to balance with the other taps
 * @param sampleRate audio sample rate
 */
void BMLongLoopFDN_init(BMLongLoopFDN *This, size_t numDelays, float minDelaySeconds, float maxDelaySeconds, bool hasZeroTaps, float sampleRate);


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
 *
 * That this process function is 100% wet. You must handle wet/dry mix
 * outside.
 */
void BMLongLoopFDN_process(BMLongLoopFDN *This,
						   const float* inputL, const float* inputR,
						   float *outputL, float *outputR,
						   size_t numSamples);

#endif /* BMLongLoopFDN_h */
