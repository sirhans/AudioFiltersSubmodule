//
//  BMReverb.h
//  CReverb
//
//  Created by Hans on 7/2/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMReverb_h
#define BMReverb_h

#include <stdio.h>
#include "BMFirstOrderArray.h"
#include "BMMultiLevelBiquad.h"
#include "BMWetDryMixer.h"
#include "BMStereoWidener.h"
#include <math.h>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#include <simd/simd.h>
#else
#include "BMCrossPlatformVDSP.h"
#endif

// default settings
#define BMREVERB_WETMIX 0.15 // dryMix = sqrt(1 - wetMix^2)
#define BMREVERB_NUMDELAYUNITS 4 // each unit contains 4 delays
#define BMREVERB_NUMDELAYS 4 * BMREVERB_NUMDELAYUNITS
#define BMREVERB_PREDELAY 0.007 // (in seconds)
#define BMREVERB_ROOMSIZE 0.100 // (seconds of sound travel time)
#define BMREVERB_DEFAULTSAMPLERATE 44100.0
#define BMREVERB_HIGHSHELFFC 1400.0 // above this frequency decay is faster
#define BMREVERB_HFDECAYMULTIPLIER 6.0 // HF decay is this many times faster
#define BMREVERB_LOWSHELFFC 500.0 // above this frequency decay is faster
#define BMREVERB_LFDECAYMULTIPLIER 6.0 // LF decay is this many times faster
#define BMREVERB_RT60 1.2 // overall decay time
#define BMREVERB_HIGHPASS_FC 30.0 // highpass filter on wet out
#define BMREVERB_MID_SCOOP_FC 1400.0f // mid scoop on wet signal
#define BMREVERB_MID_SCOOP_GAIN -6.0f // mid scoop gain
#define BMREVERB_MID_SCOOP_Q 0.6f // mid scoop Q
#define BMREVERB_LOWPASS_FC 1200.0 // lowpass filter on wet out
#define BMREVERB_STEREOWIDTH 1.0 // mixing betwee L and R wet outputs
#define BMREVERB_SLOWDECAYRT60 8.0 // RT60 time when hold pedal is down

#ifdef __cplusplus
extern "C" {
#endif

// the CReverb struct
typedef struct BMReverb {
	simd_float4 feedbackBuffers[BMREVERB_NUMDELAYUNITS];
	simd_float4 decayGainAttenuation [BMREVERB_NUMDELAYUNITS];
	simd_float4 delayOutputSigns[BMREVERB_NUMDELAYUNITS];
	float *delayLines, *leftOutputTemp, *dryL, *dryR;
	//float delayTimes[BMREVERB_NUMDELAYS];
	size_t bufferLengths[BMREVERB_NUMDELAYS];
	size_t bufferStartIndices[BMREVERB_NUMDELAYS];
	size_t bufferEndIndices[BMREVERB_NUMDELAYS];
	size_t rwIndices[BMREVERB_NUMDELAYS];
	float inputAttenuation, minDelay_seconds, maxDelay_seconds, sampleRate, wetGain, dryGain, straightStereoMix, crossStereoMix, hfDecayMultiplier, lfDecayMultiplier, highShelfFC, lowShelfFC, rt60, slowDecayRT60, highpassFC, lowpassFC;
	size_t delayUnits, newNumDelayUnits, numDelays, halfNumDelays, fourthNumDelays, samplesTillNextWrap, totalSamples;
	bool settingsQueuedForUpdate, preDelayUpdate;
	BMFirstOrderArray4x4 HSFArray;
	BMFirstOrderArray4x4 LSFArray;
	BMMultiLevelBiquad mainFilter;
	BMWetDryMixer wetDryMixer;
	BMStereoWidener stereoWidth;
} BMReverb;



/*
 * publicly usable functions
 */


// initialisation and cleanup
void BMReverbInit(struct BMReverb *This, float sampleRate);
void BMReverbFree(struct BMReverb *This);

// main audio processing function
void BMReverbProcessBuffer(struct BMReverb *This, const float* inputL, const float* inputR, float* outputL, float* outputR, size_t numSamples);


/*
 * settings that can be safely changed during reverb operation
 */

/*!
 *BMReverbSetWetGain
 *
 * @param This pointer to a wet-dry mixer struct (doesn't have to be initialized)
 * @param wetMix 0=all dry; 1=all wet
 */
void BMReverbSetWetMix(struct BMReverb *This, float wetMix);


/*!
 *BMReverbSetStereoWidth
 *
 * @param This pointer to an initialised struct
 * @param width 0=mono, 1=fullstereo, 2=L and R channels are opposite signs and cancel out when mixed to mono.
 */
void BMReverbSetStereoWidth(struct BMReverb *This, float width);


// setting multiplier=n means that high frequencies decay n times faster
// than the rest of the signal. The filter that does the HF decay is
// gently sloped, so this setting will have some effect on the decay
// time of frequencies below the HFDecayFC.
// multiplier must be >= 1.0;
void BMReverbSetHFDecayMultiplier(struct BMReverb *This, float multiplier);
//
void BMReverbSetLFDecayMultiplier(struct BMReverb *This, float multiplier);


// sets the cutoff frequency of the filters that do high frequency decay
void BMReverbSetHFDecayFC(struct BMReverb *This, float fc);

// sets the cutoff frequency of the filters that do low frequency decay
void BMReverbSetLFDecayFC(struct BMReverb *This, float fc);


// RT60 measures the time it takes for the reverb to decay to -60db
// relative to its original level. Examples:
// 0.2 = speaker cabinet simulator
// 0.5 = closet sized room
// 0.9 = bedroom
// 1.3 = moderately sized auditorium
// 2.3 = huge auditorium
// 10  = world's longest echoing cathedral
void BMReverbSetRT60DecayTime(struct BMReverb *This, float rt60);


// sets the sustain mode on=true or off=false.  This can be used to
// simulate a sustain pedal effect by temporarily switching the reverb
// to a long RT60 decay time.
void BMReverbSetSlowDecayState(struct BMReverb *This, bool slowDecay);



/*
 * Settings for which changes will queue until the end of the next buffer.
 * Call these functions any time, but changes won't take effect until
 * it's safe to apply them.
 */

// A delay unit is a set of four delay lines.  We are using a sparse
// mixing matrix that works in groups of four.  So, to get an FDN with
// 20 delays, set delayUnits = 5.
//
// Larger numbers of delay units consume more processing power and produce
// denser, smoother echoes.  However, the smaller networks have a more
// dynamic sound that changes over time so using more delayUnits does not
// necessarily give you better sound.
void BMReverbSetNumDelayUnits(struct BMReverb *This, size_t delayUnits);



// roomSize controls the length of the longest delay in the network.
// Changing this setting may not actually make you feel like you are
// hearing a larger room, since preDelay and RT60 also affect the
// perception of room size.  Setting longer values of roomSize make
// a sparser reverb, which may sound grainy, especially with drums.
// When using very long RT60 times, greater than 5 seconds, we recommend
// using a large setting for roomSize to get a reverb sound that changes
// over time as it echoes.
//
// The shortest delay time in the network is the predelay.  Between the
// moment a signal enters the reverb and when the predelay time is
// no wet reverb output is generated. Long pre-delay gives the feeling
// of a very wide, tall room. Examples:
//
// preDelay = 0.0001; // speaker cabinet
// preDelay = 0.001;  // tiny room
// preDelay = 0.007;  // mid sized room
// preDelay = 0.015;  // big auditorium
//
// setting a long pre-delay makes the sound muddled.
//
// preDelay << roomSize
void BMReverbSetRoomSize(struct BMReverb *This,
						 float preDelay_seconds,
						 float roomSize_seconds);



// sets the sample rate of the input audio.  Reverb will work at any
// sample rate you set, even if it's not correct, but setting this
// correctly will ensure that delay times and filter frequencies are
// calculated correctly.
void BMReverbSetSampleRate(struct BMReverb *This, float sampleRate);


// sets the cutoff frequency of a second order butterworth highpass
// filter on the wet signal.  (that's 12db cutoff slope).  This does
// not affect the dry signal at all.
void BMReverbSetHighPassFC(struct BMReverb *This, float fc);


// sets the cutoff frequency of a second order butterworth lowpass
// filter on the wet signal.  (that's 12db cutoff slope).  This does
// not affect the dry signal at all.
void BMReverbSetLowPassFC(struct BMReverb *This, float fc);

/*!
 *BMReverbDelayGainFromRT60
 *
 * calculates the gain coefficient for a feedback delay
 *
 * @param rt60 time in seconds for the signal to decay from 0 dB to -60 dB
 * @param delayTime length in seconds of the delay
 */
static double BMReverbDelayGainFromRT60(double rt60, double delayTime){
	return pow(10.0, (-3.0 * delayTime) / rt60 );
}


/*!
*BMReverbDelayGainFromRT30
*
* calculates the gain coefficient for a feedback delay
*
* @param rt30 time in seconds for the signal to decay from 0 dB to -30 dB
* @param delayTime length in seconds of the delay
*/
static double BMReverbDelayGainFromRT30(double rt30, double delayTime){
	return pow(10.0, (-2.0 * delayTime) / rt30 );
}


/*!
 *BMReverbRandomsInRange
 *
 * @param min minimum output value
 * @param max maximum output value
 * @param randomOutput output array
 * @param length length of output array
 *
 * @abstract generate length random unsigned integers between min and max, ensuring that the average is (max-min)/2 and that there will be values equal to min and max. Also, there will be no duplicates.
 */
void BMReverbRandomsInRange(size_t min, size_t max, size_t *randomOutput, size_t length);


#ifdef __cplusplus
}
#endif

#endif /* BMReverb_h */
