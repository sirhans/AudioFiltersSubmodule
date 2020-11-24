//
//  BMMIDITranslation.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 6/12/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMMIDITranslation.h"
#include <math.h>

enum BMChromaticNote MIDINoteToBMChromaticNote(uint8_t MIDINoteNumber){
	return (enum BMChromaticNote)(MIDINoteNumber % 12);
}




uint8_t MIDINoteToOctave(uint8_t MIDINoteNumber){
	return MIDINoteNumber / 12;
}




float pitchToFrequency(uint8_t octave,
					   enum BMChromaticNote note,
					   int intonationInCents,
					   float a4ReferenceFrequency){
	// Mathematica Prototype
	//
	// pitchToFrequency[octave_,note_,tuningCents_,A4frequency_]:=A4frequency * (2^((note-10)/12)) *(2^(octave-4))*(2^(tuningCents/1200))
	
	float octaveExp = (float)octave - 4.0f;
	float noteExp = ((float)note-10.0f) / 12.0f;
	float tuningExp = (float)intonationInCents / 1200.0f;
	return a4ReferenceFrequency * powf(2.0f, octaveExp + noteExp + tuningExp);
}




float MIDINoteToFrequency(uint8_t MIDINote){
	uint8_t octave = MIDINoteToOctave(MIDINote);
	enum BMChromaticNote note = MIDINoteToBMChromaticNote(MIDINote);
	int intonation = 0;
	float a4 = 440.0f;
	return pitchToFrequency(octave, note, intonation, a4);
}
