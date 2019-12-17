//
//  BMInterpolatedDelay.h
//  BMAudioFilters
//
//  Created by Hans on 4/9/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMInterpolatedDelay_h
#define BMInterpolatedDelay_h

#include <stdio.h>
#include <stdbool.h>
#include "BMMultiTapDelay.h"
#include "BMLagrangeInterpolationTable.h"
    
    #define INTERP_DELAY_MAX_ORDER 9
    
    
    typedef struct BMInterpolatedDelay {
        BMMultiTapDelay delayBuffer;
        BMLagrangeInterpolationTable interpolationTable;
        size_t interpolationOrder;
        
        // arrays used for temp storage during updates
        float gL [INTERP_DELAY_MAX_ORDER + 1];
        float gR [INTERP_DELAY_MAX_ORDER + 1];
        size_t iL [INTERP_DELAY_MAX_ORDER + 1];
        size_t iR [INTERP_DELAY_MAX_ORDER + 1];
    } BMInterpolatedDelay;
    
    
    
    void BMInterpolatedDelay_init(BMInterpolatedDelay *This,
                                  bool stereo,
                                  float initialLengthInSamples,
                                  size_t interpolationOrder,
                                  size_t maxLengthSamples);
    
    
    void BMInterpolatedDelay_destroy(BMInterpolatedDelay *This);
    
    
    static __inline__ __attribute__((always_inline)) void BMInterpolatedDelay_processSampleLinearInterpMono(BMInterpolatedDelay *This,
                                                          float in,
                                                          float* out);
    
    
    
    
    static __inline__ __attribute__((always_inline)) void BMInterpolatedDelay_processSampleLinearInterpStereo(BMInterpolatedDelay *This,
                                                            float inL,
                                                            float inR,
                                                            float* outL,
                                                            float* outR);
    
    
    void BMInterpolatedDelay_setSpeed(float newSpeed);
    
    
    

#endif /* BMInterpolatedDelay_h */

#ifdef __cplusplus
}
#endif
