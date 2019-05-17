//
//  BMBMPeakMeter.h
//  
//
//  Created by Hans Anderson on 5/17/19.
//  This file is free for use without restrictions of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMPeakMeter_h
#define BMPeakMeter_h
    
    
#define BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS 3

#include <stdio.h>
#include "BMEnvelopeFollower.h"

    typedef struct BMPeakMeter {
        float sampleRate;
        BMReleaseFilter fastReleaseL, fastReleaseR;
        BMReleaseFilter slowReleaseL [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
        BMReleaseFilter slowReleaseR [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
    } BMPeakMeter;
    
    
    /*!
     * BMPeakMeter_init
     */
    void BMPeakMeter_init(BMPeakMeter* This, float fastReleaseTime, float slowReleaseTime, float sampleRate);
    
    
    /*!
     * BMPeakMeter_setBufferSize
     *
     * @abstract We are using digital filters to slow down the release of the peak value. Since we only update the peak value once with every buffer we process, the sample rate of the smoothing filter is equal to audioSampleRate/bufferSize. Therefore, if the buffer size changes, the sample rate of the filters should also update. This function sets the appropriate filter cutoff frequencies for a given buffer size. The audio sample rate is assumed to be constant.
     */
    void BMPeakMeter_setBufferSize(BMPeakMeter* This, size_t bufferSize);
    
    
    /*!
     * BMPeakmeter_processBufferStereo
     *
     * @abstract scan a stereo pair of audio buffers to get the peak value. Then filter the time series of peaks so that we get a value that goes up instantaneously and down more slowly. Filter a second time to get a value that goes down even more slowly. The fastPeak values are useful for a level meter. The slowPeak values are useful for an indicator that shows what level the peak has reached in recent history.
     *
     * @param inputL an array of audio samples with length = bufferLength
     * @param inputR an array of audio samples with length = bufferLength
     * @param fastPeakL pointer to a scalar value that tracks the peak quickly
     * @param fastPeakR pointer to a scalar value that tracks the peak quickly
     * @param slowPeakL pointer to a scalar that tracks the peak more slowly
     * @param slowPeakR pointer to a scalar that tracks the peak more slowly
     * @param bufferLength length of inputL and inputR. This should be the length of the actual audio buffer. If the buffer length changes from one function call to the next, this class will automatically adjust the filter frequencies to keep the release time constant.
     */
    void BMPeakmeter_processBufferStereo(BMPeakMeter* This,
                                         const float* inputL, const float* inputR,
                                         float* fastPeakL, float* fastPeakR,
                                         float* slowPeakL, float* slowPeakR,
                                         size_t bufferLength);

#endif /* BMPeakMetre_h */


#ifdef __cplusplus
}
#endif
