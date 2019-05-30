//
//  BMVariableFilter.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 30/5/19.
//  Anyone may use this file without restrictions
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMVariableFilter.h"
#include <Accelerate/Accelerate.h>

//    void BMReleaseFilter_setCutoff(BMReleaseFilter* This, float fc){
//        This->fc = fc;
//
//        // compute the input gain to the integrators
//        float g = tanf(M_PI * fc / This->sampleRate);
//
//        // update the first integrator state variable to ensure that the second
//        // derivative remains continuous when changing the cutoff freuency
//        This->ic1 *= This->g / g;
//
//        // then save the new value of the gain coefficient
//        This->g = g;
//
//        // compute the three filter coefficients
//        This->a1 = 1.0f / (1.0f + This->g * (This->g + This->k));
//        This->a2 = This->a1 * This->g;
//        This->a3 = This->a2 * This->g;
//    }
    
    
    
    void BMVariableFilter_freqSweepMono(BMVariableFilter* This,
                                        const float* audioInput,
                                        const float* cutoffControl,
                                        float* audioOutput,
                                        size_t numSamples){
        
        // chunk processing to use buffer memory
        while (numSamples > 0){
            size_t samplesThisChunk = BM_MIN(numSamples,BM_BUFFER_CHUNK_SIZE);
            int samplesTCInt = (int)samplesThisChunk;
            
            // compute the input gain to the integrators at each control sample
            float* tempBuffer = This->a1;
            float alpha = M_PI / This->sampleRate;
            vDSP_vsmul(cutoffControl, 1, &alpha, This->g, 1, samplesThisChunk);
            vvtanf(This->g, tempBuffer, &samplesTCInt);
            
            // compute the filter coefficients
            // a1 = 1.0f / (1.0f + This->g * (This->g + This->k));
            vDSP_vsadd(This->g, 1, &This->k, This->a1, 1, samplesThisChunk);
            float one = 1.0f;
            vDSP_vmsa(This->a1, 1, This->g, 1, &one, This->a1, 1, samplesThisChunk);
            vvrecf(This->a1,This->a1,&samplesTCInt);
            // a2 = g * a1
            vDSP_vmul(This->a1, 1, This->g, 1, This->a2, 1, samplesThisChunk);
            // a3 = g * a2
            vDSP_vmul(This->a2, 1, This->g, 1, This->a3, 1, samplesThisChunk);
            
            // compute the anti-click coefficients that will multiply by ic1 to
            // get smoother sound when the filter frequency changes abruptly
            vDSP_vdiv(This->g+1, 1, This->g, 1, This->antiClick+1, 1, samplesThisChunk-1);
            This->antiClick[0] = This->lastG / This->g[0];
            This->lastG = This->g[samplesThisChunk-1];
            
            // filter the audio
            
        }
    }
    
    
#ifdef __cplusplus
}
#endif
