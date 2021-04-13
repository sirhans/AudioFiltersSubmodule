//
//  BMMultiLevelBiquad.c
//  VelocityFilter
//
//  Created by Hans on 14/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMMultiLevelBiquad.h"
#include "Constants.h"
#include "BMComplexMath.h"
#include "BMGetOSVersion.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

//#ifdef __cplusplus
//extern "C" {
//#endif

/*
 * function declarations for use within this file
 */

// evaluate the combined transfer function of all levels of the filter
// at the frequency specified by z
DSPDoubleComplex BMMultiLevelBiquad_tfEval(BMMultiLevelBiquad *This, DSPDoubleComplex z);
DSPDoubleComplex BMMultiLevelBiquad_tfEvalAtLevel(BMMultiLevelBiquad *This, DSPDoubleComplex z,size_t level);

// Destroy the current filter setup and create a new one.
// This function must be called only from the audio processing thread.
void BMMultiLevelBiquad_recreate(BMMultiLevelBiquad *This);

// Create a filter setup object
void BMMultiLevelBiquad_create(BMMultiLevelBiquad *This);


// this is a thread-safe way to update filter coefficients
void BMMultiLevelBiquad_enqueueUpdate(BMMultiLevelBiquad *This);


// this function updates the filter immediately and is safe to call
// only from the audio thread. It changes the filter coefficients in realtime
// if possible; otherwise it calls _recreate.
void BMMultiLevelBiquad_updateNow(BMMultiLevelBiquad *This);

//Enable all levels in foreground, disable in-active level in background
extern inline void BMMultiLevelBiquad_updateLevels(BMMultiLevelBiquad *This);

// returns true if the operating system supports vDSP_biquadm_SetCoefficentsSingle()
bool BMMultiLevelBiquad_OSSupportsRealtimeUpdate(void);

/* end internal function declarations */





void BMMultiLevelBiquad_processBufferStereo(BMMultiLevelBiquad *This, const float* inL, const float* inR, float* outL, float* outR, size_t numSamples){
    // this function is only for two channel filtering
    assert(This->numChannels == 2);
    
    // update filter coefficients if necessary
    if (This->needsUpdate) BMMultiLevelBiquad_updateNow(This);
    
    //Levels
    BMMultiLevelBiquad_updateLevels(This);
    
    // link the two input buffers into a single multidimensional array
    const float* twoChannelInput [2] = {inL, inR};
    
    // link the two output buffers into a single multidimensional array
    float* twoChannelOutput [2] = {outL, outR};
    
    // apply a multilevel biquad filter to both channels
    vDSP_biquadm(This->multiChannelFilterSetup, (const float* _Nonnull * _Nonnull)twoChannelInput, 1, twoChannelOutput, 1, numSamples);
    
    BMSmoothGain_processBuffer(&This->gain, outL, outR, outL, outR, numSamples);
}






void BMMultiLevelBiquad_processBuffer4(BMMultiLevelBiquad *This,
                                       const float* in1, const float* in2, const float* in3, const float* in4,
                                       float* out1, float* out2, float* out3, float* out4,
                                       size_t numSamples){
    // this function is only for four channel filtering
    assert(This->numChannels == 4);
    
    // update filter coefficients if necessary
    if (This->needsUpdate) BMMultiLevelBiquad_updateNow(This);
    
    //Levels
    BMMultiLevelBiquad_updateLevels(This);
    
    // link the two input buffers into a single multidimensional array
    const float* fourChannelInput [4] = {in1, in2, in3, in4};
    
    // link the two output buffers into a single multidimensional array
    float* fourChannelOutput [4] = {out1, out2, out3, out4};
    
    
    // apply a multilevel biquad filter to both channels
    vDSP_biquadm(This->multiChannelFilterSetup, (const float* _Nonnull * _Nonnull)fourChannelInput, 1, fourChannelOutput, 1, numSamples);
    
    // apply a gain adjustment
    const float *inputs [4] = {out1, out2, out3, out4};
	float *outputs [4] = {out1, out2, out3, out4};
    BMSmoothGain_processBuffers(&This->gain, inputs, outputs, 4, numSamples);
}





void BMMultiLevelBiquad_processBufferMono(BMMultiLevelBiquad *This, const float* input, float* output, size_t numSamples){
    
    // this function is only for single channel filtering
    assert(This->numChannels == 1);
    
    
    // update filter coefficients if necessary
    if (This->needsUpdate) BMMultiLevelBiquad_updateNow(This);
    
    //Levels
    BMMultiLevelBiquad_updateLevels(This);
    
    // if using the multiChannel filter for single channel processing
    if(This->useBiquadm){
        // biquadm requires arrays of pointers as input and output
        const float* inputP [1] = {input};
        float* outputP [1] = {output};
        
        // apply a multiChannel biquad filter
        vDSP_biquadm(This->multiChannelFilterSetup, (const float* _Nonnull * _Nonnull)inputP, 1, outputP, 1, numSamples);
        
        
        // if using the single channel filter
    } else {
        vDSP_biquad(This->singleChannelFilterSetup, This->monoDelays, input, 1, output, 1, numSamples);
    }
    
    BMSmoothGain_processBufferMono(&This->gain, output, output, numSamples);
}





// Find out if the OS supports vDSP_biquadm updates in realtime
bool BMMultiLevelBiquad_OSSupportsRealtimeUpdate(){
    
    bool OSSupportsRealtimeUpdate = false;
    
    // iOS >= 9.0 (build# 15) supports realtime updates
    if (BM_isiOS() && BM_getOSMajorBuildNumber() >= 15)OSSupportsRealtimeUpdate = true;
    
    // Mac OS X >= 10.0 (build#14) supports them too
    if (BM_isMacOS() && BM_getOSMajorBuildNumber() >= 14)OSSupportsRealtimeUpdate = true;
    
    return OSSupportsRealtimeUpdate;
}





void BMMultiLevelBiquad_init(BMMultiLevelBiquad *This,
                             size_t numLevels,
                             float sampleRate,
                             bool isStereo,
                             bool monoRealTimeUpdate,
                             bool smoothUpdate){
    // initialize pointers to null to prevent errors when calling
    // free() in the destroy function
    This->multiChannelFilterSetup = NULL;
    This->singleChannelFilterSetup = NULL;
    This->coefficients_d = NULL;
    // This->coefficients_f = NULL;
    This->monoDelays = NULL;
    
    This->needsUpdate = false;
    This->sampleRate = sampleRate;
    This->numLevels = numLevels;
    This->numChannels = isStereo ? 2 : 1;
    This->useSmoothUpdate = smoothUpdate;
    This->needUpdateActiveLevels = false;
    This->activeLevels = malloc(sizeof(bool)*numLevels);
    for(int i=0;i<numLevels;i++){
        This->activeLevels[i] = true;
    }
    
	// EDITED: the use of vDSP_biquadm was optional in older versions. Now we always use it.
    This->useBiquadm = true;
    
    // We will update in realtime if the OS supports it and we are using
    // vDSP_biquadm
    This->useRealTimeUpdate = false;
    if(BMMultiLevelBiquad_OSSupportsRealtimeUpdate() && This->useBiquadm)
        This->useRealTimeUpdate = true;
    
    
    // Allocate memory for 5 coefficients per filter,
    // 2 filters per level (left and right channels)
    This->coefficients_d = malloc(numLevels*5*This->numChannels*sizeof(double));
    
    // repeat the allocation for floating point coefficients. We need
    // both double and float to support realtime updates
    // This->coefficients_f = malloc(numLevels*5*This->numChannels*sizeof(float));
    
    // Allocate 2*numLevels + 2 floats for mono delay memory
    if(!This->useBiquadm)
        This->monoDelays = calloc((2*numLevels + 2),sizeof(float));
    
    
    // start with all levels on bypass
    for (size_t i=0; i<numLevels; i++) {
        BMMultiLevelBiquad_setBypass(This, i);
    }
    
    // set 0db of gain
    BMSmoothGain_init(&This->gain, sampleRate);
    BMSmoothGain_init(&This->gain2, sampleRate);
    BMMultiLevelBiquad_setGain(This,0.0);
    
    // setup filter struct
    BMMultiLevelBiquad_create(This);
}




/*!
 *BMMultiLevelBiquad_init4
 */
void BMMultiLevelBiquad_init4(BMMultiLevelBiquad *This,
                             size_t numLevels,
                             float sampleRate,
                             bool smoothUpdate){
    
    // init as stereo to make use of the code that is in the existing init function
    BMMultiLevelBiquad_init(This, numLevels, sampleRate, true, false, smoothUpdate);
    
    // change the number of channels to 4
    This->numChannels = 4;
    
    // Allocate memory for 5 coefficients per filter,
    // 2 filters per level (left and right channels)
    free(This->coefficients_d);
    This->coefficients_d = malloc(numLevels*5*This->numChannels*sizeof(double));
    
    // repeat the allocation for floating point coefficients. We need
    // both double and float to support realtime updates
    // free(This->coefficients_f);
    // This->coefficients_f = malloc(numLevels*5*This->numChannels*sizeof(float));
    
    
    // start with all levels on bypass
    for (size_t i=0; i<numLevels; i++) {
        BMMultiLevelBiquad_setBypass(This, i);
    }
    
    // set 0db of gain
    BMMultiLevelBiquad_setGain(This,0.0f);
    
    // update the filter struct
    BMMultiLevelBiquad_recreate(This);
}





void BMMultiLevelBiquad_setGain(BMMultiLevelBiquad *This, float gain_db){
    BMSmoothGain_setGainDb(&This->gain, gain_db);
    BMSmoothGain_setGainDb(&This->gain2, gain_db);
}

void BMMultiLevelBiquad_setGainInstant(BMMultiLevelBiquad *This, float gain_db){
    BMSmoothGain_setGainDbInstant(&This->gain, gain_db);
    BMSmoothGain_setGainDbInstant(&This->gain2, gain_db);
}



void BMMultiLevelBiquad_queueUpdate(BMMultiLevelBiquad *This){
    This->needsUpdate = true;
}




inline void BMMultiLevelBiquad_updateNow(BMMultiLevelBiquad *This){
    
    // using realtime updates
    if(This->useRealTimeUpdate){
//        // convert the coefficients to floating point
//        for(size_t i=0; i<This->numLevels*This->numChannels*5; i++){
//            This->coefficients_f[i] = This->coefficients_d[i];
//        }
        if(This->useSmoothUpdate){
            //rate close to 1 mean it's change more slowly
            vDSP_biquadm_SetTargetsDouble(This->multiChannelFilterSetup, This->coefficients_d, 0.995, 0.05, 0, 0, This->numLevels, This->numChannels);
        }else{
            // update the coefficients
            vDSP_biquadm_SetCoefficientsDouble(This->multiChannelFilterSetup, This->coefficients_d, 0, 0, This->numLevels, This->numChannels);
        }
    }
	// not using realtime updates
	else {
        BMMultiLevelBiquad_recreate(This);
    }
    
    This->needsUpdate = false;
}


void BMMultiLevelBiquad_create(BMMultiLevelBiquad *This){
    
    if(This->useBiquadm){
        This->multiChannelFilterSetup =
        vDSP_biquadm_CreateSetup(This->coefficients_d, This->numLevels, This->numChannels);
    }
    
    else {
        This->singleChannelFilterSetup =
        vDSP_biquad_CreateSetup(This->coefficients_d, This->numLevels);
        memset(This->monoDelays,0,sizeof(float) * (2*This->numLevels + 2));
    }
}



inline void BMMultiLevelBiquad_recreate(BMMultiLevelBiquad *This){
    
    if(This->useBiquadm){
        vDSP_biquadm_DestroySetup(This->multiChannelFilterSetup);
        This->multiChannelFilterSetup =
        vDSP_biquadm_CreateSetup(This->coefficients_d, This->numLevels, This->numChannels);
    }
    
    else {
        vDSP_biquad_DestroySetup(This->singleChannelFilterSetup);
        This->singleChannelFilterSetup =
        vDSP_biquad_CreateSetup(This->coefficients_d, This->numLevels);
        memset(This->monoDelays,0,sizeof(float) * (2*This->numLevels + 2));
    }
}




// we are doing this to change the name of the function from destroy to free
// without breaking old code that calls destroy
void BMMultiLevelBiquad_free(BMMultiLevelBiquad* This){
	// the pragma commands silence the compiler warning when we call this deprecated function
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	BMMultiLevelBiquad_destroy(This);
	#pragma clang diagnostic pop
}




void BMMultiLevelBiquad_destroy(BMMultiLevelBiquad *This){
    if(This->coefficients_d) free(This->coefficients_d);
    // if(This->coefficients_f) free(This->coefficients_f);
    if(This->monoDelays) free(This->monoDelays);
    This->coefficients_d = NULL;
    // This->coefficients_f = NULL;
    This->monoDelays = NULL;
    
    if(This->numChannels > 1)
        vDSP_biquadm_DestroySetup(This->multiChannelFilterSetup);
    else
        vDSP_biquad_DestroySetup(This->singleChannelFilterSetup);
    This->multiChannelFilterSetup = NULL;
    This->singleChannelFilterSetup = NULL;
}




#pragma mark - Active level
inline void BMMultiLevelBiquad_updateLevels(BMMultiLevelBiquad *This){
	// vDSP_biquad doesn't support deactivating filters. To use vDSP_biquadm
	// for mono filters, set the monoRealtimeUpdate argument to true when you
	// init the filter.
	assert(This->useBiquadm);
	
    if(This->needUpdateActiveLevels){
        printf("disabling inactive filter levels\n");
        This->needUpdateActiveLevels = false;
		vDSP_biquadm_SetActiveFilters(This->multiChannelFilterSetup, This->activeLevels);
    }
}

/*
 MultiLevelBiquad posses several filters inside. We can active/disable each of them by the setActiveOnLevel function.
 */
void BMMultiLevelBiquad_setActiveOnLevel(BMMultiLevelBiquad *This,bool active,size_t level){
    //Keep track which should be active
    This->activeLevels[level] = active;
    This->needUpdateActiveLevels = true;
}

#pragma mark - Set filter parameters
void BMMultiLevelBiquad_setCoefficientZ(BMMultiLevelBiquad *This,size_t level,double* coeff){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        double* a1 = b0 + 3;
        double* a2 = b0 + 4;
        
        *b0 = coeff[0 + i*5];
        *b1 = coeff[1 + i*5];
        *b2 = coeff[2 + i*5];
        *a1 = coeff[3 + i*5];
        *a2 = coeff[4 + i*5];
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}

//Bypass function is still allow filter to be processed. It only set all parameters back to 0 to achieve the bypass effects. If you want to actually disable it, call setActiveOnLevel function.
void BMMultiLevelBiquad_setBypass(BMMultiLevelBiquad *This, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        double* a1 = b0 + 3;
        double* a2 = b0 + 4;
        
        *b0 = 1.0;
        *b1 = *b2 = *a1 = *a2 = 0.0;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
    
}



// based on formula in 2.3.10 of Digital Filters for Everyone by Rusty Allred
void BMMultiLevelBiquad_setHighShelf(BMMultiLevelBiquad *This, float fc, float gain_db, size_t level){
    assert(level < This->numLevels);
    
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        double* a1 = b0 + 3;
        double* a2 = b0 + 4;
        
        float gainV = BM_DB_TO_GAIN(gain_db);
        
        double gamma = tanf(M_PI * fc / This->sampleRate);
        double gamma_2 = gamma*gamma;
        double sqrt_gain = sqrtf(gainV);
        double g_d;
        
        // conditionally set G
        double G;
        if (gainV > 2.0){
            G = gainV * M_SQRT2 * 0.5;
            double G_2 = G*G;
            g_d = pow((G_2 - 1.0)/(gainV*gainV - G_2), 0.25);
        }
        else {
            if (gainV >= 0.5) {
                G = sqrt_gain;
                g_d = pow(1/gainV,0.25);
            }
            else{
                G = gainV * M_SQRT2;
                double G_2 = G*G;
                g_d = pow((G_2 - 1.0)/(gainV*gainV - G_2), 0.25);
            }
        }
        
        // compute reuseable variables
        double g_d_2 = g_d*g_d;
        double g_n = g_d * sqrt_gain;
        double g_n_2 = g_n * g_n;
        double sqrt_2_g_d_gamma = M_SQRT2 * g_d * gamma;
        double sqrt_2_g_n_gamma = M_SQRT2 * g_n * gamma;
        double gamma_2_plus_g_d_2 = gamma_2 + g_d_2;
        double gamma_2_plus_g_n_2 = gamma_2 + g_n_2;
        
        double one_over_denominator = 1.0f / (gamma_2_plus_g_d_2 + sqrt_2_g_d_gamma);
        
        *b0 = (gamma_2_plus_g_n_2 + sqrt_2_g_n_gamma) * one_over_denominator;
        *b1 = 2.0f * (gamma_2 - g_n_2) * one_over_denominator;
        *b2 = (gamma_2_plus_g_n_2 - sqrt_2_g_n_gamma) * one_over_denominator;
        
        *a1 = 2.0f * (gamma_2 - g_d_2) * one_over_denominator;
        *a2 = (gamma_2_plus_g_d_2 - sqrt_2_g_d_gamma)*one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}





/*!
 *BMMultiLevelBiquad_setHighShelfAdjustableSlope
 *
 * @abstract adjustable high shelf filter from Robert Bristow-Johnson cookbook
 *
 * @param This pointer to an initialized filter struct
 * @param fc   filter cutoff frequency
 * @param gain_db shelf gain in decibels
 * @param slope in [0.3,1], where 0.5 is equivalent to first order shelf slope and 1 is equivalent to second order shelf slope
 * @param level the index of the filter in the biquad cascade
 */
void BMMultiLevelBiquad_setHighShelfAdjustableSlope(BMMultiLevelBiquad *This, float fc, float gain_db, float slope, size_t level){
    assert(level < This->numLevels);
    assert(0.3 <= slope && slope <= 1.0);
    

    double A = pow(10.0,gain_db/40.0);
    double w0 = 2.0 * M_PI * (fc / This->sampleRate);
    double alpha = sin(w0)/2.0 * sqrt( (A + 1.0/A) * (1.0/slope - 1.0) + 2.0);
    double twoSqrtAalpha = 2.0 * sqrt(A) * alpha;

    double a0, a1, a2, b0, b1, b2;
    
    b0 =      A*( (A+1.0) + (A-1.0)*cos(w0) + twoSqrtAalpha );
    b1 = -2.0*A*( (A-1.0) + (A+1.0)*cos(w0)                 );
    b2 =      A*( (A+1.0) + (A-1.0)*cos(w0) - twoSqrtAalpha );
    a0 =          (A+1.0) - (A-1.0)*cos(w0) + twoSqrtAalpha;
    a1 =    2.0*( (A-1.0) - (A+1.0)*cos(w0)                 );
    a2 =          (A+1.0) - (A-1.0)*cos(w0) - twoSqrtAalpha;

    // normalize a0 to 1
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    
    // for left and right channels, set coefficients
    // these formulae are from the RBJ filter cookbook
    for(size_t i=0; i < This->numChannels; i++){
        double* b0p = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1p = b0p + 1;
        double* b2p = b0p + 2;
        double* a1p = b0p + 3;
        double* a2p = b0p + 4;
        
        *b0p = b0;
        *b1p = b1;
        *b2p = b2;
        *a1p = a1;
        *a2p = a2;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}



void BMMultiLevelBiquad_setHighShelfFirstOrder(BMMultiLevelBiquad *This, float fc, float gain_db, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        double* a1 = b0 + 3;
        double* a2 = b0 + 4;
        
        float gainV = BM_DB_TO_GAIN(gain_db);
        
        // if the gain is nontrivial
        {
            double gamma = tanf(M_PI * fc / This->sampleRate);
            double one_over_denominator;
            if(gainV>1.0f){
                one_over_denominator = 1.0f / (gamma + 1.0f);
                *b0 = (gamma + gainV) * one_over_denominator;
                *b1 = (gamma - gainV) * one_over_denominator;
                *a1 = (gamma - 1.0f) * one_over_denominator;
            }else{
                one_over_denominator = 1.0f / (gamma*gainV + 1.0f);
                *b0 = gainV*(gamma + 1.0f) * one_over_denominator;
                *b1 = gainV*(gamma - 1.0f) * one_over_denominator;
                *a1 = (gainV*gamma - 1.0f) * one_over_denominator;
            }
            
            *b2 = 0.0f;
            *a2 = 0.0f;
        }
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}





void BMMultiLevelBiquad_setLowShelfFirstOrder(BMMultiLevelBiquad *This, float fc, float gain_db, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        double* a1 = b0 + 3;
        double* a2 = b0 + 4;
        
        float gainV = BM_DB_TO_GAIN(gain_db);
        
        // if the gain is nontrivial
        {
            double gamma = tanf(M_PI * fc / This->sampleRate);
            double one_over_denominator;
            if(gainV>1.0f){
                one_over_denominator = 1.0f / (gamma + 1.0f);
                *b0 = (gamma * gainV + 1.0f) * one_over_denominator;
                *b1 = (gamma * gainV - 1.0f) * one_over_denominator;
                *a1 = (gamma - 1.0f) * one_over_denominator;
            }else{
                one_over_denominator = 1.0f / (gamma + gainV);
                *b0 = gainV*(gamma + 1.0f) * one_over_denominator;
                *b1 = gainV*(gamma - 1.0f) * one_over_denominator;
                *a1 = (gamma - gainV) * one_over_denominator;
            }
            
            *b2 = 0.0f;
            *a2 = 0.0f;
        }
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}






// set a low shelf filter at on the specified level in both
// channels and update filter settings
// based on formula in 2.3.10 of Digital Filters for Everyone by Rusty Allred
void BMMultiLevelBiquad_setLowShelf(BMMultiLevelBiquad *This, float fc, float gain_db, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        double* a1 = b0 + 3;
        double* a2 = b0 + 4;
        
        float gainV = BM_DB_TO_GAIN(gain_db);
        
        
        double gamma = tanf(M_PI * fc / This->sampleRate);
        double gamma_2 = gamma*gamma;
        double sqrt_gain = sqrtf(gainV);
        double g_d;
        
        // conditionally set G
        double G;
        if (gainV > 2.0){
            G = gainV * M_SQRT2 * 0.5;
            double G_2 = G*G;
            g_d = pow((G_2 - 1.0)/(gainV*gainV - G_2), 0.25);
        }
        else {
            if (gainV >= 0.5) {
                G = sqrt_gain;
                g_d = pow(1/gainV,0.25);
            }
            else{
                G = gainV * M_SQRT2;
                double G_2 = G*G;
                g_d = pow((G_2 - 1.0)/(gainV*gainV - G_2), 0.25);
            }
        }
        
        // compute reuseable variables
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
        
        *b0 = (g_n_2_gamma_2_plus_1 + sqrt_2_g_n_gamma) * one_over_denominator;
        *b1 = 2.0 * (g_n_2_gamma_2 - 1.0) * one_over_denominator;
        *b2 = (g_n_2_gamma_2_plus_1 - sqrt_2_g_n_gamma) * one_over_denominator;
        
        *a1 = 2.0 * (g_d_2_gamma_2 - 1.0) * one_over_denominator;
        *a2 = (g_d_2_gamma_2_plus_1 - sqrt_2_g_d_gamma)*one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}




/*!
 *BMMultiLevelBiquad_setLowShelfAdjustableSlope
 *
 * @abstract adjustable high shelf filter from Robert Bristow-Johnson cookbook
 *
 * @param This pointer to an initialized filter struct
 * @param fc   filter cutoff frequency
 * @param gain_db shelf gain in decibels
 * @param slope in [0.3,1], where 0.5 is equivalent to first order shelf slope and 1 is equivalent to second order shelf slope
 * @param level the index of the filter in the biquad cascade
 */
void BMMultiLevelBiquad_setLowShelfAdjustableSlope(BMMultiLevelBiquad *This, float fc, float gain_db, float slope, size_t level){
    assert(level < This->numLevels);
    assert(0.3 <= slope && slope <= 1.0);
    

    double A = pow(10.0,gain_db/40.0);
    double w0 = 2.0 * M_PI * (fc / This->sampleRate);
    double alpha = sin(w0)/2.0 * sqrt( (A + 1.0/A) * (1.0/slope - 1.0) + 2.0);
    double twoSqrtAalpha = 2.0 * sqrt(A) * alpha;

    double a0, a1, a2, b0, b1, b2;
    
    b0 =      A*( (A+1.0) - (A-1.0)*cos(w0) + twoSqrtAalpha );
    b1 =  2.0*A*( (A-1.0) - (A+1.0)*cos(w0)                 );
    b2 =      A*( (A+1.0) - (A-1.0)*cos(w0) - twoSqrtAalpha );
    a0 =          (A+1.0) + (A-1.0)*cos(w0) + twoSqrtAalpha;
    a1 =   -2.0*( (A-1.0) + (A+1.0)*cos(w0)                 );
    a2 =          (A+1.0) + (A-1.0)*cos(w0) - twoSqrtAalpha;

    // normalize a0 to 1
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    
    // for left and right channels, set coefficients
    // these formulae are from the R B-J filter cookbook
    for(size_t i=0; i < This->numChannels; i++){
        double* b0p = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1p = b0p + 1;
        double* b2p = b0p + 2;
        double* a1p = b0p + 3;
        double* a2p = b0p + 4;
        
        *b0p = b0;
        *b1p = b1;
        *b2p = b2;
        *a1p = a1;
        *a2p = a2;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}




/*!
 *BMMultiLevelBiquad_QToBW
 *
 * @abstract This keeps the filter width approximately constant as fc gets near Nyquist. It is not based on any theory; we derived it by observing and guessing.
 *
 * @param This pointer to an initialised struct
 * @param Q the Q factor of the filter
 * @param fc the cutoff frequency of the filter
 */
float BMMultiLevelBiquad_QToBW(BMMultiLevelBiquad *This, float Q, float fc){
	float nyq = This->sampleRate / 2.0f;
	float c = fc / nyq;
	if(Q <= 1.0f)
		// for fc=0 return fc/Q. For fc=Nyquist return 0.5*fc
		// note that this means the Q does nothing when fc=Nyquist, but it still
		// works ok when fc is near Nyquist.
		return (fc/Q) * powf(0.5f*Q,c);
	// else Q > 1.0f
	// for fc=0 return fc/Q. For fc=Nyquist return 0.66*fc/Q
	return (fc / Q) * powf(0.66f,c);
}






void BMMultiLevelBiquad_setBellQ(BMMultiLevelBiquad *This, float fc, float Q, float gain_db, size_t level){
	BMMultiLevelBiquad_setBell(This,
							   fc,
							   BMMultiLevelBiquad_QToBW(This,Q,fc),
							   gain_db,
							   level);
}






// based on formulae in 2.3.8 in Digital Filters are for Everyone,
// 2nd ed. by Rusty Allred
void BMMultiLevelBiquad_setBell(BMMultiLevelBiquad *This, float fc, float bandwidth, float gain_db, size_t level){
    assert(level < This->numLevels);
    
    float gainV = BM_DB_TO_GAIN(gain_db);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        double* a1 = b0 + 3;
        double* a2 = b0 + 4;
        
        // if gain is close to 1.0, bypass the filter
        if (fabsf(gain_db) < 0.01){
            *b0 = 1.0;
            *b1 = *b2 = *a1 = *a2 = 0.0;
        }
        
        // if the gain is nontrivial
        else {
            double alpha =  tan( (M_PI * bandwidth)   / This->sampleRate);
            double beta  = -cos( (2.0 * M_PI * fc) / This->sampleRate);
            double oneOverD;
            
            if (gainV < 1.0) {
                oneOverD = 1.0 / (alpha + gainV);
                // feed-forward coefficients
                *b0 = (gainV + alpha*gainV) * oneOverD;
                *b1 = 2.0 * beta * gainV * oneOverD;
                *b2 = (gainV - alpha*gainV) * oneOverD;
                
                // recursive coefficients
                *a1 = 2.0 * beta * gainV * oneOverD;
                *a2 = (gainV - alpha) * oneOverD;
            } else { // gain >= 1
                oneOverD = 1.0 / (alpha + 1.0);
                // feed-forward coefficients
                *b0 = (1.0 + alpha*gainV) * oneOverD;
                *b1 = 2.0 * beta * oneOverD;
                *b2 = (1.0 - alpha*gainV) * oneOverD;
                
                // recursive coefficients
                *a1 = 2.0 * beta * oneOverD;
                *a2 = (1.0 - alpha) * oneOverD;
            }
        }
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}






/*!
 *BMMultiLevelBiquad_setBellWithSkirt
 *
 * @abstract In addition to the usual controls for a bell filter, this function allows you to set the skirt gain, which is the gain at the DC and Nyquist frequencies.
 *
 *  @param This pointer to an initialised struct
 *  @param fc   bell centre frequency
 *  @param Q    Q = fc / bandwidth see Effect Design part 1 by Jon Dattorro for details
 *  @param bellGainDb the gain of the bell at peak or valley
 *  @param skirtGainDb the gain at DC and Nyquist
 *  @param level The index within this BMMultiLevelBiquad filter array
 */
void BMMultiLevelBiquad_setBellWithSkirt(BMMultiLevelBiquad *This, float fc, float Q, float bellGainDb, float skirtGainDb, size_t level){
    assert(level < This->numLevels);
    
	float bellGainV = BM_DB_TO_GAIN(bellGainDb);
	float skirtGainV = BM_DB_TO_GAIN(skirtGainDb);
	float bellFilterGainV = bellGainV / skirtGainV;
	
	double bandwidth = BMMultiLevelBiquad_QToBW(This, Q, fc);
	double alpha =  tan( (M_PI * bandwidth)   / This->sampleRate);
	double beta  = -cos( (2.0 * M_PI * fc) / This->sampleRate);
	double oneOverD;
	
	double b0b,b1b,b2b,a1b,a2b;
	
	// set up bell filter coefficients to make the bell affect the
	// difference (in dB) between the bell and skirt
	if (bellFilterGainV < 1.0) {
		oneOverD = 1.0 / (alpha + bellFilterGainV);
		// feed-forward coefficients
		b0b = (bellFilterGainV + alpha*bellFilterGainV) * oneOverD;
		b1b = 2.0 * beta * bellFilterGainV * oneOverD;
		b2b = (bellFilterGainV - alpha*bellFilterGainV) * oneOverD;
		
		// recursive coefficients
		a1b = 2.0 * beta * bellFilterGainV * oneOverD;
		a2b = (bellFilterGainV - alpha) * oneOverD;
	} else { // gain >= 1
		oneOverD = 1.0 / (alpha + 1.0);
		// feed-forward coefficients
		b0b = (1.0 + alpha*bellFilterGainV) * oneOverD;
		b1b = 2.0 * beta * oneOverD;
		b2b = (1.0 - alpha*bellFilterGainV) * oneOverD;
		
		// recursive coefficients
		a1b = 2.0 * beta * oneOverD;
		a2b = (1.0 - alpha) * oneOverD;
	}
	
	// scale the entire filter so that the skirt matches the skirt gain
	float b0bs = b0b*skirtGainV;
	float b1bs = b1b*skirtGainV;
	float b2bs = b2b*skirtGainV;
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double *b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double *b1 = b0 + 1;
        double *b2 = b0 + 2;
        double *a1 = b0 + 3;
        double *a2 = b0 + 4;
        
        *b0 = b0bs;
        *b1 = b1bs;
        *b2 = b2bs;
        *a1 = a1b;
        *a2 = a2b;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}





/*
  *This function approximates the integral of the magnitude transfer function
 * of a bell filter on [0,Nyquist], normalized so that the result is 1 when
 * the gain setting is 0 db.
 */
double approximateBellIntegral(BMMultiLevelBiquad *This,double bwHertz, double gainDb){
    // we normalize the bandwidth so that Nyquist = M_PI
    double bw = M_PI * bwHertz / (This->sampleRate/2.0f);
    
    // the following approximation was generated by Mathematica and output
    // using the "// CForm" command so that it could be entered below in
    // the exact form in which it came out from Mathematica. The method
    // of approximation was not clever and this approximation is accurate
    // only to about 92%. However, it obeys some important boundary conditions:
    //
    // 1. output is 1.0 when bw == 0
    // 2. output is 1.0 when gain == 0
    // 3. output is dbToGain(gainDb) when bw == Nyquist frequency
    //
    // positive gain approximation
    if(gainDb >= 0.0)
        return pow(pow(10.,gainDb/20.),0.07062002693643497*
                   pow(1. - 0.5797783118516802*pow(bw,0.47619047619047616),4.)*pow(bw,1.9047619047619047)*
                   gainDb + pow(bw,11./(11. + gainDb))/pow(M_PI,11./(11. + gainDb)));
    //
    // negative gain approximation
    return  pow(pow(10.,-gainDb/20.),-0.000282948025718253*
                pow(pow(bw,2.1) - 0.09036188195622674*pow(bw,4.2),1.5)*gainDb -
                pow(M_PI,25./(-25. + gainDb))/pow(bw,25./(-25. + gainDb)));
    
    // the following is the result of an exact symbolic integration of the integral
    //        double gainL = BM_DB_TO_GAIN(gainDb);
    //        if (gainL < 1.0)
    //           return (bw*M_PI*(cos(gainL/2.) + sin(gainL/2.)))
    //                    /
    //                  (bw*cos(gainL/2.) + sin(gainL/2.));
    //
    //        return (M_PI*(cos(gainL/2.) + bw*sin(gainL/2.)))
    //                /
    //               (cos(gainL/2.) + sin(gainL/2.));
}


/*
  *This is based on the setBell function above. The difference is that it
 * attempts to keep the overall gain at unity by adjusting the broadband
 * gain to compensate for the boost or cut of the bell filter. This allows
 * us to acheive extreme filter curves that approach the behavior of bandpass
 * and notch filters without clipping or loosing too much signal gain.
 */
void BMMultiLevelBiquad_setNormalizedBell(BMMultiLevelBiquad *This, float fc, float bandwidth, float controlGainDb, size_t level){
    assert(level < This->numLevels);
    
    // we start by finding out what gain we actually need to set on the bell
    // filter to place the peak at gain_db.
    // we iteratively converge on a value of gain_db that puts the peak of
    // of the bell near controlGainDb
    float gain_db = controlGainDb;
    float actualGain = FLT_MAX;
    while(fabs(actualGain - controlGainDb) > 0.1f){
        // how much will the gain be compensated, as compared to the bell filter
        float gainCompensation = BM_GAIN_TO_DB(1.0 / approximateBellIntegral(This,bandwidth, gain_db));
        // find out what the actual gain at fc will be
        actualGain = gain_db + gainCompensation;
        // adjust the gain setting of the bell filter to get the actual gain
        // closer to the gain control setting
        gain_db -= 0.5f * (actualGain - controlGainDb);
    }
    
    
    // compute the final gain compensation value in volt scale (not dB)
    float gainCompensationV = 1.0 / approximateBellIntegral(This,bandwidth, gain_db);
    
    // set the standard bell filter
    BMMultiLevelBiquad_setBell(This, fc, bandwidth, gain_db, level);
    
    // adjust the gain to keep the full spectrum magnitude near unity
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b0 + 2;
        
        *b0 *= gainCompensationV;
        *b1 *= gainCompensationV;
        *b2 *= gainCompensationV;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}


void BMMultiLevelBiquad_setLowPass12db(BMMultiLevelBiquad *This, double fc, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double gamma_sq = gamma * gamma;
        double sqrt_2_gamma = gamma * M_SQRT2;
        double one_over_denominator = 1.0 / (gamma_sq + sqrt_2_gamma + 1.0);
        
        *b0 = gamma_sq * one_over_denominator;
        *b1 = 2.0 * *b0;
        *b2 = *b0;
        
        *a1 = 2.0 * (gamma_sq - 1.0) * one_over_denominator;
        *a2 = (gamma_sq - sqrt_2_gamma + 1.0) * one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}





void BMMultiLevelBiquad_setLowPassQ12db(BMMultiLevelBiquad *This, double fc,double q, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double gamma_sq = gamma * gamma;
        double one_over_denominator = 1.0 / (q*gamma_sq + gamma + q);
        
        *b0 = q * gamma_sq * one_over_denominator;
        *b1 = 2.0 * *b0;
        *b2 = *b0;
        
        *a1 = 2.0 * q * (gamma_sq - 1.0) * one_over_denominator;
        *a2 = (q*gamma_sq - gamma + q) * one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}

void BMMultiLevelBiquad_setHighPass12db(BMMultiLevelBiquad *This, double fc,size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double gamma_sq = gamma * gamma;
        double sqrt_2_gamma = gamma * M_SQRT2;
        double one_over_denominator = 1.0 / (gamma_sq + sqrt_2_gamma + 1.0);
        
        *b0 = 1.0 * one_over_denominator;
        *b1 = -2.0 * one_over_denominator;
        *b2 = *b0;
        
        *a1 = 2.0 * (gamma_sq - 1.0) * one_over_denominator;
        *a2 = (gamma_sq - sqrt_2_gamma + 1.0) * one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}




void BMMultiLevelBiquad_setHighPass12dbNeg(BMMultiLevelBiquad *This, double fc,size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double gamma_sq = gamma * gamma;
        double sqrt_2_gamma = gamma * M_SQRT2;
        double one_over_denominator = 1.0 / (gamma_sq + sqrt_2_gamma + 1.0);
        
        *b0 = -1.0 * one_over_denominator;
        *b1 = 2.0 * one_over_denominator;
        *b2 = *b0;
        
        *a1 = 2.0 * (gamma_sq - 1.0) * one_over_denominator;
        *a2 = (gamma_sq - sqrt_2_gamma + 1.0) * one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}





void BMMultiLevelBiquad_setHighPassQ12db(BMMultiLevelBiquad *This, double fc,double q,size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double gamma_sq = gamma * gamma;
        double one_over_denominator = 1.0 / (q*gamma_sq + gamma + q);
        
        *b0 = q * one_over_denominator;
        *b1 = -2.0 * *b0;
        *b2 = *b0;
        
        *a1 = 2.0 * q * (gamma_sq - 1.0) * one_over_denominator;
        *a2 = (q*gamma_sq - gamma + q) * one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}


void BMMultiLevelBiquad_setLinkwitzRileyLP(BMMultiLevelBiquad *This, double fc, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        /*
         * filter coefficient code below was tested in Mathematica
         * formulae are from Digital Filters for Everyone, 2nd. ed.
         * by Rusty Allred
         *
         * Dn[gamma_] := gamma^2 + 2 gamma + 1
         *
         * b0[gamma_] := gamma^2/Dn[gamma]
         * b1[gamma_] := 2 lb0[gamma]
         * b2[gamma_] := lb0[gamma]
         * a1[gamma_] := 2 (gamma^2 - 1)/Dn[gamma]
         * a2[gamma_] := (gamma^2 - 2 gamma + 1)/Dn[gamma]
         *
         * gamma[fc_] := Tan[fc/2] (* in radians *)
         *
         */
        
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double gamma_sq = gamma * gamma;
        double two_gamma = gamma * 2.0;
        double one_over_denominator = 1.0 / (gamma_sq + two_gamma + 1.0);
        
        *b0 = gamma_sq * one_over_denominator;
        *b1 = 2.0 * *b0;
        *b2 = *b0;
        
        *a1 = 2.0 * (gamma_sq - 1.0) * one_over_denominator;
        *a2 = (gamma_sq - two_gamma + 1.0) * one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}








void BMMultiLevelBiquad_setLinkwitzRileyHP(BMMultiLevelBiquad *This, double fc, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        /*
         * filter coefficient code below was tested in Mathematica
         * formulae are from Digital Filters for Everyone, 2nd. ed.
         * by Rusty Allred
         *
         * gamma[fc_] := Tan[fc/2] (* in radians *)
         *
         * Dn[gamma_] := gamma^2 + 2 gamma + 1
         *
         * b0[gamma_] := -1 / Dn[gamma]
         * b1[gamma_] := 2 / Dn[gamma]
         * b2[gamma_] := -1 / Dn[gamma]
         *
         * a1[gamma_] := 2 (gamma^2 - 1) / Dn[gamma]
         * a2[gamma_] := (gamma^2 - 2 gamma + 1) / Dn[gamma]
         *
         * gamma[fc_] := Tan[fc/2]
         *
         */
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double gamma_sq = gamma * gamma;
        double two_gamma = gamma * 2.0;
        double one_over_denominator = 1.0 / (gamma_sq + two_gamma + 1.0);
        
        *b0 = -1.0 * one_over_denominator;
        *b1 = 2.0 * one_over_denominator;
        *b2 = -1.0 * one_over_denominator;
        
        *a1 = 2.0 * (gamma_sq - 1.0) * one_over_denominator;
        *a2 = (gamma_sq - two_gamma + 1.0) * one_over_denominator;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}




void BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(BMMultiLevelBiquad *This, double fc, size_t firstLevel){
    
    // a cascade of two butterworth filters makes a linkwitz-riley filter.
    // the highpass and lowpass pair sums to unity gain without the need to
    // invert the sign of one pair
    BMMultiLevelBiquad_setLowPass12db(This, fc, firstLevel);
    BMMultiLevelBiquad_setLowPass12db(This, fc, firstLevel+1);
}




void BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(BMMultiLevelBiquad *This, double fc, size_t firstLevel){
    
    // a cascade of two butterworth filters makes a linkwitz-riley filter.
    // the highpass and lowpass pair sums to unity gain without the need to
    // invert the sign of one pair
    BMMultiLevelBiquad_setHighPass12db(This, fc, firstLevel);
    BMMultiLevelBiquad_setHighPass12db(This, fc, firstLevel+1);
}




void BMMultiLevelBiquad_setLowPass6db(BMMultiLevelBiquad *This, double fc, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double one_over_denominator = 1.0 / (gamma + 1.0);
        
        *b0 = gamma * one_over_denominator;
        *b1 = *b0;
        *b2 = 0.0;
        
        *a1 = (gamma - 1.0) * one_over_denominator;
        *a2 = 0.0;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}


/*!
 *packFirstOrder
 *
 * packs two first order filters into a single biquad section. The first six inputs are the coefficients of the two first order filters. the last five outputs are the packed biquad coefficients.
 */
void packFirstOrder(double b0a, double b1a, double a1a,
					double b0b, double b1b, double a1b,
					double* b0, double* b1, double* b2, double* a1, double* a2){
	*b0 = b0a * b0b;
	*b1 = (b0a*b1b) + (b0b*b1a);
	*b2 = b1a * b1b;
	
	*a1 = a1a + a1b;
	*a2 = a1a * a1b;
}



// packs two first order filters into a single biquad section
void BMMultiLevelBiquad_setHighPassLowPass(BMMultiLevelBiquad *This, double highPassFc, double lowPassFc, size_t level){
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        double gamma = tan(M_PI * highPassFc / This->sampleRate);
        double one_over_denominator = 1.0 / (gamma + 1.0);
        
		double b0h = 1.0 * one_over_denominator;
		double b1h = -b0h;
		double a1h = (gamma - 1.0) * one_over_denominator;
		
		
        gamma = tan(M_PI * lowPassFc / This->sampleRate);
        one_over_denominator = 1.0 / (gamma + 1.0);
        
        double b0l = gamma * one_over_denominator;
		double b1l = b0l;
        double a1l = (gamma - 1.0) * one_over_denominator;
		
		// pack the two first order filters into the biquad section
		packFirstOrder(b0h, b1h, a1h,
					   b0l, b1l, a1l,
					   b0, b1, b2, a1, a2);
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}





void BMMultiLevelBiquad_setHighPass6db(BMMultiLevelBiquad *This, double fc, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double one_over_denominator = 1.0 / (gamma + 1.0);
        
        *b0 = 1.0 * one_over_denominator;
        *b1 = -1.0 * one_over_denominator;
        *b2 = 0.0;
        
        *a1 = (gamma - 1.0) * one_over_denominator;
        *a2 = 0.0;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}



void BMMultilevelBiquad_setAllpass2ndOrder(BMMultiLevelBiquad *This, double c1, double c2, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        // 2nd order allpass transfer function, calculated in Mathematica
        // by computing the product of 2 first order allpass filters
        //
        //        1 + (c1 + c2) z + c1 c2 z^2
        // H(z) = ---------------------------
        //         c1 c2 + (c1 + c2) z + z^2
        
        *b0 = c1 * c2;
        *b1 = c1 + c2;
        *b2 = 1.0;
        *a1 = c1 + c2;
        *a2 = c1 * c2;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}






void BMMultilevelBiquad_setAllpass1stOrder(BMMultiLevelBiquad *This, double c, size_t level){
    assert(level < This->numLevels);
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        // 1st order allpass transfer function
        //
        //         c + z^-1
        // H(z) = ----------
        //        1 + c z^-1
        
        *b0 = c;
        *b1 = 1.0f;
        *b2 = 0.0;
        *a1 = c;
        *a2 = 0.0;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}




void BMMultilevelBiquad_setCriticallyDampedPhaseCompensator(BMMultiLevelBiquad  *This, double lowpassFC, size_t level){
    // We find the allpass coefficient Beta that yields the same phase response
    // as the critically damped lowpass at lowpassFC.
    //
    // The following formulae were calculated in Mathematica.
    // We used numerical minimisation to solve the equations, then used
    // Wolfram alpha search to identify exact expressions that are equivalent
    // to the numerical output up to 15 decimal places.
    double c1 = 2.0 * M_2_PI; // 4 / pi
    double c2 = -1.0 + M_SQRT2 + sqrt(10.0 - 7.0*M_SQRT2);
    double c3 = 1.0 - c2;
    
    // get the cutoff in [0,pi]
    double wc = 2.0 * M_PI * lowpassFC / This->sampleRate;
    
    double allpassBeta = -2.0 +
                         (c1 * wc) +
                         (c2 * (cos(wc/2.0) - sin(wc/2.0))) +
                         (c3 * cos(wc));
    
    BMMultilevelBiquad_setAllpass1stOrder(This,allpassBeta,level);
}




/*
 * Computes the coefficient of the s^1 term in butterworth
 * polynomials
 *
 * Mathematica prototype
 *
 * BTWC[N_, k_] :=
 * -2 Cos[\[Pi] (2 k + N - 1)/(2 N)]
 *
 * reference: https://en.wikipedia.org/wiki/Butterworth_filter#Normalized_Butterworth_polynomials
 */
double butterworthCHelper(size_t filterOrder, size_t sectionNumber){
    double k = sectionNumber;
    double N = filterOrder;
    return -2.0 * cos(M_PI * (2.0 * k + N - 1) / (2.0 * N));
}



// DigitalToAnalogFcWarp[omega_] := 2 Tan[omega /2]
double digitalToAnalogFCWarp(float fcDigital){
    return 2.0 * tan(fcDigital / 2.0);
}




/*
 * sets up a single section of a butterworth lowpass filter that
 * is factored across several biquad sections.
 *
 * @param fc  filter cutoff in hz
 */
void BMMultiLevelBiquad_setBWLPSection(BMMultiLevelBiquad *This,
                                       double fc,
                                       size_t level,
                                       size_t filterOrder,
                                       size_t sectionNumber){
    assert(level < This->numLevels);
    
    /*
     * pre-calculate some values
     */
    double c = butterworthCHelper(filterOrder, sectionNumber);
    fc = digitalToAnalogFCWarp(M_PI*fc/(This->sampleRate/2.0));
    double fc2 = fc*fc;
    
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        
        // BTWzDenominator[z_, N_, n_, fc_] :=
        //      (4 + 2 BTWC[N, n] fc + fc*fc) +
        //      (-8 + 2 fc*fc)*z^(-1) +
        //      (4 - 2 BTWC[N, n] fc  + fc*fc)*z^-2
        double a0 = 4.0 + 2.0 * c * fc + fc2;
        *a1 = (-8.0 + 2.0 * fc2);
        *a2 = (4.0 - 2.0 * c * fc + fc2);
        
        
        // BTWzNumerator[z_, fc_] := fc*fc*(1 + 2 z^(-1) + z^(-2))
        *b0 = fc2;
        *b1 = 2.0 * fc2;
        *b2 = fc2;
        
        // normalize the a0 term to 1.0
        // a0 /= a0;
        *a1 /= a0;
        *a2 /= a0;
        *b0 /= a0;
        *b1 /= a0;
        *b2 /= a0;
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}






/*
 * sets up a butterworth lowpass filter of order 2*numLevels
 *
 * @param fc           cutoff frequency in hz
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use (order / 2)
 */
void BMMultiLevelBiquad_setHighOrderBWLP(BMMultiLevelBiquad *This, double fc, size_t firstLevel, size_t numLevels){
    
    // N is the filter order
    size_t N = numLevels * 2;
    
    for(size_t i=0; i<numLevels; i++)
        BMMultiLevelBiquad_setBWLPSection(This, fc, i+firstLevel, N, i+1);
}





/*
 * generate the s-domain coefficients for the analog prototype filter
 *
 * @param sectionNumber  biquad section number (1 is first section)
 * @param output         {a0, a1, a2, b0, b1, b2}
 */
void BMMultiLevelBiquad_getAnalogLegendreLPSection(size_t filterOrder,
                                                   size_t sectionNumber,
                                                   float* output){
    
    // filter order is an even number in [2,10]
    assert(filterOrder <= 10 && filterOrder >= 2);
    assert(filterOrder % 2 == 0);
    
    // number of sections is filterOrder / 2
    assert(sectionNumber <= filterOrder / 2);
    
    // o2s1 = {1, 1.4142, 1, 0, 0, 1};
    if (filterOrder == 2){
        float coefficients [6] = {1, 1.4142, 1, 0, 0, 1};
        memcpy(output, coefficients, sizeof(float)*6);
        return;
    }
    
    // o4s1 = {1, 1.0994, 0.4308, 0, 0, 0.4308};
    // o4s2 = {1, 0.4634, 0.9477, 0, 0, 0.9477};
    if (filterOrder == 4){
        if (sectionNumber == 1){
            float coefficients [6] = {1, 1.0994, 0.4308, 0, 0, 0.4308};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 2){
            float coefficients [6] = {1, 0.4634, 0.9477, 0, 0, 0.9477};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
    }
    
    // o6s1 = {1, 0.6180, 0.5830, 0, 0, 0.5830};
    // o6s2 = {1, 0.8778, 0.2502, 0, 0, 0.2502};
    // o6s3 = {1, 0.2304, 0.9696, 0, 0, 0.9696};
    if (filterOrder == 6){
        if (sectionNumber == 1){
            float coefficients [6] = {1, 0.6180, 0.5830, 0, 0, 0.5830};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 2){
            float coefficients [6] = {1, 0.8778, 0.2502, 0, 0, 0.2502};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 3){
            float coefficients [6] = {1, 0.2304, 0.9696, 0, 0, 0.9696};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
    }
    
    // o8s1 = {1, 0.6006, 0.3829, 0, 0, 0.3829};
    // o8s2 = {1, 0.3886, 0.7180, 0, 0, 0.7180};
    // o8s3 = {1, 0.13788, 0.9809, 0, 0, 0.9809};
    // o8s4 = {1, 0.7344, 0.1676, 0, 0, 0.1676};
    if (filterOrder == 8){
        if (sectionNumber == 1){
            float coefficients [6] = {1, 0.6006, 0.3829, 0, 0, 0.3829};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 2){
            float coefficients [6] = {1, 0.3886, 0.7180, 0, 0, 0.7180};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 3){
            float coefficients [6] = {1, 0.13788, 0.9809, 0, 0, 0.9809};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 4){
            float coefficients [6] = {1, 0.7344, 0.1676, 0, 0, 0.1676};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
    }
    
    // o10s1 = {1, 0.5548, 0.2702, 0, 0, 0.2702};
    // o10s2 = {1, 0.0918, 0.9870, 0, 0, 0.9870};
    // o10s3 = {1, 0.4284, 0.5282, 0, 0, 0.5282};
    // o10s4 = {1, 0.2650, 0.8013, 0, 0, 0.8013};
    // o10s5 = {1, 0.6344, 0.1218, 0, 0, 0.1218};
    if (filterOrder == 10){
        if (sectionNumber == 1){
            float coefficients [6] = {1, 0.5548, 0.2702, 0, 0, 0.2702};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 2){
            float coefficients [6] = {1, 0.0918, 0.9870, 0, 0, 0.9870};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 3){
            float coefficients [6] = {1, 0.4284, 0.5282, 0, 0, 0.5282};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 4){
            float coefficients [6] = {1, 0.2650, 0.8013, 0, 0, 0.8013};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
        if (sectionNumber == 5){
            float coefficients [6] = {1, 0.6344, 0.1218, 0, 0, 0.1218};
            memcpy(output, coefficients, sizeof(float)*6);
            return;
        }
    }
}




/*
 * adjust the coefficients of an analog s-domain prototype with cutoff
 * frequency at 1 radian so that the cutoff frequency will be omega radians
 * after we do the bilinear transform to go to the digital domain.
 *
 * @param omega             desired cutoff frequency in z domain, in radians
 * @param coefficientListIn s-domain prototype filter coefficients {a0, a1, a2, b0, b1, b2}
 * @param coefficientListOut s-domain coefficients after warping {a0, a1, a2, b0, b1, b2}
 */
void BMMuiltiLevelBiquad_freqWarpSDomain(float omega,
                                         const float* coefficientListIn,
                                         float* coefficientListOut){
    
    coefficientListOut[0] = coefficientListIn[0] / (omega*omega);
    coefficientListOut[1] = coefficientListIn[1] / omega;
    coefficientListOut[2] = coefficientListIn[2];
    coefficientListOut[3] = coefficientListIn[3] / (omega*omega);
    coefficientListOut[4] = coefficientListIn[4] / omega;
    coefficientListOut[5] = coefficientListIn[5];
    
}




/*
 * Convert the coefficient list of an s-domain filter to the z-domain
 * coefficients list using the bilinear transform:
 *
 * s = (z-1)/(z+1)
 *
 * @param sDomainCoefficients   {a0, a1, a2, b0, b1, b2}
 * @param zDomainCoefficients   {a0, a1, a2, b0, b1, b2}
 */
void BMMuiltiLevelBiquad_coefficientsStoZ(const float* sDomainCoefficients,
                                          float* zDomainCoefficients){
    float a0s = sDomainCoefficients[0];
    float a1s = sDomainCoefficients[1];
    float a2s = sDomainCoefficients[2];
    float b0s = sDomainCoefficients[3];
    float b1s = sDomainCoefficients[4];
    float b2s = sDomainCoefficients[5];
    
    // {a0s + a1s + a2s, -2 a0s + 2 a2s, a0s - a1s + a2s,
    //  b0s + b1s + b2s, -2 b0s + 2 b2s, b0s - b1s + b2s}
    zDomainCoefficients[0] = a0s + a1s + a2s;
    zDomainCoefficients[1] = -2.0 * a0s + 2.0 * a2s;
    zDomainCoefficients[2] = a0s - a1s + a2s;
    zDomainCoefficients[3] = b0s + b1s + b2s;
    zDomainCoefficients[4] = -2.0 * b0s + 2.0 * b2s;
    zDomainCoefficients[5] = b0s - b1s + b2s;
}





/*
 * Normalize the a0 coefficient to 1
 *
 * @param inputCoefficients          {a0, a1, a2, b0, b1, b2}
 * @param normalizedA0Coefficients   {a0, a1, a2, b0, b1, b2}
 */
void BMMuiltiLevelBiquad_NormalizeA0(const float* inputCoefficients,
                                     float* normalizedA0Coefficients){
    float a0 = inputCoefficients[0];
    float a1 = inputCoefficients[1];
    float a2 = inputCoefficients[2];
    float b0 = inputCoefficients[3];
    float b1 = inputCoefficients[4];
    float b2 = inputCoefficients[5];
    
    normalizedA0Coefficients[0] = 1.0;
    normalizedA0Coefficients[1] = a1/a0;
    normalizedA0Coefficients[2] = a2/a0;
    normalizedA0Coefficients[3] = b0/a0;
    normalizedA0Coefficients[4] = b1/a0;
    normalizedA0Coefficients[5] = b2/a0;
}






/*
 * sets up a single section of a Legendre lowpass filter that
 * is factored across several biquad sections.
 *
 * @param fc  filter cutoff in hz
 */
void BMMultiLevelBiquad_setLegendreLPSection(BMMultiLevelBiquad *This,
                                             double fc,
                                             size_t level,
                                             size_t filterOrder,
                                             size_t sectionNumber){
    assert(level < This->numLevels);
    
    // get the coefficients for the prototype filter in the analog domain
    float sDomainPrototypeCoefficients [6];
    BMMultiLevelBiquad_getAnalogLegendreLPSection(filterOrder,
                                                  sectionNumber,
                                                  sDomainPrototypeCoefficients);
    
    // compute the warped cutoff frequency of the analog prototype to
    // prepare for s to z domain transformation.
    double fcInRadians = M_PI * (fc / (This->sampleRate/2.0));
    double warpedAnalogFc = tan(0.5*fcInRadians);
    
    // warp the frequency of the s-domain prototype
    float sDomainCoefficientsWarped [6];
    BMMuiltiLevelBiquad_freqWarpSDomain(warpedAnalogFc,
                                        sDomainPrototypeCoefficients,
                                        sDomainCoefficientsWarped);
    
    // convert from s-domain to z-domain using the bilinear transform
    float zDomainCoefficients [6];
    BMMuiltiLevelBiquad_coefficientsStoZ(sDomainCoefficientsWarped,
                                         zDomainCoefficients);
    
    // normalize the A0 coefficient to 1
    BMMuiltiLevelBiquad_NormalizeA0(zDomainCoefficients,zDomainCoefficients);
    
    
    // for left and right channels, set coefficients
    for(size_t i=0; i < This->numChannels; i++){
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + i*5;
        double* b1 = b0 + 1;
        double* b2 = b1 + 1;
        double* a1 = b2 + 1;
        double* a2 = a1 + 1;
        
        *a1 = zDomainCoefficients[1];
        *a2 = zDomainCoefficients[2];
        *b0 = zDomainCoefficients[3];
        *b1 = zDomainCoefficients[4];
        *b2 = zDomainCoefficients[5];
    }
    
    BMMultiLevelBiquad_queueUpdate(This);
}






/*
 * sets up a Legendre Lowpass filter of order 2*numLevels
 *
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use. numLevels = (filterOrder / 2)
 */
void BMMultiLevelBiquad_setLegendreLP(BMMultiLevelBiquad *This, double fc, size_t firstLevel, size_t numLevels){
    
    // N is the filter order
    size_t N = numLevels * 2;
    
    for(size_t i=0; i<numLevels; i++)
        BMMultiLevelBiquad_setLegendreLPSection(This, fc, i+firstLevel, N, i+1);
}






/*!
 * besselFilterSectionGetQAndFcMultiplier
 *
 * @param filterOrder     the order of the filter, must be even
 * @param sectionNumber   the filter is divided into second order sections, numbered starting with one
 * @param fcMultiplier    multiply this number by the desired cutoff frequency when you set the cutoff for this second order section
 * @param Q               the Q for this section
 *
 * @abstract gets the Q and the factor to be multiplied by fc to build a bessel lowpass filter by cascading 2nd order adjustable Q lowpass filters. Source: https://gist.github.com/endolith/4982787 .
 * @discussion this method is questionable because the bilinear transform does not preserve group delay when converting bessel filters from the s to the z domain. We have not tested to see how well this method works in that respect. However, these filters are mainly intended to be used for smoothing low-frequency control signals, where the cutoff will be near zero, so this issue will not be a problem.
 */
void besselFilterSectionGetQAndFcMultiplier(size_t filterOrder,
                                            size_t sectionNumber,
                                            float* fcMultiplier,
                                            float* Q){
    assert(filterOrder % 2 == 0);
    assert(sectionNumber <= filterOrder/2);
    
    /*
     https://gist.github.com/endolith/4982787
     2: 0.57735026919
     4: 0.805538281842 0.521934581669
     6: 1.02331395383  0.611194546878 0.510317824749
     8: 1.22566942541  0.710852074442 0.559609164796 0.505991069397
     10: 1.41530886916  0.809790964842 0.620470155556 0.537552151325 0.503912727276
     12: 1.59465693507  0.905947107025 0.684008068137 0.579367238641 0.525936202016 0.502755558204
     14: 1.76552743493  0.998998442993 0.747625068271 0.624777082395 0.556680772868 0.519027293158 0.502045428643
     16: 1.9292718407   1.08906376917  0.810410302962 0.671382379377 0.591144659703 0.542678365981 0.514570953471 0.501578400482
     18: 2.08691792612  1.17637337045  0.872034231424 0.718163551101 0.627261751983 0.569890924765 0.533371782078 0.511523796759 0.50125489338
     20: 2.23926560629  1.26117120993  0.932397288146 0.764647810579 0.664052481472 0.598921924986 0.555480327396 0.526848630061 0.509345928377 0.501021580965
     */
    float QArray [10][10] = {
        {0.57735026919f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {0.805538281842f, 0.521934581669f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {1.02331395383f,  0.611194546878f, 0.510317824749f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {1.22566942541f,  0.710852074442f, 0.559609164796f, 0.505991069397f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {1.41530886916f,  0.809790964842f, 0.620470155556f, 0.537552151325f, 0.503912727276f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {1.59465693507f,  0.905947107025f, 0.684008068137f, 0.579367238641f, 0.525936202016f, 0.502755558204f,0.0f,0.0f,0.0f,0.0f},
        {1.76552743493f,  0.998998442993f, 0.747625068271f, 0.624777082395f, 0.556680772868f, 0.519027293158f, 0.502045428643f,0.0f,0.0f,0.0f},
        {1.9292718407f,   1.08906376917f,  0.810410302962f, 0.671382379377f, 0.591144659703f, 0.542678365981f, 0.514570953471f, 0.501578400482f, 0.0f, 0.0f},
        {2.08691792612f,  1.17637337045f,  0.872034231424f, 0.718163551101f, 0.627261751983f, 0.569890924765f, 0.533371782078f, 0.511523796759f, 0.50125489338f,0.0f},
        {2.23926560629f,  1.26117120993f,  0.932397288146f, 0.764647810579f, 0.664052481472f, 0.598921924986f, 0.555480327396f, 0.526848630061f, 0.509345928377f, 0.501021580965f}
    };
    
    /*
     2: 1.27201964951
     4: 1.60335751622   1.43017155999
     6: 1.9047076123    1.68916826762   1.60391912877
     8: 2.18872623053   1.95319575902   1.8320926012    1.77846591177
     10: 2.45062684305   2.20375262593   2.06220731793   1.98055310881   1.94270419166
     12: 2.69298925084   2.43912611431   2.28431825401   2.18496722634   2.12472538477   2.09613322542
     14: 2.91905714471   2.66069088948   2.49663434571   2.38497976939   2.30961462222   2.26265746534   2.24005716132
     16: 3.13149167404   2.87016099416   2.69935018044   2.57862945683   2.49225505119   2.43227707449   2.39427710712   2.37582307687
     18: 3.33237300564   3.06908580184   2.89318259511   2.76551588399   2.67073340527   2.60094950474   2.55161764546   2.52001358804   2.50457164552
     20: 3.52333123464   3.25877569704   3.07894353744   2.94580435024   2.84438325189   2.76691082498   2.70881411245   2.66724655259   2.64040228249   2.62723439989
     
     */
    float fMulArray [10][10] = {
        {1.27201964951f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {1.60335751622f, 1.43017155999f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {1.9047076123f,    1.68916826762f,   1.60391912877f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {2.18872623053f,   1.95319575902f,   1.8320926012f,    1.77846591177f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {2.45062684305f,   2.20375262593f,   2.06220731793f,   1.98055310881f,   1.94270419166f,0.0f,0.0f,0.0f,0.0f,0.0f},
        {2.69298925084f,   2.43912611431f,   2.28431825401f,   2.18496722634f,   2.12472538477f,   2.09613322542f,0.0f,0.0f,0.0f,0.0f},
        {2.91905714471f,   2.66069088948f,   2.49663434571f,   2.38497976939f,   2.30961462222f,   2.26265746534f,   2.24005716132f,0.0f,0.0f,0.0f},
        {3.13149167404f,   2.87016099416f,   2.69935018044f,   2.57862945683f,   2.49225505119f,   2.43227707449f,   2.39427710712f,   2.37582307687f,0.0f,0.0f},
        {3.33237300564f,   3.06908580184f,   2.89318259511f,   2.76551588399f,   2.67073340527f,   2.60094950474f,   2.55161764546f,   2.52001358804f,   2.50457164552f,0.0f},
        {3.52333123464f,   3.25877569704f,   3.07894353744f,   2.94580435024f,   2.84438325189f,   2.76691082498f,   2.70881411245f,   2.66724655259f,   2.64040228249f,   2.62723439989f}
    };
    
    *Q = QArray[(filterOrder/2)-1][sectionNumber-1];
    *fcMultiplier = fMulArray[(filterOrder/2)-1][sectionNumber-1];
}






/*!
 * BMMultiLevelBiquad_setBesselLPSection
 *
 * @param fc            filter cutoff in hz
 * @param level         level in the BMMultiLevelBiquad filter cascade
 * @param filterOrder   must be an even number
 * @param sectionNumber because there could be other unrelated filters in the cascade, level = startLevel + sectionNumber - 1. Note that section numbers start from one, not zero.
 *
 * @abstract sets up a single section of a Bessel lowpass filter that is factored across several biquad sections.
 */
void BMMultiLevelBiquad_setBesselLPSection(BMMultiLevelBiquad *This,
                                           double fc,
                                           size_t level,
                                           size_t filterOrder,
                                           size_t sectionNumber){
    
    for(size_t i = 0; i<filterOrder/2; i++){
        float fcMultiplier, q;
        besselFilterSectionGetQAndFcMultiplier(filterOrder,sectionNumber,&fcMultiplier,&q);
        BMMultiLevelBiquad_setLowPassQ12db(This, fc*fcMultiplier, q, level);
    }
}


/*!
 * BMMultiLevelBiquad_setBesselLP
 *
 * @abstract sets up a Bessel Lowpass filter of order 2*numLevels
 *
 * @param fc           cutoff frequency
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use. numLevels = (filterOrder / 2)
 */
void BMMultiLevelBiquad_setBesselLP(BMMultiLevelBiquad *This, double fc, size_t firstLevel, size_t numLevels){
    
    // N is the filter order
    size_t N = numLevels * 2;
    
    for(size_t i=0; i<numLevels; i++)
        BMMultiLevelBiquad_setBesselLPSection(This, fc, i+firstLevel, N, i+1);
}






/*!
 * BMMultiLevelBiquad_setCriticallyDampedLPSection
 *
 * @param fc            filter cutoff in hz
 * @param level         level in the BMMultiLevelBiquad filter cascade
 * @param filterOrder   must be an even number
 *
 * @abstract sets up a single section of a Bessel lowpass filter that is factored across several biquad sections.
 */
void BMMultiLevelBiquad_setCriticallyDampedLPSection(BMMultiLevelBiquad *This,
                                                     double fc,
                                                     size_t level,
                                                     size_t filterOrder){
    
//    // shift the cutoff frequency following formula 2 of the following paper:
//    // https://www.researchgate.net/publication/9043065_Design_and_responses_of_Butterworth_and_critically_damped_digital_filters
//    // \omega_0 = \frac{\omega_c}{\sqrt{2^\frac{1}{n} - 1}}
//    float fcShifted = fc / sqrtf(powf(2.0f, 1.0f/(2.0*filterOrder)) - 1.0f);
    
    // critically damped second order filter has Q of 0.5
    float Q = 0.5;
    
    for(size_t i = 0; i<filterOrder/2; i++)
        BMMultiLevelBiquad_setLowPassQ12db(This, fc, Q, level);
}






/*!
 * BMMultiLevelBiquad_setCriticallyDampedLP
 *
 * @abstract sets up a Critically-damped Lowpass filter of order 2*numLevels
 *
 * @param fc           cutoff frequency
 * @param firstLevel   the filter will consume numLevels contiguous
 *                     biquad sections, beginning with firstLevel
 * @param numLevels    number of biquad sections to use. numLevels = (filterOrder / 2)
 */
void BMMultiLevelBiquad_setCriticallyDampedLP(BMMultiLevelBiquad *This, double fc, size_t firstLevel, size_t numLevels){
    
    // N is the filter order
    size_t N = numLevels * 2;
    
    for(size_t i=0; i<numLevels; i++)
        BMMultiLevelBiquad_setCriticallyDampedLPSection(This, fc, i+firstLevel, N);
}




// evaluate the transfer function of the filter at all levels for the
// frequency specified by the complex number z
inline DSPDoubleComplex BMMultiLevelBiquad_tfEval(BMMultiLevelBiquad *This, DSPDoubleComplex z){
    
    DSPDoubleComplex z2 = DSPDoubleComplex_cmul(z, z);
    
    DSPDoubleComplex out = DSPDoubleComplex_init(BMSmoothGain_getGainLinear(&This->gain), 0.0);
    
    
    for (size_t level = 0; level < This->numLevels; level++) {
        
        // both channels are the same so we just check the left one
        size_t channel = 0;
        
        double* b0 = This->coefficients_d + level*This->numChannels*5 + channel*5;
        double* b1 = b0+1;
        double* b2 = b0+2;
        double* a1 = b0+3;
        double* a2 = b0+4;
        
        
        DSPDoubleComplex numerator =
        DSPDoubleComplex_add3(DSPDoubleComplex_smul(*b0, z2),
                              DSPDoubleComplex_smul(*b1, z),
                              DSPDoubleComplex_init(*b2, 0.0));
        
        DSPDoubleComplex denominator =
        DSPDoubleComplex_add3(z2,
                              DSPDoubleComplex_smul(*a1, z),
                              DSPDoubleComplex_init(*a2, 0.0));
        
        out = DSPDoubleComplex_cmul(out,
                                    DSPDoubleComplex_divide(numerator,
                                                            denominator));
    }
    
    return out;
}


inline DSPDoubleComplex BMMultiLevelBiquad_tfEvalAtLevel(BMMultiLevelBiquad *This, DSPDoubleComplex z,size_t level){
    
    DSPDoubleComplex z2 = DSPDoubleComplex_cmul(z, z);
    
    DSPDoubleComplex out = DSPDoubleComplex_init(BMSmoothGain_getGainLinear(&This->gain), 0.0);
    
    // both channels are the same so we just check the left one
    size_t channel = 0;
    
    double* b0 = This->coefficients_d + level*This->numChannels*5 + channel*5;
    double* b1 = b0+1;
    double* b2 = b0+2;
    double* a1 = b0+3;
    double* a2 = b0+4;
    
    
    DSPDoubleComplex numerator =
    DSPDoubleComplex_add3(DSPDoubleComplex_smul(*b0, z2),
                          DSPDoubleComplex_smul(*b1, z),
                          DSPDoubleComplex_init(*b2, 0.0));
    
    DSPDoubleComplex denominator =
    DSPDoubleComplex_add3(z2,
                          DSPDoubleComplex_smul(*a1, z),
                          DSPDoubleComplex_init(*a2, 0.0));
    
    out = DSPDoubleComplex_cmul(out,
                                DSPDoubleComplex_divide(numerator,
                                                        denominator));
    
    return out;
}








/*!
 * BMMultiLevelBiquad_tfMagVector
 *
 * @param frequency   an an array specifying frequencies at which we want to evaluate
 * the transfer function magnitude of the filter
 *
 * @param magnitude   an array for storing the result
 * @param length      the number of elements in frequency and magnitude
 *
 */
void BMMultiLevelBiquad_tfMagVector(BMMultiLevelBiquad *This, const float *frequency, float *magnitude, size_t length){
    for (size_t i = 0; i<length; i++) {
        // convert from frequency into z (complex angular velocity)
        DSPDoubleComplex z = DSPDoubleComplex_z(frequency[i], This->sampleRate);
        // evaluate the transfer function at z and take absolute value
        magnitude[i] = DSPDoubleComplex_abs(BMMultiLevelBiquad_tfEval(This,z));
    }
}

void BMMultiLevelBiquad_tfMagVectorAtLevel(BMMultiLevelBiquad *This, const float *frequency, float *magnitude, size_t length,size_t level){
    for (size_t i = 0; i<length; i++) {
        // convert from frequency into z (complex angular velocity)
        DSPDoubleComplex z = DSPDoubleComplex_z(frequency[i], This->sampleRate);
        // evaluate the transfer function at z and take absolute value
        magnitude[i] = DSPDoubleComplex_abs(BMMultiLevelBiquad_tfEvalAtLevel(This,z,level));
    }
}





/*!
 * BMMultiLevelBiquad_groupDelay
 *
 * returns the total group delay (in samples) of all levels of the filter at the specified frequency.
 *
 * @discussion uses a cookbook formula for group delay of biquad filters, based on the fft derivative method.
 *
 * @param freq the frequency at which you need to compute the group delay of the filter cascade
 * @return the group delay in samples at freq
 */
double BMMultiLevelBiquad_groupDelay(BMMultiLevelBiquad *This, double freq){
    double delay = 0.0;
    
    for (size_t level=0; level<This->numLevels; level++) {
        
        double b0 = This->coefficients_d[5*level];
        double b1 = This->coefficients_d[5*level + 1];
        double b2 = This->coefficients_d[5*level + 2];
        double a1 = This->coefficients_d[5*level + 3];
        double a2 = This->coefficients_d[5*level + 4];
        
        // normalize the feed forward coefficients so that b0=1
        // see: see: http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
        b1 /= b0;
        b2 /= b0;
        b0 = 1.0;
        
        // radian normalised frequency
        double w = freq / (0.5*This->sampleRate);
        
        // calculate the group delay of the normalized filter using a cookbook formula
        // http://music-dsp.music.columbia.narkive.com/9F6BIvHy/group-delay
        // or
        // http://music.columbia.edu/pipermail/music-dsp/1998-April/053307.html
        //
        //    T(w) =
        //
        //      b1^2 + 2*b2^2 + b1*(1 + 3*b2)*cos(w) + 2*b2*cos(2*w)
        //    --------------------------------------------------------
        //     1 + b1^2 + b2^2 + 2*b1*(1 + b2)*cos(w) + 2*b2*cos(2*w)
        //
        //
        //        a1^2 + 2*a2^2 + a1*(1 + 3*a2)*cos(w) + 2*a2*cos(2*w)
        //    - --------------------------------------------------------
        //        1 + a1^2 + a2^2 + 2*a1*(1 + a2)*cos(w) + 2*a2*cos(2*w)
        //
        //
        //    w is normalized radian frequency and T(w) is measured in sample units.
        
        
        //      b1^2 + 2*b2^2 + b1*(1 + 3*b2)*cos(w) + 2*b2*cos(2*w)
        double num1 = b1*b1 + 2.0*b2*b2 + b1*(1.0 + 3.0*b2)*cos(w) + 2.0*b2*cos(2.0*w);
        //     1 + b1^2 + b2^2 + 2*b1*(1 + b2)*cos(w) + 2*b2*cos(2*w)
        double den1 = 1.0 + b1*b1 + b2*b2 + 2.0*b1*(1.0 + b2)*cos(w) + 2.0*b2*cos(2.0*w);
        double frac1 = num1/den1;
        
        
        //        a1^2 + 2*a2^2 + a1*(1 + 3*a2)*cos(w) + 2*a2*cos(2*w)
        double num2 = a1*a1 + 2.0*a2*a2 + a1*(1.0 + 3.0*a2)*cos(w) + 2.0*a2*cos(2.0*w);
        //        1 + a1^2 + a2^2 + 2*a1*(1 + a2)*cos(w) + 2*a2*cos(2*w)
        double den2 = 1.0 + a1*a1 + a2*a2 + 2.0*a1*(1.0 + a2)*cos(w) + 2.0*a2*cos(2.0*w);
        double frac2 = num2/den2;
        
        // add the delay of the current level to the total delay
        delay += frac1 - frac2;
    }
    
    return delay;
}

/*!
 * BMMiltiLevelBiquad_phaseResponse
 *
 * @abstract Calculates the real-valued phase response of the filter cascade at the frequency freq. The formula for this generalizes to filters of any order. It is based on equation (24) in this document: http://www.rs-met.com/documents/dsp/BasicDigitalFilters.pdf
 * @param This       pointer to an initialized struct
 * @param freq      the frequency at which we want to evaluate the phase response
 */
double BMMiltiLevelBiquad_phaseResponse(BMMultiLevelBiquad *This, double freq){
    double totalPhaseShift = 0;
    double w = 2.0 * M_PI * freq / This->sampleRate;
    
    for (size_t level=0; level<This->numLevels; level++) {
        
        double b0 = This->coefficients_d[5*level];
        double b1 = This->coefficients_d[5*level + 1];
        double b2 = This->coefficients_d[5*level + 2];
        double a1 = This->coefficients_d[5*level + 3];
        double a2 = This->coefficients_d[5*level + 4];
        
    // Mathematica prototype:
    //
    // biquadPR[w_, b0_, b1_, b2_, a0_, a1_, a2_] :=
    // -ArcTan[(b0 Sin[0 w] + b1 Sin[1 w] + b2 Sin[2 w]),
    //         (b0 Cos[0 w] + b1 Cos[1 w] + b2 Cos[2 w])] +
    //  ArcTan[-(a0 Sin[0 w] + a1 Sin[1 w] + a2 Sin[2 w]),
    //         -(a0 Cos[0 w] + a1 Cos[1 w] + a2 Cos[2 w])]
    //
    //
        // direct C port from Mathematica:
//        double sb = b0 * sin(0.0*w) + b1 * sin(1.0*w) + b2 * sin(2*w);
//        double cb = b0 * cos(0.0*w) + b1 * cos(1.0*w) + b2 * cos(2*w);
//        double sa = a0 * sin(0.0*w) + a1 * sin(1.0*w) + a2 * sin(2*w);
//        double ca = a0 * cos(0.0*w) + a1 * cos(1.0*w) + a2 * cos(2*w);
        //
        // more efficient C port:
        double w2 = w * 2.0;
        double sinw = sin(w);
        double cosw = cos(w);
        double sin2w = sin(w2);
        double cos2w = cos(w2);
        //
        double sb =       (b1 * sinw) + (b2 * sin2w);
        double cb = b0  + (b1 * cosw) + (b2 * cos2w);
        double sa =       (a1 * sinw) + (a2 * sin2w);
        double ca = 1.0 + (a1 * cosw) + (a2 * cos2w);

        
        totalPhaseShift += atan2(sb, cb);
        totalPhaseShift -= atan2(sa, ca);
    }
    
    // bound the result to [0, 2*pi] and return
    double twoPi = 2.0 * M_PI;
    return modf(totalPhaseShift, &twoPi);
}


//#ifdef __cplusplus
//}
//#endif

