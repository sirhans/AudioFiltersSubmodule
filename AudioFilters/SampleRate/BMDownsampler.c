//
//  BMDownsampler.c
//  BMAudioFilters
//
//  Created by Hans on 13/9/17.
//  Anyone may use this file without restrictions
//

#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>
#include "BMDownsampler.h"
#include "BMIntegerMath.h"
#include "BMPolyphaseIIR2Designer.h"
#include "Constants.h"
    
#define BM_DOWNSAMPLER_STAGE0_TRANSITION_BANDWIDTH 0.05
#define BM_DOWNSAMPLER_STOPBAND_ATTENUATION_DB 110.0
#define BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS 4
#define BM_DOWNSAMPLER_ANTIRINGING_FILTER_FC 19750.0
    
    
    
    void BMDownsampler_init(BMDownsampler *This, bool stereo, size_t downsampleFactor){
        assert(isPowerOfTwo(downsampleFactor));
        
        This->stereo = stereo;
        This->downsampleFactor = downsampleFactor;
        
        // the number of 2x downsampling stages is log2(upsampleFactor)
        This->numStages = log2i((uint32_t)downsampleFactor);
        
        // allocate the array of 2x downsamplers
        This->downsamplers2x = malloc(sizeof(BMIIRDownsampler2x)*This->numStages);
        
        // initialise filters for each stage of downsampling
        for(size_t i=0; i<This->numStages; i++){
            // the transition bandwidth is narrower for later stages
            float transitionBandwidth = BMPolyphaseIIR2Designer_transitionBandwidthForStage(BM_DOWNSAMPLER_STAGE0_TRANSITION_BANDWIDTH, i);
            
            // the stages go in the reverse order from how they are in the upsampler
            // when sorted in terms of transition bandwidth
            BMIIRDownsampler2x_init(&This->downsamplers2x[This->numStages - i - 1], BM_DOWNSAMPLER_STOPBAND_ATTENUATION_DB, transitionBandwidth, stereo);
        }
        
        // allocate memory for buffers
        This->bufferL1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
        This->bufferL2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
        if(stereo){
            This->bufferR1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
            This->bufferR2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
        } else {
            This->bufferR1 = NULL;
            This->bufferR2 = NULL;
        }
        
        BMMultiLevelBiquad_init(&This->antiRingingFilter, BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS, 96000.0, true, false, false);
        BMMultiLevelBiquad_setLegendreLP(&This->antiRingingFilter, BM_DOWNSAMPLER_ANTIRINGING_FILTER_FC, 0, BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS);
    }
    
    
    
    
    
    /*
     * upsample a buffer that contains a single channel of audio samples
     *
     * @param input    length = numSamplesIn
     * @param output   length = numSamplesIn / upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMDownsampler_processBufferMono(BMDownsampler *This, float* input, float* output, size_t numSamplesIn){
        // the input length must be divisible by the downsample factor
        assert(numSamplesIn % This->downsampleFactor == 0);
        
        while(numSamplesIn > 0){
            size_t samplesProcessing = BM_MIN(numSamplesIn, BM_BUFFER_CHUNK_SIZE*This->downsampleFactor);
            size_t inputSize = samplesProcessing;
        
            if(This->numStages == 1){
                BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[0], input, output, inputSize);
            }
            
            // if there is more than one stage
            else {
                // we have only one buffer and the process functions don't work in place
                // so we have to alternate between using the internal buffer and using
                // the output array as a buffer
                bool outputToBuffer1 = This->numStages % 2 == 0;
                
                // process stage 0
                if(outputToBuffer1)
                    BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[0], input, This->bufferL1, inputSize);
                else
                    BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[0], input, This->bufferL2, inputSize);
                outputToBuffer1 = !outputToBuffer1;
                
                // process other stages if they are available
                for(size_t i=1; i<This->numStages-1; i++){
                    inputSize /= 2;
                    
                    // alternate between caching in the buffer and in the output for even and odd numbered stages;
                    if(outputToBuffer1)
                        BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[i], This->bufferL2, This->bufferL1, inputSize);
                    else {
                        BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[i], This->bufferL1, This->bufferL2, inputSize);
                    }
                    outputToBuffer1 = !outputToBuffer1;
                }
                
                // when we reach the last stage, output straight to the output buffer
                inputSize /= 2;
                if(outputToBuffer1)
                    BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[This->numStages-1], This->bufferL2, output, inputSize);
                else
                    BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[This->numStages-1], This->bufferL1, output, inputSize);
                
            }
            
            numSamplesIn -= samplesProcessing;
            input += samplesProcessing;
            output += samplesProcessing/This->downsampleFactor;
        }
    }
    
    
    
    void BMDownsampler_processBufferStereo(BMDownsampler *This, float* inputL, float* inputR, float* outputL, float* outputR, size_t numSamplesIn){
        // the input length must be divisible by the downsample factor
        assert(numSamplesIn % This->downsampleFactor == 0);
        
        while(numSamplesIn > 0){
            size_t samplesProcessing = BM_MIN(numSamplesIn, BM_BUFFER_CHUNK_SIZE*This->downsampleFactor);
            size_t inputSize = samplesProcessing;
            
            if(This->numStages == 1){
                BMIIRDownsampler2x_processBufferStereo(&This->downsamplers2x[0], inputL, inputR, outputL, outputR, inputSize);
            }
            
            // if there is more than one stage
            else {
                // we have only one buffer and the process functions don't work in place
                // so we have to alternate between using the internal buffer and using
                // the output array as a buffer
                bool outputToBuffer1 = This->numStages % 2 == 0;
                
                // process stage 0
                if(outputToBuffer1)
                    BMIIRDownsampler2x_processBufferStereo(&This->downsamplers2x[0], inputL, inputR, This->bufferL1, This->bufferR1, inputSize);
                else
                    BMIIRDownsampler2x_processBufferStereo(&This->downsamplers2x[0], inputL, inputR, This->bufferL2, This->bufferR2, inputSize);
                outputToBuffer1 = !outputToBuffer1;
                
                // process other stages if they are available
                for(size_t i=1; i<This->numStages-1; i++){
                    inputSize /= 2;
                    
                    // alternate between caching in the buffer and in the output for even and odd numbered stages;
                    if(outputToBuffer1)
                        BMIIRDownsampler2x_processBufferStereo(&This->downsamplers2x[i], This->bufferL2, This->bufferR2, This->bufferL1, This->bufferR1, inputSize);
                    else {
                        BMIIRDownsampler2x_processBufferStereo(&This->downsamplers2x[i], This->bufferL1, This->bufferR1, This->bufferL2, This->bufferR2, inputSize);
                    }
                    outputToBuffer1 = !outputToBuffer1;
                }
                
                // when we reach the last stage, output straight to the output buffer
                inputSize /= 2;
                if(outputToBuffer1){
                    BMMultiLevelBiquad_processBufferStereo(&This->antiRingingFilter, This->bufferL2, This->bufferR2, This->bufferL2, This->bufferR2, inputSize);
                    BMIIRDownsampler2x_processBufferStereo(&This->downsamplers2x[This->numStages-1], This->bufferL2, This->bufferR2, outputL, outputR, inputSize);
                }
                else{
                    BMMultiLevelBiquad_processBufferStereo(&This->antiRingingFilter, This->bufferL1, This->bufferR1, This->bufferL1, This->bufferR1, inputSize);
                    BMIIRDownsampler2x_processBufferStereo(&This->downsamplers2x[This->numStages-1], This->bufferL1, This->bufferR1, outputL, outputR, inputSize);
                }
                
            }
            
            numSamplesIn -= samplesProcessing;
            inputL += samplesProcessing;
            inputR += samplesProcessing;
            outputL += samplesProcessing/This->downsampleFactor;
            outputR += samplesProcessing/This->downsampleFactor;
        }
    }
    
    
    
    
    void BMDownsampler_free(BMDownsampler *This){
        // free internal memory for each 2x stage
        for(size_t i=0; i<This->numStages; i++)
            BMIIRDownsampler2x_free(&This->downsamplers2x[i]);
        
        BMMultiLevelBiquad_destroy(&This->antiRingingFilter);
        
        // free the stages array
        free(This->downsamplers2x);
        This->downsamplers2x = NULL;
        
        free(This->bufferL1);
        free(This->bufferL2);
        if(This->stereo){
            free(This->bufferR1);
            free(This->bufferR2);
        }
        
        This->bufferL1 = NULL;
        This->bufferR1 = NULL;
        This->bufferL2 = NULL;
        This->bufferR2 = NULL;
    }
    
    
    
    
    void BMDownsampler_impulseResponse(BMDownsampler *This, float* IR, size_t IRLength){
        // the input length is the output length * the upsample factor
        size_t inputLength = IRLength  *This->downsampleFactor;
        
        // allocate the array for the impulse input
        float* impulse = malloc(sizeof(float)*inputLength);
        
        // set the input to all zeros
        memset(impulse,0,sizeof(float)*inputLength);
        
        // set the first sample of input to 1.0f
        impulse[0] = 1.0f;
        
        // process the impulse
        BMDownsampler_processBufferMono(This, impulse, IR, inputLength);
    }
    
    
#ifdef __cplusplus
}
#endif

