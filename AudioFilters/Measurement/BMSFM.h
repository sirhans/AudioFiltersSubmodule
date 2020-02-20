//
//  BMSFM.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 9/14/19.
//  Anyone may use this file without restrictions
//

#ifndef BMSFM_h
#define BMSFM_h

#include <stdio.h>
#include "BMFFT.h"


typedef struct BMSFM {
    BMFFT fft;
    float *b1, *b2;
} BMSFM;



/*!
 *BMSFM_init
 *
 * @param This   pointer to a BMSFM struct
 * @param inputLength  FFT window length
 */
void BMSFM_init(BMSFM *This, size_t inputLength);




/*!
 *BMSFM_free
 *
 * @param This   pointer to an initialised BMSFM struct
 * @abstract free memory
 */
void BMSFM_free(BMSFM *This);



/*!
 *BMSFM_process
 *
 * @param This   pointer to an initialised BMSFM struct
 * @param input  an array of floats with length = inputLength
 * @param inputLength   length of input
 */
float BMSFM_process(BMSFM *This, float* input, size_t inputLength);


/*!
 *BMGeometricMean
 *
 * @abstract returns the geometric mean of the elements in the array input
 *
 * @param input array of inputs with length inputLength
 * @param inputLength length of input array
 */
float BMGeometricMean(float* input, size_t inputLength);


/*!
 *BMGeometricMean2
 *
 * @abstract returns the geometric mean of a and b
 */
float BMGeometricMean2(float a, float b);


#endif /* BMSFM_h */
