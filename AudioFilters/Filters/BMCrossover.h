//
//  BMCrossover.h
//  BMAudioFilters
//
//  This is a Linkwitz-Riley crossover, created by cascading pairs
//  of Butterworth filters.
//
//  Created by Hans on 17/2/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMCrossover_h
#define BMCrossover_h

#include "BMMultiLevelBiquad.h"
#include "Constants.h"
    
    
    
    
    typedef struct BMCrossover {
        BMMultiLevelBiquad lp;
        BMMultiLevelBiquad hp;
        bool stereo;
        bool fourthOrder;
    } BMCrossover;
    
    
    
    
    
    
    /*
     * This function must be called prior to use
     *
     * @param cutoff      - cutoff frequency in hz
     * @param sampleRate  - sample rate in hz
     * @param fourthOrder - true: 4th order, false: 2nd order
     * @param stereo      - true: stereo, false: mono
     */
    void BMCrossover_init(BMCrossover* This,
                          float cutoff,
                          float sampleRate,
                          bool fourthOrder,
                          bool stereo);
    
    
    
    
    
    /*
     * Free memory used by the filters
     */
    void BMCrossover_destroy(BMCrossover* This);
    
    
    
    
    
    /*
     * @param cutoff - cutoff frequency in Hz
     */
    void BMCrossover_setCutoff(BMCrossover* This, float cutoff);
    
    
    
    
    
    
    /*
     * Apply the crossover in stereo
     *
     * @param inL        - array of left channel audio input
     * @param inR        - array of right channel audio input
     * @param lowpassL   - lowpass output left
     * @param lowpassR   - lowpass output right
     * @param highpassL  - highpass output left
     * @param highpassR  - highpass output right
     * @param numSamples - number of samples to process. all arrays must have at least this length
     */
    void BMCrossover_processStereo(BMCrossover* This,
                                   const float* inL, const float* inR,
                                   float* lowpassL, float* lowpassR,
                                   float* highpassL, float* highpassR,
                                   size_t numSamples);
    
    
    
    
    
    
    
    
    
    /*
     * Apply the crossover to mono audio
     *
     * @param input      - array of audio input
     * @param lowpass    - lowpass output
     * @param highpass   - highpass output
     * @param numSamples - number of samples to process. all arrays must have at least this length
     */
    void BMCrossover_processMono(BMCrossover* This,
                                 float* input,
                                 float* lowpass,
                                 float* highpass,
                                 size_t numSamples);
    
    
    
#endif /* BMCrossover_h */

#ifdef __cplusplus
}
#endif
