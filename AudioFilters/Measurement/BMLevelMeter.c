//
//  BMLevelMeter.c
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
    
#include "BMLevelMeter.h"
#include "BMRMSPower.h"
#include "Constants.h"
#include <Accelerate/Accelerate.h>
    
#define BM_LEVEL_METER_DEFAULT_BUFFER_LENGTH 256
#define BM_LEVEL_METER_DEFAULT_FAST_RELEASE_TIME 0.25
#define BM_LEVEL_METER_DEFAULT_SLOW_RELEASE_TIME 5.0
    
    
    
    /*!
     *BMLevelMeter_setBufferSize
     *
     * @abstract We are using digital filters to slow down the release of the peak value. Since we only update the peak value once with every buffer we process, the sample rate of the smoothing filter is equal to audioSampleRate/bufferSize. Therefore, if the buffer size changes, the sample rate of the filters should also update. This function sets the appropriate filter cutoff frequencies for a given buffer size. The audio sample rate is assumed to be constant.
     */
    void BMLevelMeter_setBufferSize(BMLevelMeter* This, size_t bufferSize){
        float buffersPerSecond = This->sampleRate / (float)bufferSize;
        BMReleaseFilter_updateSampleRate(&This->fastReleaseL_peak, buffersPerSecond);
        BMReleaseFilter_updateSampleRate(&This->fastReleaseR_peak, buffersPerSecond);
        BMReleaseFilter_updateSampleRate(&This->fastReleaseL_RMS, buffersPerSecond);
        BMReleaseFilter_updateSampleRate(&This->fastReleaseR_RMS, buffersPerSecond);
        for(size_t i=0; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++){
            BMReleaseFilter_updateSampleRate(&This->slowReleaseL_peak[i], buffersPerSecond);
            BMReleaseFilter_updateSampleRate(&This->slowReleaseR_peak[i], buffersPerSecond);
            BMReleaseFilter_updateSampleRate(&This->slowReleaseL_RMS[i], buffersPerSecond);
            BMReleaseFilter_updateSampleRate(&This->slowReleaseR_RMS[i], buffersPerSecond);
        }
    }
    
    
    
    
    void BMLevelMeter_init(BMLevelMeter* This, float sampleRate){
        This->expectedBufferLength = BM_LEVEL_METER_DEFAULT_BUFFER_LENGTH;
        
        This->sampleRate = sampleRate;
        
        float buffersPerSecond = sampleRate / (float)BM_LEVEL_METER_DEFAULT_BUFFER_LENGTH;
        
        // init the fast release filters
        float fastReleaseFc = ARTimeToCutoffFrequency(BM_LEVEL_METER_DEFAULT_FAST_RELEASE_TIME, 1);
        BMReleaseFilter_init(&This->fastReleaseL_peak, fastReleaseFc, buffersPerSecond);
        BMReleaseFilter_init(&This->fastReleaseR_peak, fastReleaseFc, buffersPerSecond);
        BMReleaseFilter_init(&This->fastReleaseL_RMS, fastReleaseFc, buffersPerSecond);
        BMReleaseFilter_init(&This->fastReleaseR_RMS, fastReleaseFc, buffersPerSecond);
                      
        // init the slow release filters
        float slowReleaseFc = ARTimeToCutoffFrequency(BM_LEVEL_METER_DEFAULT_SLOW_RELEASE_TIME, BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS);
        for(size_t i=0; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++){
            BMReleaseFilter_init(&This->slowReleaseL_peak[i], slowReleaseFc, buffersPerSecond);
            BMReleaseFilter_init(&This->slowReleaseR_peak[i], slowReleaseFc, buffersPerSecond);
            BMReleaseFilter_init(&This->slowReleaseL_RMS[i], slowReleaseFc, buffersPerSecond);
            BMReleaseFilter_init(&This->slowReleaseR_RMS[i], slowReleaseFc, buffersPerSecond);
        }
    }
    
    
    
    
    /*!
     *BMLevelMeter_processStereo
     */
    void BMLevelMeter_processStereo(BMLevelMeter* This,
                                    const float* inputL, const float* inputR,
                                    float* fastPeakL, float* fastPeakR,
                                    float* slowPeakL, float* slowPeakR,
                                    size_t bufferLength,
                                    bool RMS){
        
        // if the buffer size has changed, update the filters to keep the release
        // times correct
        if(bufferLength != This->expectedBufferLength){
            This->expectedBufferLength = bufferLength;
            BMLevelMeter_setBufferSize(This, bufferLength);
        }
        
        float maxMagnitudeL, maxMagnitudeR;
        BMReleaseFilter *fastL, *fastR, *slowL, *slowR;
        
        // if we are measuring RMS level, calculate that
        if(RMS){
            // get the max RMS power in each channel
            maxMagnitudeL = BMRMSPower_process(inputL, bufferLength);
            maxMagnitudeR = BMRMSPower_process(inputR, bufferLength);
            // set pointers
            fastL = &This->fastReleaseL_RMS;
            fastR = &This->fastReleaseR_RMS;
            slowL = This->slowReleaseL_RMS;
            slowR = This->slowReleaseR_RMS;
        }
        // else get peak level
        else {
            // get the max magnitude in each channel
            vDSP_maxmgv(inputL, 1, &maxMagnitudeL, bufferLength);
            vDSP_maxmgv(inputR, 1, &maxMagnitudeR, bufferLength);
            // set pointers
            fastL = &This->fastReleaseL_peak;
            fastR = &This->fastReleaseR_peak;
            slowL = This->slowReleaseL_peak;
            slowR = This->slowReleaseR_peak;
        }
        
        // convert to decibels
        if (maxMagnitudeL > 0.0f) maxMagnitudeL = BM_GAIN_TO_DB(maxMagnitudeL);
        else maxMagnitudeL = -128.0f;
        if (maxMagnitudeR > 0.0f) maxMagnitudeR = BM_GAIN_TO_DB(maxMagnitudeR);
        else maxMagnitudeR = -128.0f;
        
        // release filter the maxMagnitudes to get the peak with fast release time
        BMReleaseFilter_processBuffer(fastL, &maxMagnitudeL, fastPeakL, 1);
        BMReleaseFilter_processBuffer(fastR, &maxMagnitudeR, fastPeakR, 1);
        
        // release filter in multiple stages to get the peak with slow release time
        BMReleaseFilter_processBuffer(&slowL[0], &maxMagnitudeL, slowPeakL, 1);
        BMReleaseFilter_processBuffer(&slowR[0], &maxMagnitudeR, slowPeakR, 1);
        for(size_t i=1; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++){
            BMReleaseFilter_processBuffer(&slowL[i], slowPeakL, slowPeakL, 1);
            BMReleaseFilter_processBuffer(&slowR[i], slowPeakL, slowPeakR, 1);
        }
    }
    
    
    /*!
     *BMLevelMeter_processMono
     */
    void BMLevelMeter_processMono(BMLevelMeter* This,
                                    const float* input,
                                    float* fastPeak,
                                    float* slowPeak,
                                    size_t bufferLength,
                                    bool RMS){
        
        // if the buffer size has changed, update the filters to keep the release
        // times correct
        if(bufferLength != This->expectedBufferLength){
            This->expectedBufferLength = bufferLength;
            BMLevelMeter_setBufferSize(This, bufferLength);
        }
        
        float maxMagnitudeL;
        BMReleaseFilter *fast;
        BMReleaseFilter *slow;
        
        // if we are measuring RMS level, calculate that
        if(RMS){
            // get the max RMS power in each channel
            maxMagnitudeL = BMRMSPower_process(input, bufferLength);
            // set pointers to use the correct release filters
            fast = &This->fastReleaseL_RMS;
            slow = This->slowReleaseL_RMS;
        }
        // else get peak level
        else {
            // get the max magnitude in each channel
            vDSP_maxmgv(input, 1, &maxMagnitudeL, bufferLength);
            // set pointers to use the correct release filters
            fast = &This->fastReleaseL_peak;
            slow = This->slowReleaseL_peak;
        }
        
        // convert to decibels
        if (maxMagnitudeL > 0.0f) maxMagnitudeL = BM_GAIN_TO_DB(maxMagnitudeL);
        else maxMagnitudeL = -128.0f;
        
        // release filter the maxMagnitudes to get the peak with fast release time
        BMReleaseFilter_processBuffer(fast, &maxMagnitudeL, fastPeak, 1);
        
        // release filter in multiple stages to get the peak with slow release time
        BMReleaseFilter_processBuffer(&slow[0], &maxMagnitudeL, slowPeak, 1);
        for(size_t i=1; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++)
            BMReleaseFilter_processBuffer(&slow[i], slowPeak, slowPeak, 1);
    }
    
    
    
    
    void BMLevelMeter_peakLevelMono(BMLevelMeter* This,
                                    const float* input,
                                    float* fastRelease_dB,
                                    float* slowRelease_dB,
                                    size_t bufferLength){
        BMLevelMeter_processMono(This,
                                 input,
                                 fastRelease_dB,
                                 slowRelease_dB,
                                 bufferLength,
                                 false);
    }

      

    void BMLevelMeter_RMSPowerMono(BMLevelMeter* This,
                                      const float* input,
                                      float* fastRelease_dB,
                                      float* slowRelease_dB,
                                   size_t bufferLength){
        BMLevelMeter_processMono(This,
                                 input,
                                 fastRelease_dB,
                                 slowRelease_dB,
                                 bufferLength,
                                 true);
    }

    
    
    
    
    void BMLevelMeter_peakLevelStereo(BMLevelMeter* This,
                                         const float* inputL, const float* inputR,
                                         float* fastReleaseL_dB, float* fastReleaseR_dB,
                                         float* slowReleaseL_dB, float* slowReleaseR_dB,
                                         size_t bufferLength){
        BMLevelMeter_processStereo(This,
                                   inputL, inputR,
                                   fastReleaseL_dB, fastReleaseR_dB,
                                   slowReleaseL_dB, slowReleaseR_dB,
                                   bufferLength,
                                   false);
    }
    
    
    
    
    
     void BMLevelMeter_RMSPowerStereo(BMLevelMeter* This,
                                       const float* inputL, const float* inputR,
                                       float* fastReleaseL_dB, float* fastReleaseR_dB,
                                       float* slowReleaseL_dB, float* slowReleaseR_dB,
                                       size_t bufferLength){
        BMLevelMeter_processStereo(This,
                                   inputL, inputR,
                                   fastReleaseL_dB, fastReleaseR_dB,
                                   slowReleaseL_dB, slowReleaseR_dB,
                                   bufferLength,
                                   true);
    }
    
    
#ifdef __cplusplus
}
#endif

