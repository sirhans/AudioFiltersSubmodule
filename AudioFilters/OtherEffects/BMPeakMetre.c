//
//  BMPeakMetre.c
//  
//
//  Created by Hans Anderson on 5/17/19.
//  This file is free for use without restrictions of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMPeakMetre.h"
#include <Accelerate/Accelerate.h>

#define BM_PEAK_METRE_DEFAULT_BUFFER_LENGTH 256;
#define BM_PEAK_METRE_NUM_SLOW_RELEASE_FILTERS 3;


void BMPeakMetre_setBufferSize(BMPeakMetre* This, size_t bufferSize){
    float buffersPerSecond = This->sampleRate / bufferSize;
    BMReleaseFilter_setSpeed(???)
}


void BMPeakMetre(BMPeakMetre* This, float fastReleaseTime, float slowReleaseTime, float sampleRate){
    float buffersPerSecond = sampleRate / (float)BM_PEAK_METRE_DEFAULT_BUFFER_LENGTH;
    BMReleaseFilter_init(&This->fastRelease, fastReleaseTime, buffersPerSecond);
    for(size_t i=0; i<BM_PEAK_METRE_NUM_SLOW_RELEASE_FILTERS; i++){
        BMReleaseFilter_init(&This->slowRelease[i], slowReleaseTime, buffersPerSecond);
    }
}


void BMPeakMetre_free(BMPeakMetre* This);


void BMReleaseFilter_init(float fastReleaseTime, float slowReleaseTime, float sampleRate);


/*!
 *BMPeakMetre_processBufferStereo
 *
 * @abstract scan a stereo pair of audio buffers to get the peak value. Then filter the time series of peaks so that we get a value that goes up instantaneously and down more slowly. Filter a second time to get a value that goes down even more slowly. The fastPeak values are useful for a level metre. The slowPeak values are useful for an indicator that shows what level the peak has reached in recent history.
 *
 * @param inputL an array of audio samples with length = bufferLength
 * @param inputR an array of audio samples with length = bufferLength
 * @param fastPeakL pointer to a scalar value that tracks the peak quickly
 * @param fastPeakR pointer to a scalar value that tracks the peak quickly
 * @param slowPeakL pointer to a scalar that tracks the peak more slowly
 * @param slowPeakR pointer to a scalar that tracks the peak more slowly
 * @param bufferLength length of inputL and inputR. This should be the length of the actual audio buffer. If the buffer length changes from one function call to the next, this class will automatically adjust the filter frequencies to keep the release time constant.
 */
void BMPeakMetre_processBufferStereo(const float* inputL, const float* inputR,
                                     float* fastPeakL, float* fastPeakR,
                                     float* slowPeakL, float* slowPeakR,
                                     size_t bufferLength){
    float maxMagnitudeL, maxMagnitudeR;
    
    // get the max magnitude in each channel
    vDSP_maxmgv(inputL, 1, &maxMagnitudeL, bufferLength);
    vDSP_maxmgv(inputR, 1, &maxMagnitudeR, bufferLength);
    
    // release filter the maxMagnitudes to get the peak with fast release time
    BMReleaseFilter_
    
    // release filter again to get the peak with slow release time
}


#ifdef __cplusplus
}
#endif
