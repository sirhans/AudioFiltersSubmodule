//
//  BMUpsampler.c
//  BMAudioFilters
//
//  Created by Hans on 10/9/17.
//  This file may be used by anyone without any restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMUpsampler.h"
#include <Accelerate/Accelerate.h>

    
    void BMUpsampler_init(BMUpsampler* This, size_t upsampleFactor){
        
    }
    
    
    
    
    
    /*
     * upsample a buffer that contains a single channel of audio samples
     *
     * @param input    length = numSamplesIn
     * @param output   length = numSamplesIn * upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMUpsampler_processBuffer(BMUpsampler* This, float* input, float* output, size_t numSamplesIn){
    }

    
    
    
    
    
    void BMUpsampler_free(BMUpsampler* This){
        
    }
    
    
    
    
    /*
     * The calling function must ensure that the length of IR is
     * at least This->IRLength
     */
    void BMUpsampler_impulseResponse(BMUpsampler* This, float* IR){
        
    }
    
    
#ifdef __cplusplus
}
#endif
