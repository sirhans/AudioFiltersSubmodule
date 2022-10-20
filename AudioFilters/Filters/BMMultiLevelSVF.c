//
//  BMMultiLevelSVF.c
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 1/9/19.
//  Copyright © 2019 Hans. All rights reserved.
//

#include "BMMultiLevelSVF.h"
#include <stdlib.h>
#include <assert.h>
#include "Constants.h"

#define SVF_Param_Count 3

static inline void BMMultiLevelSVF_processBufferAtLevel(BMMultiLevelSVF *This,
                                                        size_t level, size_t channel,
                                                        const float* input,
                                                        float* output,
                                                        size_t numSamples);

static inline void BMMultiLevelSVF_updateSVFParam(BMMultiLevelSVF *This);

void BMMultiLevelSVF_init(BMMultiLevelSVF *This, size_t numLevels,float sampleRate,
						  bool isStereo){
	
	// init the biquad helper. This allows us to set biquad filters using the
	// functions in BMMultiLevelBiquad and copy them into the SVF.
	BMMultiLevelBiquad_init(&This->biquadHelper, numLevels, sampleRate, isStereo, false, false);
	
	This->sampleRate = sampleRate;
	This->numChannels = isStereo? 2 : 1;
	This->numLevels = numLevels;
	This->filterSweep = FALSE;
	//If stereo -> we need totalnumlevel = numlevel *2
	size_t totalNumLevels = numLevels * This->numChannels;
	This->a = (float**)malloc(sizeof(float*) * numLevels);
	This->m = (float**)malloc(sizeof(float*) * numLevels);
	This->target_a = (float**)malloc(sizeof(float*) * numLevels);
	This->target_m = (float**)malloc(sizeof(float*) * numLevels);
	for(int i=0;i<numLevels;i++){
		This->a[i] = malloc(sizeof(float) * SVF_Param_Count);
		This->m[i] = malloc(sizeof(float) * SVF_Param_Count);
		This->target_a[i] = malloc(sizeof(float) * SVF_Param_Count);
		This->target_m[i] = malloc(sizeof(float) * SVF_Param_Count);
	}
	
	This->ic1eq = malloc(sizeof(float)* totalNumLevels);
	This->ic2eq = malloc(sizeof(float)* totalNumLevels);
	for(int i=0;i<totalNumLevels;i++){
		This->ic1eq[i] = 0;
		This->ic2eq[i] = 0;
	}
	
	This->a1interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->a2interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->a3interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->m0interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->m1interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->m2interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}


void BMMultiLevelSVF_free(BMMultiLevelSVF *This){
    
    for(int i=0;i<This->numLevels;i++){
        free(This->a[i]);
        This->a[i] = NULL;
//        This->a[i] = malloc(sizeof(float) * SVF_Param_Count);
        
        free(This->m[i]);
        This->m[i] = NULL;
//        This->m[i] = malloc(sizeof(float) * SVF_Param_Count);
        
        free(This->target_a[i]);
        This->target_a[i] = NULL;
//        This->tempA[i] = malloc(sizeof(float) * SVF_Param_Count);
        
        free(This->target_m[i]);
        This->target_m[i] = NULL;
//        This->tempM[i] = malloc(sizeof(float) * SVF_Param_Count);
    }
    
    free(This->a);
    This->a = NULL;
    //This->a = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->m);
    This->m = NULL;
//    This->m = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->target_a);
    This->target_a = NULL;
//    This->tempA = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->target_m);
    This->target_m = NULL;
//    This->tempM = (float**)malloc(sizeof(float*) * totalNumLevels);
    
    free(This->ic1eq);
    free(This->ic2eq);
    This->ic1eq = NULL;
    This->ic2eq = NULL;
//    This->ic1eq = malloc(sizeof(float)* totalNumLevels);
//    This->ic2eq = malloc(sizeof(float)* totalNumLevels);
    
	
	free(This->a1interp);
	free(This->a2interp);
	free(This->a3interp);
	free(This->m0interp);
	free(This->m1interp);
	free(This->m2interp);
	This->a1interp = NULL;
	This->a2interp = NULL;
	This->a3interp = NULL;
	This->m0interp = NULL;
	This->m1interp = NULL;
	This->m2interp = NULL;
}


void BMMultiLevelSVF_enableFilterSweep(BMMultiLevelSVF *This, bool sweepOn){
	This->filterSweep = sweepOn;
}


#pragma mark - process
void BMMultiLevelSVF_processBufferMono(BMMultiLevelSVF *This,
                                       const float* input,
                                       float* output,
                                       size_t numSamples){
    assert(This->numChannels == 1);
	
	// if not filter sweeping, update parameters now
    if(This->shouldUpdateParam && !This->filterSweep)
        BMMultiLevelSVF_updateSVFParam(This);
    
    for(int level = 0;level<This->numLevels;level++){
        if(level==0){
            //Process into output
            BMMultiLevelSVF_processBufferAtLevel(This, level, 0, input, output, numSamples);
        }else
            BMMultiLevelSVF_processBufferAtLevel(This, level, 0, output, output, numSamples);
    }
	
	// if we are filter sweeping, call the function to update the parameters
	// after filtering. This updates the current parameters to match the targets,
	// which ends the sweep.
	if(This->shouldUpdateParam && This->filterSweep)
		BMMultiLevelSVF_updateSVFParam(This);
}

void BMMultiLevelSVF_processBufferStereo(BMMultiLevelSVF *This,
                                         const float* inputL, const float* inputR,
                                         float* outputL, float* outputR, size_t numSamples){
    assert(This->numChannels == 2);
    
	// if we aren't filter sweeping, update the parameters before filtering
    if(This->shouldUpdateParam && !This->filterSweep)
        BMMultiLevelSVF_updateSVFParam(This);
    
    for(int level = 0;level<This->numLevels;level++){
        if(level==0){
            //Process into output
            //Left channel
            BMMultiLevelSVF_processBufferAtLevel(This, level,0, inputL, outputL, numSamples);
            //Right channel
            BMMultiLevelSVF_processBufferAtLevel(This,level,1, inputR, outputR, numSamples);
            // + This->numLevels
        }else{
            //Left channel
            BMMultiLevelSVF_processBufferAtLevel(This,level,0, outputL, outputL, numSamples);
            //Right channel
            BMMultiLevelSVF_processBufferAtLevel(This,level,1, outputR,outputR, numSamples);
            // + This->numLevels
        }
    }
	
	// if we are filter sweeping, call the function to update the parameters
	// after filtering. This updates the current parameters to match the targets,
	// which ends the sweep.
	if(This->shouldUpdateParam && This->filterSweep)
		BMMultiLevelSVF_updateSVFParam(This);
}


inline void BMMultiLevelSVF_processBufferAtLevel(BMMultiLevelSVF *This,
												 size_t level,size_t channel,
                                                 const float* input,
                                                 float* output,size_t numSamples){
	
	if(This->filterSweep){
		// for filter sweep, we do not allow long buffers because linear
		// interpolation of filter coefficients is inaccurate. To process longer
		// buffers in a filter sweep, you need to break the audio up into
		// smaller sized buffers and call the filter parameter update function
		// after each one. 
		assert (numSamples <= BM_BUFFER_CHUNK_SIZE);
		
		// compute interpolated coefficients.
		//
		// with each sample of audio input, the filter coefficients change by
		// how much?
		float a1inc = (This->target_a[level][0] - This->a[level][0]) / numSamples;
		float a2inc = (This->target_a[level][1] - This->a[level][1]) / numSamples;
		float a3inc = (This->target_a[level][2] - This->a[level][2]) / numSamples;
		float m0inc = (This->target_m[level][0] - This->m[level][0]) / numSamples;
		float m1inc = (This->target_m[level][1] - This->m[level][1]) / numSamples;
		float m2inc = (This->target_m[level][2] - This->m[level][2]) / numSamples;
		// These ramps will stop one sample short of reaching the target value.
		// The targets will be reached on the first sample of the next call to
		// this function because the calling function will call the coefficient
		// update function, which will copy from target_a to a and target_m to m;
		vDSP_vramp(&This->a[level][0], &a1inc, This->a1interp, 1, numSamples);
		vDSP_vramp(&This->a[level][1], &a2inc, This->a2interp, 1, numSamples);
		vDSP_vramp(&This->a[level][2], &a3inc, This->a3interp, 1, numSamples);
		vDSP_vramp(&This->m[level][0], &m0inc, This->m0interp, 1, numSamples);
		vDSP_vramp(&This->m[level][1], &m0inc, This->m1interp, 1, numSamples);
		vDSP_vramp(&This->m[level][2], &m0inc, This->m2interp, 1, numSamples);
		
		// process the filter
		size_t icLvl = This->numLevels*channel + level;
		for(size_t i=0;i<numSamples;i++){
			float v3 = input[i] - This->ic2eq[icLvl];
			float v1 = This->a1interp[i]*This->ic1eq[icLvl] + This->a2interp[i]*v3;
			float v2 = This->ic2eq[icLvl] + This->a2interp[i]*This->ic1eq[icLvl] + This->a3interp[i] * v3;
			This->ic1eq[icLvl] = 2*v1 - This->ic1eq[icLvl];
			This->ic2eq[icLvl] = 2*v2 - This->ic2eq[icLvl];
			output[i] = This->m0interp[i] * input[i] + This->m1interp[i] * v1 + This->m2interp[i]*v2;
		}
	} else {
		
		size_t icLvl = This->numLevels*channel + level;
		for(size_t i=0;i<numSamples;i++){
			float v3 = input[i] - This->ic2eq[icLvl];
			float v1 = This->a[level][0]*This->ic1eq[icLvl] + This->a[level][1]*v3;
			float v2 = This->ic2eq[icLvl] + This->a[level][1]*This->ic1eq[icLvl] + This->a[level][2] * v3;
			This->ic1eq[icLvl] = 2*v1 - This->ic1eq[icLvl];
			This->ic2eq[icLvl] = 2*v2 - This->ic2eq[icLvl];
			output[i] = This->m[level][0] * input[i] + This->m[level][1] * v1 + This->m[level][2]*v2;
		}
	}
}


inline void BMMultiLevelSVF_updateSVFParam(BMMultiLevelSVF *This){
    if(This->shouldUpdateParam){
        //Update all param
        for(int i=0;i<This->numLevels;i++){
            for(int j=0;j<SVF_Param_Count;j++){
                This->a[i][j] = This->target_a[i][j];
                This->m[i][j] = This->target_m[i][j];
            }
        }
		This->shouldUpdateParam = false;
    }
}

#pragma mark - Filters
void BMMultiLevelSVF_setLowpass(BMMultiLevelSVF *This, double fc, size_t level){
    BMMultiLevelSVF_setLowpassQ(This, fc, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setLowpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 0;
    This->target_m[level][1] = 0;
    This->target_m[level][2] = 1;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setBandpass(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 0;
    This->target_m[level][1] = 1;
    This->target_m[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setHighpass(BMMultiLevelSVF *This, double fc, size_t level){
    BMMultiLevelSVF_setHighpassQ(This, fc, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setHighpassQ(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 1;
    This->target_m[level][1] = -k;
    This->target_m[level][2] = -1;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setNotchpass(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 1;
    This->target_m[level][1] = -k;
    This->target_m[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setPeak(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 1;
    This->target_m[level][1] = -k;
    This->target_m[level][2] = -2;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setAllpass(BMMultiLevelSVF *This, double fc, double q, size_t level){
    assert(level < This->numLevels);
    
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 1;
    This->target_m[level][1] = -2*k;
    This->target_m[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setBell(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level){
    assert(level < This->numLevels);
    
    float A = powf(10, gain/40.);
    float g = tanf(M_PI*fc/This->sampleRate);
    float k = 1/(q*A);
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 1;
    This->target_m[level][1] = k*(A*A -1);
    This->target_m[level][2] = 0;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setLowShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level){
    BMMultiLevelSVF_setLowShelfQ(This, fc, gain, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setLowShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level){
    assert(level < This->numLevels);
    
    float A = powf(10, gain/40.);
    float g = tanf(M_PI*fc/This->sampleRate)/sqrtf(A);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = 1;
    This->target_m[level][1] = k*(A -1);
    This->target_m[level][2] = A*A - 1;
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setHighShelf(BMMultiLevelSVF *This, double fc, double gain, size_t level){
    BMMultiLevelSVF_setHighShelfQ(This, fc, gain, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setHighShelfQ(BMMultiLevelSVF *This, double fc, double gain, double q, size_t level){
    assert(level < This->numLevels);
    
    float A = powf(10, gain/40.);
    float g = tanf(M_PI*fc/This->sampleRate)*sqrtf(A);
    float k = 1/q;
    This->target_a[level][0] = 1/(1 + g*(g+k));
    This->target_a[level][1] = g*This->target_a[level][0];
    This->target_a[level][2] = g*This->target_a[level][1];
    This->target_m[level][0] = A*A;
    This->target_m[level][1] = k*(1 - A)*A;
    This->target_m[level][2] = 1 - A*A;
    
    This->shouldUpdateParam = true;
}


/*
 * This sets the coefficients of the SVF using the coefficients of the biquad filter.
 * Doing it this way permits us to reuse the existing code for setting coefficients
 * of biquad filters. The transfer function of the SVF will be equivalent to the
 * transfer function of the specified biquad filter.
 */
void BMMultiLevelSVF_setFromBiquad(BMMultiLevelSVF *This,
								   double b0, double b1, double b2,
								   double a0, double a1, double a2,
								   size_t level){
	assert(level < This->numLevels);
	
	// https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf
	// *** See section: "Solve for the mixed output SVF to show relation to regular DF1 coefficients..."
	double g = - sqrt(-1.0 - b1 - b2) / sqrt(-1.0 + b1 - b2);
	double k = (1.0 - b2) / (sqrt(-1.0 - b1 - b2) * sqrt(-1.0 + b1 - b2));
	
	// https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
	// *** See section: "Algorithm for every response"
	double svf_a1 = 1.0 / (1.0 + g * (g + k));
	double svf_a2 = g * svf_a1;
	double svf_a3 = g * svf_a2;
	
	This->target_a[level][0] = svf_a1;
	This->target_a[level][1] = svf_a2;
	This->target_a[level][2] = svf_a3;
	
	// https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf
	// *** See section: "Solve for the mixed output SVF to show relation to regular DF1 coefficients..."
	double svf_m0 = (a0 - a1 + a2) / (1.0 - b1 + b2);
	double svf_m1 = 2.0 * (a0 - a2) /  (sqrt(-1.0 - b1 - b2) * sqrt(-1.0 + b1 - b2));
	double svf_m2 = (a0 + a1 + a2) / (1.0 + b1 + b2);
	
	This->target_m[level][0] = svf_m0;
	This->target_m[level][1] = svf_m1;
	This->target_m[level][2] = svf_m2;
	
	This->shouldUpdateParam = true;
}



/*
 * The biquadHelper is a BMMultiLevelBiquad struct that functions as a model
 * for this SVF to follow. First we set the biquad filter to the desired
 * configuration. Then we copy that configuration over to the SVF. This allows
 * us to use code written for the biquad filter in the SVF filter without
 * deriving new formulae for the filter coefficients.
 *
 * The key to this is the formula in _setFromBiquad() that converts biquad
 * filter coefficient values into SVF coefficients and results in an SVF filter
 * with the same transfer function.
 */
void BMMultiLevelSVF_copyStateFromBiquadHelper(BMMultiLevelSVF *This){
	for(size_t lv=0; lv<This->numLevels; lv++){
		
		double b0 = This->biquadHelper.coefficients_d[0 + lv*This->numChannels*5 + lv*5];
		double b1 = This->biquadHelper.coefficients_d[1 + lv*This->numChannels*5 + lv*5];
		double b2 = This->biquadHelper.coefficients_d[2 + lv*This->numChannels*5 + lv*5];
		double a0 = 1.0f;
		double a1 = This->biquadHelper.coefficients_d[3 + lv*This->numChannels*5 + lv*5];
		double a2 = This->biquadHelper.coefficients_d[4 + lv*This->numChannels*5 + lv*5];
		
		BMMultiLevelSVF_setFromBiquad(This,
									  b0, b1, b2,
									  a0, a1, a2,
									  lv);
	}
}



void BMMultiLevelSVF_impulseResponse(BMMultiLevelSVF *This,size_t frameCount){
    float* irBuffer = malloc(sizeof(float)*frameCount);
    float* irBufferR = malloc(sizeof(float)*frameCount);
    float* outBuffer = malloc(sizeof(float)*frameCount);
    float* outBufferR = malloc(sizeof(float)*frameCount);
    for(int i=0;i<frameCount;i++){
        if(i==0){
            irBuffer[i] = 1;
            irBufferR[i] = 1;
        }else{
            irBuffer[i] = 0;
            irBufferR[i] = 0;
        }
    }
    
    BMMultiLevelSVF_processBufferStereo(This, irBuffer, irBufferR, outBuffer, outBufferR, frameCount);
    
    printf("\[");
    for(size_t i=0; i<(frameCount-1); i++)
        printf("%f\n", outBufferR[i]);
    printf("%f],",outBufferR[frameCount-1]);
    
}

