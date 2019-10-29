//
//  BMEarlyReflections.h
//  BMAudioFilters
//
//  Created by Hans on 1/6/17.
//
//  The author releases this file into the public domain with no restrictions
//  of any kind.
//


#ifdef __cplusplus
extern "C" {
#endif


#ifndef BMEarlyReflections_h
#define BMEarlyReflections_h
    
#include "BMMultiTapDelay.h"
#include "Constants.h"

    typedef struct BMEarlyReflectionsSetting {
        size_t** indices;
        float** gain;
        
        size_t numberOfChannel;
        size_t numberTaps;
        float sampleRate;
        
        float startTimeMS, endTimeMS;
        float wet, dry;
    } BMEarlyReflectionsSetting;
    
    
    typedef struct BMEarlyReflections {
        BMMultiTapDelay delay;
        
        float** gain;
        
        BMEarlyReflectionsSetting setting;
    } BMEarlyReflections;
    
    
    /*
     * Initialises a stereo early reflections simulation with the first
     * reflection starting after startTimeMS and last reflection ending 
     * before endTimeMS.  Between startTimeMS and endTimeMS, each channel
     * has numTapsPerChannel output taps.
     *
     * Tap times are set to produce "Velvet Noise"
     * (See: "Reverberation modeling using velvet noise" by M. Karjalainen 
     * and Hanna Järveläinen. 
     * https://www.researchgate.net/publication/283247924_Reverberation_modeling_using_velvet_noise
     *
     * @param This         pointer to the struct to initialise
     * @param startTimeMS  start time in milliseconds after the input impulse
     * @param endTimeMS    end time in milliseconds after the impulse
     * @param sampleRate   sample rate of audio system in samples per second
     * @param wetAmount    wet^2 + dry^2 == 1
     * @param numTapsPerChannel  output taps on each channel L and R
     */
    void BMEarlyReflections_init(BMEarlyReflections *This,
                                 float startTimeMS,
                                 float endTimeMS,
                                 float sampleRate,
                                 float wetAmount,
                                 size_t numTapsPerChannel);
    
    
    
    /*
     * set the portion of wet signal to mix with output. This also
     * sets dry gain so that wet^2 + dry^2 == 1.0
     *
     * @param wetAmount  gain of wet signal; must be in [0.0,1.0]
     */
    void BMEarlyReflections_setWetAmount(BMEarlyReflections *This,
                                         float wetAmount);
    
    
    
    /*
     * free memory used by the struct at *This
     */
    void BMEarlyReflections_destroy(BMEarlyReflections *This);

    
    
    void BMEarlyReflections_processBuffer(BMEarlyReflections *This,
                                          float* inputL, float* inputR,
                                          float* outputL, float* ouputR,
                                          size_t numSamples);
    
    
    /*
     * new note will change indice & gain
     */
    void BMEarlyReflection_RegenIndicesAndGain(BMEarlyReflections *This);
    
    /*
     * Prints the impulse response to standard output, for testing
     */
    void BMEarlyReflections_impulseResponse(BMEarlyReflections *This);
    
    
    
#endif /* BMEarlyReflections_h */


    
#ifdef __cplusplus
}
#endif
