//
//  BMVelvetNoiseDecorrelator.h
//  Saturator
//
//  Created by TienNM on 12/4/17
//  Rewritten by Hans on 9 October 2019
//  Anyone may use this file without restrictions
//

#ifndef BMVelvetNoiseDecorrelator_h
#define BMVelvetNoiseDecorrelator_h

#include <stdio.h>
#include "BMMultiTapDelay.h"


typedef struct BMVelvetNoiseDecorrelator {
    BMMultiTapDelay multiTapDelay;
    float sampleRate, wetMix, rt60, maxDelayTimeS;
	size_t *delayLengthsL, *delayLengthsR;
	float *gainsL, *gainsR;
    float fadeInSamples;
	bool hasDryTap, evenTapDensity;
	size_t numWetTaps;
    float lastTapGainL;
    float lastTapGainR;
    bool resetNumTaps;
    bool resetRT60DecayTime;
    bool resetFadeIn;
    float* tempBuffer;
    size_t numInput;
} BMVelvetNoiseDecorrelator;




/*!
 *BMVelvetNoiseDecorrelator_init
 *
 * @param This  pointer to an unitialised struct
 * @param maxDelaySeconds maximum delay time in seconds
 * @param numTaps number of delay taps on each channel including the one used to pass the dry signal through
 * @param rt60DecayTimeSeconds controls the decay envelope used to slope the volume of the taps
 * @param hasDryTap set true if you want to mix wet and dry signal; false for 100% wet
 * @param sampleRate sample rate in Hz
 */
void BMVelvetNoiseDecorrelator_init(BMVelvetNoiseDecorrelator *This,
									float maxDelaySeconds,
									size_t numTaps,
									float rt60DecayTimeSeconds,
									bool hasDryTap,
									float sampleRate);

/*!
*BMVelvetNoiseDecorrelator_initWithEvenTapDensity
*
* @param This  pointer to an unitialised struct
* @param maxDelaySeconds maximum delay time in seconds
* @param numTaps number of delay taps on each channel including the one used to pass the dry signal through
* @param rt60DecayTimeSeconds controls the decay envelope used to slope the volume of the taps
* @param hasDryTap set true if you want to mix wet and dry signal; false for 100% wet
* @param sampleRate sample rate in Hz
*/
void BMVelvetNoiseDecorrelator_initWithEvenTapDensity(BMVelvetNoiseDecorrelator *This,
													  float maxDelaySeconds,
													  size_t numTaps,
													  float rt60DecayTimeSeconds,
													  bool hasDryTap,
													  float sampleRate);

/*!
 *BMVelvetNoiseDecorrelator_setWetMix
 */
void BMVelvetNoiseDecorrelator_setWetMix(BMVelvetNoiseDecorrelator *This, float wetMix01);


void BMVelvetNoiseDecorrelator_setRT60DecayTime(BMVelvetNoiseDecorrelator *This, float rt60DT);

/*!
 *BMVelvetNoiseDecorrelator_randomiseAll
 */
void BMVelvetNoiseDecorrelator_randomiseAll(BMVelvetNoiseDecorrelator *This);

void BMVelvetNoiseDecorrelator_setFadeIn(BMVelvetNoiseDecorrelator *This,float fadeInS);

/*!
 *BMVelvetNoiseDecorrelator_free
 */
void BMVelvetNoiseDecorrelator_free(BMVelvetNoiseDecorrelator *This);




/*!
 *BMVelvetNoiseDecorrelator_processBufferStereo
 */
void BMVelvetNoiseDecorrelator_processBufferStereo(BMVelvetNoiseDecorrelator *This,
                                                   float* inputL,
                                                   float* inputR,
                                                   float* outputL,
                                                   float* outputR,
                                                   size_t length);

void BMVelvetNoiseDecorrelator_processMultiChannelInput(BMVelvetNoiseDecorrelator *This,
                                                    float** inputL,
                                                    float** inputR,
                                                    float* outputL,
                                                    float* outputR,
                                                    size_t length);

void BMVelvetNoiseDecorrelator_processBufferStereoWithFinalOutput(BMVelvetNoiseDecorrelator *This,
                                                    float* inputL,
                                                    float* inputR,
                                                    float* outputL,
                                                    float* outputR,
                                                    float* finalOutputL,
                                                    float* finalOutputR,
                                                    size_t length);
/*!
 *BMVelvetNoiseDecorrelator_processBufferMonoToStereo
 */
void BMVelvetNoiseDecorrelator_processBufferMonoToStereo(BMVelvetNoiseDecorrelator *This,
                                                   float* inputL,
                                                   float* outputL, float* outputR,
                                                   size_t length);


void BMVelvetNoiseDecorrelator_setNumTaps(BMVelvetNoiseDecorrelator *This, size_t numTaps);

#endif /* BMVelvetNoiseDecorrelator_h */
