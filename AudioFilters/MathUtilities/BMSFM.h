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
    float* buffer;
} BMSFM;



/*!
 *BMSFM_init
 *
 * @param This   pointer to a BMSFM struct
 * @param inputLength  FFT window length
 */
void BMSFM_init(BMSFM* This, size_t inputLength);




/*!
 *BMSFM_free
 *
 * @param This   pointer to an initialised BMSFM struct
 * @abstract free memory
 */
void BMSFM_free(BMSFM* This);



/*!
 *BMSFM_process
 *
 * @param This   pointer to an initialised BMSFM struct
 * @param input  an array of floats with length = inputLength
 * @param inputLength   length of input
 */
float BMSFM_process(BMSFM* This, float* input, size_t inputLength);


#endif /* BMSFM_h */
