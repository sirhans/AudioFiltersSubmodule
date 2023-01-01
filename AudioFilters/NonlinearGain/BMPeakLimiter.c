//
//  BMPeakLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 12/5/19.
//  Anyone may use this file without restrictions.
//

#include "BMPeakLimiter.h"
#include <string.h>
#include <Accelerate/Accelerate.h>
#include "Constants.h"

#define BM_PEAK_LIMITER_LOOKAHEAD_TIME_DV 0.00025f
#define BM_PEAK_LIMITER_LOOKAHEAD_TIME_NO_REALTIME 0.050f
#define BM_PEAK_LIMITER_ATTACK_LOOKAHEAD_RATIO 2.5f
#define BM_PEAK_LIMITER_RELEASE_TIME_DV 0.200f
#define BM_PEAK_LIMITER_THRESHOLD_GAIN_DV BM_DB_TO_GAIN(-2.0f)
#define BM_PEAK_LIMITER_THRESHOLD_GAIN_NO_REALTIME BM_DB_TO_GAIN(-0.5f)



void BMPeakLimiter_init(BMPeakLimiter *This, bool stereo, float sampleRate){
    This->sampleRate = sampleRate;
    This->lookaheadTime = This->targetLookaheadTime = BM_PEAK_LIMITER_LOOKAHEAD_TIME_DV;
    
    // init the envelope follower
    BMEnvelopeFollower_initWithCustomNumStages(&This->envelopeFollower, 3, 3, sampleRate);
    BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, This->lookaheadTime * BM_PEAK_LIMITER_ATTACK_LOOKAHEAD_RATIO);
    BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, BM_PEAK_LIMITER_RELEASE_TIME_DV);
    
    // init the delay
    size_t numChannels = (stereo == true) ? 2 : 1;
    size_t delayInSamples = This->lookaheadTime * sampleRate;
    BMShortSimpleDelay_init(&This->delay, numChannels, delayInSamples);
    
    // set the threshold
    This->thresholdGain = BM_PEAK_LIMITER_THRESHOLD_GAIN_DV;
    
    // not limiting yet on startup
    This->isLimiting = false;
}




void BMPeakLimiter_initNoRealtime(BMPeakLimiter *This, bool stereo, float sampleRate){
	This->sampleRate = sampleRate;
	This->lookaheadTime = This->targetLookaheadTime = BM_PEAK_LIMITER_LOOKAHEAD_TIME_NO_REALTIME;
	
	// init the envelope follower
	BMEnvelopeFollower_initWithCustomNumStages(&This->envelopeFollower, 3, 3, sampleRate);
	BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, This->lookaheadTime * BM_PEAK_LIMITER_ATTACK_LOOKAHEAD_RATIO);
	BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, BM_PEAK_LIMITER_RELEASE_TIME_DV);
	
	// init the delay
	size_t numChannels = (stereo == true) ? 2 : 1;
	size_t delayInSamples = This->lookaheadTime * sampleRate;
	BMShortSimpleDelay_init(&This->delay, numChannels, delayInSamples);
	
	// set the threshold
	This->thresholdGain = BM_PEAK_LIMITER_THRESHOLD_GAIN_NO_REALTIME;
	
	// not limiting yet on startup
	This->isLimiting = false;
}





void BMPeakLimiter_free(BMPeakLimiter *This){
    BMEnvelopeFollower_free(&This->envelopeFollower);
    BMShortSimpleDelay_free(&This->delay);
}





void BMPeakLimiter_setLookahead(BMPeakLimiter *This, float timeInSeconds){
    This->targetLookaheadTime = timeInSeconds;
}



void BMPeakLimiter_setReleaseTime(BMPeakLimiter *This, float timeInSeconds){
    BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, timeInSeconds);
}



void BMPeakLimiter_setThreshold(BMPeakLimiter *This, float thresholdDb){
    This->thresholdGain = BM_DB_TO_GAIN(thresholdDb);
}



void BMPeakLimiter_update(BMPeakLimiter *This){
    size_t delayInSamples = This->targetLookaheadTime * This->sampleRate;
    BMShortSimpleDelay_changeLength(&This->delay, delayInSamples);
    BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, This->targetLookaheadTime * BM_PEAK_LIMITER_ATTACK_LOOKAHEAD_RATIO);
    
    // mark the job done
    This->lookaheadTime = This->targetLookaheadTime;
}





void BMPeakLimiter_processStereo(BMPeakLimiter *This,
                                 const float *inL, const float *inR,
                                 float *outL, float *outR,
                                 size_t numSamples){
    // if the lookahead needs an update, do that instead of processing audio
    if(This->lookaheadTime != This->targetLookaheadTime){
        // clear the output buffers in case we don't finish the output on time
        memset(outL,0,sizeof(float)*numSamples);
        memset(outR,0,sizeof(float)*numSamples);
        
        // update settings
        BMPeakLimiter_update(This);
    }
    
    // chunked processing
    size_t samplesProcessed = 0;
    while(samplesProcessed< numSamples){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples-samplesProcessed);
        
        // rectify the left and right channels into the buffers
        vDSP_vabs(inL+samplesProcessed, 1, This->bufferL, 1, samplesProcessing);
        vDSP_vabs(inR+samplesProcessed, 1, This->bufferR, 1, samplesProcessing);
        
        // make an alias pointer for code readability
        float* envelope = This->controlSignal;
        
        // take the max of the left and right channels into the envelope buffer
        vDSP_vmax(This->bufferL, 1, This->bufferR, 1, envelope, 1, samplesProcessing);
        
        // replace values below the threshold with the threshold itself
        vDSP_vthr(envelope, 1, &This->thresholdGain, envelope, 1, samplesProcessing);
        
        // apply an envelope follower to the control signal to smooth it out
        BMEnvelopeFollower_processBuffer(&This->envelopeFollower, envelope, envelope, samplesProcessing);
        
        // control signal = threshold / envelope
        vDSP_svdiv(&This->thresholdGain, envelope, 1, This->controlSignal, 1, samplesProcessing);
        
        // set the state indicator variable for apps that require a meter to indicate limiting
        This->isLimiting = This->controlSignal[samplesProcessing - 1] < 0.999f;
        
        // delay the input
        const float* inputs [2] = {inL+samplesProcessed,inR+samplesProcessed};
        float* outputs [2] = {This->bufferL, This->bufferR};
        BMShortSimpleDelay_process(&This->delay, inputs, outputs, 2, samplesProcessing);
        
        // multiply the delayed inputs by the control signal and write to the output
        vDSP_vmul(This->bufferL,1,This->controlSignal,1,outL+samplesProcessed,1,samplesProcessing);
        vDSP_vmul(This->bufferR,1,This->controlSignal,1,outR+samplesProcessed,1,samplesProcessing);
        
        samplesProcessed += samplesProcessing;
    }
}






void BMPeakLimiter_processMono(BMPeakLimiter *This,
                               const float *input,
                               float *output,
                               size_t numSamples){
    // if the lookahead needs an update, do that instead of processing audio
    if(This->lookaheadTime != This->targetLookaheadTime){
        // clear the output buffer in case we don't finish the output on time
        memset(output,0,sizeof(float)*numSamples);
        
        // update settings
        BMPeakLimiter_update(This);
    }
    
    
    // chunked processing
    size_t samplesProcessed = 0;
    while(samplesProcessed < numSamples){
        size_t samplesProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, numSamples-samplesProcessed);
        
        // make an alias pointer for code readability
        float* envelope = This->controlSignal;
        
        // rectify the input into the envelope buffer
        vDSP_vabs(input+samplesProcessed, 1, envelope, 1, samplesProcessing);
        
        // replace values below the threshold with the threshold itself
        vDSP_vthr(envelope, 1, &This->thresholdGain, envelope, 1, samplesProcessing);
        
        // apply an envelope follower to the envelope to smooth it out
        BMEnvelopeFollower_processBuffer(&This->envelopeFollower, envelope, envelope, samplesProcessing);
        
        // control signal = threshold / envelope
        vDSP_svdiv(&This->thresholdGain, envelope, 1, This->controlSignal, 1, samplesProcessing);
        
        // set the state indicator variable for apps that require a meter to indicate limiting
        This->isLimiting = This->controlSignal[samplesProcessing - 1] < 0.999f;
        
        // delay the input
        const float* inputs [1] = {input+samplesProcessed};
        float* outputs [1] = {This->bufferL};
        BMShortSimpleDelay_process(&This->delay, inputs, outputs, 1, samplesProcessing);
        
        // multiply the delayed input by the control signal and write to the output
        vDSP_vmul(This->bufferL,1,This->controlSignal,1,output+samplesProcessed,1,samplesProcessing);
        
        samplesProcessed += samplesProcessing;
    }
}





bool BMPeakLimiter_isLimiting(BMPeakLimiter *This){
    return This->isLimiting;
}
