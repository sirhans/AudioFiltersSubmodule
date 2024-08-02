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

/*!
 *BMIntonationOptimiser_processNoteList
 *
 * @param MIDINoteList is an array of ints where each number is a MIDI note
 * @param pitchCorrectionList output will be written here. the nth element of this list is the correction for the nth MIDI note. A correction of 1.0 means bend up 1 full MIDI note. Actual correction will always be less than 0.16 MIDI notes.
 * @param numNotes length of MIDINoteList and pitchCorrectionList
 */
int BMIntonationOptimiser_processNoteList(const int *MIDINoteList, float *pitchCorrectionList, size_t numNotes);


#endif /* BMIntonationOptimiser_h */
