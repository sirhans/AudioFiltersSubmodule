//
//  BMStereoWidener.h
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
    
#ifndef BMStereoWidener_h
#define BMStereoWidener_h
    
#include "BMMidSide.h"
#include "Constants.h"
    
    
    
    typedef struct BMStereoWidener {
        float mid [BM_BUFFER_CHUNK_SIZE];
        float side [BM_BUFFER_CHUNK_SIZE];
        float width;
    } BMStereoWidener;
    

    /*
     * Works in place
     */
    void BMStereoWidener_processAudio(BMStereoWidener* This,
                                      float* inL, float* inR,
                                      float* outL, float* outR,
                                      size_t numSamples);
    
    void BMStereoWidener_setWidth(BMStereoWidener* This, float width);
    
    
#endif /* BMStereoWidener_h */
    
#ifdef __cplusplus
}
#endif

