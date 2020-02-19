//
//  BMAttackShaper.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 18/2/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMAttackShaper_h
#define BMAttackShaper_h

#include <stdio.h>
#include "BMEnvelopeFollower.h"
#include "BMMultiLevelBiquad.h"
#include "BMDynamicSmoothingFilter.h"
#include "BMShortSimpleDelay.h"
#include "Constants.h"

typedef struct BMAttackShaper {
    BMReleaseFilter rf1, rf2;
    BMMultiLevelBiquad lpf;
    BMDynamicSmoothingFilter dsf;
    BMShortSimpleDelay dly;
    float b1 [BM_BUFFER_CHUNK_SIZE];
    float b2 [BM_BUFFER_CHUNK_SIZE];
} BMAttackShaper;

#endif /* BMAttackShaper_h */
