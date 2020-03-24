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
#include "BMShortSimpleDelay.h"


typedef struct BMLongLoopFDN{
	TPCircularBuffer *delays;
	float **readPointers, **writePointers, **mixBuffers;
	float *feedbackCoefficients, *delayTimes, *inputBufferL, *inputBufferR;
	float inputAttenuation, matrixAttenuation, inverseMatrixAttenuation, inputPan;
	bool *tapSigns;
	size_t numDelays, minDelaySamples, blockSize, feedbackShiftByDelay;
	bool hasZeroTaps;
} BMLongLoopFDN;



/*!
 *BMLongLoopFDN_init
 *
 * @param This pointer to an initialised struct
 * @param numDelays number of delays in the FDN. Must be an even number
 * @param minDelaySeconds the shortest delay time
 * @param maxDelaySeconds longest delay time
 * @param hasZeroTaps set this true to get an output tap with zero delay in both channels
 * @param blockSize set n={1,2,4,8} for nxn sized blocks in the mixing matrix
 * @param feedbackShiftByBlock the nth block feeds back to the n+feedbackShift block.
 * @param sampleRate audio sample rate
 */
void BMLongLoopFDN_init(BMLongLoopFDN *This,
						size_t numDelays,
						float minDelaySeconds,
						float maxDelaySeconds,
						bool hasZeroTaps,
						size_t blockSize,
						size_t feedbackShiftByBlock,
						float sampleRate);


/*!
 *BMLongLoopFDN_free
 */
void BMLongLoopFDN_free(BMLongLoopFDN *This);


/*!
 *BMLongLoopFDN_setRT60Decay
 */
void BMLongLoopFDN_setRT60Decay(BMLongLoopFDN *This, float timeSeconds);


/*!
 *BMLongLoopFDN_setInputBalance
 *
 * panning the input creates a side-to-side rocking in the impulse response
 *
 * @param This pointer to an initialised struct
 * @param pan01 0 is hard left pan and 1 is hard right. 0.5 is centre
 */
void BMLongLoopFDN_setInputPan(BMLongLoopFDN *This, float pan01);



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
