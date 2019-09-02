//
//  BMUpsampler.c
//  BMAudioFilters
//
//  Created by Hans on 10/9/17.
//  This file may be used by anyone without any restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include "BMUpsampler.h"
#include "BMIntegerMath.h"
#include "BMPolyphaseIIR2Designer.h"
#include "Constants.h"
    
#define BM_UPSAMPLER_STAGE0_TRANSITION_BANDWIDTH 0.05
#define BM_UPSAMPLER_STOPBAND_ATTENUATION_DB 110.0
#define BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_NUMLEVELS 4
#define BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_FC 19750.0

    
    
    void BMUpsampler_init(BMUpsampler* This, bool stereo, size_t upsampleFactor){
        assert(isPowerOfTwo(upsampleFactor));
        
        This->upsampleFactor = upsampleFactor;
        
        // the number of 2x upsampling stages is log2(upsampleFactor)
        This->numStages = log2i((uint32_t)upsampleFactor);
        
        // allocate the array of 2x upsamplers
        This->upsamplers2x = malloc(sizeof(BMIIRUpsampler2x)*This->numStages);
        
        // initialise each stage of upsampling
        for(size_t i=0; i<This->numStages; i++){
            // the transition bandwidth is wider for later stages
            float transitionBandwidth = BMPolyphaseIIR2Designer_transitionBandwidthForStage(BM_UPSAMPLER_STAGE0_TRANSITION_BANDWIDTH,
                                                                i);
            BMIIRUpsampler2x_init(&This->upsamplers2x[i], BM_UPSAMPLER_STOPBAND_ATTENUATION_DB, transitionBandwidth, stereo);
        }
        
        // allocate memory for buffers
        This->bufferL = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*upsampleFactor/2);
        if(stereo)
            This->bufferR = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*upsampleFactor/2);
        else
            This->bufferR = NULL;
        
        BMMultiLevelBiquad_init(&This->secondStageAAFilter, BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_NUMLEVELS, 96000.0, true, false, false);
        BMMultiLevelBiquad_setLegendreLP(&This->secondStageAAFilter, BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_FC, 0, BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_NUMLEVELS);
    }
    
    
    
    
    
    /*
     * upsample a buffer that contains a single channel of audio samples
     *
     * @param input    length = numSamplesIn
     * @param output   length = numSamplesIn * upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMUpsampler_processBufferMono(BMUpsampler* This, const float* input, float* output, size_t numSamplesIn){
        
        while(numSamplesIn > 0){
            size_t samplesProcessing = BM_MIN(numSamplesIn, BM_BUFFER_CHUNK_SIZE);
            size_t inputSize = samplesProcessing;
            
            // we have only one buffer and the process functions don't work in place
            // so we have to alternate between using the internal buffer and using
            // the output array as a buffer
            bool outputToBuffer = This->numStages % 2 == 0;
            
            // process stage 0
            if(outputToBuffer)
                BMIIRUpsampler2x_processBufferMono(&This->upsamplers2x[0], input, This->bufferL, inputSize);
            else
                BMIIRUpsampler2x_processBufferMono(&This->upsamplers2x[0], input, output, inputSize);
            outputToBuffer = !outputToBuffer;
            
            // process other stages if they are available
            for(size_t i=1; i<This->numStages; i++){
                inputSize *= 2;
                
                // alternate between caching in the buffer and in the output for even and odd numbered stages;
                if(outputToBuffer)
                    BMIIRUpsampler2x_processBufferMono(&This->upsamplers2x[i], output, This->bufferL, inputSize);
                else {
                    BMIIRUpsampler2x_processBufferMono(&This->upsamplers2x[i], This->bufferL, output, inputSize);
                }
                outputToBuffer = !outputToBuffer;
            }
            
            numSamplesIn -= samplesProcessing;
            input += samplesProcessing;
            output += samplesProcessing*This->upsampleFactor;
        }
    }

    
    
    void BMUpsampler_processBufferStereo(BMUpsampler* This, const float* inputL, const float* inputR, float* outputL, float* outputR, size_t numSamplesIn){
        
        while(numSamplesIn > 0){
            size_t samplesProcessing = BM_MIN(numSamplesIn, BM_BUFFER_CHUNK_SIZE);
            size_t inputSize = samplesProcessing;
            
            // we have only one buffer and the process functions don't work in place
            // so we have to alternate between using the internal buffer and using
            // the output array as a buffer
            bool outputToBuffer = This->numStages % 2 == 0;
            
            // process stage 0
            if(outputToBuffer){
                BMIIRUpsampler2x_processBufferStereo(&This->upsamplers2x[0], inputL, inputR, This->bufferL, This->bufferR, inputSize);
                BMMultiLevelBiquad_processBufferStereo(&This->secondStageAAFilter, This->bufferL, This->bufferR, This->bufferL, This->bufferR, inputSize*2);
            }
            else{
                BMIIRUpsampler2x_processBufferStereo(&This->upsamplers2x[0], inputL, inputR, outputL, outputR, inputSize);
                BMMultiLevelBiquad_processBufferStereo(&This->secondStageAAFilter, outputL, outputR, outputL, outputR, inputSize*2);
            }
            outputToBuffer = !outputToBuffer;
            
            // process other stages if they are available
            for(size_t i=1; i<This->numStages; i++){
                inputSize *= 2;
                
                // alternate between caching in the buffer and in the output for even and odd numbered stages;
                if(outputToBuffer)
                    BMIIRUpsampler2x_processBufferStereo(&This->upsamplers2x[i], outputL, outputR, This->bufferL, This->bufferR, inputSize);
                else {
                    BMIIRUpsampler2x_processBufferStereo(&This->upsamplers2x[i], This->bufferL, This->bufferR, outputL, outputR, inputSize);
                }
                outputToBuffer = !outputToBuffer;
            }
            
            numSamplesIn -= samplesProcessing;
            inputL += samplesProcessing;
            inputR += samplesProcessing;
            outputL += samplesProcessing*This->upsampleFactor;
            outputR += samplesProcessing*This->upsampleFactor;
        }
    }
    
    
    
    
    void BMUpsampler_free(BMUpsampler* This){
        // free internal memory for each 2x stage
        for(size_t i=0; i<This->numStages; i++)
            BMIIRUpsampler2x_free(&This->upsamplers2x[i]);
        
        BMMultiLevelBiquad_destroy(&This->secondStageAAFilter);
        
        // free the stages array
        free(This->upsamplers2x);
        This->upsamplers2x = NULL;
        
        free(This->bufferL);
        free(This->bufferR);
        This->bufferL = NULL;
        This->bufferR = NULL;
        
    }
    
    
    
    
    void BMUpsampler_impulseResponse(BMUpsampler* This, float* IR, size_t IRLength){
        // IRLength must be divisible by the upsampling factor
        assert(IRLength % This->upsampleFactor);
        
        // the input length is the output length / the upsample factor
        size_t inputLength = IRLength / This->upsampleFactor;
        
        // allocate the array for the impulse input
        float* impulse = malloc(sizeof(float)*inputLength);
        
        // set the input to all zeros
        memset(impulse,0,sizeof(float)*inputLength);
        
        // set the first sample of input to 1.0f
        impulse[0] = 1.0f;
        
        // process the impulse
        BMUpsampler_processBufferMono(This, impulse, IR, inputLength);
    }
    
    
#ifdef __cplusplus
}
#endif
