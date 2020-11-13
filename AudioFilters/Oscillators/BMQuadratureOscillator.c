//
//  BMQuadratureOscillator.c
//  BMAudioFilters
//
//  A sinusoidal quadrature oscillator suitable for efficient LFO. However, it
//  is not restricted to low frequencies and may also be used for other
//  applications.
//
//  Created by Hans on 31/10/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include <math.h>
#include "BMQuadratureOscillator.h"
#include <stdio.h>
#include <string.h>
#include "BM2x2Matrix.h"
#include <Accelerate/Accelerate.h>

#ifdef __cplusplus
extern "C" {
#endif
	
	
    
	
    void BMQuadratureOscillator_initMatrix(simd_float2x2* m,
                                           float frequency,
                                           float sampleRate){
        // we want a rotation matrix that completes a rotation of 2*M_PI in
        // (sampleRate / fHz) samples. That means that in 1 sample, we need to
        // rotate by an angle of fHz * 2 * M_PI / sampleRate.
        float oneSampleAngle = frequency * 2.0f * M_PI / sampleRate;
        *m = BM2x2Matrix_rotationMatrix(oneSampleAngle);
    }
    
    
    
    
    void BMQuadratureOscillator_init(BMQuadratureOscillator *This,
                                     float fHz,
                                     float sampleRate){
        //This->sampleRate = sampleRate;
        
        BMQuadratureOscillator_initMatrix(&This->m, fHz, sampleRate);
        
        // set the initial values for an oscillation amplitude of 1
        This->rq.x = 1.0f;
        This->rq.y = 0.0f;
    }

	
	
	
	void BMQuadratureOscillator_setFrequency(BMQuadratureOscillator *This, float fHz){
		This->modRateHz = fHz;
		BMQuadratureOscillator_initMatrix(&This->m, fHz, This->sampleRate);
	}
    
	
	
    
    /*
     * // Mathematica Prototype code
     *
     * For[i = 1, i <= n, i++,
     *   out[[i]] = r[[1]];
     *
     *   r = m.r
     * ];
     *
     */
    void BMQuadratureOscillator_process(BMQuadratureOscillator *This,
                                            float* r,
                                            float* q,
                                            size_t numSamples){
        for(size_t i=0; i<numSamples; i++){
            // copy the current sample value to output
            r[i] = This->rq.x;
            q[i] = This->rq.y;
            
            // multiply the vector rq by the rotation matrix m to generate the
            // next sample of output
			This->rq = simd_mul(This->m, This->rq);
        }
    }
    
	

	
	void BMQuadratureOscillator_volumeEnvelope4Stereo(BMQuadratureOscillator *This,
												float** buffersL, float** buffersR,
												bool* zeros,
												size_t numSamples){
		
        for(size_t i=0; i<numSamples; i++){
			// multiply channels 0 and 1 by the positive side of the quadrature
			// oscillator
			float g = simd_max(This->rq.x, 0.0f);
			buffersL[0][i] *= g;
			buffersR[0][i] *= g;

			g = simd_min(This->rq.x, 0.0f);
			buffersL[1][i] *= g;
			buffersR[1][i] *= g;
			
			// multiple channels 2 and 3 by the negative side of the oscillator
			g = simd_max(This->rq.y, 0.0f);
			buffersL[2][i] *= g;
			buffersR[2][i] *= g;
			
			g = simd_min(This->rq.y, 0.0f);
			buffersL[3][i] *= g;
			buffersR[3][i] *= g;
            
            // multiply the vector rq by the rotation matrix m to generate the
            // next sample of the envelope
			This->rq = simd_mul(This->m, This->rq);
        }
		
		// output an array of 4 bools to let the calling function know which
		// of the channels are at zero gain. This allows the calling function
		// to make changes to the filters on those channels without causing clicks.
		//
		// the following 4 statements will be true if the gain is non-zero
		zeros[0] = This->rq.x > 0.0f;
		zeros[1] = This->rq.x < 0.0f;
		zeros[2] = This->rq.y > 0.0f;
		zeros[3] = This->rq.y < 0.0f;
	}
	
	

#ifdef __cplusplus
}
#endif
