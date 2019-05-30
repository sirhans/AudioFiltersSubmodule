//
//  BMVariableFilter.h
//  AudioFiltersXcodeProject
//
//  This is a filter with continuously variable parameters designed for doing
//  filter sweeps
//
//  Created by Hans on 30/5/19.
//  Anyone may use this file without restrictions
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMVariableFilter_h
#define BMVariableFilter_h

#include <stdio.h>
#include "Constants.h"
    
    
    typedef struct BMVariableFilter {
        float sampleRate, fc;
        float ic1, ic2;
        float antiClick [BM_BUFFER_CHUNK_SIZE];
        float g [BM_BUFFER_CHUNK_SIZE];
        float k, lastG;
        float a1 [BM_BUFFER_CHUNK_SIZE];
        float a2 [BM_BUFFER_CHUNK_SIZE];
        float a3 [BM_BUFFER_CHUNK_SIZE];
        float previousOutputValue;
    } BMVariableFilter;
    
    
    
    /*!
     * BMVariableFilter_freqSweepMono
     *
     * @param audioInput     array of values for filter inout, length = numSamples
     * @param cutoffControl  an array of filter cutoff frequencies length = numSamples
     * @param audioOutput    an array of values for the filter output, length = numSamples
     * @param numSamples     length of input and output arrays
     */
    void BMVariableFilter_freqSweepMono(BMVariableFilter* This,
                                        const float* audioInput,
                                        const float* cutoffControl,
                                        float* audioOutput,
                                        size_t numSamples);
    

#endif /* BMVariableFilter_h */
    
#ifdef __cplusplus
}
#endif
