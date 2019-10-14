//
//  BMStereoMod2.h
//  SaturatorAU
//
//  Created by hans anderson on 10/14/19.
//  Copyright © 2019 bluemangoo. All rights reserved.
//

#ifndef BMStereoMod2_h
#define BMStereoMod2_h

#include <stdio.h>
#include "BMCrossover.h"
#include "BMVelvetNoiseDecorrelator.h"
#include "BMQuadratureOscillator.h"
#include "BMWetDryMixer.h"


typedef struct BMStereoMod2 {
	BMCrossover3way crossover;
	BMVelvetNoiseDecorrelator decorrelators [4];
	BMQuadratureOscillator qOscillator;
	BMWetDryMixer wetDryMixer;
	
	// buffers for low, mid, and high (mid has wet and dry)
	float *lowL, *lowR, *midLwet, *midRwet, *midLdry, *midRdry, *highL, *highR;
	
	// buffers for 4 stereo decorrelators
	float *vndL [4], *vndR [4];
} BMStereoMod2;


/*!
 *BMStereoMod2_init
 */
void BMStereoMod2_init(BMStereoMod2 *This, float sampleRate);


/*!
 *BMStereoMod2_free
 */
void BMStereoMod2_free(BMStereoMod2 *This);


/*!
 *BMStereoMod2_setModRate
 */
void BMStereoMod2_setModRate(BMStereoMod2 *This, float rateHz);


/*!
 *BMStereoMod2_setWetMix
 */
void BMStereoMod2_setWetMix(BMStereoMod2 *This, float wetMix01);


/*!
 *BMStereoMod2_process
 */
void BMStereoMod2_process(BMStereoMod2 *This,
						  const float* inL, const float* inR,
						  float* outL, float* outR,
						  size_t numSamples);

#endif /* BMStereoMod2_h */
