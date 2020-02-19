//
//  BMLowpassedLimiter.h
//  AudioFiltersXcodeProject
//
//  This class computes a smoothed volume envelope to approximate the input
//  gain. It then computes a control signal that would apply a soft-clipping
//  saturation to the volume envelope. But instead of applying the control
//  signal to saturate the volume envelope, it applies it to reduce the volume
//  of the original input signal. The result is a limiter that responds more
//  slowly than an ordinary limiter would, permitting us to achieve unusually
//  high compression ratios without causing much harmonic distortion. When the
//  input signal has extremely high gain, limiting in this way leaves very high
//  peaks at each attack transient. In order to prevent clipping, these peaks
//  have to be dealt with in some way. One such approach is used in the
//  BMExtremeCompressor class.
//
//  Created by hans anderson on 2/19/20.
//  Anyone may use this file. No restrictions
//

#ifndef BMLowpassedLimiter_h
#define BMLowpassedLimiter_h

#include <stdio.h>
#include "BMAsymptoticLimiter.h"
#include "BMMultiLevelBiquad.h"
#include "Constants.h"

typedef struct BMLowpassedLimiter {
	BMMultiLevelBiquad lpf;
	float b1 [BM_BUFFER_CHUNK_SIZE];
	float b2 [BM_BUFFER_CHUNK_SIZE];
} BMLowpassedLimiter;


/*!
 *BMLowpassedLimiter_init
 */
void BMLowpassedLimiter_init(BMLowpassedLimiter *This, float sampleRate);

/*!
 *BMLowpassedLimiter_free
 */
void BMLowpassedLimiter_free(BMLowpassedLimiter *This);

/*!
 *BMLopassedLimiter_setLowpassFc
 */
void BMLopassedLimiter_setLowpassFc(BMLowpassedLimiter *This, float fc);

/*!
 *BMLowpassedLimiter_process
 */
void BMLowpassedLimiter_process(BMLowpassedLimiter *This,
								const float* in,
								float *out,
								size_t numSamples);


#endif /* BMLowpassedLimiter_h */
