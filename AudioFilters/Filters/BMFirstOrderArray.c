//
//  BMFirstOrderArray.c
//  BMAudioFilters
//
//  Created by hans anderson on 1/1/18.
//  Anyone may use this file without restrictions
//

#include "BMFirstOrderArray.h"


#ifdef __cplusplus
extern "C" {
#endif

    
    
    // forward declarations
    void BMFirstOrderArray4x4_setHighDecayFDN(BMFirstOrderArray4x4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    void BMFirstOrderArray4x4_setLowDecayFDN(BMFirstOrderArray4x4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    void BMFirstOrderArray4x4_setBypass(simd_float4* b0, simd_float4* b1, simd_float4* a1);
    
    
	
	void BMFirstOrderArray4x4_init(BMFirstOrderArray4x4 *This, size_t numChannels, float sampleRate){
        This->sampleRate = sampleRate;
        
        // clear delays
        memset(This->x1,0,sizeof(simd_float4)*4);
        memset(This->y1,0,sizeof(simd_float4)*4);
        
        // bypass all the filters
        for(size_t i=0; i<4; i++)
            BMFirstOrderArray4x4_setBypass(&This->b0[i], &This->b1[i], &This->a1neg[i]);
    }
    
    

    
	
	
    /*
     * Set the given coefficients to bypass the filter
     */
    void BMFirstOrderArray4x4_setBypass(simd_float4 *b0,
					simd_float4 *b1,
					simd_float4 *a1){
        *b0 = 1.0f;
        *b1 = *a1 = 0.0f;
    }
    
//    
//	
//	
//	void BMFirstOrderArray4x4_setBypassSingle(float* b0, float* b1, float* a1){
//		*b0 = 1.0f;
//		*b1 = *a1 = 0.0f;
//	}
//	
	
	
	
    
	void BMFirstOrderArray4x4_setHighShelfSingle(float *b0, float* b1, float* a1Neg, float fc, float gain, float sampleRate){
		
		double gamma = tanf(M_PI * fc / sampleRate);
		double one_over_denominator;
		
		if(gain>1.0f){
			one_over_denominator = 1.0f / (gamma + 1.0f);
			*b0 = (gamma + gain) * one_over_denominator;
			*b1 = (gamma - gain) * one_over_denominator;
			*a1Neg = -(gamma - 1.0f) * one_over_denominator;
		}
		else{
			one_over_denominator = 1.0f / (gamma*gain + 1.0f);
			*b0 = gain*(gamma + 1.0f) * one_over_denominator;
			*b1 = gain*(gamma - 1.0f) * one_over_denominator;
			*a1Neg = -(gain*gamma - 1.0f) * one_over_denominator;
		}
	}
	
	
	
	
	
	
	void BMFirstOrderArray4x4_setHighShelf(BMFirstOrderArray4x4 *This, float *gains, float fc, size_t numChannels){
        for (size_t i=0; i<numChannels/4; i++){
			// get the coefficients for a set of four filters
			float b0x, b1x, a1Negx;
			BMFirstOrderArray4x4_setHighShelfSingle(&b0x,&b1x,&a1Negx,fc,gains[i*4 + 0],This->sampleRate);
			float b0y, b1y, a1Negy;
			BMFirstOrderArray4x4_setHighShelfSingle(&b0y,&b1y,&a1Negy,fc,gains[i*4 + 1],This->sampleRate);
			float b0z, b1z, a1Negz;
			BMFirstOrderArray4x4_setHighShelfSingle(&b0z,&b1z,&a1Negz,fc,gains[i*4 + 2],This->sampleRate);
			float b0w, b1w, a1Negw;
			BMFirstOrderArray4x4_setHighShelfSingle(&b0w,&b1w,&a1Negw,fc,gains[i*4 + 3],This->sampleRate);
			
			// create vectors from the floats
			This->b0[i] = simd_make_float4(b0x, b0y, b0z, b0w);
			This->b1[i] = simd_make_float4(b1x, b1y, b1z, b1w);
			This->a1neg[i] = simd_make_float4(a1Negx, a1Negy, a1Negz, a1Negw);
        }
    }
    
    
	
	
	
	
	void BMFirstOrderArray4x4_setLowShelfSingle(float *b0, float* b1, float* a1Neg, float fc, float gain, float sampleRate){
		
		double gamma = tanf(M_PI * fc / sampleRate);
		double one_over_denominator;
		
		if(gain>1.0f){
			one_over_denominator = 1.0f / (gamma + 1.0f);
			*b0 = (gamma * gain + 1.0f) * one_over_denominator;
			*b1 = (gamma * gain - 1.0f) * one_over_denominator;
			*a1Neg = -(gamma - 1.0f) * one_over_denominator;
		}else{
			one_over_denominator = 1.0f / (gamma + gain);
			*b0 = gain*(gamma + 1.0f) * one_over_denominator;
			*b1 = gain*(gamma - 1.0f) * one_over_denominator;
			*a1Neg = -(gamma - gain) * one_over_denominator;
		}
	}
	
	
    
    
	
	
	void BMFirstOrderArray4x4_setLowShelf(BMFirstOrderArray4x4 *This, float *gains, float fc, size_t numChannels){
        for (size_t i=0; i<numChannels/4; i++){
			// get the coefficients for a set of four filters
			float b0x, b1x, a1Negx;
			BMFirstOrderArray4x4_setLowShelfSingle(&b0x,&b1x,&a1Negx,fc,gains[i*4 + 0],This->sampleRate);
			float b0y, b1y, a1Negy;
			BMFirstOrderArray4x4_setLowShelfSingle(&b0y,&b1y,&a1Negy,fc,gains[i*4 + 1],This->sampleRate);
			float b0z, b1z, a1Negz;
			BMFirstOrderArray4x4_setLowShelfSingle(&b0z,&b1z,&a1Negz,fc,gains[i*4 + 2],This->sampleRate);
			float b0w, b1w, a1Negw;
			BMFirstOrderArray4x4_setLowShelfSingle(&b0w,&b1w,&a1Negw,fc,gains[i*4 + 3],This->sampleRate);
			
			// create vectors from the floats
			This->b0[i] = simd_make_float4(b0x, b0y, b0z, b0w);
			This->b1[i] = simd_make_float4(b1x, b1y, b1z, b1w);
			This->a1neg[i] = simd_make_float4(a1Negx, a1Negy, a1Negz, a1Negw);
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
    void BMFirstOrderArray4x4_shelfFilterGainHelper(float* delayTimes, float* gains, float unfilteredRT60, float filteredRT60, size_t numChannels){
        for(size_t i=0; i<numChannels; i++){

            float t = (delayTimes[i] / filteredRT60) - (delayTimes[i] / unfilteredRT60);
            float filterGain = pow(10.0, -3.0 * t);
            
            gains[i] = filterGain;
        }
    }
    
    
    
	
	
	
	void BMFirstOrderArray4x4_setHighDecayFDN(BMFirstOrderArray4x4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels){
		   
		   // allocate memory to store the gain for each filter
		   float* gains = malloc(sizeof(float)*numChannels);
		   
		   // find the gain setting to produce the specified decay time
		   BMFirstOrderArray4x4_shelfFilterGainHelper(delayTimesSeconds, gains, unfilteredRT60, filteredRT60, numChannels);
		   
		   // set the filters
		   BMFirstOrderArray4x4_setHighShelf(This, gains, fc, numChannels);
		   
		   // release the temporary memory
		   free(gains);
	   }
    
    
    

	
	
	
	void BMFirstOrderArray4x4_setLowDecayFDN(BMFirstOrderArray4x4 *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels){
            
            // allocate memory to store the gain for each filter
            float* gains = malloc(sizeof(float)*numChannels);
            
            // find the gain setting to produce the specified decay time
            BMFirstOrderArray4x4_shelfFilterGainHelper(delayTimesSeconds, gains, unfilteredRT60, filteredRT60, numChannels);
            
            // set the filters
            BMFirstOrderArray4x4_setLowShelf(This, gains, fc, numChannels);
            
            // release the temporary memory
            free(gains);
    }
    
    
    
    

#ifdef __cplusplus
}
#endif
