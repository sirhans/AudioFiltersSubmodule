//
//  BMReleaseShaper.h
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/12/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMReleaseShaper_h
#define BMReleaseShaper_h

#define BMRS_NUM_RELEASE_FILTERS 3

#include <stdio.h>
#include "BMEnvelopeFollower.h"

typedef struct BMReleaseShaper {
    float *b1, *b2;
    float releaseAmount;
    BMReleaseFilter fastRelease [BMRS_NUM_RELEASE_FILTERS];
    BMReleaseFilter slowRelease;
} BMReleaseShaper;

#endif /* BMReleaseShaper_h */
