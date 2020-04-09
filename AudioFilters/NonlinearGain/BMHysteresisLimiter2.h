//
//  BMHysteresisLimiter2.h
//  AudioFiltersXcodeProject
//
//  This simulates the amplifier sag from the power supply output being drained
//  by the tubes. In the context of this model it does not matter whether the
//  sag is caused by a transformer or by a capacitor or some other phenomenon.
//
//  The code generically models any effect where high level output from an amp
//  temporarily causes a reduction its output capacity, but after allowed to
//  rest it recovers its capacity again. If multiple sources of hysteresis are
//  required, for example, both capacitor and transformer, preamp and power amp,
//  then you should run several of these limiters in series, each with
//  appropriate charge rates.
//
//  Note that the correct mathematical model for this requires that the input
//  be processed through an asymptotic limiter beforehand:
//
//    limitedOut = input / (1.0 + abs(input)
//
//  This class does not include the limiter because in the case where you want
//  to simulate multiple stages of hysteresis it is not strictly necessary to
//  re-apply the asymptotic limiter between stages as long as the input to each
//  stage is strictly within (-1,1).
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions
//

#ifndef BMHysteresisLimiter2_h
#define BMHysteresisLimiter2_h

#include <stdio.h>
#include <simd/simd.h>
#include "BMMultiLevelBiquad.h"

typedef struct BMHysteresisLimiter2 {
	BMMultiLevelBiquad AAFilter1, AAFilter2;
    float c, R, oneOverR, sampleRate, sag, s, sR, halfSR;
	simd_float2 cs;
} BMHysteresisLimiter2;


/*!
 *BMHysteresisLimiter2_init
 *
 * @param This pointer to an uninitialised struct
 * @param sampleRate audio system sample rate
 * @param aaFilterNumLevels number of biquad sections in the antialiasing filter
 * @param aaFilterFc cutoff frequency of antialiasing filter
 * @param hpFilterFc cutoff frequency of highpass filter
 * @param numChannels for non-rectified signal, mono=1 and stereo = 2. For rectified signal, stereo=2 and mono=4.
 */
void BMHysteresisLimiter2_init(BMHysteresisLimiter2 *This,
							  float sampleRate,
							  size_t aaFilterNumLevels,
							  float aaFilterFc,
						      float hpFilterFc,
							  size_t numChannels);



/*!
 *BMHysteresisLimiter2_free
 */
void BMHysteresisLimiter2_free(BMHysteresisLimiter2 *This);




/*!
 *BMHysteresisLimiter2_setChargeTime
 *
 * @param This pointer to an initialised struct
 * @param limitDb this is the maximum output value the limiter is capable of sustaining without clipping.
 */
void BMHysteresisLimiter2_setPowerLimit(BMHysteresisLimiter2 *This, float limitDb);

/*!
 *BMHysteresisLimiter2_setSag
 *
 * @param This pointer to an initialised struct
 * @param sag larger values give slower charge time
 */
void BMHysteresisLimiter2_setSag(BMHysteresisLimiter2 *This, float sag);




/*!
 *BMHysteresisLimiter2_processMonoRectified
 *
 * @param This pointer to an initialised struct
 * @param inputPos positive side rectified input. MUST BE IN [0,FLT_MAX]
 * @param inputNeg negative side rectified input. MUST BE IN [FLT_MIN,0]
 * @param outputPos output array with length numSamples
 * @param outputNeg output array with length numSamples
 * @param numSamples length of inputs and outputs
 */
void BMHysteresisLimiter2_processMonoRectified(BMHysteresisLimiter2 *This,
                                      const float *inputPos, const float *inputNeg,
                                      float* outputPos, float* outputNeg,
													size_t numSamples);

/*!
 *BMHysteresisLimiter2_processMonoSigned
 *
 * We suspect this sounds the same as the rectified version but runs faster
 *
 * @param This pointer to an initialised struct
 * @param input array of length numSamples
 * @param output array of length numSamples
 * @param numSamples length of input and output
 */
void BMHysteresisLimiter2_processMonoSigned(BMHysteresisLimiter2 *This,
											const float *input,
											float* output,
											size_t numSamples);


/*!
*BMHysteresisLimiter2_processMonoClassA
*
* @param This pointer to an initialised struct
* @param inputPos positive side rectified input. MUST BE IN [0,FLT_MAX]
* @param outputPos output array with length numSamples
* @param numSamples length of inputs and outputs
*/
void BMHysteresisLimiter2_processMonoClassA(BMHysteresisLimiter2 *This,
										   const float *inputPos,
										   float* outputPos,
										   size_t numSamples);



/*!
 *BMHysteresisLimiter2_processStereoSimple
 *
 * @notes this is a simpler hysteresis model in the sense that it does not do zero-delay-feedback modeling of charge consumption. Rather the charge consumed at the current sample index removes charge from the capacitor for the next sample index.
 *
 * @param This pointer to an initialised struct
 * @param inputL positive side rectified input.
 * @param inputR positive side rectified input.
 * @param outputL output array with length numSamples
 * @param outputR output array with length numSamples
 * @param numSamples length of inputs and outputs
 */
void BMHysteresisLimiter2_processStereoSimple(BMHysteresisLimiter2 *This,
													  const float *inputL, const float *inputR,
													  float *outputL, float *outputR,
													  size_t numSamples);



/*!
 *BMHysteresisLimiter2_processStereoRectified
 *
 * @param This pointer to an initialised struct
 * @param inputPosL positive side rectified input. MUST BE IN [0,FLT_MAX]
 * @param inputPosR positive side rectified input. MUST BE IN [0,FLT_MAX]
 * @param inputNegL negative side rectified input. MUST BE IN [FLT_MIN,0]
 * @param inputNegR negative side rectified input. MUST BE IN [FLT_MIN,0]
 * @param outputPosL output array with length numSamples
 * @param outputPosR output array with length numSamples
 * @param outputNegL output array with length numSamples
 * @param outputNegR output array with length numSamples
 * @param numSamples length of inputs and outputs
 */
void BMHysteresisLimiter2_processStereoRectified(BMHysteresisLimiter2 *This,
													  const float *inputPosL, const float *inputPosR,
													  const float *inputNegL, const float *inputNegR,
													  float *outputPosL, float *outputPosR,
													  float *outputNegL, float *outputNegR,
													  size_t numSamples);





/*!
*BMHysteresisLimiter2_processStereoClassA
*
* @param This pointer to an initialised struct
* @param inputPosL positive side rectified input. MUST BE IN [0,FLT_MAX]
* @param inputPosR positive side rectified input. MUST BE IN [0,FLT_MAX]
* @param outputPosL output array with length numSamples
* @param outputPosR output array with length numSamples
* @param numSamples length of inputs and outputs
*/
void BMHysteresisLimiter2_processStereoClassA(BMHysteresisLimiter2 *This,
											 const float *inputPosL,
											 const float *inputPosR,
											 float *outputPosL,
											 float *outputPosR,
											 size_t numSamples);

#endif /* BMHysteresisLimiter2_h */
