//
//  BMRoundRobinFilter.c
//  BMAudioFilters
//
//  Created by Hans on 23/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMRoundRobinFilter.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    void randomiseOrder(bool* list, size_t length);
    
    void BMRoundRobinFilter_processBufferStereo(BMRoundRobinFilter* f, const float* inL, const float* inR, float* outL, float* outR, size_t numSamples){
        BMMultiLevelBiquad_processBufferStereo(&f->This, inL, inR, outL, outR, numSamples);
    }
    
    void BMRoundRobinFilter_processBufferMono(BMRoundRobinFilter* f, const float* input, float* output, size_t numSamples){
        BMMultiLevelBiquad_processBufferMono(&f->This, input, output, numSamples);
    }
    
    void BMRoundRobinFilter_init(BMRoundRobinFilter* f, float sampleRate, size_t numBands, size_t numActiveBands, float minFc, float maxFc, float gainRange_db, bool stereo){
        // the filter random
        assert(numActiveBands < numBands);
        
        f->numBands = numBands;
        f->numActiveBands = numActiveBands;
        f->minFc = minFc;
        f->maxFc = maxFc;
        f->gainRange_db = gainRange_db;
        f->activeBands = malloc(sizeof(bool)*numBands);
        f->bandSpacing = (maxFc - minFc)/(float)(numBands - 1);
        
        // set the correct number of active bands
        for(size_t i=0; i<numBands; i++){
            if(i<numActiveBands)
                f->activeBands[i]=true;
            else
                f->activeBands[i]=false;
        }
        
        BMMultiLevelBiquad_init(&f->This, numActiveBands, sampleRate, stereo, true,false);
        
        // initialize the round robin filter with a random setting
        BMRoundRobinFilter_newNote(f);
        
    }
    
    
    //inline void BMRoundRobinFilter_newNote(BMRoundRobinFilter* f){
    //    float fc = f->minFc;
    //
    //    // find the space between individual peaks in the filters
    //    // spacing them evenly in linear frequency
    //    float peakSpacing = (f->maxFc - f->minFc)/(float)(f->numBands - 1);
    //
    //    for (size_t i=0; i<f->numBands; i++) {
    //        float gain_db = (((float)rand()/(float)RAND_MAX)-0.5)*2.0*f->gainRange_db;
    //
    //        // note that Q = peakSpacing
    //        BMMultiLevelBiquad_setBell(&(f->This), fc, peakSpacing, gain_db, i);
    //
    //        // move the fc over for the next peak
    //        fc += peakSpacing;
    //    }
    //}
    
    // randomise the order of a list of booleans
    inline void randomiseOrder(bool* list, size_t length){
        
        for (size_t i = 0; i<length; i++) {
            size_t j = (size_t)rand() % length;
            
            // swap i with j
            bool temp = list[i];
            list[i] = list[j];
            list[j] = temp;
        }
    }
    
    void BMRoundRobinFilter_newNote(BMRoundRobinFilter* f){
        
        /*
         * randomise the selection of active bands
         */
        randomiseOrder(f->activeBands, f->numBands);
        
        
        float fc = f->minFc;
        size_t level = 0;
        for (size_t i=0; i<f->numBands; i++) {
            
            // if the ith band is active, set a filter there
            if(f->activeBands[i]){
                float q = f->bandSpacing;
                float gain_db = f->gainRange_db;
                // randomise the sign of the gain
                if (rand()%2==0)
                    gain_db *= -1.0f;
                BMMultiLevelBiquad_setBell(&(f->This), fc, q, gain_db, level++);
            }
            
            // move the fc over for the next peak
            fc += f->bandSpacing;
        }
    }
    
    void BMRoundRobinFilter_destroy(BMRoundRobinFilter* f){
        BMMultiLevelBiquad_free(&f->This);
        free(f->activeBands);
        f->activeBands = NULL;
    }
    
    
#ifdef __cplusplus
}
#endif
