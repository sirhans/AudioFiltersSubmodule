//
//  BMDownsampler.c
//  BMAudioFilters
//
//  Created by Hans on 13/9/17.
//  Anyone may use this file without restrictions
//

#ifdef __cplusplus
extern "C" {
#endif
    
    
#include "BMDownsampler.h"
#include "BMFastHadamard.h"
#include <Accelerate/Accelerate.h>
    
#define BM_DOWNSAMPLER_LAST_STAGE_NUM_COEFFICIENTS 4
#define BM_DOWNSAMPLER_LAST_STAGE_TRANSITION_BANDWIDTH 0.05
    
    
    
    
    void BMDownsampler_init(BMDownsampler* This,
                            size_t decimationFactor){
        // decimation factor must be a power of 2
        assert(BMPowerOfTwoQ(decimationFactor));
        
        BMHIIRDownsampler2x_init(&This->lastStageDS,
                                 BM_DOWNSAMPLER_LAST_STAGE_NUM_COEFFICIENTS,
                                 BM_DOWNSAMPLER_LAST_STAGE_TRANSITION_BANDWIDTH);
        
        // we haven't implemented any code for higher than 2x downsampling yet
        // so don't allow it.
        assert(decimationFactor == 2);
    }
    
    
    
    
    
    void BMDownsampler_processBufferMono(BMDownsampler* This,
                                         float* input,
                                         float* output,
                                         size_t numSamplesIn){
        BMHIIRDownsampler2x_processBufferMono(&This->lastStageDS, input, output, numSamplesIn);
    }
    
    
    
    
    void BMDownsampler_free(BMDownsampler* This){
        BMHIIRDownsampler2x_free(&This->lastStageDS);
    }
    
    
    
    void BMDownsampler_impulseResponse(BMDownsampler* This, float* IR, size_t numSamples){
        BMHIIRDownsampler2x_impulseResponse(&This->lastStageDS, IR, numSamples);
    }
    
    
#ifdef __cplusplus
}
#endif

