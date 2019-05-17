//
//  BMBMPeakMeter.h
//  
//
//  Created by Hans Anderson on 5/17/19.
//  This file is free for use without restrictions of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMPeakMeter_h
#define BMPeakMeter_h
    
    
#define BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS 3

#include <stdio.h>
#include "BMEnvelopeFollower.h"

typedef struct BMPeakMeter {
    float sampleRate;
    BMReleaseFilter fastReleaseL, fastReleaseR;
    BMReleaseFilter slowReleaseL [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
    BMReleaseFilter slowReleaseR [BM_PEAK_METER_NUM_SLOW_RELEASE_FILTERS];
} BMPeakMeter;

void BMPeakMeter_init(BMPeakMeter* This, float fastReleaseTime, float slowReleaseTime, float sampleRate);

void BMPeakMeter_free(BMPeakMeter* This);

#endif /* BMPeakMetre_h */


#ifdef __cplusplus
}
#endif
