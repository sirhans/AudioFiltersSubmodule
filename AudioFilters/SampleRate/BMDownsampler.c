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
    

#define BM_DOWNSAMPLER_STOPBAND_ATTENUATION_DB BM_UPSAMPLER_STOPBAND_ATTENUATION_DB
#define BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS_FULL_SPECTRUM BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_NUMLEVELS_FULL_SPECTRUM
#define BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS_96KHZ_INPUT BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_NUMLEVELS_96KHZ_INPUT
#define BM_DOWNSAMPLER_ANTIRINGING_FILTER_BW_FULL_SPECTRUM BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_BW_FULL_SPECTRUM
#define BM_DOWNSAMPLER_ANTIRINGING_FILTER_BW_96KHZ_INPUT BM_UPSAMPLER_SECOND_STAGE_AA_FILTER_BW_96KHZ_INPUT
    
    
    
    void BMDownsampler_init(BMDownsampler* This, bool stereo, size_t downsampleFactor, enum resamplerType type){
        assert(isPowerOfTwo(downsampleFactor));
        
        This->stereo = stereo;
		This->bufferEntriesFilled = 0;
        This->downsampleFactor = downsampleFactor;
        
        if(downsampleFactor > 1){
            // the number of 2x downsampling stages is log2(upsampleFactor)
            This->numStages = log2i((uint32_t)downsampleFactor);
            
            // allocate the array of 2x downsamplers
            This->downsamplers2x = malloc(sizeof(BMIIRDownsampler2x)*This->numStages);
            
            float stage0TransitionBW = BM_UPSAMPLER_STAGE0_TRANSITION_BANDWIDTH_FULL_SPECTRUM;
            if(type == BMRESAMPLER_GUITAR) stage0TransitionBW = BM_UPSAMPLER_STAGE0_TRANSITION_BANDWIDTH_FULL_SPECTRUM;
            if(type == BMRESAMPLER_INPUT_96KHZ) stage0TransitionBW = BM_UPSAMPLER_STAGE0_TRANSITION_BANDWIDTH_96KHZ_INPUT;
            
            // initialise filters for each stage of downsampling
            for(size_t i=0; i<This->numStages; i++){
                // the transition bandwidth is narrower for later stages
                float transitionBandwidth = BMPolyphaseIIR2Designer_transitionBandwidthForStage(stage0TransitionBW, i);
                
                // the stages go in the reverse order from how they are in the upsampler
                // when sorted in terms of transition bandwidth
                BMIIRDownsampler2x_init(&This->downsamplers2x[This->numStages - i - 1], BM_DOWNSAMPLER_STOPBAND_ATTENUATION_DB, transitionBandwidth, stereo);
            }
            
            // allocate memory for buffers
            This->bufferL1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
            This->bufferL2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
			This->remnantBufferL = malloc(sizeof(float)*downsampleFactor);
            if(stereo){
                This->bufferR1 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
                This->bufferR2 = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE*downsampleFactor/2);
				This->remnantBufferR = malloc(sizeof(float)*downsampleFactor);
            } else {
                This->bufferR1 = NULL;
                This->bufferR2 = NULL;
            }
            
            // set up the anti-ringing filter
            float antiRingingFilterFc = 24000.0*(1.0 - BM_DOWNSAMPLER_ANTIRINGING_FILTER_BW_FULL_SPECTRUM);
            size_t numLevels = BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS_FULL_SPECTRUM;
            if(type == BMRESAMPLER_GUITAR){
                antiRingingFilterFc = 24000.0*(1.0 - BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS_96KHZ_INPUT);
                numLevels = BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS_96KHZ_INPUT;
            }
            if(type == BMRESAMPLER_INPUT_96KHZ){
                antiRingingFilterFc = 24000.0*(1.0 - BM_DOWNSAMPLER_ANTIRINGING_FILTER_BW_96KHZ_INPUT);
                numLevels = BM_DOWNSAMPLER_ANTIRINGING_FILTER_NUMLEVELS_96KHZ_INPUT;
            }
            BMMultiLevelBiquad_init(&This->antiRingingFilter, numLevels, 96000.0, stereo, true, false);
            BMMultiLevelBiquad_setLegendreLP(&This->antiRingingFilter, antiRingingFilterFc, 0, numLevels);
        }
    }
    
    
    
    
    
    /*
     * upsample a buffer that contains a single channel of audio samples
     *
     * @param input    length = numSamplesIn
     * @param output   length = numSamplesIn / upsampleFactor
     * @param numSamplesIn  number of input samples to process
     */
    void BMDownsampler_processBufferMono(BMDownsampler *This, float* input, float* output, size_t numSamplesIn){
        
        // if the downsample factor is non-trivial
        if(This->downsampleFactor > 1){
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
                    if (outputToBuffer1) {
                        BMMultiLevelBiquad_processBufferMono(&This->antiRingingFilter, This->bufferL2, This->bufferL2, inputSize);
                        BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[This->numStages-1], This->bufferL2, output, inputSize);
                    }
                    else {
                        BMMultiLevelBiquad_processBufferMono(&This->antiRingingFilter, This->bufferL1, This->bufferL1, inputSize);
                        BMIIRDownsampler2x_processBufferMono(&This->downsamplers2x[This->numStages-1], This->bufferL1, output, inputSize);
                    }
                }
                
                numSamplesIn -= samplesProcessing;
                input += samplesProcessing;
                output += samplesProcessing/This->downsampleFactor;
            }
        }
        else {
            // bypass the downsampler
			if(output != input)
				memcpy(output,input,numSamplesIn * sizeof(float));
        }
    }
    
	
	
	void BMDownsampler_processBufferStereoOddInputLength(BMDownsampler *This, 
														 float* inputL, float* inputR,
														 float* outputL, float* outputR,
														 size_t numSamplesIn, size_t *numSamplesOut){
		// refuse to process empty input
		if(numSamplesIn == 0) {
			*numSamplesOut = 0;
			return;
		}
		
		/***********************************************************************
		 * if the input buffer length is divisible by the downsample factor
		 * and the buffer is empty then...
		 **********************************************************************/
		if((numSamplesIn % This->downsampleFactor) == 0 && This->bufferEntriesFilled == 0){
			// process as usual
			BMDownsampler_processBufferStereo(This, inputL, inputR, outputL, outputR, numSamplesIn);
		
			// inform the calling function about the length of the output
			*numSamplesOut = numSamplesIn / This->downsampleFactor;
			
			return;
		}
		
		
	
		size_t remnantBufferLength = This->downsampleFactor;
		
		
//		
//		/***********************************************************************
//		 * if the input buffer length is divisible by the downsample factor
//		 * and the buffer is occupied then...
//		 **********************************************************************/
//		if(numSamplesIn % This->downsampleFactor == 0 && This->bufferEntriesFilled > 0){
//			size_t samplesToFillRemnantBuffer = remnantBufferLength - This->bufferEntriesFilled;
//			
//			// copy the first samples of input into the back of the buffer until it's filled
//			for(size_t i=0; i < samplesToFillRemnantBuffer; i++){
//				This->remnantBufferL[i+This->bufferEntriesFilled] = inputL[i];
//				This->remnantBufferR[i+This->bufferEntriesFilled] = inputR[i];
//			}
//			
//			// process all the samples in the buffer to get one sample of output
//			BMDownsampler_processBufferStereo(This,
//											  This->remnantBufferL, This->remnantBufferR,
//											  outputL, outputR,
//											  remnantBufferLength);
//			
//			// calculate the number of samples left to process and the number to cache
//			// in the remnant buffer
//			size_t inputSamplesRemaining = numSamplesIn - samplesToFillRemnantBuffer;
//			size_t remnantLength = inputSamplesRemaining % This->downsampleFactor;
//			size_t inputSamplesToDownsample = inputSamplesRemaining - remnantLength;
//			
//			// if there are enough samples in the input to downsample
//			BMDownsampler_processBufferStereo(This,
//											  inputL + remnantLength, inputR + remnantLength,
//											  outputL + 1, outputR + 1,
//											  inputSamplesToDownsample);
//			
//			// copy the last remnantLength samples into the front of the buffer
//			for(size_t i=0; i < remnantLength; i++){
//				This->remnantBufferL[i] = inputL[numSamplesIn - This->bufferEntriesFilled + i];
//				This->remnantBufferL[i] = inputR[numSamplesIn - This->bufferEntriesFilled + i];
//			}
//			
//			// inform the calling function about the length of the output
//			*numSamplesOut = (remnantBufferLength + inputSamplesToDownsample) / This->downsampleFactor;
//			
//			return;
//		}
		
		
		
		
		/***********************************************************************
		 * if the input buffer length is not divisible
		 * and the buffer is empty then...
		 **********************************************************************/
		if((numSamplesIn % This->downsampleFactor) != 0 && This->bufferEntriesFilled == 0){
			size_t remnantLength = numSamplesIn % This->downsampleFactor;
			
			// process the first part of the input (leaving back the remnant)
			BMDownsampler_processBufferStereo(This,
											  inputL, inputR,
											  outputL, outputR, 
											  numSamplesIn - remnantLength);
			
			// copy the last samples to the front of the buffer
			for(size_t i=0; i<remnantLength; i++){
				This->remnantBufferL[i] = inputL[numSamplesIn - remnantLength + i];
				This->remnantBufferR[i] = inputR[numSamplesIn - remnantLength + i];
			}
			
			// set the number of entries of the remnant buffer that are occupied
			This->bufferEntriesFilled = remnantLength;
			
			// inform the calling function about the length of the output
			*numSamplesOut = (numSamplesIn-remnantLength) / This->downsampleFactor;
			
			return;
		}
		
		
		
		
		
		/***********************************************************************
		 * if we're still going and the buffer is not empty then...
		 **********************************************************************/
		//if((numSamplesIn % This->downsampleFactor) != 0 && This->bufferEntriesFilled > 0){
		if(This->bufferEntriesFilled > 0){
			size_t bufferEntriesEmpty = remnantBufferLength - This->bufferEntriesFilled;
			
			// if there's not enough input to fill the remnant buffer
			if(bufferEntriesEmpty > numSamplesIn){
				// copy what we have and stop without processing anything
				for(size_t i=0; i < numSamplesIn; i++){
					This->remnantBufferL[i+This->bufferEntriesFilled] = inputL[i];
					This->remnantBufferR[i+This->bufferEntriesFilled] = inputR[i];
				}
				
				// update the number of samples occupied
				This->bufferEntriesFilled += numSamplesIn;
				
				// and we didn't get any output
				*numSamplesOut = 0;
				
			// else, if there's enough input to fill the remnant buffer
			} else {
				
				// copy the first samples of input into the back of the buffer until it's filled
				for(size_t i=0; i < bufferEntriesEmpty; i++){
					This->remnantBufferL[i+This->bufferEntriesFilled] = inputL[i];
					This->remnantBufferR[i+This->bufferEntriesFilled] = inputR[i];
				}
				
				// process the downsampleFactor samples in the buffer to get one sample of output
				BMDownsampler_processBufferStereo(This,
												  This->remnantBufferL, This->remnantBufferR,
												  outputL, outputR,
												  remnantBufferLength);
				
				
				// if the input had exactly enough to fill the buffer
				if (numSamplesIn == bufferEntriesEmpty){
					// then clear the remnant buffer
					This->bufferEntriesFilled = 0;
					
					// and we only got one output sample
					*numSamplesOut = 1;
					
				// if the buffer isn't exactly filled by the input then we have a new remnant
				} else {
					// calculate how many samples will remain un-processed at the end
					size_t remnantLength2 = (numSamplesIn - bufferEntriesEmpty) % This->downsampleFactor;
					
					// calculate the number of samples to process now
					size_t inputSamplesProcessing = numSamplesIn - (bufferEntriesEmpty + remnantLength2);
					
					// process the samples in the input buffers, possibly leaving another remnant
					BMDownsampler_processBufferStereo(This,
													  inputL + bufferEntriesEmpty, inputR + bufferEntriesEmpty,
													  outputL + 1, outputR + 1,
													  inputSamplesProcessing);
					
					// copy the second remnant into the remnant buffer
					for(size_t i=0; i<remnantLength2; i++){
						This->remnantBufferL[i] = inputL[numSamplesIn - remnantLength2 + i];
						This->remnantBufferR[i] = inputR[numSamplesIn - remnantLength2 + i];
					}
					
					// set the number of elements in the remnant buffer
					This->bufferEntriesFilled = remnantLength2;
					
					// inform the calling function about the length of the output
					*numSamplesOut = (inputSamplesProcessing + remnantBufferLength) / This->downsampleFactor;
				}
			}
			
			return;
		}
		
		// if we get here then there was an error
		assert(false);
	}
	
    
    
    void BMDownsampler_processBufferStereo(BMDownsampler *This, float* inputL, float* inputR, float* outputL, float* outputR, size_t numSamplesIn){
        
        // if the downsample factor is non-trivial
        if(This->downsampleFactor > 1){
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
        else {
            // bypass the downsampler
            memcpy(outputL, inputL, sizeof(float)*numSamplesIn);
            memcpy(outputR, inputR, sizeof(float)*numSamplesIn);
        }
    }
    
    
    
    
    void BMDownsampler_free(BMDownsampler *This){
        // free internal memory for each 2x stage
        for(size_t i=0; i<This->numStages; i++)
            BMIIRDownsampler2x_free(&This->downsamplers2x[i]);
        
        BMMultiLevelBiquad_free(&This->antiRingingFilter);
        
        // free the stages array
        free(This->downsamplers2x);
        This->downsamplers2x = NULL;
        
        free(This->bufferL1);
        free(This->bufferL2);
		free(This->remnantBufferL);
        if(This->stereo){
            free(This->bufferR1);
            free(This->bufferR2);
			free(This->remnantBufferR);
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
	
	
	
	
	float BMDownsampler_getLatencyInSamples(BMDownsampler *This){
		// latency is frequency dependent. We will check it at this frequency
		float groupDelayTestFrequency = 300.0f;
		
		// get the latency at each stage
		float latency = 0.0f;
		for(size_t i=0; i<This->numStages; i++){
			float stageILatencyInSamples = BMMultiLevelBiquad_groupDelay(&This->downsamplers2x[i].even, groupDelayTestFrequency);
			float stageIOversampleFactor = powf(2.0f,(float)(This->numStages - i - 1));
			latency += stageILatencyInSamples / stageIOversampleFactor;
		}
		
		// add the latency of the anti-ringing filter
		latency += BMMultiLevelBiquad_groupDelay(&This->antiRingingFilter, groupDelayTestFrequency);
		
		return latency;
	}
	
	
    
    
#ifdef __cplusplus
}
#endif

