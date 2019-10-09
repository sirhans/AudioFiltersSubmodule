//
//  BMCrossover.c
//  BMAudioFilters
//
//  Created by Hans on 17/2/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include <Accelerate/Accelerate.h>
#include "BMCrossover.h"
#include <assert.h>
    
    /*!
     *BMCrossover_init
     * @abstract This function must be called prior to use
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
                          bool stereo){
        
        This->fourthOrder = fourthOrder;
        This->stereo      = stereo;
        
        
        // we need one biquad section for 2nd order; two for 4th
        size_t numLevels;
        numLevels = fourthOrder ? 2:1;
        
        
        // initialise the filters
        BMMultiLevelBiquad_init(&This->lp,
                                numLevels,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->hp,
                                numLevels,
                                sampleRate,
                                stereo,
                                false,false);
        
        
        BMCrossover_setCutoff(This, cutoff);
    }
    
    
    
    
    
    
    /*
     * Free memory used by the filters
     */
    void BMCrossover_destroy(BMCrossover* This){
        BMMultiLevelBiquad_destroy(&This->lp);
        BMMultiLevelBiquad_destroy(&This->hp);
    }
    
    
    
    
    
    
    /*
     * @param cutoff - cutoff frequency in Hz
     */
    void BMCrossover_setCutoff(BMCrossover* This, float cutoff){
        
        if(This->fourthOrder){
            // the fourth order crossover uses pairs of
            // 2nd order butterworth filters
            //
            // lowpass pair
            BMMultiLevelBiquad_setLowPass12db(&This->lp,
                                              cutoff,
                                              0);
            BMMultiLevelBiquad_setLowPass12db(&This->lp,
                                              cutoff,
                                              1);
            // highpass pair
            BMMultiLevelBiquad_setHighPass12db(&This->hp,
                                               cutoff,
                                               0);
            BMMultiLevelBiquad_setHighPass12db(&This->hp,
                                               cutoff,
                                               1);
        } else {
            // the second order crossover uses 2nd order linkwitz-riley
            // filters. The transfer function of these filters is
            // equivalent to a cascade of first order butterworth
            // filters. However, the vDSP library only has machine
            // optimised code for biquads so don't want to implement
            // this as a cascade of first-order filters
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->lp,
                                                  cutoff,
                                                  0);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->hp,
                                                  cutoff,
                                                  0);
        }
    }
    
    
    
    /*
     * Apply the crossover to stereo audio
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
                                   size_t numSamples){
        assert(This->stereo);
        
        BMMultiLevelBiquad_processBufferStereo(&This->lp,
                                               inL, inR,
                                               lowpassL, lowpassR,
                                               numSamples);
        BMMultiLevelBiquad_processBufferStereo(&This->hp,
                                               inL, inR,
                                               highpassL, highpassR,
                                               numSamples);
        
        // if the filter is second order, one of the two outputs must be
        // inverted so that the sum of the two have unity gain transfer
        // function when we sum the lowpass and highpass outputs back together.
        if(!This->fourthOrder){
            // negate the highpass outputs
            vDSP_vneg(highpassL, 1, highpassL, 1, numSamples);
            vDSP_vneg(highpassR, 1, highpassR, 1, numSamples);
        }
    }
    
    
    
    
    
    
    
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
                                 size_t numSamples){
        assert(!This->stereo);
        
        
        BMMultiLevelBiquad_processBufferMono(&This->lp,
                                             input,
                                             lowpass,
                                             numSamples);
        BMMultiLevelBiquad_processBufferMono(&This->hp,
                                             input,
                                             highpass,
                                             numSamples);
        
        
        // if the filter is second order, one of the two outputs must be
        // inverted so that the sum of the two have unity gain transfer
        // function when we sum the lowpass and highpass outputs back
        // together.
        if(!This->fourthOrder){
            // negate the highpass outputs
            vDSP_vneg(highpass, 1, highpass, 1, numSamples);
        }
    }

    
    
    
#ifdef __cplusplus
}
#endif
