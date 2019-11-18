//
//  BMBiquadArray.c
//  BMAudioFilters
//
//  Created by hans anderson on 1/1/18.
//  Anyone may use this file without restrictions
//

#include "BMBiquadArray.h"


#ifdef __cplusplus
extern "C" {
#endif

    
    
    // forward declarations
    void BMBiquadArray_setHighDecayFDN(BMBiquadArray *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    void BMBiquadArray_setLowDecayFDN(BMBiquadArray *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    void BMBiquadArray_setBypass(float* b0, float* b1, float* b2, float* a1, float* a2);
    void BMBiquadArray4_setHighDecayFDN(BMBiquadArray4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    void BMBiquadArray4_setLowDecayFDN(BMBiquadArray4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    void BMBiquadArray4_setBypass(simd_float4* b0, simd_float4* b1, simd_float4* b2, simd_float4* a1, simd_float4* a2);
    
    
    
    /*
      *This function initialises memory and puts all the filters
     * into bypass mode.
     */
    void BMBiquadArray_init(BMBiquadArray *This, size_t numChannels, float sampleRate){
        This->sampleRate = sampleRate;
        This->numChannels = numChannels;
        
        // malloc filter coefficients
        This->a1neg = malloc(sizeof(float)*numChannels);
        This->a2neg = malloc(sizeof(float)*numChannels);
        This->b0 = malloc(sizeof(float)*numChannels);
        This->b1 = malloc(sizeof(float)*numChannels);
        This->b2 = malloc(sizeof(float)*numChannels);
        
        // malloc delays
        This->x1 = malloc(sizeof(float)*numChannels);
        This->x2 = malloc(sizeof(float)*numChannels);
        This->y1 = malloc(sizeof(float)*numChannels);
        This->y2 = malloc(sizeof(float)*numChannels);
        
        // clear delays
        memset(This->x1,0,sizeof(float)*numChannels);
        memset(This->x2,0,sizeof(float)*numChannels);
        memset(This->y1,0,sizeof(float)*numChannels);
        memset(This->y2,0,sizeof(float)*numChannels);
        
        // bypass all the filters
        for(size_t i=0; i<numChannels; i++)
            BMBiquadArray_setBypass(&This->b0[i], &This->b1[i], &This->b2[i],
                      &This->a1neg[i], &This->a2neg[i]);
    }
    
	
	void BMBiquadArray4_init(BMBiquadArray4 *This, size_t numChannels, float sampleRate){
        This->sampleRate = sampleRate;
        This->numChannels = numChannels;
		This->numChannelsOver4 = numChannels / 4;
        
        // malloc filter coefficients
        This->a1neg = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        This->a2neg = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        This->b0 = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        This->b1 = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        This->b2 = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        
        // malloc delays
        This->x1 = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        This->x2 = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        This->y1 = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        This->y2 = malloc(sizeof(simd_float4)*This->numChannelsOver4);
        
        // clear delays
        memset(This->x1,0,sizeof(simd_float4)*This->numChannelsOver4);
        memset(This->x2,0,sizeof(simd_float4)*This->numChannelsOver4);
        memset(This->y1,0,sizeof(simd_float4)*This->numChannelsOver4);
        memset(This->y2,0,sizeof(simd_float4)*This->numChannelsOver4);
        
        // bypass all the filters
        for(size_t i=0; i<This->numChannelsOver4; i++)
            BMBiquadArray4_setBypass(&This->b0[i], &This->b1[i], &This->b2[i],
                      &This->a1neg[i], &This->a2neg[i]);
    }
    
    

    
    

    
    /*
     * Set the given coefficients to bypass the filter
     */
    void BMBiquadArray_setBypass(float* b0, float* b1, float* b2, float* a1, float* a2){
        *b0 = 1.0;
        *b1 = *b2 = *a1 = *a2 = 0.0;
    }
	
	
	
    /*
     * Set the given coefficients to bypass the filter
     */
    void BMBiquadArray4_setBypass(simd_float4 *b0,
					simd_float4 *b1,
					simd_float4 *b2,
					simd_float4 *a1,
					simd_float4 *a2){
        *b0 = 1.0f;
        *b1 = *b2 = *a1 = *a2 = 0.0f;
    }
    
    
	
	
    
	void setHighShelfSingle(float *b0, float* b1, float* b2, float* a1Neg, float* a2Neg, float fc, float gain, float sampleRate){
		
		double gamma = tanf(M_PI * fc / sampleRate);
		double gamma_2 = gamma*gamma;
		double sqrt_gain = sqrt(gain);
		
		// conditionally set G
		double G;
		if (gain > 2.0) G = gain * M_SQRT2 * 0.5;
		else {
			if (gain >= 0.5) G = sqrt_gain;
			else G = gain * M_SQRT2;
		}
		double G_2 = G*G;
		
		// compute reuseable variables
		double g_d = powf((G_2 - 1.0)/(gain*gain - G_2), 0.25);
		double g_d_2 = g_d*g_d;
		double g_n = g_d * sqrt_gain;
		double g_n_2 = g_n * g_n;
		double sqrt_2_g_d_gamma = M_SQRT2 * g_d * gamma;
		double sqrt_2_g_n_gamma = M_SQRT2 * g_n * gamma;
		double gamma_2_plus_g_d_2 = gamma_2 + g_d_2;
		double gamma_2_plus_g_n_2 = gamma_2 + g_n_2;
		double one_over_denominator = 1.0 / (gamma_2_plus_g_d_2 + sqrt_2_g_d_gamma);
		
		
		// set the filter coefficients
		*b0 = (gamma_2_plus_g_n_2 + sqrt_2_g_n_gamma) * one_over_denominator;
		*b1 = 2.0 * (gamma_2 - g_n_2) * one_over_denominator;
		*b2 = (gamma_2_plus_g_n_2 - sqrt_2_g_n_gamma) * one_over_denominator;
		
		*a1Neg = -2.0 * (gamma_2 - g_d_2) * one_over_denominator;
		*a2Neg = -(gamma_2_plus_g_d_2 - sqrt_2_g_d_gamma)*one_over_denominator;
		
		
		/*
		 * the formulae above do not work when the gain is 1.0, so for
		 * that case we bypass the filter
		 */
		if (gain == 1.0) BMBiquadArray_setBypass(b0, b1, b2, a1Neg, a2Neg);
	}
	
    
    
	
    
    /*
     * @param gain         set gain for each channel (linear scale)
     * @param fc           cutoff frequency (Hz)
     * @param numChannels  length(gain) == numChannels
     */
    void BMBiquadArray_setHighShelf(BMBiquadArray *This, float* gains, float fc, size_t numChannels){
        assert(numChannels <= This->numChannels);
        
        for (size_t i=0; i<numChannels; i++){
            // make nice names for the filter coefficients
            float* b0 = This->b0 + i;
            float* b1 = This->b1 + i;
            float* b2 = This->b2 + i;
            float* a1Neg = This->a1neg + i;
            float* a2Neg = This->a2neg + i;
            
			setHighShelfSingle(b0,b1,b2,a1Neg,a2Neg,
							   fc, gains[i],This->sampleRate);
        }
    }
   
	
	
	
	
	
	void BMBiquadArray4_setHighShelf(BMBiquadArray4 *This, float *gains, float fc, size_t numChannels){
        assert(numChannels <= This->numChannels);
        
        for (size_t i=0; i<numChannels/4; i++){
			// get the coefficients for a set of four filters
			float b0x, b1x, b2x, a1Negx, a2Negx;
			setHighShelfSingle(&b0x,&b1x,&b2x,&a1Negx,&a2Negx,fc,gains[i*4 + 0],This->sampleRate);
			float b0y, b1y, b2y, a1Negy, a2Negy;
			setHighShelfSingle(&b0y,&b1y,&b2y,&a1Negy,&a2Negy,fc,gains[i*4 + 1],This->sampleRate);
			float b0z, b1z, b2z, a1Negz, a2Negz;
			setHighShelfSingle(&b0z,&b1z,&b2z,&a1Negz,&a2Negz,fc,gains[i*4 + 2],This->sampleRate);
			float b0w, b1w, b2w, a1Negw, a2Negw;
			setHighShelfSingle(&b0w,&b1w,&b2w,&a1Negw,&a2Negw,fc,gains[i*4 + 3],This->sampleRate);
			
			// create vectors from the floats
			This->b0[i] = simd_make_float4(b0x, b0y, b0z, b0w);
			This->b1[i] = simd_make_float4(b1x, b1y, b1z, b1w);
			This->b2[i] = simd_make_float4(b2x, b2y, b2z, b2w);
			This->a1neg[i] = simd_make_float4(a1Negx, a1Negy, a1Negz, a1Negw);
			This->a2neg[i] = simd_make_float4(a2Negx, a2Negy, a2Negz, a2Negw);
        }
    }
    
    
	
	
	
	void setLowShelfSingle(float *b0, float* b1, float* b2, float* a1Neg, float* a2Neg, float fc, float gain, float sampleRate){
		// compute some preliminary values
		double gamma = tanf(M_PI * fc / sampleRate);
		double gamma_2 = gamma*gamma;
		double sqrt_gain = sqrt(gain);
		
		// conditionally set G
		double G;
		if (gain > 2.0) G = gain * M_SQRT2 * 0.5;
		else {
			if (gain >= 0.5) G = sqrt_gain;
			else G = gain * M_SQRT2;
		}
		double G_2 = G*G;
		
		// compute reuseable variables
		double g_d = powf((G_2 - 1.0)/(gain*gain - G_2), 0.25);
		double g_d_2 = g_d*g_d;
		double g_n = g_d * sqrt_gain;
		double g_n_2 = g_n * g_n;
		double g_n_2_gamma_2 = g_n_2 * gamma_2;
		double g_d_2_gamma_2 = g_d_2 * gamma_2;
		double sqrt_2_g_d_gamma = M_SQRT2 * g_d * gamma;
		double sqrt_2_g_n_gamma = M_SQRT2 * g_n * gamma;
		double g_d_2_gamma_2_plus_1 = g_d_2_gamma_2 + 1.0;
		double g_n_2_gamma_2_plus_1 = g_n_2_gamma_2 + 1.0;
		double one_over_denominator = 1.0 / (g_d_2_gamma_2_plus_1 + sqrt_2_g_d_gamma);
		
		
		// set the filter coefficients
		*b0 = (g_n_2_gamma_2_plus_1 + sqrt_2_g_n_gamma) * one_over_denominator;
		*b1 = 2.0 * (g_n_2_gamma_2 - 1.0) * one_over_denominator;
		*b2 = (g_n_2_gamma_2_plus_1 - sqrt_2_g_n_gamma) * one_over_denominator;
		
		*a1Neg = -2.0 * (g_d_2_gamma_2 - 1.0) * one_over_denominator;
		*a2Neg = -(g_d_2_gamma_2_plus_1 - sqrt_2_g_d_gamma)*one_over_denominator;
		
		
		/*
		 * the formulae above do not work when the gain is 1.0, so for
		 * that case we bypass the filter
		 */
		if (gain == 1.0) BMBiquadArray_setBypass(b0, b1, b2, a1Neg, a2Neg);
	}
	
	
	
    
    

    /*
     * @param gain         set gain for each channel (linear scale)
     * @param fc           cutoff frequency (Hz)
     * @param numChannels  length(gain) == numChannels
     */
    void BMBiquadArray_setLowShelf(BMBiquadArray *This, float* gains, float fc, size_t numChannels){
        assert(numChannels <= This->numChannels);
        
        for (size_t i=0; i<numChannels; i++){
            // make nice names for the filter coefficients
            float* b0 = This->b0 + i;
            float* b1 = This->b1 + i;
            float* b2 = This->b2 + i;
            float* a1neg = This->a1neg + i;
            float* a2neg = This->a2neg + i;
            
			setLowShelfSingle(b0, b1, b2, a1neg, a2neg, fc, gains[i], This->sampleRate);
        }
    }
    
    
	
	
	void BMBiquadArray4_setLowShelf(BMBiquadArray4 *This, float *gains, float fc, size_t numChannels){
        assert(numChannels <= This->numChannels);
        
        for (size_t i=0; i<numChannels/4; i++){
			// get the coefficients for a set of four filters
			float b0x, b1x, b2x, a1Negx, a2Negx;
			setLowShelfSingle(&b0x,&b1x,&b2x,&a1Negx,&a2Negx,fc,gains[i*4 + 0],This->sampleRate);
			float b0y, b1y, b2y, a1Negy, a2Negy;
			setLowShelfSingle(&b0y,&b1y,&b2y,&a1Negy,&a2Negy,fc,gains[i*4 + 1],This->sampleRate);
			float b0z, b1z, b2z, a1Negz, a2Negz;
			setLowShelfSingle(&b0z,&b1z,&b2z,&a1Negz,&a2Negz,fc,gains[i*4 + 2],This->sampleRate);
			float b0w, b1w, b2w, a1Negw, a2Negw;
			setLowShelfSingle(&b0w,&b1w,&b2w,&a1Negw,&a2Negw,fc,gains[i*4 + 3],This->sampleRate);
			
			// create vectors from the floats
			This->b0[i] = simd_make_float4(b0x, b0y, b0z, b0w);
			This->b1[i] = simd_make_float4(b1x, b1y, b1z, b1w);
			This->b2[i] = simd_make_float4(b2x, b2y, b2z, b2w);
			This->a1neg[i] = simd_make_float4(a1Negx, a1Negy, a1Negz, a1Negw);
			This->a2neg[i] = simd_make_float4(a2Negx, a2Negy, a2Negz, a2Negw);
        }
    }
    
    
    
    
    /*
     * Set filter gain to acheive desired RT60 decay time
     *
     * @param delayTimes     length in seconds of the delay line where the filter is applied
     * @param gains          this is our output
     * @param unfilteredRT60 decay time in seconds before filtering
     * @param filteredRT60   total decay time of the filtered part of the spectrum
     */
    void shelfFilterGainHelper(float* delayTimes, float* gains, float unfilteredRT60, float filteredRT60, size_t numChannels){
        for(size_t i=0; i<numChannels; i++){
            
            // find the gain of the decay coefficient
            //float unfilteredGain = pow(10.0, -3.0 * delayTimes[i] / unfilteredRT60);
            
            // find the total gain of the filtered part of
            // the spectrum (including the decay coeffient)
            //float filteredGainTotal = pow(10.0, -3.0 * delayTimes[i] / filteredRT60);
            
            // find the gain to set on the filter, taking into
            // account the filter coefficient already applied to
            // the full spectrum
            //float filterGain = filteredGainTotal / unfilteredGain;
            
            
            /*
             * The calculation above may suffer from floating point overflow error
             * in cases where the delay time is long. To avoid this, here is an
             * alternative way of calculating.
             */
            float t = (delayTimes[i] / filteredRT60)
                        - (delayTimes[i] / unfilteredRT60);
            float filterGain = pow(10.0, -3.0 * t);
            
            gains[i] = filterGain;
        }
    }
    
    
    
    
    
    /*
     * Set high frequency decay for High shelf filter array in an FDN
     *
     * @param delayTimeSeconds  the length in seconds of each delay in the FDN
     * @param fc                cutoff frequency
     * @param unfilteredRT60    decay time below fc (seconds)
     * @param filteredRT60      decay time at nyquist (seconds)
     * @param numChannels       size of FDN
     */
    void BMBiquadArray_setHighDecayFDN(BMBiquadArray *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels){
        assert(numChannels <= This->numChannels);
        
        // allocate memory to store the gain for each filter
        float* gains = malloc(sizeof(float)*numChannels);
        
        // find the gain setting to produce the specified decay time
        shelfFilterGainHelper(delayTimesSeconds, gains, unfilteredRT60, filteredRT60, numChannels);
        
        // set the filters
        BMBiquadArray_setHighShelf(This, gains, fc, numChannels);
        
        // release the temporary memory
        free(gains);
    }
	
	
	
	void BMBiquadArray4_setHighDecayFDN(BMBiquadArray4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels){
		   assert(numChannels <= This->numChannels);
		   
		   // allocate memory to store the gain for each filter
		   float* gains = malloc(sizeof(float)*numChannels);
		   
		   // find the gain setting to produce the specified decay time
		   shelfFilterGainHelper(delayTimesSeconds, gains, unfilteredRT60, filteredRT60, numChannels);
		   
		   // set the filters
		   BMBiquadArray4_setHighShelf(This, gains, fc, numChannels);
		   
		   // release the temporary memory
		   free(gains);
	   }
    
    
    
    
    /*
     * Set low frequency decay for High shelf filter array in an FDN
     *
     * @param delayTimeSeconds  the length in seconds of each delay in the FDN
     * @param fc                cutoff frequency
     * @param unfilteredRT60    decay time above fc (seconds)
     * @param filteredRT60      decay time at DC (seconds)
     * @param numChannels       size of FDN
     */
    void BMBiquadArray_setLowDecayFDN(BMBiquadArray *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels){
            assert(numChannels <= This->numChannels);
            
            // allocate memory to store the gain for each filter
            float* gains = malloc(sizeof(float)*numChannels);
            
            // find the gain setting to produce the specified decay time
            shelfFilterGainHelper(delayTimesSeconds, gains, unfilteredRT60, filteredRT60, numChannels);
            
            // set the filters
            BMBiquadArray_setLowShelf(This, gains, fc, numChannels);
            
            // release the temporary memory
            free(gains);
    }
	
	
	
	
	
	void BMBiquadArray4_setLowDecayFDN(BMBiquadArray4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels){
            assert(numChannels <= This->numChannels);
            
            // allocate memory to store the gain for each filter
            float* gains = malloc(sizeof(float)*numChannels);
            
            // find the gain setting to produce the specified decay time
            shelfFilterGainHelper(delayTimesSeconds, gains, unfilteredRT60, filteredRT60, numChannels);
            
            // set the filters
            BMBiquadArray4_setLowShelf(This, gains, fc, numChannels);
            
            // release the temporary memory
            free(gains);
    }
    
    
    
    
    
    /*
     * Free memory
     */
    void BMBiquadArray_free(BMBiquadArray *This){
        free(This->a1neg);
        This->a1neg = NULL;
        free(This->a2neg);
        This->a2neg = NULL;
        free(This->b0);
        This->b0 = NULL;
        free(This->b1);
        This->b1 = NULL;
        free(This->b2);
        This->b2 = NULL;
        
        free(This->x1);
        This->x1 = NULL;
        free(This->x2);
        This->x2 = NULL;
        free(This->y1);
        This->y1 = NULL;
        free(This->y2);
        This->y2 = NULL;
    }
	
	
	
	
	
	void BMBiquadArray4_free(BMBiquadArray4 *This){
        free(This->a1neg);
        This->a1neg = NULL;
        free(This->a2neg);
        This->a2neg = NULL;
        free(This->b0);
        This->b0 = NULL;
        free(This->b1);
        This->b1 = NULL;
        free(This->b2);
        This->b2 = NULL;
        
        free(This->x1);
        This->x1 = NULL;
        free(This->x2);
        This->x2 = NULL;
        free(This->y1);
        This->y1 = NULL;
        free(This->y2);
        This->y2 = NULL;
    }
	
    
    
    
    
    
    void BMBiquadArray_impulseResponse(BMBiquadArray *This){
        
        // define the length of the impulse response
        size_t IRLength = 1000;
        
        // this array will be used for both input and output
        float* inputOutput = malloc(sizeof(float)*IRLength);
        
        // set the input to all zeros
        memset(inputOutput+1, 0, sizeof(float)*IRLength);
        
        // set the first sample to be an impulse
        inputOutput[0] = 1.0;
        
        
        // process and print the impulse response
        printf("Impulse response of 1st filter:\n");
        printf("{");
        for(size_t i=0; i < IRLength - 1; i++){
            BMBiquadArray_processSample(This, inputOutput + i, inputOutput + i, 1);
            printf("%f, ", inputOutput[i]);
        }
        BMBiquadArray_processSample(This, inputOutput + IRLength - 1, inputOutput + IRLength - 1, 1);
        printf("%f", inputOutput[IRLength - 1]);
        printf("}");
        
        
        free(inputOutput);
    }
    
    

#ifdef __cplusplus
}
#endif
