//
//  BMSmoothGain.h
//  AudioFiltersXcodeProject
//
//  Manages a gain control so that it can be adjusted without causing clicks
//
//  Created by hans anderson on 4/24/19.
//  Anyone may use this file without restrictions of any kind
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMSmoothGain_h
#define BMSmoothGain_h

#include <stdio.h>
#include <stdbool.h>

    
    typedef struct BMSmoothGain {
        float gain, gainTarget, nextGainTarget, perSampleRatioUp, perSampleRatioDown;
        bool inTransition;
    } BMSmoothGain;
    
    
    
    /*!
     * BMSmoothGain_init
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param sampleRate  audio system sample rate; if you need to change this, you must re-initialize the struct
     */
    void BMSmoothGain_init(BMSmoothGain *This, float sampleRate);
    
    
    /*!
     * BMSmoothGain_processBuffer
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param inputL      left channel input buffer length >= numSamples
     * @param inputR      right channel input buffer length >= numSamples
     * @param outputL     left channel output buffer length >= numSamples
     * @param outputR     right channel output buffer length >= numSamples
     */
    void BMSmoothGain_processBuffer(BMSmoothGain *This,
                                    const float* inputL, const float* inputR,
                                    float* outputL, float* outputR,
                                    size_t numSamples);
    
    /*!
     * BMSmoothGain_processBufferMono
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param input       input buffer length >= numSamples
     * @param output      output buffer length >= numSamples
     */
    void BMSmoothGain_processBufferMono(BMSmoothGain *This,
                                        const float* input,
                                        float* output,
                                        size_t numSamples);


    /*!
     * BMSmoothGain_processBuffers
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param inputs      input buffers with size [numChannels,numSamples]
     * @param outputs     output buffers with size [numChannels,numSamples]
     * @param numChannels major dimension of inputs and outputs
     * @param numSamples  minor diminsion of inputs and outputs
     */
    void BMSmoothGain_processBuffers(BMSmoothGain *This,
                                     const float** inputs,
                                     float** outputs,
                                     size_t numChannels,
                                     size_t numSamples);
        
    
    /*!
     * BMSmoothGain_setGainDb
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @param gainDb      new target gain in decibels to be approached smoothly
     */
    void BMSmoothGain_setGainDb(BMSmoothGain *This, float gainDb);
    
    
    
    /*!
     *BMSmoothGain_getGainLinear
     *
     * @abstract   returns the linear (voltage scale) gain 
     */
    float BMSmoothGain_getGainLinear(BMSmoothGain *This);
    
    
    
    
    /*!
     * BMSmoothGain_getGainDb
     *
     * @param This        pointer to an initialized BMSmoothGain struct
     * @return            gain in decibels
     */
    float BMSmoothGain_getGainDb(BMSmoothGain *This);

#endif /* BMSmoothGain_h */

#ifdef __cplusplus
}
#endif
