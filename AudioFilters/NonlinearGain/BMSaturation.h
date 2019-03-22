//
//  BMSaturation.h
//  BMAudioFilters
//
//  Created by Hans on 31/7/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMSaturation_h
#define BMSaturation_h

#define BMSAT_NUMSTAGES 10

#ifdef __cplusplus
extern "C" {
#endif
    
    #include <stdio.h>
    #include "BMGainStage.h"
    #include "Constants.h"
    
    typedef struct BMSaturation {
        BMGainStage gainStages [BMSAT_NUMSTAGES];
        float* tBuffer;
    } BMSaturation;
    

    /*
     * Does not work in place (input != output)
     */
    void BMSaturation_processBufferMono(BMSaturation* This,
                                        float* input,
                                        float* output,
                                        size_t numSamples);
    
    void BMSaturation_init(BMSaturation* This, float sampleRate, float AAFilterFc);
    
    void BMSaturation_destroy(BMSaturation* This);
    
    void BMSaturation_setGain(BMSaturation* This, float gain);
    
    void BMSaturation_setAAFC(BMSaturation* This, float fc);
    
    void BMSaturation_setSampleRate(BMSaturation* This, float sampleRate );
    
    void BMSaturation_setTightness(BMSaturation* This, float tightness );
    
    
    
#ifdef __cplusplus
}
#endif

#endif /* BMSaturation_h */
