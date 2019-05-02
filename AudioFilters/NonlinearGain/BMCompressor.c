//
//  BMCompressor.c
//  CompressorApp
//
//  Created by Duc Anh on 7/17/18.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#include "BMCompressor.h"
#include <stdlib.h>
#include <math.h>
#include "fastlog.h"
#include "fastpow.h"
#include "BMVectorOps.h"
#include "Constants.h"


void BMCompressor_init(BMCompressor* This, float sampleRate){
    float threshold = -10.0f;
    float kneeWidth = 25.0f;
    float ratio = 4.0f;
    float attackTime = 0.010;
    float releaseTime = 0.080;
    
    BMCompressor_initWithSettings(This,
                                  sampleRate,
                                  threshold,
                                  kneeWidth,
                                  ratio,
                                  attackTime,
                                  releaseTime);
}


/*!
 * BMCompressor_Init
 *
 * @param sampleRate       audio system sampling rate
 * @param thresholdInDB    above this threshold, we start compressing
 * @param kneeWidthInDB    soften the transition from compressing to not compressing in [threshold-knee,thresthold+knee]
 * @param ratio            compressor ratio
 * @param attackTime       time from onset of note to 90% compressor gain change
 * @param releaseTime      time from release of note to 90% compressor gain change
 */
void BMCompressor_initWithSettings(BMCompressor* This, float sampleRate, float thresholdInDB, float kneeWidthInDB, float ratio, float attackTime, float releaseTime){
    
    //user setting
    BMCompressorSetting *settings = &This->settings;
    settings->slope = 1.0f - (1.0f / ratio);
    settings->thresholdInDB = thresholdInDB;
    settings->kneeWidthInDB = kneeWidthInDB;
    
    BMEnvelopeFollower_init(&This->envelopeFollower, sampleRate);
    BMEnvelopeFollower_setAttackTime(&This->envelopeFollower, attackTime);
    BMEnvelopeFollower_setReleaseTime(&This->envelopeFollower, releaseTime);
    
    
    //init buffers
    This->buffer1 = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE);
    This->buffer2 = malloc(sizeof(float) * BM_BUFFER_CHUNK_SIZE);
}


/*
 * Free memory used by temp buffers
 */
void BMCompressor_Free(BMCompressor* compressor){
    free(compressor->buffer1);
    compressor->buffer1 = NULL;
    free(compressor->buffer2);
    compressor->buffer2 = NULL;
}




/*
 * Mathematica code:
 *
 * qThreshold[x_, l_, w_] :=
 *       If[x < l - w, l, If[x > l + w, x, (- l x + (l + w + x)^2/4)/w]]
 */
void quadraticThreshold(const float* input, float* output, float threshold, float softKneeWidth, size_t numFrames){
    assert(input != output);
    
    float l = threshold;
    float w = softKneeWidth;
    
    /*
     * we want to express (- l x + (l + w + x)^2/4)/w
     * as a polynomial A*x^2 * Bx + C
     *
     * Mathematica: (l^2 + 2 l w + w^2 + (-2 l + 2 w) x + x^2) / (4 w)
     */
    float lPlusW = l + w;
    float lMinusW = l - w;
    float fourWinv = 1.0 / (4.0f * w);
    // C = (l + w)^2 / (4 w)
    float C = lPlusW * lPlusW * fourWinv;
    // B = 2 (w - l) / (4 w)
    float B = 2.0f * (-lMinusW) * fourWinv;
    // A = 1 / (4 w)
    float A = fourWinv;
    float coefficients [3] = {A, B, C};
    
    // clip values to [l-w,l+w];
    vDSP_vclip(input, 1, &lMinusW, &lPlusW, output, 1, numFrames);

    // process the polynomial
    vDSP_vpoly(coefficients, 1, output, 1, output, 1, numFrames, 2);
    
    // where the input is greater than the polynomial output, return the input
    vDSP_vmax(input,1,output,1,output,1,numFrames);
}


/*!
 * BMCompressor_ProcessBufferMono
 * @param This         pointer to a compressor struct
 * @param input        input array of length "length"
 * @param output       output array of length "length"
 * @param minGainDb    the lowest gain setting the compressor reached while processing the buffer
 * @param numSamples   length of arrays
 * @brief apply dynamic range compression to input; result in output (MONO)
 * @abstract result[i] is 1.0 where X[i] is within limits, 0.0 otherwise
 * @discussion returns floating point output for use in vectorised code without conditional branching
 * @code result[i] = -1.0f * (X[i] >= lowerLimit && X[i] <= upperLimit);
 * @warning no warnings
 */
void BMCompressor_ProcessBufferMono(BMCompressor* This, const float* input, float* output, float* minGainDb, size_t numSamples){
    
    // prepare to track the minimum gain
    float minGainThisChunk;
    float minGainWholeBuffer = FLT_MAX;
    
    // get a shorter name for settings
    BMCompressorSetting* settings = &This->settings;
    
    while(numSamples > 0){
        size_t framesProcessing = MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
        
        // get a shorter name for the buffer
        float* buffer1 = This->buffer1;
        
        // rectify the input signal
        vDSP_vabs(input, 1, buffer1, 1, framesProcessing);
        
        // convert linear gain to decibel scale
        float one = 1.0f;
        vDSP_vdbcon(buffer1,1,&one,buffer1,1,framesProcessing,0);
        
        // clip values below the threshold with a soft knee
        quadraticThreshold(buffer1, This->buffer2, settings->thresholdInDB, settings->kneeWidthInDB, framesProcessing);
        
        // shift the values up so that zero is the minimum
        float oppositeThreshold = -settings->thresholdInDB;
        vDSP_vsadd(This->buffer2, 1, &oppositeThreshold, buffer1, 1, framesProcessing);
        
        // apply the compression ratio
        vDSP_vsmul(buffer1,1,&settings->slope,buffer1,1,framesProcessing);
        
        // filter to get a smooth volume change envelope
        BMEnvelopeFollower_processBuffer(&This->envelopeFollower, buffer1, buffer1, framesProcessing);
        
        // negate the signal to get the dB change required to apply the compression
        vDSP_vneg(buffer1, 1, buffer1, 1, framesProcessing);
        
        // get the minimum gain set by the compressor in this chunk
        vDSP_minv(buffer1, 1, &minGainThisChunk, framesProcessing);
        
        // if this chunk min gain is less than the min gain for the whole
        // buffer, update the min gain for the buffer
        minGainWholeBuffer = MIN(minGainThisChunk,minGainWholeBuffer);
        
        // convert to linear gain control signal
        vector_fastDbToGain(buffer1,buffer1,framesProcessing);

        // apply the gain adjustment to the audio signal
        vDSP_vmul(buffer1,1,input,1,output,1,framesProcessing);
        
        // advance pointers to the next chunk
        numSamples -= framesProcessing;
        input += framesProcessing;
        output += framesProcessing;
    }
    
    *minGainDb = minGainWholeBuffer;
}




void BMCompressor_ProcessBufferStereo(BMCompressor* This,
                                      float* inputL, float* inputR,
                                      float* outputL, float* outputR,
                                      float* minGainDb, size_t numSamples){
    
    // prepare to track the minimum gain
    float minGainThisChunk;
    *minGainDb = FLT_MAX;
    
    // get a shorter name for settings
    BMCompressorSetting* settings = &This->settings;
    
    while(numSamples > 0){
        size_t framesProcessing = MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
        
        // get a shorter name for the buffer
        float* buffer1 = This->buffer1;
        
        // take the arithmetic mean of the left and right channels
        float half = 0.5;
        vDSP_vasm(inputL, 1, inputR, 1, &half, buffer1, 1, framesProcessing);
        
        // rectify the input signal
        vDSP_vabs(buffer1, 1, buffer1, 1, framesProcessing);
        
        // convert linear gain to decibel scale
        float one = 1.0f;
        vDSP_vdbcon(buffer1,1,&one,buffer1,1,framesProcessing,0);
        
        // clip values below the threshold with a soft knee
        quadraticThreshold(buffer1, This->buffer2, settings->thresholdInDB, settings->kneeWidthInDB, framesProcessing);
        
        // shift the values up so that zero is the minimum
        float oppositeThreshold = -settings->thresholdInDB;
        vDSP_vsadd(This->buffer2, 1, &oppositeThreshold, buffer1, 1, framesProcessing);
        
        // apply the compression ratio
        vDSP_vsmul(buffer1,1,&settings->slope,buffer1,1,framesProcessing);
        
        // filter to get a smooth volume change envelope
        BMEnvelopeFollower_processBuffer(&This->envelopeFollower, buffer1, buffer1, framesProcessing);
        
        // negate the signal to get the dB change required to apply the compression
        vDSP_vneg(buffer1, 1, buffer1, 1, framesProcessing);
        
        // get the minimum gain set by the compressor in this chunk
        vDSP_minv(buffer1, 1, &minGainThisChunk, framesProcessing);
        
        // if this chunk min gain is less than the min gain for the whole
        // buffer, update the min gain for the buffer
        *minGainDb = MIN(minGainThisChunk,*minGainDb);
        
        // convert to linear gain control signal
        vector_fastDbToGain(buffer1,buffer1,framesProcessing);
        
        // apply the gain adjustment to the audio signal
        vDSP_vmul(buffer1,1,inputL,1,outputL,1,framesProcessing);
        vDSP_vmul(buffer1,1,inputR,1,outputR,1,framesProcessing);
        
        // advance pointers to the next chunk
        numSamples -= framesProcessing;
        inputL += framesProcessing;
        inputR += framesProcessing;
        outputL += framesProcessing;
        outputR += framesProcessing;
    }
}


void BMCompressor_SetThresholdInDB(BMCompressor* compressor, float threshold){
    BMCompressorSetting *setting = &compressor->settings;
    setting->thresholdInDB = threshold;
}

void BMCompressor_SetKneeWidthInDB(BMCompressor* compressor, float knee){
    BMCompressorSetting *setting = &compressor->settings;
    setting->kneeWidthInDB = knee;
}

void BMCompressor_SetRatio(BMCompressor* compressor, float ratio){
    BMCompressorSetting *setting = &compressor->settings;
    setting->slope = 1.0f - (1.0f / ratio);
}

void BMCompressor_SetAttackTime(BMCompressor* compressor, float attackTime){
    BMEnvelopeFollower_setAttackTime(&compressor->envelopeFollower, compressor->settings.attackTime);
}

void BMCompressor_SetReleaseTime(BMCompressor* compressor, float releaseTime){
    BMEnvelopeFollower_setReleaseTime(&compressor->envelopeFollower, releaseTime);
}

void BMCompressor_SetSampleRate(BMCompressor* compressor, float sampleRate){
    BMEnvelopeFollower_init(&compressor->envelopeFollower, sampleRate);
    BMEnvelopeFollower_setAttackTime(&compressor->envelopeFollower, compressor->settings.attackTime);
    BMEnvelopeFollower_setReleaseTime(&compressor->envelopeFollower, compressor->settings.releaseTime);
}


