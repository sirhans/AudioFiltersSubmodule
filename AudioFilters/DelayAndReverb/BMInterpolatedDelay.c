//
//  BMInterpolatedDelay.c
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "BMInterpolatedDelay.h"
    
#define INTRP_DLY_CFCT_TBL_LENGTH 100
    
    void BMInterpolatedDelay_init(BMInterpolatedDelay *This,
                                  bool stereo,
                                  float initialLengthInSamples,
                                  size_t interpolationOrder,
                                  size_t maxLengthSamples){
        
        // make sure the inputs are reasonable
        assert((float)maxLengthSamples >= initialLengthInSamples);
        assert(initialLengthInSamples > 0.0f);
        assert(interpolationOrder <= INTERP_DELAY_MAX_ORDER);
        
        This->interpolationOrder = interpolationOrder;
        
        // nth order interpolation requires n+1 taps
        size_t numTaps = interpolationOrder + 1;
        
        
        // inititalise the gains and indices to zero
        memset(This->iL,0,sizeof(size_t)*numTaps);
        memset(This->iR,0,sizeof(size_t)*numTaps);
        memset(This->gL,0,sizeof(float)*numTaps);
        memset(This->gR,0,sizeof(float)*numTaps);
        
        // set one index to max delay length (this tells MultiTapDelay_init what
        // the max delay length is.)
        This->iL[0]=maxLengthSamples;
        
        // init the multiTapDelay that manages memory for us
//        BMMultiTapDelay_init(&This->delayBuffer,
//                            stereo,
//                            This->iL,
//                            This->iR,
//                            This->gL,
//                            This->gR,
//                            numTaps,
//                            numTaps);
        
        // init the table of interpolation coefficients
        BMLagrangeInterpolationTable_init(&This->interpolationTable,
                                          interpolationOrder,
                                          INTRP_DLY_CFCT_TBL_LENGTH);
    }
    
    
    
    
    
    
    void BMInterpolatedDelay_destroy(BMInterpolatedDelay *This){
        //BMMultiTapDelay_destroy(&This->delayBuffer);
    }
    
    
    
    
    
    
    void BMInterpolatedDelay_setLength(BMInterpolatedDelay *This, float length){
        // separate length into fractional and integer parts
        float intDelayF = floorf(length);
        size_t intDelayI = (size_t) intDelayF;
        float fracDelay = length - intDelayF;
        
        // how many delay taps do we need?
        size_t numTaps = This->interpolationOrder + 1;
        
        // the shortest delay tap time in the interpolated read
        size_t firstTapTime;
        // the fractional delay between the first tap and the interpolated read
        // location
        float delta;
        
        
        /*
         * We need to keep the interpolation point centred around the dCentreF
         * to prevent phase discontinuities. To do this, we handle the
         * interpolation differently depending on where the fractional delay
         * position is located relative to dCentre.
         */
        if (fracDelay > 0.5){ // before centre
            // increment the integer part of the delay
            firstTapTime = intDelayI + 1 + This->interpolationTable.dCentreI;
            // and decrement the fractional part of the delay
            delta = (fracDelay - 1.0f) + This->interpolationTable.dCentreF;
        } else { // after centre
            // process directly
            firstTapTime = intDelayI - This->interpolationTable.dCentreI;
            delta = fracDelay + This->interpolationTable.dCentreF;
        }
        
        
        // what are the integer delay times we need to do the interpolated read?
        for(size_t i=0; i<numTaps; i++)
            This->iL[i] = firstTapTime + i;
        
        // set the delay times
//        BMMultiTapDelay_setDelayTimes(&This->delayBuffer,
//                                      This->iL, numTaps,
//                                      This->iL, numTaps);
        
        // find the index in the interpolation table
        size_t idx =
            BMLagrangInterpolationTable_getIndex(&This->interpolationTable,
                                                 delta);
        
        // copy the interpolation coefficients into place
        memcpy(This->gL, This->interpolationTable.table[idx], sizeof(float)*numTaps);
        memcpy(This->gR, This->interpolationTable.table[idx], sizeof(float)*numTaps);
    }
    
    
    
    void BMInterpolatedDelay_setSpeed(float newSpeed){
        
    }
    

#ifdef __cplusplus
}
#endif
