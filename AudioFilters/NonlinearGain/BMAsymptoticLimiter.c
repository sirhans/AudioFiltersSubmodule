//
//  BMAsymptoticLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 29/10/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMAsymptoticLimiter.h"
#include <simd/simd.h>


/*!
 *BMAsymptoticLimitNoSag
 */
void BMAsymptoticLimitNoSag(const float *input,
							float *output,
							size_t numSamples){
	assert(input != output);
	
	vDSP_vabs(input, 1, output, 1, numSamples);
	float one = 1.0f;
	vDSP_vsadd(output, 1, &one, output, 1, numSamples);
	vDSP_vdiv(output, 1, input, 1, output, 1, numSamples);
}



/*!
 *BMAsymptoticLimit
 */
void BMAsymptoticLimit(const float *input,
					   float *output,
					   float sampleRate,
					   float sag,
					   size_t numSamples){
	simd_float1 s = 1.0f / (sampleRate * sag);
	
	simd_float4 *in4 = (simd_float4*)input;
	simd_float4 *out4 = (simd_float4*)output;
	size_t numSamples4 = numSamples/4;
	
	// main processing loop
	for(size_t i=0; i<numSamples4; i++)
		out4[i] = in4[i] / (1.0f + s * simd_abs(in4[i]));
	
	// if numSamples isn't divisible by 4, clean up the last few
	size_t unprocessedSamples = numSamples % 4;
	size_t cleanupStartIndex = numSamples - unprocessedSamples;
	for(size_t i=cleanupStartIndex; i<numSamples; i++)
		output[i] = input[i] / (1.0f + s * fabsf(input[i]));
}




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
								size_t numSamples){
	simd_float1 s = 1.0f / (sampleRate * sag);
	
	simd_float4 *in4Pos = (simd_float4*)inputPos;
	simd_float4 *out4Pos = (simd_float4*)outputPos;
	simd_float4 *in4Neg = (simd_float4*)inputNeg;
	simd_float4 *out4Neg = (simd_float4*)outputNeg;
	size_t numSamples4 = numSamples/4;
	
	// main processing loop
	for(size_t i=0; i<numSamples4; i++){
		out4Pos[i] = in4Pos[i] / (1.0f + s * in4Pos[i]);
		out4Neg[i] = in4Neg[i] / (1.0f - s * in4Neg[i]);
	}
	
	// if numSamples isn't divisible by 4, clean up the last few
	size_t unprocessedSamples = numSamples % 4;
	size_t cleanupStartIndex = numSamples - unprocessedSamples;
	for(size_t i=cleanupStartIndex; i<numSamples; i++){
		outputPos[i] = inputPos[i] / (1.0f + s * inputPos[i]);
		outputNeg[i] = inputNeg[i] / (1.0f - s * inputNeg[i]);
	}
}



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
							   size_t numSamples){
	simd_float1 s = 1.0f / (sampleRate * sag);
	
	simd_float4 *in4Pos = (simd_float4*)inputPos;
	simd_float4 *out4Pos = (simd_float4*)outputPos;
	size_t numSamples4 = numSamples/4;
	
	// main processing loop
	for(size_t i=0; i<numSamples4; i++){
		out4Pos[i] = in4Pos[i] / (1.0f + s * in4Pos[i]);
	}
	
	// if numSamples isn't divisible by 4, clean up the last few
	size_t unprocessedSamples = numSamples % 4;
	size_t cleanupStartIndex = numSamples - unprocessedSamples;
	for(size_t i=cleanupStartIndex; i<numSamples; i++){
		outputPos[i] = inputPos[i] / (1.0f + s * inputPos[i]);
	}
}





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
									size_t numSamples){
	assert(inputPos != outputPos);
	
	float one = 1.0f;
	vDSP_vsadd(inputPos, 1, &one, outputPos, 1, numSamples);
	vDSP_vdiv(outputPos,1,inputPos,1,outputPos,1,numSamples);
}
