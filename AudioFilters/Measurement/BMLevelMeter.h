//
//  BMBMLevelMeter.h
//
//  This is a level meter that rises immediately but falls slowly.
//  It supports either peak level metering or RMS power level metering, but not
//  both at the same time. If you need both peak and RMS levels, please run two
//  instances of this class in parallel.
//
//  Created by Hans Anderson on 5/17/19.
//  This file is free for use without restrictions of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMLevelMeter_h
#define BMLevelMeter_h
    
    
#define BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS 3

#include <stdio.h>
#include "BMEnvelopeFollower.h"

    typedef struct BMLevelMeter {
        float sampleRate;
        size_t expectedBufferLength;
        
        // filters for smoothing the peak level
        BMReleaseFilter fastReleaseL_peak, fastReleaseR_peak;
        BMReleaseFilter slowReleaseL_peak [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
        BMReleaseFilter slowReleaseR_peak [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
        
        // filters for smoothing the RMS power level
        BMReleaseFilter fastReleaseL_RMS, fastReleaseR_RMS;
        BMReleaseFilter slowReleaseL_RMS [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
        BMReleaseFilter slowReleaseR_RMS [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
    } BMLevelMeter;
    
    
    
    /*!
     * BMLevelMeter_init
     */
    void BMLevelMeter_init(BMLevelMeter* This, float sampleRate);
    
    

    
    
    /*!
     * BMLevelMeter_peakLevelStereo
     *
     * @abstract scan a stereo pair of audio buffers to get the peak value. Then filter the time series of peaks so that we get a value that goes up instantaneously and down more slowly. Filter a second time to get a value that goes down even more slowly. The fastPeak values are useful for a level meter. The slowPeak values are useful for an indicator that shows what level the peak has reached in recent history.
     *
     * @param inputL an array of audio samples with length = bufferLength
     * @param inputR an array of audio samples with length = bufferLength
     * @param fastPeakL pointer to a scalar decibel value that tracks the peak quickly
     * @param fastPeakR pointer to a scalar decibel value that tracks the peak quickly
     * @param slowPeakL pointer to a scalar decibel that tracks the peak more slowly
     * @param slowPeakR pointer to a scalar decibel that tracks the peak more slowly
     * @param bufferLength length of inputL and inputR. This should be the length of the actual audio buffer. If the buffer length changes from one function call to the next, this class will automatically adjust the filter frequencies to keep the release time constant.
     */
    void BMLevelMeter_peakLevelStereo(BMLevelMeter* This,
                                         const float* inputL, const float* inputR,
                                         float* fastPeakL, float* fastPeakR,
                                         float* slowPeakL, float* slowPeakR,
                                         size_t bufferLength);
    
    
    
    /*!
     * BMLevelMeter_RMSPowerStereo
     *
     * @abstract find the RMS power level in a stereo pair of buffers. Then filter the time series of peaks so that we get a value that goes up instantaneously and down more slowly. Filter a second time to get a value that goes down even more slowly. The fastRelease values are useful for a level meter. The slowRelease values are useful for an indicator that shows the highest level the RMS power has reached in recent history.
     *
     * @param inputL an array of audio samples with length = bufferLength
     * @param inputR an array of audio samples with length = bufferLength
     * @param fastReleaseL pointer to a scalar decibel value that tracks the peak quickly
     * @param fastReleaseR pointer to a scalar decibel value that tracks the peak quickly
     * @param slowReleaseL pointer to a scalar decibel that tracks the peak more slowly
     * @param slowReleaseR pointer to a scalar decibel that tracks the peak more slowly
     * @param bufferLength length of inputL and inputR. This should be the length of the actual audio buffer. If the buffer length changes from one function call to the next, this class will automatically adjust the filter frequencies to keep the release time constant.
     */
    void BMLevelMeter_RMSPowerStereo(BMLevelMeter* This,
                                      const float* inputL, const float* inputR,
                                      float* fastReleaseL, float* fastReleaseR,
                                      float* slowReleaseL, float* slowReleaseR,
                                      size_t bufferLength);


/*!
   * BMLevelMeter_peakLevelStereo
   *
   * @abstract scan a stereo pair of audio buffers to get the peak value. Then filter the time series of peaks so that we get a value that goes up instantaneously and down more slowly. Filter a second time to get a value that goes down even more slowly. The fastPeak values are useful for a level meter. The slowPeak values are useful for an indicator that shows what level the peak has reached in recent history.
   *
   * @param input an array of audio samples with length = bufferLength
   * @param fastPeak pointer to a scalar decibel value that tracks the peak quickly
   * @param slowPeak pointer to a scalar decibel that tracks the peak more slowly
   * @param bufferLength length of inputL and inputR. This should be the length of the actual audio buffer. If the buffer length changes from one function call to the next, this class will automatically adjust the filter frequencies to keep the release time constant.
   */
  void BMLevelMeter_peakLevelMono(BMLevelMeter* This,
                                  const float* input,
                                  float* fastPeak,
                                  float* slowPeak,
                                  size_t bufferLength);
  

/*!
 * BMLevelMeter_RMSPowerMono
 *
 * @abstract find the RMS power level in a stereo pair of buffers. Then filter the time series of peaks so that we get a value that goes up instantaneously and down more slowly. Filter a second time to get a value that goes down even more slowly. The fastRelease values are useful for a level meter. The slowRelease values are useful for an indicator that shows the highest level the RMS power has reached in recent history.
 *
 * @param input an array of audio samples with length = bufferLength
 * @param fastRelease pointer to a scalar decibel value that tracks the peak quickly
 * @param slowRelease pointer to a scalar decibel that tracks the peak more slowly
 * @param bufferLength length of inputL and inputR. This should be the length of the actual audio buffer. If the buffer length changes from one function call to the next, this class will automatically adjust the filter frequencies to keep the release time constant.
 */
void BMLevelMeter_RMSPowerMono(BMLevelMeter* This,
                                  const float* input,
                                  float* fastRelease,
                                  float* slowRelease,
                                  size_t bufferLength);


  
  /*!
   * BMLevelMeter_RMSPowerStereo
   *
   * @abstract find the RMS power level in a stereo pair of buffers. Then filter the time series of peaks so that we get a value that goes up instantaneously and down more slowly. Filter a second time to get a value that goes down even more slowly. The fastRelease values are useful for a level meter. The slowRelease values are useful for an indicator that shows the highest level the RMS power has reached in recent history.
   *
   * @param inputL an array of audio samples with length = bufferLength
   * @param inputR an array of audio samples with length = bufferLength
   * @param fastReleaseL pointer to a scalar decibel value that tracks the peak quickly
   * @param fastReleaseR pointer to a scalar decibel value that tracks the peak quickly
   * @param slowReleaseL pointer to a scalar decibel that tracks the peak more slowly
   * @param slowReleaseR pointer to a scalar decibel that tracks the peak more slowly
   * @param bufferLength length of inputL and inputR. This should be the length of the actual audio buffer. If the buffer length changes from one function call to the next, this class will automatically adjust the filter frequencies to keep the release time constant.
   */
  void BMLevelMeter_RMSPowerStereo(BMLevelMeter* This,
                                    const float* inputL, const float* inputR,
                                    float* fastReleaseL, float* fastReleaseR,
                                    float* slowReleaseL, float* slowReleaseR,
                                    size_t bufferLength);

    
#endif /* BMPeakMetre_h */


#ifdef __cplusplus
}
#endif
