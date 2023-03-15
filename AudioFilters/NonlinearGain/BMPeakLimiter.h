//
//  BMPeakLimiter.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 12/5/19.
//  Anyone may use this file without restrictions
//

#ifndef BMPeakLimiter_h
#define BMPeakLimiter_h

#include <stdio.h>
#include "BMShortSimpleDelay.h"
#include "BMEnvelopeFollower.h"
#include "Constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BMPeakLimiter {
	BMShortSimpleDelay delay;
	BMEnvelopeFollower envelopeFollower;
	float sampleRate, lookaheadTime, targetLookaheadTime, thresholdGain;
	float bufferL [BM_BUFFER_CHUNK_SIZE];
	float bufferR [BM_BUFFER_CHUNK_SIZE];
	float controlSignal [BM_BUFFER_CHUNK_SIZE];
	bool isLimiting, needsClearBuffers;
} BMPeakLimiter;


/*!
 *BMPeakLimiter_init
 */
void BMPeakLimiter_init(BMPeakLimiter *This, bool stereo, float sampleRate);


/*!
 *BMPeakLimiter_initNoRealtime
 */
void BMPeakLimiter_initNoRealtime(BMPeakLimiter *This, bool stereo, float sampleRate);


/*!
 *BMPeakLimiter_free
 */
void BMPeakLimiter_free(BMPeakLimiter *This);



/*!
 *BMPeakLimiter_setThreshold
 */
void BMPeakLimiter_setThreshold(BMPeakLimiter *This, float thresholdDb);



/*!
 *BMPeakLimiter_setLookahead
 */
void BMPeakLimiter_setLookahead(BMPeakLimiter *This, float timeInSeconds);


/*!
 *BMPeakLimiter_setReleaseTime
 */
void BMPeakLimiter_setReleaseTime(BMPeakLimiter *This, float timeInSeconds);


/*!
 *BMPeakLimiter_processStereo
 */
void BMPeakLimiter_processStereo(BMPeakLimiter *This,
								 const float *inL, const float *inR,
								 float *outL, float *outR,
								 size_t numSamples);

/*!
 *BMPeakLimiter_processMono
 */
void BMPeakLimiter_processMono(BMPeakLimiter *This,
								 const float *input,
								 float *output,
								 size_t numSamples);


/*!
 *BMPeakLimiter_clearBuffers
 *
 * Sets a flag that will cause buffers to be cleared before processing the next audio buffer
 */
void BMPeakLimiter_clearBuffers(BMPeakLimiter *This);



/*!
 *BMPeakLimiter_isLimiting
 *
 * @returns true if the limiter is currently limiting
 */
bool BMPeakLimiter_isLimiting(BMPeakLimiter *This);


#ifdef __cplusplus
}
#endif

#endif /* BMPeakLimiter_h */
