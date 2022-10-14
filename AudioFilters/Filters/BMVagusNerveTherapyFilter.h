//
//  BMVagusNerveTherapyFilter.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 10/13/22.
//  Copyright © 2022 BlueMangoo. All rights reserved.
//

#ifndef BMVagusNerveTherapyFilter_h
#define BMVagusNerveTherapyFilter_h

#include <stdio.h>
#include "BMMultiLevelSVF.h"
#include "BMLFO.h"

typedef struct BMVagusNerveTherapyFilter {
	BMMultiLevelSVF svf;
	BMLFO lfo;
} BMVagusNerveTherapyFilter;

#endif /* BMVagusNerveTherapyFilter_h */
