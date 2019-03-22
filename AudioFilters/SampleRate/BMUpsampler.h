//
//  BMUpsampler.h
//  BMAudioFilters
//
//  Created by Hans on 10/9/17.
//  This file may be used by anyone without any restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMUpsampler_h
#define BMUpsampler_h

#define BMUPSAMPLER_FLTR_0_LENGTH 18
#define BMUPSAMPLER_FLTR_1_LENGTH 12
#define BMUPSAMPLER_FLTR_2_LENGTH 6
#define BMUPSAMPLER_FLTR_SECTIONS 2
    
#include <MacTypes.h>
#include "BMUpsampler2x.h"
    
    typedef struct BMUpsampler {
        BMUpsampler2x* upsamplerBank;
        size_t numStages;
        size_t IRLength;
    } BMUpsampler;
    
    
    /*
     * Initialise the struct at *This
     *
     * @param upsampleFactor supported values: 2, 4, 8
     */
    void BMUpsampler_init(BMUpsampler* This, size_t upsampleFactor);
    
    
    
    /*
     * upsample a buffer that contains a single channel of audio samples
     *
     * @param input    length = numSamplesIn
     * @param output   length = numSamplesIn * upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMUpsampler_processBuffer(BMUpsampler* This, float* input, float* output, size_t numSamplesOut);

    
    void BMUpsampler_free(BMUpsampler* This);
    
    
    /*
     * The calling function must ensure that the length of IR is 
     * at least This->IRLength
     */
    void BMUpsampler_impulseResponse(BMUpsampler* This, float* IR);
    
#endif /* BMUpsampler_h */

#ifdef __cplusplus
}
#endif
