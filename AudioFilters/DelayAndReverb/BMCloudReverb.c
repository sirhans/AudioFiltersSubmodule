//
//  BMCloudReverb.c
//  AUCloudReverb
//
//  Created by Nguyen Minh Tien on 2/27/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMCloudReverb.h"
#include "BMReverb.h"

#define Filter_Level_Lowpass 0
#define Filter_Level_Bell 1
#define Filter_Level_Highpass 2
#define Filter_Level_Lowshelf 3


void BMCloudReverb_init(BMCloudReverb* This,float sr){
    This->sampleRate = sr;
    //BIQUAD FILTER
    BMMultiLevelBiquad_init(&This->biquadFilter, 4, sr, true, false, true);
    //Tone control - use 12db
    BMMultiLevelBiquad_setLowPass12db(&This->biquadFilter, 4000, Filter_Level_Lowpass);
    //Bell
    BMMultiLevelBiquad_setBellQ(&This->biquadFilter, 1300, sqrtf(0.5f), -5, Filter_Level_Bell);
    //High passs
    BMMultiLevelBiquad_setHighPass6db(&This->biquadFilter, 100, Filter_Level_Highpass);
    //Low shelf
    BMMultiLevelBiquad_setLowShelf(&This->biquadFilter, 120, 2, Filter_Level_Lowshelf);
    
    //VND
    BMVelvetNoiseDecorrelator_init(&This->vnd1, 1.0/3.0f, 16, 100, true, sr);
    BMVelvetNoiseDecorrelator_init(&This->vnd2, 1.0/3.0f, 16, 100, true, sr);
    //Last vnd dont have dry tap -> always wet 100%
    BMVelvetNoiseDecorrelator_init(&This->vnd3, 1.0/3.0f, 16, 100, true, sr);
    
    BMVelvetNoiseDecorrelator_setWetMix(&This->vnd1, 1.0);
    BMVelvetNoiseDecorrelator_setWetMix(&This->vnd2, 1.0);
    BMVelvetNoiseDecorrelator_setWetMix(&This->vnd3, 1.0);
    
    //Pitch shifting
    float delaySampleRange = 0.02f*sr;
    float duration = 20.0f;
    BMPitchShiftDelay_init(&This->pitchDelay, duration,delaySampleRange , delaySampleRange, sr);
    
    //Delay
    float delayRange = 0.5 * sr;
    BMSimpleDelayStereo_init(&This->simpleDelay1, delayRange*1.05f);
    BMSimpleDelayStereo_init(&This->simpleDelay2, delayRange*0.95f);
    BMSimpleDelayStereo_init(&This->simpleDelay3, delayRange*1.03f);
    BMSimpleDelayStereo_init(&This->simpleDelay4, delayRange*0.97f);
    
    This->buffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->buffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->loopInput.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->loopInput.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->lastLoopBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->lastLoopBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->wetBuffer.bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    This->wetBuffer.bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
    
    
    
    BMSmoothGain_init(&This->loopGain, sr);
    BMCloudReverb_setLoopGain(This, -10);
    
    BMCloudReverb_setDelayPitchMixer(This, 0.5f);
    
    BMWetDryMixer_init(&This->reverbMixer, sr);
    BMCloudReverb_setOutputMixer(This, 0.5f);
}

void BMCloudReverb_destroy(BMCloudReverb* This){
    BMMultiLevelBiquad_free(&This->biquadFilter);
    
    BMVelvetNoiseDecorrelator_free(&This->vnd1);
    BMVelvetNoiseDecorrelator_free(&This->vnd2);
    BMVelvetNoiseDecorrelator_free(&This->vnd3);
    
    BMPitchShiftDelay_destroy(&This->pitchDelay);
    
    BMSimpleDelayStereo_destroy(&This->simpleDelay1);
    BMSimpleDelayStereo_destroy(&This->simpleDelay2);
    BMSimpleDelayStereo_destroy(&This->simpleDelay3);
    BMSimpleDelayStereo_destroy(&This->simpleDelay4);
    
    free(This->buffer.bufferL);
    This->buffer.bufferL = nil;
    free(This->buffer.bufferR);
    This->buffer.bufferR = nil;
    free(This->loopInput.bufferL);
    This->loopInput.bufferL = nil;
    free(This->loopInput.bufferR);
    This->loopInput.bufferR = nil;
    free(This->lastLoopBuffer.bufferL);
    This->lastLoopBuffer.bufferL = nil;
    free(This->lastLoopBuffer.bufferR);
    This->lastLoopBuffer.bufferR = nil;
    free(This->wetBuffer.bufferL);
    This->wetBuffer.bufferL = nil;
    free(This->wetBuffer.bufferR);
    This->wetBuffer.bufferR = nil;
}

void BMCloudReverb_processStereo(BMCloudReverb* This,float* inputL,float* inputR,float* outputL,float* outputR,size_t numSamples){
    //Filters
    BMMultiLevelBiquad_processBufferStereo(&This->biquadFilter, inputL, inputR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    
    //VND
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd1, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd2, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    BMVelvetNoiseDecorrelator_processBufferStereo(&This->vnd3, This->buffer.bufferL, This->buffer.bufferR, This->buffer.bufferL, This->buffer.bufferR, numSamples);
    
    memcpy(outputL, This->buffer.bufferL, sizeof(float)*numSamples);
    memcpy(outputR, This->buffer.bufferR, sizeof(float)*numSamples);
    return;
    
    //PitchShifting delay into wetbuffer
    BMPitchShiftDelay_processStereoBuffer(&This->pitchDelay, This->buffer.bufferL, This->buffer.bufferR, This->wetBuffer.bufferL, This->wetBuffer.bufferR, numSamples);
    
    //Delay loop - add lastloop buffer to current wetbuffer
    vDSP_vadd(This->wetBuffer.bufferL, 1, This->lastLoopBuffer.bufferL, 1, This->loopInput.bufferL, 1, numSamples);
    vDSP_vadd(This->wetBuffer.bufferR, 1, This->lastLoopBuffer.bufferR, 1, This->loopInput.bufferR, 1, numSamples);
    
    BMSimpleDelayStereo_process(&This->simpleDelay1, This->loopInput.bufferL, This->loopInput.bufferR, This->loopInput.bufferL, This->loopInput.bufferR, numSamples);
    //Add it to wetbuffer
    vDSP_vadd(This->wetBuffer.bufferL, 1, This->loopInput.bufferL, 1, This->wetBuffer.bufferL, 1, numSamples);
    vDSP_vadd(This->wetBuffer.bufferR, 1, This->loopInput.bufferR, 1, This->wetBuffer.bufferR, 1, numSamples);
    
    BMSimpleDelayStereo_process(&This->simpleDelay2, This->loopInput.bufferL, This->loopInput.bufferR, This->loopInput.bufferL, This->loopInput.bufferR, numSamples);
    //Add it to wetbuffer
    vDSP_vadd(This->wetBuffer.bufferL, 1, This->loopInput.bufferL, 1, This->wetBuffer.bufferL, 1, numSamples);
    vDSP_vadd(This->wetBuffer.bufferR, 1, This->loopInput.bufferR, 1, This->wetBuffer.bufferR, 1, numSamples);
    
    BMSimpleDelayStereo_process(&This->simpleDelay3, This->loopInput.bufferL, This->loopInput.bufferR, This->loopInput.bufferL, This->loopInput.bufferR, numSamples);
    //Add it to wetbuffer
    vDSP_vadd(This->wetBuffer.bufferL, 1, This->loopInput.bufferL, 1, This->wetBuffer.bufferL, 1, numSamples);
    vDSP_vadd(This->wetBuffer.bufferR, 1, This->loopInput.bufferR, 1, This->wetBuffer.bufferR, 1, numSamples);
    
    BMSimpleDelayStereo_process(&This->simpleDelay4, This->loopInput.bufferL, This->loopInput.bufferR, This->loopInput.bufferL, This->loopInput.bufferR, numSamples);
    //Add it to wetbuffer
    vDSP_vadd(This->wetBuffer.bufferL, 1, This->loopInput.bufferL, 1, This->wetBuffer.bufferL, 1, numSamples);
    vDSP_vadd(This->wetBuffer.bufferR, 1, This->loopInput.bufferR, 1, This->wetBuffer.bufferR, 1, numSamples);
    
    //Adjust volume of wetmixer
    float gain = 1.0f/sqrt(5.0f);
    vDSP_vsmul(This->wetBuffer.bufferL, 1, &gain, This->wetBuffer.bufferL, 1, numSamples);
    vDSP_vsmul(This->wetBuffer.bufferR, 1, &gain, This->wetBuffer.bufferR, 1, numSamples);
    
    //Process reverb dry/wet mixer
    BMWetDryMixer_processBufferInPhase(&This->reverbMixer, This->wetBuffer.bufferL, This->wetBuffer.bufferR, inputL, inputR, outputL, outputR, numSamples);
    
    //Apply loop gain & store into lastLoopBuffer to use in next frame
//    BMSmoothGain_processBuffer(&This->loopGain, This->loopInput.bufferL, This->loopInput.bufferR, This->lastLoopBuffer.bufferR, This->lastLoopBuffer.bufferL, numSamples);
    float loopGain = BMReverbDelayGainFromRT60(10, numSamples/This->sampleRate);
    vDSP_vsmul(This->loopInput.bufferL, 1, &loopGain, This->lastLoopBuffer.bufferL, 1, numSamples);
    vDSP_vsmul(This->loopInput.bufferR, 1, &loopGain, This->lastLoopBuffer.bufferR, 1, numSamples);
}

#pragma mark - Set
void BMCloudReverb_setLoopGain(BMCloudReverb* This,float gainDb){
    
    BMSmoothGain_setGainDb(&This->loopGain, gainDb);
}

void BMCloudReverb_setDelayPitchMixer(BMCloudReverb* This,float wetMix){
    BMPitchShiftDelay_setWetGain(&This->pitchDelay, wetMix);
}

void BMCloudReverb_setOutputMixer(BMCloudReverb* This,float wetMix){
    BMWetDryMixer_setMix(&This->reverbMixer, wetMix);
}


#pragma mark - Test
void BMCloudReverb_impulseResponse(BMCloudReverb* This,float* outputL,float* outputR,size_t length){
    float* data = malloc(sizeof(float)*length);
    memset(data, 0, sizeof(float)*length);
    data[0] = 1.0f;
    size_t sampleProcessed = 0;
    size_t sampleProcessing = 0;
    while(sampleProcessed<length){
        sampleProcessing = BM_MIN(BM_BUFFER_CHUNK_SIZE, length - sampleProcessed);
        
        BMCloudReverb_processStereo(This, data+sampleProcessed, data+sampleProcessed, outputL+sampleProcessed, outputR+sampleProcessed, sampleProcessing);
        
        sampleProcessed += sampleProcessing;
    }
    
}
