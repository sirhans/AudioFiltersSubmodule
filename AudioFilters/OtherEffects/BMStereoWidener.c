//
//  BMStereoWidener.c
//  BMAudioFilters
//
//  This does the following:
//
//  1. Convert from stereo to mid-side audio format
//  2. Adjust the mid / side balance
//  3. Convert back to stereo
//
//  Created by Blue Mangoo on 15/2/17.
//
//  This file is public domain without restrictions of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif
    
#include "BMStereoWidener.h"
#include <assert.h>

    void BMStereoWidener_processAudio(BMStereoWidener *This,
                                      float* inL, float* inR,
                                      float* outL, float* outR,
                                      size_t numSamples){
        
        // convert from stereo to mid / side
        BMMidSideMatrixConvert(inL, inR, This->mid, This->side, numSamples);
        
        
        
        
        // adjust the width
        //
        // if width = 0 then mid *= 2, side *= 0
        // if width = 1 then mid *= 1, side *= 1
        // if width = 2 then mid *= 0, side *= 2
        
        // make sure width is in [0,2]
        assert(This->width >= 0.0 && This->width <= 2.0);
        
        // save variables to adjust the mid and side channel signal gains
        float midAdjust  = sqrt(1.0 - This->width*0.5)*M_SQRT2;
        float sideAdjust = sqrt(This->width*0.5)*M_SQRT2;
        
        // adjust the width by multiplying the adjustment coefficients
        vDSP_vsmul(This->mid, 1, &midAdjust, This->mid, 1, numSamples);
        vDSP_vsmul(This->side, 1, &sideAdjust, This->side, 1, numSamples);
        
        
        
        
        // convert back to stereo
        BMMidSideMatrixConvert(This->mid, This->side, outL, outR, numSamples);
    }
    
    
    /*
     * Set the width
     *
     * @param width - range of values is [0,2]
     0 is mono mixdown, 1 is bypass, 2 is max width
     */
    void BMStereoWidener_setWidth(BMStereoWidener *This, float width){
        assert(width >= 0.0 && width <= 2.0);
        
        This->width = width;
    }
    
#ifdef __cplusplus
}
#endif

