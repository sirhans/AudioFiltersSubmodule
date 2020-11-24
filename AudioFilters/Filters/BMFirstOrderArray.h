//
//  BMFirstOrderArray.h
//  BMAudioFilters
//
//  This is useful for situations where we need to filter a large number
//  of audio channels in parallel. For example, for filtering inside a FDN
//  reverberator
//
//  To use this in an FDN, initialise with FDNHighShelfInit or FDNLowShelfInit.
//  If you need low, mid, and high frequency decay time settings, use two
//  filter arrays. One for low, one for high. Mid will be controlled by the
//  decay rate of the unfiltered part of the spectrum between low and high.
//  You must set cutoff frequencies to ensure that an unfiltered part does
//  exist between low and high. Remember that these filters have gradual
//  cutoff so you need to leave a wide space in the middle to avoid overlap.
//
//  To actually process audio in an FDN, pass an array* of input and another
//  array for output and call the processSample function (so called because
//  it processes a single sample of input and output on all N channels).
//
//  Created by hans anderson on 1/1/18.
//  Anyone may use this file without restrictions
//

#ifndef BMFirstOrderArray_h
#define BMFirstOrderArray_h

#include <stdio.h>
#include <Accelerate/Accelerate.h>
#include <assert.h>
#include <simd/simd.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct BMFirstOrderArray4x4 {
	simd_float4 x1 [4];
	simd_float4 y1 [4];
	simd_float4 a1neg [4];
	simd_float4 b0 [4];
	simd_float4 b1 [4];
	float sampleRate;
} BMFirstOrderArray4x4;




/*!
 *BMFirstOrderArray4x4_init
 *
 * This function initialises memory and puts all the filters
 * into bypass mode.
 */
void BMFirstOrderArray4x4_init(BMFirstOrderArray4x4 *This, size_t numChannels, float sampleRate);



/*!
 *BMFirstOrderArray4x4_setHighDecayFDN
 *
 * @notes Set high frequency decay for High shelf filter array in an FDN
 *
 * @param delayTimesSeconds  the length in seconds of each delay in the FDN
 * @param fc                cutoff frequency
 * @param unfilteredRT60    decay time below fc (seconds)
 * @param filteredRT60      decay time at nyquist (seconds)
 * @param numChannels       size of FDN
 */
void BMFirstOrderArray4x4_setHighDecayFDN(BMFirstOrderArray4x4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);





/*!
 *BMFirstOrderArray4x4_setLowDecayFDN
 *
 * Set low frequency decay for High shelf filter array in an FDN
 *
 * @param delayTimesSeconds  the length in seconds of each delay in the FDN
 * @param fc                cutoff frequency
 * @param unfilteredRT60    decay time above fc (seconds)
 * @param filteredRT60      decay time at DC (seconds)
 * @param numChannels       size of FDN
 */
void BMFirstOrderArray4x4_setLowDecayFDN(BMFirstOrderArray4x4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);



/*!
 *BMFirstOrderArray4x4_processSample
 *
 * process one sample of audio on the input array of numChannels channels
 *
 * length of input == length of output == numChannels
 * (works in place)
 */
static __inline__ __attribute__((always_inline)) void BMFirstOrderArray4x4_processSample(BMFirstOrderArray4x4 *This, simd_float4 *input, simd_float4 *output, size_t numChannelsOver4){
	
	for(size_t i=0; i<numChannelsOver4; i++){
		// biquad filter difference equation
		output[i] = This->b0[i] * input[i]
				  + This->b1[i] * This->x1[i]
				  + This->a1neg[i] * This->y1[i];
	}
	
	// copy x0 to x1
	memcpy(This->x1, input, 4*sizeof(simd_float4));
	
	// copy y0 to y1
	memcpy(This->y1, output, 4*sizeof(simd_float4));
}




#ifdef __cplusplus
}
#endif

#endif /* BMFirstOrderArray_h */
