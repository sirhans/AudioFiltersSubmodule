//
//  BMBlip.h
//  AudioFiltersXcodeProject
//
//  This class produces a Band Limited ImPulse (BLIP).
//  Use a collection of these to build an antialiased oscillator with adjustable lowpass filter frequency in the analog (analytical) domain. This class outputs a single lowpass filtered impulse that decays to zero over time. It outputs one single impulse each time you call the reset function. 
//
//  Created by Hans on 26/3/21.
//  We hereby release this file into the public domain without restrictions
//

#ifndef BMBlip_h
#define BMBlip_h

#include <stdio.h>
#include <stdbool.h>

typedef struct BMBlipFilterConfig {
    float *exp;
    float negNOverP, pHatNegN, n, p;
    size_t n_i;
} BMBlipFilterConfig;

typedef struct BMBlip{
    float lastOutput, sampleRate, t0, dt;
    float *b1, *b2;
    size_t bufferLength;
    bool filterConfNeedsFlip;
    BMBlipFilterConfig filterConf1, filterConf2;
    BMBlipFilterConfig *filterConf, *filterConfBack;
} BMBlip;


/*!
 *BMBlip_init
 */
void BMBlip_init(BMBlip *This, size_t filterOrder, float lowpassFc, float sampleRate);

/*!
 *BMBlip_free
 */
void BMBlip_free(BMBlip *This);

/*!
 *BMBlip_update
 */
void BMBlip_update(BMBlip *This, float lowpassFc, size_t filterOrder);


/*!
 *BMBlip_restart
 *
 *  Each time you call this, the BLIP resets to the beginning of the impulse response.
 *
 *  @param This pointer to an initialised struct
 *  @param offset in [0,1). The fractional sample offset at which the impulse should begin, relative to the start of the next buffer we process. When the offset is zero the impulse starts at the first sample in the buffer. Values larger than zero delay the offset. The value must be strictly less than one.
 */
void BMBlip_restart(BMBlip *This, float offset);


/*!
 *BMBlip_process
 *
 * @param This pointer to an initialised struct
 * @param output array of length length. output is summed into here so it should be initialised before calling this function for the first time.
 * @param length length of output
 */
void BMBlip_process(BMBlip *This, float *output, size_t length);


#endif /* BMBlip_h */
