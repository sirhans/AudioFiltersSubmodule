//
//  BMTNFilter.c
//  BMAudioFilters
//
//  Uses an adaptive least mean squares filter to separate the input
//  signal into tonal and noisy components
//
//  Created by Hans on 25/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMTNFilter.h"
#include <Accelerate/Accelerate.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    void BMTNFilter_processSample(BMTNFilter* f, float input, float* toneOut, float* noiseOut);
    
    
    
    
    void BMTNFilter_init(BMTNFilter* f, size_t filterOrder, float mu, size_t delayTime){
        
        // this init function is for when we don't know the fundamental
        // frequency
        f->hasF0 = false;
        
        // FIR filter length = order + 1
        f->filterLength = filterOrder+1;
        
        
        // allocate memory
        f->XMemLength = BMTNF_FILTER_WRAP_TIME + f->filterLength;
        f->Xmem = malloc(sizeof(float)*f->XMemLength);
        f->delayLine = malloc(sizeof(float)*delayTime);
        f->W = malloc(sizeof(float)*f->filterLength);
        
        
        f->mu = mu;
        f->delayTime = delayTime;
        
        
        // initialize all the time-varying data
        BMTNFilter_reset(f);
    }
    
    
    
    void BMTNFilter_initWithF0(BMTNFilter* f, size_t filterOrder, float minf0, float sampleRate, float mu){
        
        // this init function is for when we know the fundamental frequency
        f->hasF0 = true;
        
        f->sampleRate = sampleRate;
        
        // FIR filter length = order + 1
        f->filterLength = filterOrder+1;
        
        float maxWavelength = sampleRate/minf0;
        
        size_t maxDelayTime = (size_t)maxWavelength - filterOrder/2;
        
        // allocate memory
        f->XMemLength = BMTNF_FILTER_WRAP_TIME + f->filterLength;
        f->Xmem = malloc(sizeof(float)*f->XMemLength);
        f->delayLine = malloc(sizeof(float)*maxDelayTime);
        f->W = malloc(sizeof(float)*f->filterLength);
        
        
        f->mu = mu;
        f->delayTime = maxDelayTime;
        
        
        // initialize all the time-varying data
        BMTNFilter_reset(f);
    }
    
    
    void BMTNFilter_reset(BMTNFilter* f){
        // if we know the fundamental frequency, use newNote instead
        // of reset
        assert(!f->hasF0);
        
        // set initial position and the end marker for the delay line
        f->delayLineEnd = f->delayLine + f->delayTime;
        f->dp = f->delayLine;
        
        
        // set the initial position and end marker for the X delay buffer
        float* XmemEnd = f->Xmem + f->XMemLength;
        f->Xstart =  XmemEnd - f->filterLength;
        f->X = f->Xstart;
        f->Xlast = f->Xstart + f->filterLength - 1;
        
        
        // initialize arrays to zero
        memset(f->delayLine, 0, sizeof(float)*f->delayTime);
        memset(f->Xmem,0,sizeof(float)*f->XMemLength);
        memset(f->W,0,sizeof(float)*f->filterLength);
        
        // the normalisation term is zero
        f->norm = 0.0f;
    }
    
    
    
    void BMTNFilter_newNote(BMTNFilter* f, float f0){
        
        // newNote can only be used if we initialised with a specified
        // maximum fundamental frequency
        assert(f->hasF0);
        
        // f0 can not be lower than the minimum we specified when we
        // initialised.
        f->normBufferLength = f->sampleRate/f0;
        assert(f->normBufferLength <= f->normBufferMaxLength);
        
        // set initial position and the end marker for the delay line
        f->delayLineEnd = f->delayLine + f->delayTime;
        f->dp = f->delayLine;
        
        
        // set the initial position and end marker for the X delay buffer
        float* XmemEnd = f->Xmem + f->XMemLength;
        f->Xstart =  XmemEnd - f->filterLength;
        f->X = f->Xstart;
        f->Xlast = f->Xstart + f->filterLength - 1;
        
        
        // initialize arrays to zero
        memset(f->delayLine, 0, sizeof(float)*f->delayTime);
        memset(f->Xmem,0,sizeof(float)*f->XMemLength);
        memset(f->W,0,sizeof(float)*f->filterLength);
        
        // the normalisation term is zero
        f->norm = 0.0f;
    }
    
    
    
    
    void BMTNFilter_processBuffer(BMTNFilter* f, const float* input, float* toneOut, float* noiseOut, size_t numSamples){
        
        // if f is not initialised, initialise it with reasonable defaults.
        if(!f->Xmem){
            // for music
            //BMTNFilter_init(f, 64, 0.25, 150);
            
            // for voice
            BMTNFilter_init(f, 512, 0.2, 64);
        }
        
        for(size_t i=0; i<numSamples; i++)
            BMTNFilter_processSample(f, input[i], toneOut+i, noiseOut+i);
    }
    
    
    
    
    inline void BMTNFilter_processSample(BMTNFilter* f, float input, float* toneOut, float* noiseOut){
        
        // read and write from the delay line to get a delayed input
        float delayedInput = *f->dp;
        *f->dp = input;
        // advance the pointer and wrap if necessary
        f->dp++;
        if (f->dp == f->delayLineEnd) f->dp = f->delayLine;
        
        
        // remove the oldest element in X from X.X
        float xl = *f->Xlast;
        f->norm -= xl * xl;
        
        
        // advance the X delays
        //
        // pointer decrement and wrap if necessary
        f->X--;
        f->Xlast--;
        if(f->X < f->Xmem) {
            // move the data in X from the beginning to the end of the buffer
            memmove(f->Xstart+1, f->Xmem, sizeof(float)*f->filterLength-1);
            f->X = f->Xstart;
            f->Xlast = f->Xstart + f->filterLength - 1;
        }
        // write the delayed input into the first position in X
        *f->X = delayedInput;
        
        
        // add the newest element in X to X.X
        f->norm += (*f->X) * (*f->X);
        
        
        // tone is the output of the linear prediction model: tone = W.X
        vDSP_dotpr(f->W, 1, f->X, 1, toneOut, f->filterLength);
        
        
        // noise is the error in the prediction
        *noiseOut = input - *toneOut;
        
        
        // "correct" NLMS formula:
        float ue = *noiseOut * f->mu / (f->norm + 0.001f);
        //
        // alternative formula signed-squares the noise sample:
        //float ue = *noiseOut * fabsf(*noiseOut) * f->mu / (f->XDotX + 0.001f);
        
        
        // Update coefficients W = W + ue*X;
        vDSP_vsma(f->X, 1, &ue, f->W, 1, f->W, 1, f->filterLength);
    }
    
    
    
    
    void BMTNFilter_destroy(BMTNFilter* f){
        free(f->Xmem);
        free(f->delayLine);
        free(f->W);
        f->Xmem = NULL;
        f->delayLine = NULL;
        f->W = NULL;
    }
    
    
    
#ifdef __cplusplus
}
#endif
