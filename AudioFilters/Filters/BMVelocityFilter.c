//
//  BMVelocityFilter.c
//  VelocityFilter
//
//  Created by Hans on 16/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMVelocityFilter.h"
#include "Constants.h"
#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif
    
    void BMVelocityFilter_init(BMVelocityFilter* f, float sampleRate, bool stereo){
        // init the filter memory
        BMMultiLevelBiquad_init(&f->This, 1, sampleRate, stereo, true,false);
        
        // configure defaults for the velocity simulation
        float centreVelocity = 100.0f;
        float maxGainDb = 3.0f;
        float minGainDb = -15.0f;
        BMVelocityFilter_setVelocityGainRange(f, minGainDb, maxGainDb, centreVelocity);
        
        
        // initialise the filter with centre velocity so that it doesn't filter anything
        float filterCutoff = 1000.0;
        float midiVelocity = 100;
        BMVelocityFilter_newNote(f, midiVelocity, filterCutoff);
    }
    
    
    void BMVelocityFilter_processBufferStereo(BMVelocityFilter* f, const float* inL, const float* inR, float* outL, float* outR, size_t numSamples){
        BMMultiLevelBiquad_processBufferStereo(&f->This, inL, inR, outL, outR, numSamples);
    }
    
    
    
    void BMVelocityFilter_processBufferMono(BMVelocityFilter* f, const float* input, float* output, size_t numSamples){
        BMMultiLevelBiquad_processBufferMono(&f->This, input, output, numSamples);
    }
    
    
    
    void BMVelocityFilter_newNote(BMVelocityFilter* f, float velocity, float fc){
        assert(velocity >= 0.0 && velocity <= 127.0);
        
        
        /*
         *  three cases: velovity > centre
         *               velocity < centre
         *               velocity == centre
         */
        // velocity > centre => boost gain
        float gain_db;
        if (velocity > f->centreVelocity)
            gain_db = f->maxGainDb*(velocity - f->centreVelocity)/(127.0 - f->centreVelocity);
        
        // velocity < centre => cut gain
        else if (velocity < f->centreVelocity)
            gain_db = f->minGainDb*(f->centreVelocity - velocity)/(f->centreVelocity);
        
        // velocity == centre => unity gain
        else gain_db = 0.0;
        
        
        // set the high shelf filter gain and cutoff frequency
        BMMultiLevelBiquad_setHighShelf(&f->This,
                                        fc,
                                        gain_db,
                                        0);
    }
    
    
    
    
    void BMVelocityFilter_setVelocityGainRange(BMVelocityFilter* f, float minGainDb, float maxGainDb, float centreVelocity){
        assert(maxGainDb>=0.0 && minGainDb<=0.0);
        assert(centreVelocity < 127.0 && centreVelocity > 0.0);
        
        f->minGainDb = minGainDb;
        f->maxGainDb = maxGainDb;
        f->centreVelocity = centreVelocity;
    }
    
    void BMVelocityFilter_destroy(BMVelocityFilter* f){
        BMMultiLevelBiquad_destroy(&f->This);
    }
    
#ifdef __cplusplus
}
#endif
