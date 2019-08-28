//
//  BMUpsampler.h
//  BMAudioFilters
//
//  Created by Hans on 10/9/17.
//  Anyone may use this file without restrictions of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMUpsampler_h
#define BMUpsampler_h
    
    
#include <MacTypes.h>
#include "BMIIRUpsampler2x.h"
#include "BMMultiLevelBiquad.h"
    
    typedef struct BMUpsampler {
        BMIIRUpsampler2x* upsamplers2x;
        BMMultiLevelBiquad secondStageAAFilter;
        float *bufferL, *bufferR;
        size_t numStages, upsampleFactor;
    } BMUpsampler;
    
    
    /*!
     *BMUpsampler_init
     *
     * @param This   pointer to an upsampler struct
     * @param stereo set true if you need stereo processing; false for mono
     * @param upsampleFactor supported values: 2^n
     */
    void BMUpsampler_init(BMUpsampler* This, bool stereo, size_t upsampleFactor);
    
    
    
    /*!
     *BMUpsampler_processBufferMono
     * @abstract upsample a buffer that contains a single channel of audio samples
     *
     * @param input    length = numSamplesIn
     * @param output   length = numSamplesIn * upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMUpsampler_processBufferMono(BMUpsampler* This, const float* input, float* output, size_t numSamplesIn);

    
    /*!
     *BMUpsampler_processBufferStereo
     * @abstract upsample a buffer that contains a single channel of audio samples
     *
     * @param inputL    length = numSamplesIn
     * @param inputR    length = numSamplesIn
     * @param outputL   length = numSamplesIn * upsampleFactor
     * @param outputR   length = numSamplesIn * upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMUpsampler_processBufferStereo(BMUpsampler* This, const float* inputL, const float* inputR, float* outputL, float* outputR, size_t numSamplesIn);
    
    
    void BMUpsampler_free(BMUpsampler* This);
    
    
    /*!
     *BMUpsampler_impulseResponse
     * @param IR array for IR output with length = IRLength
     * @param IRLength must be divisible by upsampleFactor
     */
    void BMUpsampler_impulseResponse(BMUpsampler* This, float* IR, size_t IRLength);
    
#endif /* BMUpsampler_h */

#ifdef __cplusplus
}
#endif
