//
//  BMMultiLevelSVF.c
//  BMAudioFilters
//
//  Created by Nguyen Minh Tien on 1/9/19.
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
	This->filterSweep = false;
	This->shouldUpdateParam = false;
	This->updateImmediately = false;
	This->needsClearStateVariables = false;
	This->lock = OS_UNFAIR_LOCK_INIT;
	//If stereo -> we need totalnumlevel = numlevel *2
	size_t totalNumLevels = numLevels * This->numChannels;

	This->g0 = malloc(sizeof(float) * numLevels);
	This->g1 = malloc(sizeof(float) * numLevels);
	This->g2 = malloc(sizeof(float) * numLevels);
	This->m0 = malloc(sizeof(float) * numLevels);
	This->m1 = malloc(sizeof(float) * numLevels);
	This->m2 = malloc(sizeof(float) * numLevels);
	This->k  = malloc(sizeof(float) * numLevels);
	
	This->g0_target = malloc(sizeof(float) * numLevels);
	This->g1_target = malloc(sizeof(float) * numLevels);
	This->g2_target = malloc(sizeof(float) * numLevels);
	This->m0_target = malloc(sizeof(float) * numLevels);
	This->m1_target = malloc(sizeof(float) * numLevels);
	This->m2_target = malloc(sizeof(float) * numLevels);
	This->k_target  = malloc(sizeof(float) * numLevels);
	
	This->g0_pending = malloc(sizeof(float) * numLevels);
	This->g1_pending = malloc(sizeof(float) * numLevels);
	This->g2_pending = malloc(sizeof(float) * numLevels);
	This->m0_pending = malloc(sizeof(float) * numLevels);
	This->m1_pending = malloc(sizeof(float) * numLevels);
	This->m2_pending = malloc(sizeof(float) * numLevels);
	This->k_pending = malloc(sizeof(float) * numLevels);

	
	This->ic1eq = malloc(sizeof(float)* totalNumLevels);
	This->ic2eq = malloc(sizeof(float)* totalNumLevels);
	for(int i=0;i<totalNumLevels;i++){
		This->ic1eq[i] = 0.0;
		This->ic2eq[i] = 0.0;
	}
	
	
	This->g0_interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->g1_interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->g2_interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->m0_interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->m1_interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->m2_interp = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
	This->k_interp  = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
}


void BMMultiLevelSVF_free(BMMultiLevelSVF *This){
	free(This->g0);
	free(This->g1);
	free(This->g2);
	free(This->m0);
	free(This->m1);
	free(This->m2);
	free(This->k);
	This->g0 = NULL;
	This->g1 = NULL;
	This->g2 = NULL;
	This->m0 = NULL;
	This->m1 = NULL;
	This->m2 = NULL;
	This->k = NULL;
	
	free(This->g0_target);
	free(This->g1_target);
	free(This->g2_target);
	free(This->m0_target);
	free(This->m1_target);
	free(This->m2_target);
	free(This->k_target);
	This->g0_target = NULL;
	This->g1_target = NULL;
	This->g2_target = NULL;
	This->m0_target = NULL;
	This->m1_target = NULL;
	This->m2_target = NULL;
	This->k_target = NULL;
	
	free(This->g0_pending);
	free(This->g1_pending);
	free(This->g2_pending);
	free(This->m0_pending);
	free(This->m1_pending);
	free(This->m2_pending);
	free(This->k_pending);
	This->g0_pending = NULL;
	This->g1_pending = NULL;
	This->g2_pending = NULL;
	This->m0_pending = NULL;
	This->m1_pending = NULL;
	This->m2_pending = NULL;
	This->k_pending = NULL;
    
    free(This->ic1eq);
    free(This->ic2eq);
    This->ic1eq = NULL;
    This->ic2eq = NULL;
    
	free(This->g0_interp);
	free(This->g1_interp);
	free(This->g2_interp);
	free(This->m0_interp);
	free(This->m1_interp);
	free(This->m2_interp);
	free(This->k_interp);
	This->g0_interp = NULL;
	This->g1_interp = NULL;
	This->g2_interp = NULL;
	This->m0_interp = NULL;
	This->m1_interp = NULL;
	This->m2_interp = NULL;
	This->k_interp = NULL;
}


void BMMultiLevelSVF_enableFilterSweep(BMMultiLevelSVF *This, bool sweepOn){
	This->filterSweep = sweepOn;
}

void BMMUltiLevelSVF_forceImmediateUpdate(BMMultiLevelSVF *This){
	This->updateImmediately = true;
}

void BMMultiLevelSVF_clearStateVariables(BMMultiLevelSVF *This){
	// set all state variables to zero
	memset(This->ic1eq,0,sizeof(float)*This->numLevels*This->numChannels);
	memset(This->ic2eq,0,sizeof(float)*This->numLevels*This->numChannels);
	
	This->needsClearStateVariables = false;
}

void BMMultiLevelSVF_clearBuffers(BMMultiLevelSVF *This){
	This->needsClearStateVariables = true;
}


#pragma mark - process
void BMMultiLevelSVF_processBufferMono(BMMultiLevelSVF *This,
                                       const float* input,
                                       float* output,
                                       size_t numSamples){
    assert(This->numChannels == 1);
	
	if(This->needsClearStateVariables)
		BMMultiLevelSVF_clearStateVariables(This);
	
    if(This->shouldUpdateParam)
        BMMultiLevelSVF_updateSVFParam(This);
    
    for(int level = 0;level<This->numLevels;level++){
        if(level==0){
            //Process into output
            BMMultiLevelSVF_processBufferAtLevel(This, level, 0, input, output, numSamples);
        }else
            BMMultiLevelSVF_processBufferAtLevel(This, level, 0, output, output, numSamples);
    }
}




void BMMultiLevelSVF_processBufferStereo(BMMultiLevelSVF *This,
                                         const float* inputL, const float* inputR,
                                         float* outputL, float* outputR, size_t numSamples){
    assert(This->numChannels == 2);
	
	if(This->needsClearStateVariables)
		BMMultiLevelSVF_clearStateVariables(This);
    
	// update the parameters
    if(This->shouldUpdateParam)
        BMMultiLevelSVF_updateSVFParam(This);
    
    for(int level = 0;level<This->numLevels;level++){
        if(level==0){
            //Process into output
            //Left channel
            BMMultiLevelSVF_processBufferAtLevel(This, level, 0, inputL, outputL, numSamples);
            //Right channel
            BMMultiLevelSVF_processBufferAtLevel(This, level, 1, inputR, outputR, numSamples);
            // + This->numLevels
        }else{
            //Left channel
            BMMultiLevelSVF_processBufferAtLevel(This, level, 0, outputL, outputL, numSamples);
            //Right channel
            BMMultiLevelSVF_processBufferAtLevel(This, level, 1, outputR,outputR, numSamples);
            // + This->numLevels
        }
    }
}


void BMMultiLevelSVF_stageTwoParameterUpdate(BMMultiLevelSVF *This, size_t level){
	// parameter updates stage 2
	This->g0[level] = This->g0_target[level];
	This->g1[level] = This->g1_target[level];
	This->g2[level] = This->g2_target[level];
	This->m0[level] = This->m0_target[level];
	This->m1[level] = This->m1_target[level];
	This->m2[level] = This->m2_target[level];
	This->k[level]  = This->k_target[level];
}


void BMMultiLevelSVF_calculateInterpolatedCoefficients(BMMultiLevelSVF *This,
													   size_t level,
													   size_t numSamples){
	// compute interpolated coefficients.
	//
	// with each sample of audio input, the filter coefficients change by
	// how much?
	float g0inc = (This->g0_target[level] - This->g0[level]) / numSamples;
	float g1inc = (This->g1_target[level] - This->g1[level]) / numSamples;
	float g2inc = (This->g2_target[level] - This->g2[level]) / numSamples;
	float m0inc = (This->m0_target[level] - This->m0[level]) / numSamples;
	float m1inc = (This->m1_target[level] - This->m1[level]) / numSamples;
	float m2inc = (This->m2_target[level] - This->m2[level]) / numSamples;
	float kinc  = (This->k_target[level]  - This->k[level])  / numSamples;
	
	// These ramps will stop one sample short of reaching the target value.
	// The targets will be reached on the first sample of the next call to
	// this function because the calling function will call the coefficient
	// update function, which will copy from g_target to g and m_target to m;
	vDSP_vramp(&This->g0[level], &g0inc, This->g0_interp, 1, numSamples);
	vDSP_vramp(&This->g1[level], &g1inc, This->g1_interp, 1, numSamples);
	vDSP_vramp(&This->g2[level], &g2inc, This->g2_interp, 1, numSamples);
	vDSP_vramp(&This->m0[level], &m0inc, This->m0_interp, 1, numSamples);
	vDSP_vramp(&This->m1[level], &m1inc, This->m1_interp, 1, numSamples);
	vDSP_vramp(&This->m2[level], &m2inc, This->m2_interp, 1, numSamples);
	vDSP_vramp(&This->k[level],  &kinc,  This->k_interp,  1, numSamples);
	
	BMMultiLevelSVF_stageTwoParameterUpdate(This, level);
}







inline void BMMultiLevelSVF_processBufferAtLevel(BMMultiLevelSVF *This,
												 size_t level, size_t channel,
                                                 const float* input,
                                                 float* output,size_t numSamples){
	
	// get pointers to simplify notation for state variables
	size_t icLvl = This->numLevels*channel + level;
	float *ic1eq = &This->ic1eq[icLvl];
	float *ic2eq = &This->ic2eq[icLvl];
	
	if(This->filterSweep){
		// for filter sweep, we do not allow long buffers because linear
		// interpolation of filter coefficients is inaccurate. To process longer
		// buffers in a filter sweep, you need to break the audio up into
		// smaller sized buffers and call the filter parameter update function
		// after each one. 
		assert (numSamples <= BM_BUFFER_CHUNK_SIZE);
		
		BMMultiLevelSVF_calculateInterpolatedCoefficients(This, level, numSamples);
		
		// get pointers to simplify notation
		float *g0 = This->g0_interp;
		float *g1 = This->g1_interp;
		float *g2 = This->g2_interp;
		float *m0 = This->m0_interp;
		float *m1 = This->m1_interp;
		float *m2 = This->m2_interp;
		float *k = This->k_interp;
		
		// process the filter
		for(size_t i=0; i<numSamples; i++){
			// This code is based on the Tick 2 function in this file: https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf
			float v0 = input[i];
			float t0 = v0 - *ic2eq;
			float t1 = (g0[i] * t0) + (g1[i] * *ic1eq);
			float t2 = (g2[i] * t0) + (g0[i] * *ic1eq);
			float v1 = t1 + *ic1eq;
			float v2 = t2 + *ic2eq;
			*ic1eq += 2.0f * t1;
			*ic2eq += 2.0f * t2;
			float high = v0 - (k[i] * v1) - v2;
			float band = v1;
			float low = v2;
			output[i] = (m0[i] * high) + (m1[i] * band) + (m2[i] * low);
		}
	} else {
		BMMultiLevelSVF_stageTwoParameterUpdate(This, level);
		
		float g0 = This->g0[level];
		float g1 = This->g1[level];
		float g2 = This->g2[level];
		float m0 = This->m0[level];
		float m1 = This->m1[level];
		float m2 = This->m2[level];
		float k  = This->k[level];
		
		for(size_t i=0; i<numSamples; i++){
			// This code is based on the Tick 2 function in this file: https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf
			float v0 = input[i];
			float t0 = v0 - *ic2eq;
			float t1 = (g0 * t0) + (g1 * *ic1eq);
			float t2 = (g2 * t0) + (g0 * *ic1eq);
			float v1 = t1 + *ic1eq;
			float v2 = t2 + *ic2eq;
			*ic1eq += 2.0f * t1;
			*ic2eq += 2.0f * t2;
			float high = v0 - (k * v1) - v2;
			float band = v1;
			float low = v2;
			output[i] = (m0 * high) + (m1 * band) + (m2 * low);
		}
	}
}



inline void BMMultiLevelSVF_updateSVFParam(BMMultiLevelSVF *This){
	This->shouldUpdateParam = false;
	
	// update everything NOW, don't fade smoothly
	if(This->updateImmediately){
		This->updateImmediately = false;
		for(int i=0;i<This->numLevels;i++){
			// stage 1
			This->g0_target[i] = This->g0_pending[i];
			This->g1_target[i] = This->g1_pending[i];
			This->g2_target[i] = This->g2_pending[i];
			This->m0_target[i] = This->m0_pending[i];
			This->m1_target[i] = This->m1_pending[i];
			This->m2_target[i] = This->m2_pending[i];
			This->k_target[i]  = This->k_pending[i];
			
			// stage 2
			This->g0[i] = This->g0_target[i];
			This->g1[i] = This->g1_target[i];
			This->g2[i] = This->g2_target[i];
			This->m0[i] = This->m0_target[i];
			This->m1[i] = This->m1_target[i];
			This->m2[i] = This->m2_target[i];
			This->k[i]  = This->k_target[i];
		}
	}
	// update with a smooth fade over the next audio buffer
	else {
		for(int i=0;i<This->numLevels;i++){
			// we do updates in two stages. This eliminates conflicts that would
			// occur if the coefficient updates are executed on the UI thread
			// while processing is going on the audio thread
			
			// stage 1: this queues values to be updated
			// the lock prevents the UI thread from modifying the pending
			// coefficient values while we are copying them to the targets
			if (os_unfair_lock_trylock(&This->lock)){
				// if we got the lock, copy the updated values.
				This->g0_target[i] = This->g0_pending[i];
				This->g1_target[i] = This->g1_pending[i];
				This->g2_target[i] = This->g2_pending[i];
				This->m0_target[i] = This->m0_pending[i];
				This->m1_target[i] = This->m1_pending[i];
				This->m2_target[i] = This->m2_pending[i];
				This->k_target[i]  = This->k_pending[i];
				os_unfair_lock_unlock(&This->lock);
			}
			// if we didn't get the lock, schedule another update. Hopefully
			// we will get the lock next time.
			else {
				This->shouldUpdateParam = true;
			}
			
			// stage 2: This update should be done on the audio thread by
			// calling BMMultiLevelSVF_stageTwoParameterUpdate()
		}
	}
}

#pragma mark - Filters
void BMMultiLevelSVF_setCoefficientsHelper(BMMultiLevelSVF *This, double fc, double Q, size_t level){
	// This is from the function CalcCoeff2 in https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf
	double w = fc / This->sampleRate;
    double k = 1.0/Q;
	double s1 = sin(M_PI * w);
	double s2 = sin(2.0 * M_PI * w);
	double nrm = 1.0 / (2.0 + k * s2);
	double g0 = s2 * nrm;
	double g1 = ((-2.0 * s1 * s1) - (k * s2)) * nrm;
	double g2 = (2.0 * s1 * s1) * nrm;
	
	This->g0_pending[level] = (float)g0;
	This->g1_pending[level] = (float)g1;
	This->g2_pending[level] = (float)g2;
	This->k_pending[level] = (float)k;
}

void BMMultiLevelSVF_setLowpass(BMMultiLevelSVF *This, double fc, size_t level){
    BMMultiLevelSVF_setLowpassQ(This, fc, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setLowpassQ(BMMultiLevelSVF *This, double fc, double Q, size_t level){
    assert(level < This->numLevels);
    
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = 0.0;
	This->m1_pending[level] = 0.0;
	This->m2_pending[level] = 1.0;
	os_unfair_lock_unlock(&This->lock);
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setBandpass(BMMultiLevelSVF *This, double fc, double Q, size_t level){
    assert(level < This->numLevels);
    
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = 0.0;
	This->m1_pending[level] = 2.0 * This->k_pending[level];
	This->m2_pending[level] = 0.0;
	os_unfair_lock_unlock(&This->lock);
	
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setHighpass(BMMultiLevelSVF *This, double fc, size_t level){
    BMMultiLevelSVF_setHighpassQ(This, fc, 1./sqrtf(2.), level);
}

void BMMultiLevelSVF_setHighpassQ(BMMultiLevelSVF *This, double fc, double Q, size_t level){
    assert(level < This->numLevels);
    
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = 1.0;
	This->m1_pending[level] = 0.0;
	This->m2_pending[level] = 0.0;
	os_unfair_lock_unlock(&This->lock);
	
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setNotch(BMMultiLevelSVF *This, double fc, double Q, size_t level){
    assert(level < This->numLevels);
    
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = 1.0;
	This->m1_pending[level] = 0.0;
	This->m2_pending[level] = 1.0;
	os_unfair_lock_unlock(&This->lock);
    
    This->shouldUpdateParam = true;
}


void BMMultiLevelSVF_setAllpass(BMMultiLevelSVF *This, double fc, double Q, size_t level){
    assert(level < This->numLevels);
    
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = 1.0;
	This->m1_pending[level] = This->k_pending[level];
	This->m2_pending[level] = 1.0;
	os_unfair_lock_unlock(&This->lock);
    
    This->shouldUpdateParam = true;
}



void BMMultiLevelSVF_setBell(BMMultiLevelSVF *This, double fc, double gainDb, double Q, size_t level){
    assert(level < This->numLevels);
	
	double A = BM_DB_TO_GAIN(gainDb);
	
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = 1.0;
	This->m1_pending[level] = A * This->k_pending[level];
	This->m2_pending[level] = 1.0;
	os_unfair_lock_unlock(&This->lock);
    
    This->shouldUpdateParam = true;
}


void BMMultiLevelSVF_setBellWithSkirt(BMMultiLevelSVF *This, double fc, double bellGainDb, double skirtGainDb, double Q, size_t level){
	assert(level < This->numLevels);
	
	double A = BM_DB_TO_GAIN(bellGainDb);
	double B = BM_DB_TO_GAIN(skirtGainDb);
	
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = B;
	This->m1_pending[level] = A * This->k_pending[level];
	This->m2_pending[level] = B;
	os_unfair_lock_unlock(&This->lock);
	
	This->shouldUpdateParam = true;
}


void BMMultiLevelSVF_setLowShelf(BMMultiLevelSVF *This, double fc, double gainDb, size_t level){
    BMMultiLevelSVF_setLowShelfS(This, fc, gainDb, 1.0, level);
}

void BMMultiLevelSVF_setLowShelfS(BMMultiLevelSVF *This, double fc, double gainDb, double S, size_t level){
    assert(level < This->numLevels);
    
	double A = sqrt(BM_DB_TO_GAIN(gainDb));
	
	double Q = S / sqrt(2.0);
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = 1.0;
	This->m1_pending[level] = A * This->k_pending[level];
	This->m2_pending[level] = A * A;
	os_unfair_lock_unlock(&This->lock);
    
    This->shouldUpdateParam = true;
}

void BMMultiLevelSVF_setHighShelf(BMMultiLevelSVF *This, double fc, double gainDb, size_t level){
    BMMultiLevelSVF_setHighShelfS(This, fc, gainDb, 1., level);
}

void BMMultiLevelSVF_setHighShelfS(BMMultiLevelSVF *This, double fc, double gainDb, double S, size_t level){
	assert(level < This->numLevels);
	
	double A = sqrt(BM_DB_TO_GAIN(gainDb));
	
	double Q = S / sqrt(2.0);
	os_unfair_lock_lock(&This->lock);
	BMMultiLevelSVF_setCoefficientsHelper(This, fc, Q, level);
	This->m0_pending[level] = A * A;
	This->m1_pending[level] = A * This->k_pending[level];
	This->m2_pending[level] = 1.0;
	os_unfair_lock_unlock(&This->lock);
	
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
	// *** these formulae come from the function MatchDF1Coeff. Pay special attention to the signs and to how the function SqrSqrtSimpify affects the signs.
	//
	// This is g after running the function SqrSqrtSimplify:
	double g = sqrt( (-1.0 - b1 - b2) / (-1.0 + b1 - b2) );
	
	// we do the SqrSqrtSimplify manually on k
	// This is k after running the function SqrSqrtSimplify:
	double k = sqrt( ((b2 - 1.0)*(b2 - 1.0)) / ((1.0 + b2)*(1.0 + b2) - (b1 * b1)));
	
	double D = (1.0 + g * (g + k));
	// from coef1[[1]] on page 1 and 2 of https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf
	double g0 = g / D;
	// coef1[[2]]
	double g1 = - 1.0 + (1.0 / D);
	// coef2[[1]]
	double g2 = (g * g) / D;
	
	// https://cytomic.com/files/dsp/SvfLinearTrapezoidalSin.pdf
	// *** See section: "Solve for the mixed output SVF to show relation to regular DF1 coefficients..."
	//
	// for m0 we can use abs instead of SqrSqrtSimplify
	double m0 = fabs((a0 - a1 + a2) / (1.0 - b1 + b2));
	//
	// This is the result of SqrSqrtSimplify on m1
	double m1 = 2.0 * sqrt( ((a0 - a2) * (a0 - a2)) / ((1.0 + b2)*(1.0 + b2) - (b1 * b1)) );
	//
	// for m2 we can use abs instead of SqrSqrtSimplify
	double m2 = fabs((a0 + a1 + a2) / (1.0 + b1 + b2));
	
	// In MatchDF1Coeff[] the signs of k and m1 are opposite. We have already
	// used k above without changing the sign so we will negate m1 here.
	m1 = -m1;
	
	// set the results of g and k calculations to the pending coefficients
	os_unfair_lock_lock(&This->lock);
	This->g0_pending[level] = g0;
	This->g1_pending[level] = g1;
	This->g2_pending[level] = g2;
	This->k_pending[level]  = k;
	This->m0_pending[level] = m0;
	This->m1_pending[level] = m1;
	This->m2_pending[level] = m2;
	os_unfair_lock_unlock(&This->lock);
	
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

