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



typedef struct BMCrossover3way {
    BMMultiLevelBiquad low;
    BMMultiLevelBiquad midAndHigh;
    BMMultiLevelBiquad mid;
    BMMultiLevelBiquad high;
    bool stereo;
    bool fourthOrder;
} BMCrossover3way;



typedef struct BMCrossover4way {
    BMMultiLevelBiquad band1;
    BMMultiLevelBiquad bands2to4;
    BMMultiLevelBiquad band2;
    BMMultiLevelBiquad bands3to4;
    BMMultiLevelBiquad band3;
    BMMultiLevelBiquad band4;
    bool stereo;
    bool fourthOrder;
} BMCrossover4way;






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
void BMCrossover_free(BMCrossover* This);





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

/*!
 *BMCrossover3way_init
 * @abstract This function must be called prior to use
 *
 * @param cutoff1     - cutoff frequency between bands 1 and 2 in hz
 * @param cutoff2     - cutoff frequency between bands 2 and 3 in hz
 * @param sampleRate  - sample rate in hz
 * @param fourthOrder - true: 4th order, false: 2nd order
 * @param stereo      - true: stereo, false: mono
 */
void BMCrossover3way_init(BMCrossover3way* This,
                          float cutoff1,
                          float cutoff2,
                          float sampleRate,
                          bool fourthOrder,
                          bool stereo);
/*!
 *BMCrossover3way_free
 */
void BMCrossover3way_free(BMCrossover3way* This);


/*!
 *BMCrossover3way_setCutoff1
 */
void BMCrossover3way_setCutoff1(BMCrossover3way *This, float fc);


/*!
 *BMCrossover3way_setCutoff2
 */
void BMCrossover3way_setCutoff2(BMCrossover3way *This, float fc);


/*!
 *BMCrossover3way_processStereo
 */
void BMCrossover3way_processStereo(BMCrossover3way *This,
                                   const float* inL, const float* inR,
                                   float* lowL, float* lowR,
                                   float* midL, float* midR,
                                   float* highL, float* highR,
                                   size_t numSamples);


/*!
 *BMCrossover4way_init
 * @abstract This function must be called prior to use
 *
 * @param cutoff1     - cutoff frequency between bands 1 and 2 in hz
 * @param cutoff2     - cutoff frequency between bands 2 and 3 in hz
 * @param cutoff3     - cutoff frequency between bands 2 and 3 in hz
 * @param sampleRate  - sample rate in hz
 * @param fourthOrder - true: 4th order, false: 2nd order
 * @param stereo      - true: stereo, false: mono
 */
void BMCrossover4way_init(BMCrossover4way* This,
                          float cutoff1,
                          float cutoff2,
                          float cutoff3,
                          float sampleRate,
                          bool fourthOrder,
                          bool stereo);

/*!
 *BMCrossover4way_free
 */
void BMCrossover4way_free(BMCrossover4way* This);


/*!
 *BMCrossover4way_processStereo
 */
void BMCrossover4way_processStereo(BMCrossover4way *This,
                                   const float* inL, const float* inR,
                                   float* band1L, float* band1R,
                                   float* band2L, float* band2R,
                                   float* band3L, float* band3R,
                                   float* band4L, float* band4R,
                                   size_t numSamples);

/*!
 *BMCrossover4way_setCutoff1
 */
void BMCrossover4way_setCutoff1(BMCrossover4way *This, float fc);

/*!
 *BMCrossover4way_setCutoff2
 */
void BMCrossover4way_setCutoff2(BMCrossover4way *This, float fc);

/*!
 *BMCrossover4way_setCutoff3
 */
void BMCrossover4way_setCutoff3(BMCrossover4way *This, float fc);

#endif /* BMCrossover_h */

#ifdef __cplusplus
}
#endif

