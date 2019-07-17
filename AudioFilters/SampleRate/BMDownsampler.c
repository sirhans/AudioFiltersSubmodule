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

#include <Accelerate/Accelerate.h>
#include "BMDownsampler.h"
#include "BMFastHadamard.h"
#include "BMPolyphaseIIR2Designer.h"
    
#define BM_DOWNSAMPLER_TRANSITION_BANDWIDTH 0.025f
#define BM_DOWNSAMPLER_STOPBAND_ATTENUATION 100.0f
    
    
    
    
    void BMDownsampler_init(BMDownsampler* This,
                            size_t decimationFactor){
        // decimation factor must be a power of 2
        assert(BMPowerOfTwoQ(decimationFactor));
        
        This->numStages = (size_t)log2(decimationFactor);
        
        This->downsamplers = malloc(sizeof(BMDownsampler)*This->numStages);
        
        for(size_t i=0; i<This->numStages; i++){
            // compute the transition bandwidth for the current stage
            double transitionBw = BMPolyphaseIIR2Designer_transitionBandwidthForStage(BM_DOWNSAMPLER_TRANSITION_BANDWIDTH, i);
            
            // set up the stage
            BMHIIRDownsampler2xFPU_init(&This->downsamplers[i],
                                        BM_DOWNSAMPLER_STOPBAND_ATTENUATION,
                                        transitionBw);
        }
    }
    
    
    
    
    void BMDownsampler_processBufferMono(BMDownsampler* This,
                                         float* input,
                                         float* output,
                                         size_t numSamplesIn){
        BMHIIRDownsampler2xFPU_processBufferMono(&This->downsamplers[0], input, output, numSamplesIn);
        for(size_t i=1; i<This->numStages; i++){
            numSamplesIn *= 2;
            BMHIIRDownsampler2xFPU_processBufferMono(&This->downsamplers[i], output, output, numSamplesIn);
        }
    }
    
    
    
    
    void BMDownsampler_free(BMDownsampler* This){
        for(size_t i=0; i<This->numStages; i++)
            BMHIIRDownsampler2xFPU_free(&This->downsamplers[i]);
        free(This->downsamplers);
        This->downsamplers = NULL;
    }
    
    
    
    
    void BMDownsampler_impulseResponse(BMDownsampler* This, float* IR, size_t numSamples){
    }
    
    
#ifdef __cplusplus
}
#endif

