//
//  BMDownsampler.h
//  BMAudioFilters
//
//  Created by Hans on 13/9/17.
//  Anyone may use this file without restrictions
//


#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMDownsampler_h
#define BMDownsampler_h
    
#include <MacTypes.h>
#include "BMHIIRDownsampler2xFPU.h"

    typedef struct BMDownsampler {
        BMHIIRDownsampler2xFPU* downsamplers;
        size_t numStages;
    } BMDownsampler;
    
    
    
    
    /*!
     *BMDownsampler_init
     *
     * @param decimationFactor must be a power of 2.
     */
    void BMDownsampler_init(BMDownsampler* This,
                            size_t decimationFactor);
    
    
    
    /*!
     *BMDownsampler_processBufferMono
     *
     * @input length of input >= numSamplesIn
     * @output length of output >= numSamplesIn/decimationFactor
     * @numSamplesIn = 2 * numSamplesOut
     */
    void BMDownsampler_processBufferMono(BMDownsampler* This,
                               float* input,
                               float* output,
                               size_t numSamplesIn);
    

    
    void BMDownsampler_free(BMDownsampler* This);
    
    
    
    
    void BMDownsampler_impulseResponse(BMDownsampler* This, float* IR, size_t numSamples);
        

    
    
#endif /* BMDownsampler_h */

#ifdef __cplusplus
}
#endif
