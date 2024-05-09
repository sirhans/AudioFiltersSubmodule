//
//  BM2x2MatrixMixer.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 5/9/24.
//  Copyright © 2024 BlueMangoo. All rights reserved.
//

#ifndef BM2x2MatrixMixer_h
#define BM2x2MatrixMixer_h

#include <stdio.h>
#include "BM2x2Matrix.h"

typedef struct BM2x2MatrixMixer {
	simd_float2x2 M;
	float *buffer;
} BM2x2MatrixMixer;


/*!
 *BM2x2MatrixMixer_initWithRotation
 */
void BM2x2MatrixMixer_initWithRotation(BM2x2MatrixMixer *This, float rotationAngleInRadians);


/*!
 *BM2x2MatrixMixer_free
 */
void BM2x2MatrixMixer_free(BM2x2MatrixMixer *This);


/*!
 *BM2x2MatrixMixer_setRotation
 *
 * @abstract set the rotation matrix for this mixer struct
 */
void BM2x2MatrixMixer_setRotation(BM2x2MatrixMixer *This, float rotationAngleInRadians);


/*!
 *BM2x2MatrixMixer_processStereo
 *
 * @abstract apply a 2x2 mixing matrix to mix the stereo input to stereo output
 */
void BM2x2MatrixMixer_processStereo(BM2x2MatrixMixer *This,
									  float *inL, float *inR,
									  float *outL, float *outR,
									  size_t numSamples);

#endif /* BM2x2MatrixMixer_h */
