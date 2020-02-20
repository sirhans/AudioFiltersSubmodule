//
//  BMAsymptoticLimiter.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMAsymptoticLimiter_h
#define BMAsymptoticLimiter_h

#include <Accelerate/Accelerate.h>
#include <stdio.h>

/*!
 *BMAsymptoticLimitNoSag
 */
void BMAsymptoticLimitNoSag(const float *input,
							float *output,
							size_t numSamples);



/*!
 *BMAsymptoticLimit
 */
void BMAsymptoticLimit(const float *input,
					   float *output,
					   float sampleRate,
					   float sag,
					   size_t numSamples);




/*!
 *BMAsymptoticLimitRectified
 *
 * we can omit the absolute value from out = in / (1+abs(in)) if we have rectified inputs. Using this function will therefore be faster than its non-rectified counterpart.
 *
 * @param inputPos positive rectified input array
 * @param inputNeg negative rectified input array
 * @param outputPos positive side output array
 * @param outputNeg negative side output array
 * @param numSamples length of input and output arrays
 * @param sampleRate audio buffer sample rate
 * @param sag larger values correspond to slower power supply charge recovery
 */
void BMAsymptoticLimitRectified(const float *inputPos,
								const float *inputNeg,
								float *outputPos,
								float *outputNeg,
								float sampleRate,
								float sag,
								size_t numSamples);



/*!
 *BMAsymptoticLimitPositive
 *
 * we can omit the absolute value from out = in / (1+abs(in)) if we have rectified inputs. Using this function will therefore be faster than its non-rectified counterpart.
 *
 * @param inputPos positive rectified input array
 * @param outputPos positive side output array
 * @param numSamples length of input and output arrays
 * @param sampleRate audio buffer sample rate
 * @param sag larger values correspond to slower power supply charge recovery
 */
void BMAsymptoticLimitPositive(const float *inputPos,
							   float *outputPos,
							   float sampleRate,
							   float sag,
							   size_t numSamples);




/*!
 *BMAsymptoticLimitPositiveNoSag
 *
 * we can omit the absolute value from out = in / (1+abs(in)) if we have rectified inputs. Using this function will therefore be faster than its non-rectified counterpart.
 *
 * @param inputPos positive rectified input array
 * @param outputPos positive side output array
 * @param numSamples length of input and output arrays
 */
void BMAsymptoticLimitPositiveNoSag(const float *inputPos,
												  float *outputPos,
												  size_t numSamples);



#endif /* BMAsymptoticLimiter_h */

