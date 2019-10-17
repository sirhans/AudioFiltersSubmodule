//
//  BMQuadratureOscillator.h
//  BMAudioFilters
//
//  A sinusoidal quadrature oscillator suitable for efficient LFO. However, it
//  is not restricted to low frequencies and may also be used for other
//  applications.
//
//  This oscillator works by multiplying a 2x2 matrix by a vector of length 2
//  once per sample. The idea comes from a Mathematica model of an exchange of
//  kinetic and potential energy.  The code for that model appears in the
//  comment at the bottom of this file.
//
//  The output is a quadrature phase wave signal in two arrays of floats. Values
//  are in the range [-1,1].  The first sample of output is 1 in the array r and
//  0 in q. Other initial values can be acheived by directly editing the value
//  of the struct variable rq after initialisation.
//
//
//  USAGE EXAMPLE
//
//  Initialisation
//
//  BMQuadratureOscillator osc;
//  // 2 hz oscillation at sample rate of 48hkz
//  BMQuadratureOscillator_init(osc, 2.0f, 48000.0f);
//
//
//  oscillation
//
//  float* r = malloc(sizeof(float)*100);
//  float* q = malloc(sizeof(float)*100);
//
//  // fill buffers r and q with quadrature wave signal 100 samples long
//  BMQuadratureOscillator_processQuad(osc, r, q, 100);
//
//  Created by Hans on 31/10/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMQuadratureOscillator_h
#define BMQuadratureOscillator_h

//#include "BM2x2Matrix.h"
#include <stddef.h>
#include <stdbool.h>
#include <simd/simd.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef struct BMQuadratureOscillator {
	simd_float2x2 m;
	simd_float2 rq;
	float sampleRate, modRateHz;
} BMQuadratureOscillator;




/*!
 *BMQuadratureOscillator_init
 *
 * @abstract Initialize the rotation speed and set initial values (no memory is allocated for this)
 *
 * @param This       pointer to an oscillator struct
 * @param fHz        frequency in Hz
 * @param sampleRate sample rate in Hz
 */
void BMQuadratureOscillator_init(BMQuadratureOscillator* This,
								 float fHz,
								 float sampleRate);




/*!
 *BMQuadratureOscillator_setFrequency
 */
void BMQuadratureOscillator_setFrequency(BMQuadratureOscillator *This, float fHz);





/*!
 *BMQuadratureOscillator_process
 *
 * @abstract Generate numSamples of oscillation in quadrature phase output into the arrays q and r.  Continues smoothly from the previous function call.
 *
 * @param r          an array for output in phase
 * @param q          an array for output in quadrature with r
 * @param numSamples length of q and r
 *
 */
void BMQuadratureOscillator_process(BMQuadratureOscillator* This,
									float* r,
									float* q,
									size_t numSamples);




/*!
 *BMQuadratureOscillator_volumeEnvelope4
 *
 * @abstract applies volume envelopes directly to four channels of audio.
 *
 * @param buffersL points to an array of four buffers, each with length numSamples, for input AND output in place
 * @param buffersR points to an array of four buffers, each with length numSamples, for input AND output in place
 * @param zeros array of 4 bools, set true for channels that are at zero gain when the function call returns
 * @param numSamples length of arrays
 *
 */
void BMQuadratureOscillator_volumeEnvelope4Stereo(BMQuadratureOscillator* This,
												  float** buffersL,
												  float** buffersR,
												  bool* zeros,
												  size_t numSamples);




void BMQuadratureOscillator_initMatrix(simd_float2x2* m,
float frequency,
                                       float sampleRate);

#ifdef __cplusplus
}
#endif

#endif /* BMQuadratureOscillator_h */


