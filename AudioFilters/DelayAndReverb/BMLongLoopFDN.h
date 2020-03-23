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

/*
 * This controls the size of blocks in the feedback matrix. For example, with
 * 4x4 blocks, each group of 4 delays mixes using a Hadamard transform of size 4
 * and feedback goes into another block of 4 delays. We define the
 * Hadamard Transform of size one to be the identiy so with 1x1 blocks each
 * each delay simply routes its feedback into one other delay without mixing.
 */
enum BMLLFDNMixingMatrixBlockSize {BMMM_1x1, BMMM_2x2, BMMM_4x4, BMMM_8x8};

/*
 * The simplest block arrangement is block diagonal where each set of n delays
 * mixes using the Hadamard transform and feeds back to itself.
 *
 * channelToChannelCirculant means that the signal goes through all blocks with
 * outputs directed towards the left channel before connecting to the set of
 * blocks with right channel outputs.
 *
 * singleChannelHalfCirculant means that the blocks in one channel only feed
 * back to other blocks in that same channel.
 *
 * channelAlternatingCirculant means that after each block the feedback jumps to
 * a block from the other channel.
 */
enum BMLLFDNMixingMatrixBlockArrangement {BMMM_channelToChannelCirculant_, BMMM_singleChannelHalfCirculant, BMMM_blockDiagonal, BMMM_channelAlternatingCirculant};


typedef struct BMLongLoopFDN{
	TPCircularBuffer *delays;
	float **readPointers, **writePointers;
	float *feedbackCoefficients, *delayTimes, *inputBufferL, *inputBufferR;
	float inputAttenuation, matrixAttenuation, inverseMatrixAttenuation;
	size_t numDelays, minDelaySamples;
	enum BMLLFDNMixingMatrixBlockSize blockSize;
	enum BMLLFDNMixingMatrixBlockArrangement blockArrangement;
	bool hasZeroTaps;
	bool *tapSigns;
} BMLongLoopFDN;



/*!
 *BMLongLoopFDN_init
 *
 * @param This pointer to an initialised struct
 * @param numDelays number of delays in the FDN. Must be an even number
 * @param minDelaySeconds the shortest delay time
 * @param maxDelaySeconds longest delay time
 * @param hasZeroTaps set this true to get an output tap with zero delay in both channels
 * @param blockSize set 1x1, 2x2, or 4x4 blocks for the mixing matrix
 * @param blockArrangement determines where the feedback signal goes after processing through each block.
 * @param sampleRate audio sample rate
 */
void BMLongLoopFDN_init(BMLongLoopFDN *This,
						size_t numDelays,
						float minDelaySeconds,
						float maxDelaySeconds,
						bool hasZeroTaps,
						enum BMLLFDNMixingMatrixBlockSize blockSize,
						enum BMLLFDNMixingMatrixBlockArrangement blockArrangement,
						float sampleRate);


/*!
 *BMLongLoopFDN_free
 */
void BMLongLoopFDN_free(BMLongLoopFDN *This);


/*!
 *BMLongLoopFDN_setRT60Decay
 */
void BMLongLoopFDN_setRT60Decay(BMLongLoopFDN *This, float timeSeconds);


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

#endif /* BMLongLoopFDN_h */
