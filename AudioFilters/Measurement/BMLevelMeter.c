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
        BMReleaseFilter_updateSampleRate(&This->fastReleaseL, buffersPerSecond);
        BMReleaseFilter_updateSampleRate(&This->fastReleaseR, buffersPerSecond);
        for(size_t i=0; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++){
            BMReleaseFilter_updateSampleRate(&This->slowReleaseL[i], buffersPerSecond);
            BMReleaseFilter_updateSampleRate(&This->slowReleaseR[i], buffersPerSecond);
        }
    }
    
    
    
    void BMLevelMeter_init(BMLevelMeter* This, float sampleRate){
        This->expectedBufferLength = BM_LEVEL_METER_DEFAULT_BUFFER_LENGTH;
        
        This->sampleRate = sampleRate;
        
        float buffersPerSecond = sampleRate / (float)BM_LEVEL_METER_DEFAULT_BUFFER_LENGTH;
        
        // init the fast release filters
        float fastReleaseFc = ARTimeToCutoffFrequency(BM_LEVEL_METER_DEFAULT_FAST_RELEASE_TIME, 1);
        BMReleaseFilter_init(&This->fastReleaseL, fastReleaseFc, buffersPerSecond);
        BMReleaseFilter_init(&This->fastReleaseR, fastReleaseFc, buffersPerSecond);
        
        // init the slow release filters
        float slowReleaseFc = ARTimeToCutoffFrequency(BM_LEVEL_METER_DEFAULT_SLOW_RELEASE_TIME, BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS);
        for(size_t i=0; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++){
            BMReleaseFilter_init(&This->slowReleaseL[i], slowReleaseFc, buffersPerSecond);
            BMReleaseFilter_init(&This->slowReleaseR[i], slowReleaseFc, buffersPerSecond);
        }
    }
    

    
    
    void BMLevelMeter_peakLevelStereo(BMLevelMeter* This,
                                         const float* inputL, const float* inputR,
                                         float* fastPeakL, float* fastPeakR,
                                         float* slowPeakL, float* slowPeakR,
                                         size_t bufferLength){
        // if the buffer size has changed, update the filters to keep the release
        // times correct
        if(bufferLength != This->expectedBufferLength){
            This->expectedBufferLength = bufferLength;
            BMLevelMeter_setBufferSize(This, bufferLength);
        }
        
        float maxMagnitudeL, maxMagnitudeR;
        
        // get the max magnitude in each channel
        vDSP_maxmgv(inputL, 1, &maxMagnitudeL, bufferLength);
        vDSP_maxmgv(inputR, 1, &maxMagnitudeR, bufferLength);
        
        // convert to decibels
        if (maxMagnitudeL > 0.0f) maxMagnitudeL = BM_GAIN_TO_DB(maxMagnitudeL);
        else maxMagnitudeL = -128.0f;
        if (maxMagnitudeR > 0.0f) maxMagnitudeR = BM_GAIN_TO_DB(maxMagnitudeR);
        else maxMagnitudeR = -128.0f;
        
        // release filter the maxMagnitudes to get the peak with fast release time
        BMReleaseFilter_processBuffer(&This->fastReleaseL, &maxMagnitudeL, fastPeakL, 1);
        BMReleaseFilter_processBuffer(&This->fastReleaseR, &maxMagnitudeR, fastPeakR, 1);
        
        // release filter in multiple stages to get the peak with slow release time
        BMReleaseFilter_processBuffer(&This->slowReleaseL[0], &maxMagnitudeL, slowPeakL, 1);
        BMReleaseFilter_processBuffer(&This->slowReleaseR[0], &maxMagnitudeR, slowPeakR, 1);
        for(size_t i=1; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++){
            BMReleaseFilter_processBuffer(&This->slowReleaseL[i], slowPeakL, slowPeakL, 1);
            BMReleaseFilter_processBuffer(&This->slowReleaseR[i], slowPeakL, slowPeakR, 1);
        }
    }
    
    
    
    
    void BMLevelMeter_RMSPowerStereo(BMLevelMeter* This,
                                      const float* inputL, const float* inputR,
                                      float* fastReleaseL, float* fastReleaseR,
                                      float* slowReleaseL, float* slowReleaseR,
                                      size_t bufferLength){
        // if the buffer size has changed, update the filters to keep the release
        // times correct
        if(bufferLength != This->expectedBufferLength){
            This->expectedBufferLength = bufferLength;
            BMLevelMeter_setBufferSize(This, bufferLength);
        }
        
        float RMSLeft, RMSRight;
        
        // get the max magnitude in each channel
        RMSLeft  = BMRMSPower_process(inputL, bufferLength);
        RMSRight = BMRMSPower_process(inputR, bufferLength);
        
        // convert to decibels
        if (RMSLeft > 0.0f) RMSLeft = BM_GAIN_TO_DB(RMSLeft);
        else RMSLeft = -128.0f;
        if (RMSRight > 0.0f) RMSRight = BM_GAIN_TO_DB(RMSRight);
        else RMSRight = -128.0f;
        
        // release filter the maxMagnitudes to get the peak with fast release time
        BMReleaseFilter_processBuffer(&This->fastReleaseL, &RMSLeft, fastReleaseL, 1);
        BMReleaseFilter_processBuffer(&This->fastReleaseR, &RMSRight, fastReleaseR, 1);
        
        // release filter in multiple stages to get the peak with slow release time
        BMReleaseFilter_processBuffer(&This->slowReleaseL[0], &RMSLeft, slowReleaseL, 1);
        BMReleaseFilter_processBuffer(&This->slowReleaseR[0], &RMSRight, slowReleaseR, 1);
        for(size_t i=1; i<BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS; i++){
            BMReleaseFilter_processBuffer(&This->slowReleaseL[i], slowReleaseL, slowReleaseL, 1);
            BMReleaseFilter_processBuffer(&This->slowReleaseR[i], slowReleaseL, slowReleaseL, 1);
        }
    }
    
    
#ifdef __cplusplus
}
#endif

