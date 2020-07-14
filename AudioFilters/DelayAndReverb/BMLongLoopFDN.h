//
//  BMLongLoopFDN.h
//  AUCloudReverb
//
//  Created by hans anderson on 3/20/20.
//  This file is in the public domain.
//

#ifndef BMLongLoopFDN_h
#define BMLongLoopFDN_h

#include <stdio.h>
#include "BMSimpleDelay.h"
#include "TPCircularBuffer.h"
#include "BMShortSimpleDelay.h"
#include "BMSmoothGain.h"


typedef struct BMLongLoopFDN{
	TPCircularBuffer *delays;
	float **readPointers, **writePointers, **mixBuffers;
	float *feedbackCoefficients, *delayTimes, *inputBufferL, *inputBufferR;
	float inputAttenuation, matrixAttenuation, inverseMatrixAttenuation;
	bool *tapSigns;
	size_t numDelays, minDelaySamples, blockSize, feedbackShiftByDelay,feedbackShiftByBlock;
    float minDelayS;
    float maxDelayS;
    float decayTimeS;
	bool hasZeroTaps;
    float sampleRate;
    bool needReinit;
    //Decay smooth
    bool smoothDecay;
    BMSmoothGain* smoothGains;
} BMLongLoopFDN;



/*!
 *BMLongLoopFDN_init
 *
 * @param This pointer to an initialised struct
 * @param numDelays number of delays in the FDN. Must be an even number
 * @param minDelaySeconds the shortest delay time
 * @param maxDelaySeconds longest delay time
 * @param hasZeroTaps set this true to get an output tap with zero delay in both channels
 * @param blockSize set n={1,2,4,8} for nxn sized blocks in the mixing matrix
 * @param feedbackShiftByBlock the nth block feeds back to the n+feedbackShift block.
 * @param sampleRate audio sample rate
 */
void BMLongLoopFDN_init(BMLongLoopFDN *This,
						size_t numDelays,
						float minDelaySeconds,
						float maxDelaySeconds,
						bool hasZeroTaps,
						size_t blockSize,
						size_t feedbackShiftByBlock,
                        float decayTimeS,
                        bool smoothDecay,
						float sampleRate);


/*!
 *BMLongLoopFDN_free
 */
void BMLongLoopFDN_free(BMLongLoopFDN *This);


/*!
 *BMLongLoopFDN_setRT60Decay
 */
void BMLongLoopFDN_setRT60Decay(BMLongLoopFDN *This, float timeSeconds);


void BMLongLoopFDN_setMaxDelay(BMLongLoopFDN* This,float maxDelayS);

void BMLongLoopFDN_setRT60DecaySmooth(BMLongLoopFDN *This, float timeSeconds,bool isInstant);



/*!
 *BMLongLoopFDN_process
 *
 * That this process function is 100% wet. You must handle wet/dry mix
 * outside.
 */
void BMLongLoopFDN_process(BMLongLoopFDN *This,
						   const float* inputL, const float* inputR,
						   float *outputL, float *outputR,
						   size_t numSamples);



/*!
 *BMLongLoopFDN_processMultiChannelInput
 *
 * This class takes multi-channel inputs. We do this because when the input
 * comes from a diffuser it typically has some spectral variance that will
 * be multiplied with the spectral variance of this FDN. To mitigate that effect
 * and keep the spectrum flat, you can send multichannel input where each
 * channel comes from a different diffusion source with a different spectrum.
 * When several different spectra meet inside the FDN the resulting mixture will
 * be flatter than what we would get if all FDN inputs came from a single
 * diffusor.
 * 
 * That this process function is 100% wet. You must handle wet/dry mix
 * outside.
 *
 * @param This p
 * @param inputL 2 dimensional array of size [numInputChannels, numSamples] WARNING: DATA IN THIS ARRAY WILL BE MODIFIED
 * @param inputR 2 dimensional array of size [numInputChannels, numSamples] WARNING: DATA IN THIS ARRAY WILL BE MODIFIED
 * @param numInputChannels number of input channels in each L, R input
 * @param outputL 1 dimensional array of length numSamples
 * @param outputR 1 dimensional array of length numSamples
 * @param numSamples length of input arrays
 */
void BMLongLoopFDN_processMultiChannelInput(BMLongLoopFDN *This,
											float** inputL, float** inputR,
											size_t numInputChannels,
											float *outputL, float *outputR,
											size_t numSamples);

#endif /* BMLongLoopFDN_h */
