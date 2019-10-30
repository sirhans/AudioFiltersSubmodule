//
//  BMGainStage.c
//  BMAudioFilters
//
//  Created by Hans on 2/8/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#ifdef __cplusplus
extern "C" {
#endif
    
#include "BMGainStage.h"
#include <assert.h>
#include "BMVectorOps.h"
#include "Constants.h"
    
#ifndef MIN
#define MIN(X,Y) (X<Y)? X:Y
#endif
    
#ifndef MAX
#define MAX(X,Y) (X>Y)? X:Y
#endif
    
    
    
    
    /*!
     * Setup a new gainstage object
     *
     * @param This        Pointer to the uninitialised gain stage
     * @param sampleRate  The sample rate of the input / output
     * @param AAFilterFc  cutoff frequency of antialiasing filter
     *                    for guitar, set this to 7000 hz.
     *                    for full frequency range set it to 20khz
     */
    void BMGainStage_init(BMGainStage* This, float sampleRate, float AAfilterFc){
        
        /*
         * init the anti-aliasing filter
         */
        This->aaFc = AAfilterFc;
        BMMultiLevelBiquad_init(&This->aaFilter,
                                BMGAINSTAGE_AAFILTER_NUMLEVELS,
                                sampleRate,
                                false,             // one channel
                                false,
                                false);            // no realtime updates
        BMGainStage_setAACutoff(This, This->aaFc);
        
        
        // init DC Blocker
        BMDCBlocker_init(&This->dcFilter);
        
        
        // charge reservoirs start out empty
        This->cPos = This->cNeg = 0.0;
        
        
        /* set the tightness, depending on sample rate so that
         * it sounds the same regardless of the oversamping
         * factor.
         */
        This->sampleRate = sampleRate;
        BMGainStage_setTightness(This, 1.0);
        
        /*
         * set the input gain
         */
        BMGainStage_setGain(This,1.035f);
        
        
        /*
         * set the input bias
         */
        BMGainStage_setBiasRatio(This, 0.0);
        
        /*
         * Set the amp nonlinearity type
         */
        This->ampType = BM_AMP_TYPE_DUAL_RES_TUBE;
        
        /*
         * the lowpassed clip amp type needs a buffer for temp storage
         */
        This->tempBuffer = malloc(sizeof(float)*BM_BUFFER_CHUNK_SIZE);
        
        /*
         * the lowpassed clip amp type needs its own lowpass filter
         */
        BMMultiLevelBiquad_init(&This->clipLP, 1, sampleRate, false, false, false);
        BMMultiLevelBiquad_setLowPass12db(&This->clipLP, 1200.0f, 0);
    }
    
    
    
    
    
    /*!
     * Set the speed of limiting response
     *
     * @param This       Pointer to an initialised gain stage
     * @param tightness  In [0,1]. 0 = saggy tube; 1 = hard transistor
     *
     */
    void BMGainStage_setTightness(BMGainStage* This, float tightness){
        assert(tightness >= 0.0 && tightness <= 1.0);
        
        /*
         * We convert from the [0-1] scale to t in [1,inf]
         * t is the time it takes for the tube to recharge its reservoir,
         * measured in samples, approximately.  It recharges asymptotically,
         * so it's nevery actually full.
         */
        
        // max_t is the largest t setting we allow, for a really saggy tube
        // sound
        float max_t_exp = 10.0f;
        
        // min_t is the lowest t setting we allow, for really tight transistor
        // sound
        float min_t_exp = 1.0f;
        
        // when tightness = 0, t_exp = max_t_exp;
        // when tightness = 1, t_exp = min_t_exp.
        float t_exp = min_t_exp + (1.0f-tightness)*(max_t_exp - min_t_exp);
        This->t = powf(2.0f, t_exp)*(This->sampleRate/44100.0f);
        
        // save the inverse to avoid division in the inner loop
        This->tInv = 1.0f/This->t;
    }
    
    
    
    
    void BMGainStage_setAmpType(BMGainStage* This, BMNonlinearityType type){
        This->ampType = type;
    }
    
    
    
    
    void BMGainStage_setGain(BMGainStage* This, float gain){
        This->gain = gain;
        This->bias = This->biasRatio * gain;
    }
    
    
    
    
    void BMGainStage_setBiasRatio(BMGainStage* This, float biasRatio){
        assert(fabsf(biasRatio) <= 1.0);
        
        This->biasRatio = biasRatio;
        This->bias = biasRatio * This->gain;
    }
    
    
    
    
    void BMGainStage_setAACutoff(BMGainStage* This, float fc){
        This->aaFc = fc;
        BMMultiLevelBiquad_setHighOrderBWLP(&This->aaFilter, fc, 0,
                                            BMGAINSTAGE_AAFILTER_NUMLEVELS);
    }
    
    
    
    
    
    void BMGainStage_setSampleRate(BMGainStage* This, float sampleRate){
        // destroy the old aa filter
        BMMultiLevelBiquad_destroy(&This->aaFilter);
        
        // init a new filter with new sample rate
        BMMultiLevelBiquad_init(&This->aaFilter, 1, sampleRate, false, false,false);
        
        // reset the lowpass filter at the same cutoff as before
        BMMultiLevelBiquad_setLowPass12db(&This->aaFilter, This->aaFc, 0);
        
        // update the DCBlocker filter (this is not necessary yet but it will be when the DC blocker has a sample rate setting)
        BMDCBlocker_init(&This->dcFilter);
    }
    
    
    
    
    /*
     * scalar smoothstep function:
     *    https://en.wikipedia.org/wiki/Smoothstep
     */
    static inline float smoothstepSingle(float x){
        // Scale and bias x to [0,1] range
        //x = (x + 1.0f) / 2.0f;
        x = x * 0.5f + 0.5f;
        
        // clip x to [0,1] range
        if (x < 0.0f)
            x = 0.0f;
        if (x > 1.0f)
            x = 1.0f;
        
        // Evaluate polynomial
        x = x * x * (3.0f - 2.0f * x);
        
        // back to original range
        return x * 2.0f - 1.0f;
    }
    
    
    /*
     * scalar smootherstep function:
     *    https://en.wikipedia.org/wiki/Smoothstep#Variations
     */
    static inline float smootherstepSingle(float x){
        // Scale and bias x to [0,1] range
        //x = (x + 1.0f) / 2.0f;
        x = x * 0.5f + 0.5f;
        
        // clip x to [0,1] range
        if (x < 0.0f)
            x = 0.0f;
        if (x > 1.0f)
            x = 1.0f;
        
        // Evaluate polynomial
        x = x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
        
        // back to original range
        return x * 2.0f - 1.0f;
    }
    
    
    
    
    /*
     * vector smoothstep function:
     *      https://en.wikipedia.org/wiki/Smoothstep
     */
    static inline void smoothstepVector(float* input, float* output, size_t numSamples) {
        float zero = 0.0f;
        float one = 1.0f;
        float two = 2.0f;
        float one_half = 0.5f;
        float neg_one = -1.0f;
        
        // Scale, bias and clip x to [0,1] range
        // y = x * 0.5 + 0.5;
        vDSP_vsmsa(input, 1, &one_half, &one_half, output, 1, numSamples);
        vDSP_vclip(output, 1, &zero, &one, output, 1, numSamples);
        
        
        // Evaluate polynomial
        //return x * x * (3 - 2 * x);
        size_t n = numSamples;
        float* op = output;
        while (n >=8 ) {
            vFloat32_8 x = *(vFloat32_8 *)op;
            *(vFloat32_8 *)op = x * x * (3.0f - 2.0f * x);
            
            n -=8;
            op +=8;
        }
        while (n > 0){
            float x = *op;
            *op = x * x * (3.0f - 2.0f * x);
            
            n--;
            op++;
        }
        
        // scale back to the original place
        // y = x * 2.0f - 1.0f
        vDSP_vsmsa(output, 1, &two, &neg_one, output, 1, numSamples);
    }
    
    
    
    
    /*
     * vector smootherstep function proposed by ken perlin:
     *    https://en.wikipedia.org/wiki/Smoothstep#Variations
     */
    static inline void smootherstepVector(float* input, float* output, size_t numSamples) {
        float zero = 0.0f;
        float one = 1.0f;
        float two = 2.0f;
        float one_half = 0.5f;
        float neg_one = -1.0f;
        
        // Scale, bias and clip x to [0,1] range
        // y = x * 0.5 + 0.5;
        vDSP_vsmsa(input, 1, &one_half, &one_half, output, 1, numSamples);
        vDSP_vclip(output, 1, &zero, &one, output, 1, numSamples);
        
        // return x * x * x * (x * (x * 6 - 15) + 10);
        size_t n = numSamples;
        float* op = output;
        while (n >=8 ) {
            vFloat32_8 x = *(vFloat32_8 *)op;
            *(vFloat32_8 *)op = x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
            
            n -=8;
            op +=8;
        }
        while (n > 0){
            float x = *op;
            *op = x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
            
            n--;
            op++;
        }
        
        // scale back to the original place
        // y = x * 2.0f - 1.0f
        vDSP_vsmsa(output, 1, &two, &neg_one, output, 1, numSamples);
    }
    
    
    
    
    
    
    
    
    // f(x) = x / (1 + |x|)
    //
    // the output approaches 1.0f in the limit as the input approaches infinity
    static inline float asymptoticLimitSingle(float x){
        return x / (1.0f + fabsf(x));
    }
    
    
    
    
    
    
    
    
    /*!
     * asymptoticLimitVector
	 *
     * @param input      input array of length numSamples
     * @param output     output array of length numSamples
     * @param numSamples length of input and output
     * @brief a very soft limit clipping function
     * @discussion up to limits of floating point precision, each input value maps to a different output. This means that regardless of how loud the input gets, the waveform never gets hard-clipped. for inputs in [-1,1], this function reduces the gain by 1/2 compared to the input. It has some nice bouindary conditions: f(0) = 0; f'(0) = 1; f(+-inf) = +-limit; f'(+-inf) = 0;
     * @code f(x,limit) = x / (1 + |x/limit|)
     */
    static inline void asymptoticLimitVector(const float* input, float* output, size_t numSamples){
        
        vFloat32_8 onev = 1.0f;
        
        size_t n = numSamples;
        // process in chunks of 8 samples
        while(n >= 8){
            vFloat32_8 d = vfabsf_8(*(vFloat32_8 *)input) + onev;
            *(vFloat32_8 *)output = *(vFloat32_8 *)input / d;
            
            input += 8;
            output += 8;
            n -= 8;
        }
        
        
        // process the remaining samples individually
        while(n > 0){
            float d = fabsf(*input) + 1.0;
            *output = *input / d;
            
            input++;
            output++;
            n--;
        }
    }
    
    
    
    
    /*
     * clip to [-1,1]
     */
    static inline float hardClipSingle(float x){
        if (x > 1.0f) return 1.0f;
        if (x < -1.0f) return -1.0f;
        return x;
    }
    
    
    
    
    void dualResTube(BMGainStage* This, float* input, float* output, size_t numSamples){
        for(size_t i=0; i<numSamples;i++){
            /*
             * code comments in this section are from the Mathematica prototype
             */
            
            
            /*
             * At each sample we add a little to the charge reservoir. The rate
             * of charging is proportional to the size of the empty part of the
             * charge reservoir.
             */
            // cPos += (t - cPos)/ t;
            // cNeg += (t - cNeg) / t;
            This->cPos += (This->t - This->cPos) * This->tInv;
            This->cNeg += (This->t - This->cNeg) * This->tInv;
            
            /*
             * Blend smoothly between the push and pull sides of the class AB amp
             */
            // we use the asymptoticLimit function to do the blending.
            // the input is multiplied by 10 to force the amp to rely more heavily on one tube
            // or the other instead of using a blend of the two. The 1.0f and 0.5f are there to
            // scale and shift the range from (-1,1) to (0,1).
            float posMix = (asymptoticLimitSingle(10.0 * input[i]) + 1.0f) * 0.5f;
            float negMix = 1.0 - posMix;
            
            // c = cPos*posMix + cNeg*(1.0 - posMix);
            float c = This->cPos * posMix + This->cNeg * negMix;
            
            
            /*
             * the output is proportional to the input times the fraction of the
             * charge reservoir that is full.
             */
            // we get an error if c<0, so we limit to prevent that. Such an error only occurs
            // for extremely high input gain, on the order of +60 db outside of [-1,1]
            output[i] = c * asymptoticLimitSingle(input[i] * This->tInv);
            
            
            /*
             * Charge that went to the output is consumed; we remove it from the
             * reservoir.
             */
            // c -= Abs[out[[n]]];
            float absOut = fabsf(output[i]);
            This->cPos -= posMix*absOut;
            This->cNeg -= negMix*absOut;
            
        }
        
        // take the volume down to reduce distortion from the asymptotic limiter in the following stage
        float attenuation = 0.33f;
        vDSP_vsmul(output, 1, &attenuation, output, 1, numSamples);
        
        // limit the range of the output to (-1,1)
        asymptoticLimitVector(output, output, numSamples);
        
        // scale up to undo the attenuation we did before the asymptotic limit
        float boost = 1.0 / (2.0f * asymptoticLimitSingle(attenuation));
        vDSP_vsmul(output, 1, &boost, output, 1, numSamples);
        
        // smoothstep limit to [-1,1] because the scaling up in the previous step allows extreme peaks
        // to exceed [-1,1]. This is only necessary for when the input gain is >~ 40dB
        smoothstepVector(output, output, numSamples);
    }
    
    
    
    void singleResTube(BMGainStage* This, float* input, float* output, size_t numSamples){
        for(size_t i=0; i<numSamples;i++){
            /*
             * code comments in this section are from the Mathematica prototype
             */
            
            
            /*
             * At each sample we add a little to the charge reservoir. The rate
             * of charging is proportional to the size of the empty part of the
             * charge reservoir.
             */
            // cPos += (t - cPos)/ t;
            This->cPos += (This->t - This->cPos) * This->tInv;
            
            
            /*
             * the output is proportional to the input times the fraction of the
             * charge reservoir that is full.
             */
            // Y[[i]] = AsymptoticLimit[c * X[[i]] , c] / t;
            output[i] = This->cPos * smootherstepSingle(input[i]) * This->tInv;
            //output[i] = BMGainStage_asymptoticLimitSingle(This->cPos * input[i], This->cPos) * This->tInv;
            
            
            /*
             * Charge that went to the output is consumed; we remove it from the
             * reservoir.
             */
            // c -= Abs[out[[n]]];
            float absOut = fabsf(output[i]);
            This->cPos -= absOut;
        }
    }
    
    
    
    void lowpassClippedDifference(BMGainStage* This, float* input, float* output, size_t numSamples){
        
        while(numSamples > 0){
            size_t samplesProcessing = MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            
            // copy the input
            memcpy(This->tempBuffer, input, sizeof(float)*samplesProcessing);

            // clip the copy to [-limit,limit]
            float upperLimit = 0.5;
            float lowerLimit = -upperLimit;
            float* clippedSignal = This->tempBuffer;
            // hard clip
            //vDSP_vclip(This->tempBuffer, 1, &lowerLimit, &upperLimit, clippedSignal, 1, samplesProcessing);
            // softer clip
            smootherstepVector(This->tempBuffer, clippedSignal, samplesProcessing);

            // input - clipDiff = clippedInput
            // clipDiff = input - clippedInput
            float* clipDiff = This->tempBuffer;
            vDSP_vsub(input,1,clippedSignal,1,clipDiff,1,samplesProcessing);

            // lowpass filter the clipped difference
            BMMultiLevelBiquad_processBufferMono(&This->clipLP, clipDiff, clipDiff, samplesProcessing);

            // output = input + clipDiff;
            vDSP_vadd(input, 1, clipDiff, 1, output, 1, samplesProcessing);
            
            numSamples -= samplesProcessing;
            input += samplesProcessing;
            output += samplesProcessing;
        }
    }
    
    
    
    
    
    
    
    /*!
     * BMGainStage_processBufferMono
     *
     * @param This        Pointer to an initialised gain stage
     * @param input       buffer of input samples
     * @param output      buffer of output samples
     * @param numSamples  number of samples to process
     *                    All buffers must be at least this long.
     * @abstract Process tube-like gain on a buffer of samples
     *
     */
    void BMGainStage_processBufferMono(BMGainStage* This,
                                       float* input,
                                       float* output,
                                       size_t numSamples){
        
        // apply the antialiasing filter
        BMMultiLevelBiquad_processBufferMono(&This->aaFilter, input, input, numSamples);
        
        
        // apply input gain and bias
        vDSP_vsmsa(input, 1, &This->gain, &This->bias, input, 1, numSamples);
        
        
        // process static asymptotic type nonlinearity
        if (This->ampType == BM_AMP_TYPE_ASYMPTOTIC){
            asymptoticLimitVector(input, output, numSamples);
        }
        
        // process dual-reservoir tube type nonlinearity
        else if(This->ampType == BM_AMP_TYPE_DUAL_RES_TUBE){
            dualResTube(This,input, output, numSamples);
        }
        
        // process single-reservoir tube type nonlinearity
        else if(This->ampType == BM_AMP_TYPE_SINGLE_RES_TUBE){
            singleResTube(This,input, output, numSamples);
        }
        
        // process lowpassed clipped difference type nonlinearity
        else if(This->ampType == BM_AMP_TYPE_LOWPASS_CLIP_DIFFERENCE){
            lowpassClippedDifference(This, input, output, numSamples);
        }
        
        
        //apply dc blocker filter
        BMDCBlocker_process(&This->dcFilter, output, output, numSamples);
    }
    
    
    
    
    void BMGainStage_destroy(BMGainStage* This){
        BMMultiLevelBiquad_destroy(&This->aaFilter);
        BMMultiLevelBiquad_destroy(&This->clipLP);
        free(This->tempBuffer);
        This->tempBuffer = NULL;
    }
    
    
    
    
    
    
#ifdef __cplusplus
}
#endif
