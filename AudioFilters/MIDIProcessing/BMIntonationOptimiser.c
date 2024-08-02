//
//  BMIntonationOptimiser.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 6/12/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include <assert.h>
#include <limits.h>
#include "BMIntonationOptimiser.h"
#include "BMMIDITranslation.h"
#include <Accelerate/Accelerate.h>

#define BMINTONATION_ARRAY_LENGTH 128
#define BMINTONATION_TOLERANCE 0.16 // tuning tolerance as a fraction of 1 MIDI note



/*!
 *intListMin
 *
 * Finds the value and index of the minimum element in the list
 *
 * @param list pointer to the first element in a list of ints
 * @param minVal value of the minimum element
 * @param minIdx index of the minimum element
 * @param length length of list
 */
void intListMin(const int *list, int *minVal, size_t *minIdx, size_t length){
	*minVal = INT_MAX;
	for(size_t i=0; i<length; i++)
		if(list[i] < *minVal){
			*minVal = list[i];
			*minIdx = i;
		}
}



/*!
 *checkTolerance
 *
 * Returns true if all frequencies in A are less than toleranceInCents away from B
 */
bool checkTolerance(const double *A, const double *B, size_t length){
	// How close do we need to get to the Equal Temperament frequency before
	// we say we're on the correct note? That's the tolerance.
	//
	// BMINTONATION_TOLERANCE is given as a fraction of 1 MIDI note (half-step).
	// On the line below we're dividing by 12 to get it as a fraction of an
	// octave, because that works well with log2(frequency), which also works in
	// units of 1 octave.
	double tolerance = BMINTONATION_TOLERANCE / 12.0;
	
	double bOverA [BMINTONATION_ARRAY_LENGTH];
	double logDistance [BMINTONATION_ARRAY_LENGTH];
	
	// divide B/A
	vDSP_vdivD(B, 1, A, 1, bOverA, 1, length);
	
	// log2(B/A)
	int lengthi = (int)length;
	vvlog2(logDistance, bOverA, &lengthi);
	
	// find the maximum absolute value in the array logDistance
	double maxAbsVal;
	vDSP_maxmgvD(logDistance, 1, &maxAbsVal, length);
	
	// assert(maxAbsVal >= 0); // the result should be positive. We check to make sure.
	
	// if the maximum absolute value is less than tolerance then all notes are in tune;
	return (maxAbsVal < tolerance);
}



/*!
 *roundToNearestMultiple
 *
 * Round each element of A to the nearest integer multiple of R. Output to B.
 */
void roundToNearestMultiple(double *A, double R, double *B, size_t length){
	// divide each frequency by the root note frequency
	vDSP_vsdivD(A, 1, &R, B, 1, length);
	
	// round to the nearest integer
	for(size_t i=0; i<length; i++)
		B[i] = round(B[i]);
	
	// multiply by the root note frequency
	vDSP_vsmulD(B, 1, &R, B, 1, length);
}



/*!
 *getCorrectionList
 *
 * Populates correctionList with the correction, in number of MIDI notes
 * required to shift the frequencies in frequenciesList to match those in
 * roundedFrequenciesList.
 *
 * @param frequencies a list of un-corrected frequencies
 * @param roundedFrequencies a list of corrected frequencies
 * @param pitchCorrection each element of this list is in (-1,1), where a value of 1 means it needs to be bent up a full MIDI note. Note that in practice the range will be less than this because of BMINTONATION_TOLERANCE #defined above. Also note that BMINTONATION_TOLERANCE is given in cents, while pitchCorrectionList are measured in terms of MIDI notes.
 */
void getCorrectionList(double *frequencies, double *roundedFrequencies, double *pitchCorrection, size_t numNotes){
	int numNotesi = (int)numNotes;
	
	// pitchCorrection = roundedFrequencies / frequencies
	vDSP_vdivD(roundedFrequencies, 1, frequencies, 1, pitchCorrection, 1, numNotes);
	
	// log2(pitchCorrection)
	vvlog2(pitchCorrection, pitchCorrection, &numNotesi);
	
	/*
	 * At this point, the values in pitchCorrection represent the fraction of an
	 * octave that we would need to shift the frequencies up or down to make
	 * them match roundedFrequencies. Obviously measuring this shift in octaves
	 * is not convenient, so we will not proceed to convert units so that we
	 * have the shift as a fraction of 1 MIDI note.
	 */
	
	// pitchCorrection *= 12.0;
	double twelve = 12.0;
	vDSP_vsmulD(pitchCorrection, 1, &twelve, pitchCorrection, 1, numNotes);

	/*
	 * pitchCorrection now represents the fraction of a MIDI note that each
	 * frequency needs to shift in order to match roundedFrequencies.
	 */
}



double MIDINoteToFreq(int MIDINote){
	return 440.0 * pow(2.0, ((float)MIDINote - 69.0)/12.0);
}


/*!
 *MIDINoteToFreqArray
 *
 * vectorises the following operation:
 * 	 440.0 * pow(2.0, ((float)MIDINote - 69.0)/12.0);
 */
void MIDINoteToFreqArray(const int *MIDINotes, double *frequencies, size_t length){
	// convert int to double
	vDSP_vflt32D(MIDINotes, 1, frequencies, 1, length);
	
	// (MIDINote - 69) * (1/12)
	// Equivalent to:
	// (1/12)*MIDINote - (69/12)
	double oneOverTwelve = 1.0 / 12.0;
	double negSixtyNineOverTwelve = -69.0 / 12.0;
	vDSP_vsmsaD(frequencies, 1, &oneOverTwelve, &negSixtyNineOverTwelve, frequencies, 1, length);
	
	// pow(2, f)
	for(size_t i=0; i<length; i++)
		frequencies[i] = pow(2.0,frequencies[i]);
	
	double fourFourty = 440.0;
	vDSP_vsmulD(frequencies, 1, &fourFourty, frequencies, 1, length);
}




void normalizeCorrectionsToLowestNote(double *pitchCorrections, size_t lowestNoteIndex, size_t length){
	double lowestNoteCorrection = pitchCorrections[lowestNoteIndex];
	double compensation = -lowestNoteCorrection;
	
	// add the compensation to each element of the pitch corrections
	vDSP_vsaddD(pitchCorrections, 1, &compensation, pitchCorrections, 1, length);
}




int BMIntonationOptimiser_processNoteList(const int *MIDINoteList, float *pitchCorrectionList, size_t numNotes){
	assert(numNotes <= BMINTONATION_ARRAY_LENGTH);
	
	// find the lowest note in MIDINoteList
	int lowestNote;
	size_t lowestNoteIndex;
	intListMin(MIDINoteList, &lowestNote, &lowestNoteIndex, numNotes);
	
	// get the frequency of each note in MIDINoteList
	double frequenciesList [BMINTONATION_ARRAY_LENGTH];
	double roundedFrequenciesList [BMINTONATION_ARRAY_LENGTH];
	MIDINoteToFreqArray(MIDINoteList, frequenciesList, numNotes);
	
	// starting with the lowest note, count down until we find a MIDI note
	// that meets the tuning criteria
	bool tuned = false;
	int root = lowestNote;
	while(!tuned){
		// get the frequency of the root note
		double rootFrequency = MIDINoteToFreq(root);
		
		// round each note in frequenciesList to the nearest integer multiple of rootFrequency
		roundToNearestMultiple(frequenciesList, rootFrequency, roundedFrequenciesList, numNotes);
		
		// if all rounded frequencies are within tolerance, call it tuned
		if (checkTolerance(frequenciesList, roundedFrequenciesList, numNotes))
			tuned = true;
		// otherwise, take the root down one MIDI note and try again
		else
			root--;
	}
	
	/*
	 * We now have a list of frequencies that are tuned to integer multiples of
	 * a single root frequency. The next step is to calculate how the
	 * original notes need to be corrected in order to match the rounded notes.
	 */
	double pitchCorrectionListD [BMINTONATION_ARRAY_LENGTH];
	getCorrectionList(frequenciesList, roundedFrequenciesList, pitchCorrectionListD, numNotes);
	
	
	/*
	 * To keep the corrected tuning somewhat closer to the sound of unaligned
	 * instruments, we normalise the correction so that the lowest note will
	 * correction = 0.
	 */
	normalizeCorrectionsToLowestNote(pitchCorrectionListD, lowestNoteIndex, numNotes);
	 
									 
	// convert the output to single precision float
	vDSP_vdpsp(pitchCorrectionListD, 1, pitchCorrectionList, 1, numNotes);
	
	return root;
}
