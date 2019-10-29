//
//  BMCrossover.h
//  BMAudioFilters
//
//  This is a Linkwitz-Riley crossover. The Linkwitz-Riley preserves the gain
//  at exactly unity across the spectrum when splitting into separate bands and
//  recombining. However, it only preserves gain when the signal is not modified
//  between the crossover and the recombiner. If the phase correlation between
//  the bands can be estimated, a higher-Q crossover is appropriate. For example
//  if the phase is completely uncorrelated then we need the crossover to have
//  a +3dB boost when splitting and recombining because 3dB of gain is lost when
//  summing two uncorrelated signals. Therefore a perfect crossover should have
//  adjustable Q, according to the estimated phase correlation between the
//  signals in each frequency band. 
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
    // filters for audio processing
    BMMultiLevelBiquad low;
    BMMultiLevelBiquad midAndHigh;
    BMMultiLevelBiquad mid;
    BMMultiLevelBiquad high;
    
    // filters for graph plotting
    BMMultiLevelBiquad plotFilters [3];
    
    bool stereo;
    bool fourthOrder;
} BMCrossover3way;



typedef struct BMCrossover4way {
    // filters for audio processing
    BMMultiLevelBiquad band1;
    BMMultiLevelBiquad bands2to4;
    BMMultiLevelBiquad band2;
    BMMultiLevelBiquad bands3to4;
    BMMultiLevelBiquad band3;
    BMMultiLevelBiquad band4;
    
    // filters for graph plotting
    BMMultiLevelBiquad plotFilters [4];
    
    bool stereo;
    bool fourthOrder;
} BMCrossover4way;






/*
  *This function must be called prior to use
 *
 * @param cutoff      - cutoff frequency in hz
 * @param sampleRate  - sample rate in hz
 * @param fourthOrder - true: 4th order, false: 2nd order
 * @param stereo      - true: stereo, false: mono
 */
void BMCrossover_init(BMCrossover *This,
                      float cutoff,
                      float sampleRate,
                      bool fourthOrder,
                      bool stereo);





/*
 * Free memory used by the filters
 */
void BMCrossover_free(BMCrossover *This);





/*
 * @param cutoff - cutoff frequency in Hz
 */
void BMCrossover_setCutoff(BMCrossover *This, float cutoff);






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
void BMCrossover_processStereo(BMCrossover *This,
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
void BMCrossover_processMono(BMCrossover *This,
                             float* input,
                             float* lowpass,
                             float* highpass,
                             size_t numSamples);




/*!
 *BMCrossover_tfMagVectors
 *
 * @abstract get points to plot a graph of each of the two bands
 *
 * @param This        pointer to an initialized struct
 * @param frequencies input vector containing the x-axis coordinates of the plot
 * @param magLow      output vector containing the y-coordinates for low band
 * @param magHigh     output vector containing the y-coordinates for high band
 * @param length      length of the input and output vectors
 */
void BMCrossover_tfMagVectors(BMCrossover *This,
                                  const float* frequencies,
                                  float* magLow,
                                  float* magHigh,
                                  size_t length);




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
void BMCrossover3way_init(BMCrossover3way *This,
                          float cutoff1,
                          float cutoff2,
                          float sampleRate,
                          bool fourthOrder,
                          bool stereo);


/*!
 *BMCrossover3way_free
 */
void BMCrossover3way_free(BMCrossover3way *This);



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
 *BMCrossover3way_tfMagVectors
 *
 * @abstract get points to plot a graph of each of the three bands
 *
 * @param This        pointer to an initialized struct
 * @param frequencies input vector containing the x-axis coordinates of the plot
 * @param magLow      output vector containing the y-coordinates for low band
 * @param magMid      output vector containing the y-coordinates for mid band
 * @param magHigh     output vector containing the y-coordinates for high band
 * @param length      length of the input and output vectors
 */
void BMCrossover3way_tfMagVectors(BMCrossover3way *This,
                                  const float* frequencies,
                                  float* magLow,
                                  float* magMid,
                                  float* magHigh,
                                  size_t length);




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
void BMCrossover4way_init(BMCrossover4way *This,
                          float cutoff1,
                          float cutoff2,
                          float cutoff3,
                          float sampleRate,
                          bool fourthOrder,
                          bool stereo);



/*!
 *BMCrossover4way_free
 */
void BMCrossover4way_free(BMCrossover4way *This);



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
 *BMCrossover4way_tfMagVectors
 *
 * @abstract get points to plot a graph of each of the four bands
 *
 * @param This        pointer to an initialized struct
 * @param frequencies input vector containing the x-axis coordinates of the plot
 * @param magBand1    output vector containing the y-coordinates for band 1
 * @param magBand2    output vector containing the y-coordinates for band 2
 * @param magBand3    output vector containing the y-coordinates for band 3
 * @param magBand4    output vector containing the y-coordinates for band 4
 * @param length      length of the input and output vectors
 */
void BMCrossover4way_tfMagVectors(BMCrossover4way *This,
                                  const float* frequencies,
                                  float* magBand1,
                                  float* magBand2,
                                  float* magBand3,
                                  float* magBand4,
                                  size_t length);




/*!
 *BMCrossover4way_setCutoff3
 */
void BMCrossover4way_setCutoff3(BMCrossover4way *This, float fc);




/*!
 *BMCrossover3way_recombine
 */
void BMCrossover3way_recombine(const float* bassL, const float* bassR,
							   const float* midL, const float* midR,
							   const float* trebleL, const float* trebleR,
							   float* outL, float* outR,
							   size_t numSamples);



/*!
 *BMCrossover4way_recombine
 */
void BMCrossover4way_recombine(const float* band1L, const float* band1R,
							   const float* band2L, const float* band2R,
							   const float* band3L, const float* band3R,
							   const float* band4L, const float* band4R,
							   float* outL, 		float* outR,
							   size_t numSamples);




/*!
 *BMCrossover_impulseResponse
 */
void BMCrossover_impulseResponse(BMCrossover *This, float* IRL, float* IRR, size_t numSamples);




/*!
 *BMCrossover3way_impulseResponse
 */
void BMCrossover3way_impulseResponse(BMCrossover3way *This, float* IRL, float* IRR, size_t numSamples);




/*!
 *BMCrossover4way_impulseResponse
 */
void BMCrossover4way_impulseResponse(BMCrossover4way *This, float* IRL, float* IRR, size_t numSamples);




#endif /* BMCrossover_h */

#ifdef __cplusplus
}
#endif

