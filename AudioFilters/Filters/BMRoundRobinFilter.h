//
//  BMRoundRobinFilter.h
//  BMAudioFilters
//
//  This struct simulates round robin sampling from a single sample by
//  applying a bank of bell filters with randomised gain.  Each time
//  the functino newNote() is called, the filters are randomised anew,
//  creating subtle tonal differences between notes.
//
//  Created by Hans on 23/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMRoundRobinFilter_h
#define BMRoundRobinFilter_h

#include "BMMultiLevelBiquad.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct BMRoundRobinFilter{
        BMMultiLevelBiquad bqf;
        float minFc, maxFc, gainRange_db, bandSpacing;
        size_t numBands;
        size_t numActiveBands;
        bool* activeBands;
    } BMRoundRobinFilter;
    
    /*
     * Must be called before using the filter.
     *
     * sampleRate - self explanitory
     * numBands - the number of frequency bands to use
     * numActiveBands - the number of bands at which we actually do filtering.
     *                  this should be less than half of numBands. Each time
     *                  newNote is called, we randomly select a set of
     *                  numActiveBands < numBands to set filters.
     * minFc - centre frequency of the lowest bell
     * maxFc - centre frequency of the highest bell
     * gainRange_db - the maximum excursion from zero gain for a single bell.
     *                for example, if this is 3db, then each bell will have gain
     *                set randomly between -3 and +3 db.
     * stereo - set true if you need stereo processing.
     *
     */
    void BMRoundRobinFilter_init(BMRoundRobinFilter* f, float sampleRate, size_t numBands, size_t numActiveBands, float minFc, float maxFc, float gainRange_db, bool stereo);
    
    /*
     * to use this function, you must call _init() with stereo=true
     */
    void BMRoundRobinFilter_processBufferStereo(BMRoundRobinFilter* f, const float* inL, const float* inR, float* outL, float* outR, size_t numSamples);
    
    /*
     * to use this function, you must call _init() with stereo=false
     */
    void BMRoundRobinFilter_processBufferMono(BMRoundRobinFilter* f, const float* input, float* output, size_t numSamples);
    
    /*
     * Call this function before the start of each new note to re-randomise the
     * filters.
     */
    void BMRoundRobinFilter_newNote(BMRoundRobinFilter* f);
    
    /*
     * free the memory used by the filter
     */
    void BMRoundRobinFilter_destroy(BMRoundRobinFilter* f);
    
#ifdef __cplusplus
}
#endif

#endif /* BMRoundRobinFilter_h */
