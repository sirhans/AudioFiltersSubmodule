//
//  BMDynamicSmoothingFilter.h
//  AudioFiltersXCodeProject
//
//  Created by hans anderson on 3/19/19.
//
//  The author places no restrictions on the use of this file.
//
//  See https://cytomic.com/files/dsp/DynamicSmoothing.pdf for the source of
//  of the ideas implemented here.
//
//  In summary: when operated as a lowpass filter, the value in the first
//  integrator of a second order state variable filter indicates the gradient
//  of the incoming signal. Using the absolute value of this gradient to control
//  the cutoff frequency of the filter, we get a lowpass filter that operates
//  on audio frequency signals safely with cutoff frequencies below 1 Hz and
//  is capable moving its cutoff frequency up near the Nyquist frequency when
//  there is a large spike in the input value. This way we can smooth out small
//  changes and still react immediately to large changes.

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMDynamicSmoothingFilter_h
#define BMDynamicSmoothingFilter_h

#include <stdio.h>
    
    typedef struct BMDynamicSmoothingFilter {
        float low1z, low2z, g0, sensitivity;
    } BMDynamicSmoothingFilter;
    
    void BMDynamicSmoothingFilter_processBuffer(BMDynamicSmoothingFilter *This,
                                               const float* input,
                                               float* output,
                                               size_t numSamples);


	void BMDynamicSmoothingFilter_processBufferWithFastDescent(BMDynamicSmoothingFilter *This,
															   const float* input,
															   float* output,
															   size_t numSamples);
    
    /*!
	 *BMDynamicSmoothingFilter_initDefault
	 *
     * Use this if you don't know what settings are approrpriate
     */
    void BMDynamicSmoothingFilter_initDefault(BMDynamicSmoothingFilter *This,
                                              float sampleRate);
    

	/*!
	 *BMDynamicSmoothingFilter_init
	 *
	 * @param This pointer
	 * @param sensitivity default: 1
	 * @param minFc the cutoff frequency of the filter when the input is constant
	 * @param sampleRate audio buffer sample rate
	 */
    void BMDynamicSmoothingFilter_init(BMDynamicSmoothingFilter *This,
                                        float sensitivity,
                                        float minFc,
                                        float sampleRate);

#endif /* BMDynamicSmoothingFilter_h */
