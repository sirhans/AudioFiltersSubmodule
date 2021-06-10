//
//  BMSineWaveshaper.c
//  GSAcousticAmplifier
//
//  Created by hans anderson on 20/05/2021.
//

#include "BMSineWaveshaper.h"
#include <Accelerate/Accelerate.h>
#include "Constants.h"




void BMSineWaveshaper_init(BMSineWaveshaper *This){
	This->bufferSize = BM_BUFFER_CHUNK_SIZE * 2;
	This->b1 = malloc(sizeof(float) * This->bufferSize);
}




void BMSineWaveshaper_free(BMSineWaveshaper *This){
	free(This->b1);
	This->b1 = NULL;
}


// this raises input to the power of n, but does it with compensation variables that
// ensure that pf(0) == 0 and pf'(0) == 1
void BMSineWaveshaper_polynomialFunction(float *input, float *output, float n, size_t numSamples){
	// Mathematica: -Power(Power(1/n,1/(-1 + n)),n) + Power(Power(1/n,1/(-1 + n)) + x,n)
	//  			-Power(       q             ,n) + Power(          q           + x,n)
	//                            r                 + Power(          q           + x,n)
	//
	float q = powf(1.0f/n, 1.0f/(n - 1.0f));
	float r = -powf(q,n);
	int numSamples_i = (int)numSamples;
	//
	//                                                                q           + x
	vDSP_vsadd(input, 1, &q, output, 1, numSamples);
	//                                                Power(          q           + x,n)
	vvpowsf(output, &n, output, &numSamples_i);
	//                            r                 + Power(          q           + x,n)
	vDSP_vsadd(output, 1, &r, output, 1, numSamples);
}



void BMSineWaveshaper_processBuffer(BMSineWaveshaper *This, float *input, float *output, size_t numSamples){
	int samplesRemaining = (int)numSamples;
	size_t offset = 0;
	
	while(samplesRemaining > 0){
		int samplesProcessing = BM_MIN((int)This->bufferSize, samplesRemaining);
		
		// sine( log( abs(x)+1 ) * sign(x) )
		//
		// scale the input so the effect is less extreme on the attacks
		//       log( abs(x)+1 ) * sign(x)
		vvfabsf(This->b1, &input[offset], &samplesProcessing);
		//vvlog1pf(&This->b1[offset], &This->b1[offset], &samplesProcessing);
		BMSineWaveshaper_polynomialFunction(This->b1, This->b1, 0.5f,samplesProcessing);
		vvcopysignf(This->b1, This->b1, &input[offset], &samplesProcessing);
		//
		size_t samplesProcessing_t = samplesProcessing;
		float gain = 40.0f;
		vDSP_vsmul(This->b1, 1, &gain, This->b1, 1, samplesProcessing_t);
		//
		// sine(             "             )
		vvsinpif(&output[offset], This->b1, &samplesProcessing);
		
		// update array indices
		offset += samplesProcessing;
		samplesRemaining -= samplesProcessing;
	}
}
