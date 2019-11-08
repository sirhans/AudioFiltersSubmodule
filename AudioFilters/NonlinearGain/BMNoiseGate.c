//
//  BMNoiseGate.c
//  AudioUnitTest
//
//  Created by TienNM on 9/25/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif
    
#include "BMNoiseGate.h"
#include "BMEnvelopeFollower.h"
#include <Accelerate/Accelerate.h>
    
    
#define BM_NOISE_GATE_DEFAULT_ATTACK_TIME 0.001
#define BM_NOISE_GATE_DEFAULT_RELEASE_TIME 0.050
#define BM_NOISE_GATE_DEFAULT_RATIO 4.0f
    
    // forward declarations
    void BMNoiseGate_gainComputer(BMNoiseGate *This, size_t numSamples);
    
    
    
    void BMNoiseGate_init(BMNoiseGate *This, float thresholdDb, float sampleRate){
        // initialise the envelope follower
        BMEnvelopeFollower_initWithCustomNumStages(&This->envFollower, 5, 2, sampleRate);
        BMNoiseGate_setReleaseTime(This, BM_NOISE_GATE_DEFAULT_RELEASE_TIME);
        BMNoiseGate_setAttackTime(This, BM_NOISE_GATE_DEFAULT_ATTACK_TIME);
        
        // set the threshold
        BMNoiseGate_setThreshold(This, thresholdDb);
        
        // set default downward expansion ratio
        BMNoiseGate_setRatio(This,BM_NOISE_GATE_DEFAULT_RATIO);
        
        // init the filter for limiting the frequency range of the sidechain input
        BMMultiLevelBiquad_init(&This->sidechainFilter, 2, sampleRate, false, true, false);
        
        // init the sidechain filter group delay compensation delay
        This->sidechainMinFreq = 20.0f;
        This->sidechainMaxFreq = 10000.0f;
        BMShortSimpleDelay_init(&This->delay, 2, 10);
        
        // bypass the sidechain filters
        BMNoiseGate_setSidechainHighCut(This, 20000.0f);
        BMNoiseGate_setSidechainLowCut(This, 20.0f);
        
        // init the sidechain input level metre
        BMLevelMeter_init(&This->sidechainInputMeter, sampleRate);
        
        // init some variables that update while processing
        This->lastState = 0.0f;
        This->sidechainInputLeveldB = -128.0f;
        This->controlSignalLeveldB = -128.0f;
    }
    
    
    
    void BMNoiseGate_free(BMNoiseGate *This){
        BMMultiLevelBiquad_destroy(&This->sidechainFilter);
        BMShortSimpleDelay_free(&This->delay);
    }
    
    
    
    
    // returns the group delay in samples of the sidechain filter at the log
    // scale midpoint between the highpass and lowpass cutoff frequencies
    size_t BMNoiseGate_sidechainFilterGroupDelay(BMNoiseGate *This){
        
        // we use the geometric mean to get the midpoint between the
        // lower and upper bounds on a log frequency scale
        double centreFreq = BMGeometricMean2(This->sidechainMinFreq,
                                             This->sidechainMaxFreq);
        
        // return the group delay in samples, rounded to the nearest integer
        return (size_t)round(BMMultiLevelBiquad_groupDelay(&This->sidechainFilter, centreFreq));
    }
    
    
    
    
    
    void BMNoiseGate_updateDelay(BMNoiseGate *This){
        // find the group delay of the sidechain filter at its centre frequency
        size_t groupDelay = BMNoiseGate_sidechainFilterGroupDelay(This);
        
        // set the delay to compensate for group delay
        BMShortSimpleDelay_changeLength(&This->delay, groupDelay);
    }
    
    

    
    
    
    /*!
     *BMNoiseGate_gainComputer
     *
     * @abstract computes gain reduction for downward expansion
     */
    inline void BMNoiseGate_gainComputer(BMNoiseGate *This, size_t numSamples){
        // int version of numSamples for vForce functions
        int numSamplesI = (int)numSamples;
        
        // rectify
        vvfabsf(This->buffer, This->buffer, &numSamplesI);
        
        // gate mode
        if(This->ratio > 50.0f){
            // subtract threshold to get 0 for values below threshold
            float negThreshold = -This->thresholdGain;
            vDSP_vsadd(This->buffer, 1, &negThreshold, This->buffer, 1, numSamples);
            
            // clip to [0,1]
            float zero = 0.0f;
            float one = 0.0f;
            vDSP_vclip(This->buffer, 1, &zero, &one, This->buffer, 1, numSamples);
            
            // output zero for values < threshold and 1 for values > threshold
            vvceilf(This->buffer, This->buffer, &numSamplesI);
        }
        // downward expander mode
        else {
            
            // convert the signal to log scale
            vvlog2f(This->buffer, This->buffer, &numSamplesI);
            
            // convert the threshold to log scale
            float logThresholdNeg = -log2f(This->thresholdGain);
            
            // signalLogScale - thresholdLogScale
            // this is the opposite signof what we want but we do it this way
            // because there is no vDSP_vssub for us to do what we actually want:
            // (thresholdLogScale - signalLogScale)
            vDSP_vsadd(This->buffer, 1, &logThresholdNeg, This->buffer, 1, numSamples);
            
            // clip the negative values to zero
            float zero = 0.0f;
            vDSP_vthr(This->buffer, 1, &zero, This->buffer, 1, numSamples);
            
            // multiply the values by -ratio to flip them negative and apply the
            // ratio at the same time. The negative flip is necessary because we
            // flipped the sign of the vDSP_vsadd operation above.
            float negRatio = -This->ratio;
            vDSP_vsmul(This->buffer,1,&negRatio,This->buffer,1,numSamples);
            
            // convert back to linear gain scale
            float two = 2.0f;
            vvpowf(This->buffer, This->buffer, &two, &numSamplesI);
        }
    }
    
    
    
    
    
    /*!
     *BMNoiseGate_processExpander
     *
     * @note this function exists only to avoid rewriting the same code two times in the stereo and mono process functions
     */
    void BMNoiseGate_processExpander(BMNoiseGate *This, size_t samplesProcessing){
        // apply sidechain filters to the buffer
        BMMultiLevelBiquad_processBufferMono(&This->sidechainFilter, This->buffer, This->buffer, samplesProcessing);
        
        // measure the peak level of the sidechain input
        float unused;
        BMLevelMeter_peakLevelMono(&This->sidechainInputMeter,
                                   This->buffer,
                                   &This->sidechainInputLeveldB,
                                   &unused,
                                   samplesProcessing);
        
        // apply downward expansion to the sidechain buffer
        BMNoiseGate_gainComputer(This, samplesProcessing);
        
        // Filter the sidechain buffer with an envelope follower
        BMEnvelopeFollower_processBuffer(&This->envFollower, This->buffer, This->buffer, samplesProcessing);
        
        // save the last value in the control signal buffer for use in level
        // metre display
        This->controlSignalLeveldB = BM_GAIN_TO_DB(This->buffer[samplesProcessing-1]);
        
        // record the last gain value for use in apps that require a gain
        // indicator in the UI
        This->lastState = This->buffer[samplesProcessing-1];
    }
    
    
    
    
    
    void BMNoiseGate_processStereo(BMNoiseGate *This,
                                   const float* inputL, const float* inputR,
                                   float* outputL, float* outputR,
                                   size_t numSamples){
        assert(This->sidechainFilter.numChannels == 2);
        
        // begin chunked processing
        while (numSamples > 0){
            size_t samplesProcessing = BM_MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            
            // mix stereo input to mono and store into buffer
            float half = 0.5f;
            vDSP_vasm(inputL, 1, inputR, 1, &half, This->buffer, 1, samplesProcessing);
            
            BMNoiseGate_processExpander(This,samplesProcessing);
            
            // Apply the gain control signal to the input and write to output
            vDSP_vmul(This->buffer, 1, inputL, 1, outputL, 1, samplesProcessing);
            vDSP_vmul(This->buffer, 1, inputR, 1, outputR, 1, samplesProcessing);
            
            // advance pointers
            inputL += samplesProcessing;
            outputL += samplesProcessing;
            inputR += samplesProcessing;
            outputR += samplesProcessing;
            numSamples -= samplesProcessing;
        }
    }
    
    
    
    
    
    void BMNoiseGate_processMono(BMNoiseGate *This,
                                 const float* input,
                                 float* output,
                                 size_t numSamples){
        
        // begin chunked processing
        while (numSamples > 0){
            size_t samplesProcessing = BM_MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            
            BMNoiseGate_processExpander(This,samplesProcessing);
            
            // Apply the gain control signal to the input and write to output
            vDSP_vmul(This->buffer, 1, input, 1, output, 1, samplesProcessing);
            
            // advance pointers
            input += samplesProcessing;
            output += samplesProcessing;
            numSamples -= samplesProcessing;
        }
    }
    
    
    
    
    void BMNoiseGate_setAttackTime(BMNoiseGate *This,float attackTimeSeconds){
        BMEnvelopeFollower_setAttackTime(&This->envFollower, attackTimeSeconds);
    }
    
    
    
    
    
    void BMNoiseGate_setReleaseTime(BMNoiseGate *This,float releaseTimeSeconds){
        BMEnvelopeFollower_setReleaseTime(&This->envFollower, releaseTimeSeconds);
    }
    
    
    
    
    void BMNoiseGate_setThreshold(BMNoiseGate *This,float thresholdDb){
        This->thresholdGain = BM_DB_TO_GAIN(thresholdDb);
    }
    
    
    
    
    
    float BMNoiseGate_getGateVolumeDB(BMNoiseGate *This){
        return This->controlSignalLeveldB;
    }
    
    
    
    
    
    float BMNoiseGate_getSidechainInputLevelDB(BMNoiseGate *This){
        return This->sidechainInputLeveldB;
    }
    
    
    
    
    
    void BMNoiseGate_setSidechainHighCut(BMNoiseGate *This, float fc){
        assert(fc <= This->sidechainFilter.sampleRate/2.0f);
        
        // disable the filter if the cutoff is greater than 18000 Hz
        if (fc < 18000.0f){
            BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, true, 1);
            BMMultiLevelBiquad_setLowPass6db(&This->sidechainFilter, fc, 1);
            This->sidechainMaxFreq = fc;
        }
        else {
            BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, false, 1);
            This->sidechainMaxFreq = This->sidechainFilter.sampleRate/2.0;
        }
        
        BMNoiseGate_updateDelay(This);
    }
    
    
    
    
    void BMNoiseGate_setSidechainLowCut(BMNoiseGate *This, float fc){
        assert(fc <= This->sidechainFilter.sampleRate/2.0f);
        
        // disable the filter if the cutoff is below 20 Hz
        if (fc > 20.0f){
            BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, true, 0);
            BMMultiLevelBiquad_setHighPass6db(&This->sidechainFilter, fc, 0);
            This->sidechainMinFreq = fc;
        }
        else {
            BMMultiLevelBiquad_setActiveOnLevel(&This->sidechainFilter, false, 0);
            This->sidechainMinFreq = 0.0f;
        }
        
        BMNoiseGate_updateDelay(This);
    }
    
    
    
    
    
    void BMNoiseGate_setRatio(BMNoiseGate *This, float ratio){
        assert(ratio >= 0.0f);
        
        This->ratio = ratio;
    }
    
    
    
#ifdef __cplusplus
}
#endif
