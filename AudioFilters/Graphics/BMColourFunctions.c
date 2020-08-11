//
//  BMColourFunctions.c
//  OscilloScope
//
//  Created by hans anderson on 8/11/20.
//  Anyone may use this file without restrictions
//

#include "BMColourFunctions.h"
#include <math.h>




simd_float3 saturate3(simd_float3 v) {
	return simd_clamp(v,0.0f, 1.0f);
}




const simd_float3 bHSVZeroH = {0.5,3.5/3,2.5/3};



/*!
 *bHSVToRGB
 *
 * bHSV stands for balanced HSV. This is a modification of the hsv colour model
 * that allows for more consistent gradients without the banding effects around
 * the three primary colours or the inconsistent perceived brightness when
 * adjusting the hue.
 */
simd_float3 bHSVToRGB(float h, float s, float v){
	simd_float3 bHSV = {h,s,v};
	
	// get the pure hue as an RGB colour
	simd_float3 hueRGB = saturate3(6.0f * simd_abs(simd_fract(bHSVZeroH + bHSV.x) - .5f) - 1.f);
	
	// emphasize the Yellow, Cyan, Magenta
	simd_float3 huePow = pow(hueRGB,1.5f);
	hueRGB = 1.0f - huePow;
	
	// balance the value so that the yellow and magenta fade to black at a similar rate
	simd_float1 gamma = fabs(simd_fract(sqrt(bHSV.x) + 0.5)*2. - 1.)*bHSV.y;
	
	return ((hueRGB - 1.) * bHSV.y + 1.) * powf(bHSV.z, 1.0 + gamma);
}





/*!
 *freqToRGBColour
 *
 * Converts a frequency in Hz to an RGB colour so that notes with the same name
 * have the same hue, regardless of the octave. This is useful for using colour
 * to indicate musical pitches.
 */
void freqToRGBColour(float freqHz, float *rgb){
	float hue = log2f(freqHz);
	simd_float3 rgb3 = bHSVToRGB(hue,1.0,0.5);
	rgb[0] = rgb3.x;
	rgb[1] = rgb3.y;
	rgb[2] = rgb3.z;
}
