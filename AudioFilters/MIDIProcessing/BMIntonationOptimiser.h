//
//  BMIntonationOptimiser.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 6/12/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMIntonationOptimiser_h
#define BMIntonationOptimiser_h

#include <stdio.h>
#include <stdbool.h>

int BMIntonationOptimiser_processNoteList(const int *MIDINoteList, float *pitchCorrectionList, size_t numNotes);


#endif /* BMIntonationOptimiser_h */
