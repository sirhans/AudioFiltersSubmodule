//
//  BMVelocityFilter.h
//  VelocityFilter
//
//  Created by Hans on 16/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMVelocityFilter_h
#define BMVelocityFilter_h

#include "BMMultiLevelBiquad.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct BMVelocityFilter{
        BMMultiLevelBiquad bqf;
        float maxGainDb, minGainDb, centreVelocity;
    } BMVelocityFilter;
    
    
    void BMVelocityFilter_processBufferStereo(BMVelocityFilter* f, const float* inL, const float* inR, float* outL, float* outR, size_t numSamples);
    
    
    void BMVelocityFilter_processBufferMono(BMVelocityFilter* f, const float* input, float* output, size_t numSamples);
    
    /*
     * velocity in [0,127]
     *
     * fcForMIDINote - an array of 127 floats, specifying the cutoff frequency
     * for velocity simulation. Above this frequency the gain is increased for
     * higher velocity.  Below, it remains constant for all velocities. The
     * filter is a second order high-shelf so the cutoff is very gradual and
     * there will be some attenuation below the cutoff.
     */
    void BMVelocityFilter_newNote(BMVelocityFilter* f, float velocity, float fc);
    
    
    /*
     * call this before using the filter
     */
    void BMVelocityFilter_init(BMVelocityFilter* f, float sampleRate, bool isStereo);
    
    
    /*
     * sets the range in db from the min to the max excursion of the high-shelf
     * filter used to simulate velocity layers.  The init function sets reasonable
     * defaults of max = 3; min = -15; If those are acceptable then you do not
     * need to call this function.
     *
     * minGainDb - the gain applied for velocity=0
     *               requires minGainDB <= 0
     *
     * maxGainDb - the gain applied for velocity=127
     *               requires maxGainDB >= 0
     *
     * centreVelocity - for velocity > centreVelocity, gain is boosted
     *                  for velocity < centreVelocity, gain is cut
     *                  for velocity == centreVelocity, no gain change
     *                  for values in betwee, gain is interpolated linearly
     *                  requires: 0 < centreVelocity < 127
     *
     */
    void BMVelocityFilter_setVelocityGainRange(BMVelocityFilter* f, float minGainDb, float maxGainDb, float centreVelocity);
    
    /*
     * frees the memory used by the filter
     */
    void BMVelocityFilter_destroy(BMVelocityFilter* f);
    
#ifdef __cplusplus
}
#endif

#endif /* BMVelocityFilter_h */
