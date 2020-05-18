//
//  BMMultiTapDelay.h
//  ETSax
//
//  Created by Duc Anh on 5/29/17.
//  Copyright © 2017 BlueMangoo. All rights reserved.
//

#ifndef BMMultiTapDelay_h
#define BMMultiTapDelay_h

#include <stdio.h>
#include "TPCircularBuffer.h"


/*
 *  Settings struct for BMMultiTapDelay class
 */
typedef struct{
    size_t** delayTimes;
    size_t** indices;
    float** gains;
    bool isStereo;
} BMMultiTapDelaySetting;


/*
 *  Main struct for the class
 */
typedef struct{
    BMMultiTapDelaySetting setting;
    TPCircularBuffer *buffer;
    TPCircularBuffer *multiBuffer;
    size_t numInput;
    
    float** tempBuffer;
    float** input;
    float** output;
    float** lastTapOutput;
    float* zeroArray;
    
    size_t numberChannel;
    size_t numTaps;
    size_t maxTaps;
    size_t maxDelayTime;
    
    size_t** tempIndices;
    float** tempGains;
    bool _needUpdateIndices;
    bool _needUpdateGain;
} BMMultiTapDelay;


/*!
 *BMMultiTapDelay_Init
 *
 * @abstract initialize the delay struct
 *
 * @param This         pointer to an uninitialized delay struct
 * @param isStereo     set false for mono
 * @param delayTimesL  delay times for left channel (and mono) in samples
 * @param delayTimesR  delay times for right channel, ignored for mono
 * @param maxDelayTime the delay taps can be changed without re-initialization if they don't exceed this number
 * @param gainL        gains for taps in Left channel, same length as delayTimesL
 * @param gainR        gains for taps in Right channel, same length as delayTimesL
 * @param numTaps      the number of delay taps (must be same for L and R channels)
 * @param maxTaps      the taps can be changed as long as we don't exceed maxTaps
 */
void BMMultiTapDelay_Init(BMMultiTapDelay *This,
                          bool isStereo,
                          size_t* delayTimesL, size_t* delayTimesR,
                          size_t maxDelayTime,
                          float* gainL, float* gainR,
                          size_t numTaps, size_t maxTaps);


/*!
 *BMMultiTapDelay_initBypass
 */
void BMMultiTapDelay_initBypass(BMMultiTapDelay *This,
								bool isStereo,
								size_t maxDelayLength,
								size_t maxTapsPerChannel);

void BMMultiTapDelay_initBypassMultiChannel(BMMultiTapDelay *This,
                                            bool isStereo,
                                            size_t maxDelayLength,
                                            size_t numInput,
                                            size_t maxTapsPerChannel);

/*!
 *BMMultiTapDelay_processBufferStereo
 *
 * @notes works in place
 */
void BMMultiTapDelay_processBufferStereo(BMMultiTapDelay* delay,
                                         const float* inputL, const float* inputR,
                                         float* outputL, float* outputR,
                                         size_t frames);

void BMMultiTapDelay_processMultiChannelInput(BMMultiTapDelay* delay,
                                              float** inputL,
                                              float** inputR,
                                              float* outputL,
                                              float* outputR,
                                              size_t numInput,
                                              size_t numSamples);

void BMMultiTapDelay_processStereoWithFinalOutput(BMMultiTapDelay* delay,
                                                  const float* inputL, const float* inputR,
                                                  float* outputL, float* outputR,
                                                  float* lastTapL, float* lastTapR,
                                                  size_t numSamples);


/*
 * works in place
 */
void BMMultiTapDelay_ProcessBufferMono(BMMultiTapDelay* delay,
                                       float* input, float* output,
                                       size_t frames);




/*
 * Process a single sample of input and output
 */
void BMMultiTapDelay_ProcessOneSampleStereo(BMMultiTapDelay* delay,
                                            float* inputL, float* inputR,
                                            float* outputL, float* outputR);



/*
 *  Free memory of the struct at *This
 */
void BMMultiTapDelay_free(BMMultiTapDelay *This);


/*
 *  Restore the circularBuffer to "Empty State"
 */
void BMMultiTapDelay_clearBuffers(BMMultiTapDelay* delay);

void BMMultiTapDelay_setDelayTimes(BMMultiTapDelay *This,
                                   size_t* delayTimesL, size_t* delayTimesR);
void BMMultiTapDelay_setDelayTimeNumTap(BMMultiTapDelay *This,
                                   size_t* delayTimesL, size_t* delayTimesR,size_t numTaps);
void BMMultiTapDelay_setGains(BMMultiTapDelay *This,
                              float* gainL, float* gainR);

void BMMultiTapDelay_PerformUpdateIndices(BMMultiTapDelay *This);
void BMMultiTapDelay_PerformUpdateGains(BMMultiTapDelay *This);
        

void BMMultiTapDelay_impulseResponse(BMMultiTapDelay *This);

#endif /* BMMultiTapDelay_h */
