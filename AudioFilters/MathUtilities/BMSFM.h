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

#endif /* BMSFM_h */
