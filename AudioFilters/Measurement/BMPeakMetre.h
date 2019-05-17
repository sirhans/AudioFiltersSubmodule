//
//  BMBMPeakMetre.h
//  
//
//  Created by Hans Anderson on 5/17/19.
//  This file is free for use without restrictions of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMPeakMetre_h
#define BMPeakMetre_h

#include <stdio.h>
#include "BMEnvelopeFollower.h"

typedef struct BMPeakMetre {
    BMReleaseFilter fastRelease;
    BMReleaseFilter slowRelease [3];
} BMPeakMetre;

void BMPeakMetre_init(BMPeakMetre* This, float fastReleaseTime, float slowReleaseTime, float sampleRate);

void BMPeakMetre_free(BMPeakMetre* This);

#endif /* BMPeakMetre_h */


#ifdef __cplusplus
}
#endif
