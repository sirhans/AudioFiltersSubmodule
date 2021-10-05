//
//  BMSineWaveshaper.h
//  GSAcousticAmplifier
//
//  Created by hans anderson on 20/05/2021.
//
//  This is a vectorised implementation of the following expression:
//
//      sine( log( abs(x)+1 ) * sign(x) )
//

#ifndef BMSineWaveshaper_h
#define BMSineWaveshaper_h

#include <stdio.h>

typedef struct BMSineWaveshaper {
	float *b1;
	size_t bufferSize;
} BMSineWaveshaper;


/*!
 *BMSineWaveshaper_init
 */
void BMSineWaveshaper_init(BMSineWaveshaper *This);


/*!
 *BMSineWaveshaper_free
 */

void BMSineWaveshaper_free(BMSineWaveshaper *This);


/*!
 *BMSineWaveshaper_processBuffer
 */

void BMSineWaveshaper_processBuffer(BMSineWaveshaper *This, float *input, float *output, size_t numSamples);



#endif /* BMSineWaveshaper_h */
