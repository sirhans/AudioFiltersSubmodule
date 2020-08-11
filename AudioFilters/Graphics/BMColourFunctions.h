//
//  BMColourFunctions.h
//  OscilloScope
//
//  Created by hans anderson on 8/11/20.
//  Anyone may use this file without restrictions
//

#ifndef BMColourFunctions_h
#define BMColourFunctions_h

#include <stdio.h>
#include <simd/simd.h>





/*!
 *bHSVToRGB
 *
 * bHSV stands for balanced HSV. This is a modification of the hsv colour model
 * that allows for more consistent gradients without the banding effects around
 * the three primary colours or the inconsistent perceived brightness when
 * adjusting the hue.
 */
simd_float3 bHSVToRGB(float h, float s, float v);


/*!
 *freqToNotein0_1
 *
 * Takes a frequency in Hz and converts it to a number on [0,1] % 1 where a
 * difference of 1 indicates an octave. This is used for colourising frequencies
 * in a way that assigns the same hue to each note regardless of which octave
 * it is in.
 */
float freqToNoteIn0_1(float freqHz);




/*!
 *freqToRGBColour
 *
 * Converts a frequency in Hz to an RGB colour so that notes with the same name
 * have the same hue, regardless of the octave. This is useful for using colour
 * to indicate musical pitches.
 */
void freqToRGBColour(float freqHz, float *rgb);

#endif /* BMColourFunctions_h */
