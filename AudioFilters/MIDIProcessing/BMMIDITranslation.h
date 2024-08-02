//
//  BMMIDITranslation.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 6/12/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#ifndef BMMIDITranslation_h
#define BMMIDITranslation_h

#include <stdio.h>
#include <assert.h>


enum BMChromaticNote {BMCN_C,BMCN_Db,BMCN_D,BMCN_Eb,BMCN_E,BMCN_F,BMCN_Gb,BMCN_G,BMCN_Ab,BMCN_A,BMCN_Bb,BMCN_B};


/*!
 *MIDINoteToBMChromaticNote
 *
 * Convert a note as unsigned 8 bit int to BMChromaticNote, wrapping notes greater than 13.
 * This counts from C = 0.
 */
enum BMChromaticNote MIDINoteToBMChromaticNote(int MIDINoteNumber);



/*!
 *MIDINoteToOctave
 *
 * Find out which octave MIDINoteNumber lies in. (C0 = 12)
 */
int MIDINoteToOctave(int MIDINoteNumber);



/*!
 *pitchToFrequency
 *
 * convert from scientific pitch notation to a specific frequency in Hz
 *
 * @param octave the octave number
 * @param note a chromatic note from C to B. Note that C=0 so you can cast from int or uint type if necessary
 * @param intonationInCents 0 is the neutral value. +100 is one note above and -100 is one note below the pitch specified by note and octave
 * @param a4ReferenceFrequency default = 440. Use the default unless you have a good reason for doing otherwise.
 */
float pitchToFrequency(int octave,
					   enum BMChromaticNote note,
					   int intonationInCents,
					   float a4ReferenceFrequency);


/*!
 *MIDINoteToFrequnecy
 *
 * Convert from midi note to frequency in Hz, using A=440 tuning standard
 *
 * @param MIDINote this can be outside the normal range of MIDI notes, and may even include negative numbers
 */
float MIDINoteToFrequnecy(int MIDINote);


#endif /* BMMIDITranslation_h */
