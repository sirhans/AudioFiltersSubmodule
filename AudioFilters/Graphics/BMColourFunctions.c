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


float signedSquare(float x){
	return x * x * simd_sign(x);
}


float signedPow(float x, float p){
	return powf(fabsf(x), p) * simd_sign(x);
}



/*!
 *evenHueMod
 *
 * Scale the hue value so that in a rainbow gradient the width of the R, G, B
 * appears similar to the width of the Y, C M colours
 */
float evenHueMod(float h){
	/* prototype:
	 *
	 * SignedPow[x_,p_]:=Abs[x]^pow * Sign[x]
	 * p = 1.75;
	 * ModH[h_]:=(IntegerPart[3h+0.5]+ 2^(p-1)SignedPow[FractionalPart[3h+0.5]-0.5,p])/3
	 */
	float scaledH = 3.0f * h + 0.5f;
	float integerPart = floor(scaledH);
	float p = 1.75f;
	float fractionalPart = powf(2.0f, p - 1.0f) * signedPow(simd_fract(scaledH)-0.5f,p);
	return (integerPart + fractionalPart) / 3.0f;
}





/*!
 *bHSVToRGB
 *
 * bHSV stands for balanced HSV. This is a modification of the hsv colour model
 * that allows for more consistent gradients without the banding effects around
 * the three primary colours or the inconsistent perceived brightness when
 * adjusting the hue.
 */
simd_float3 bHSVToRGB(float h, float s, float v){
	
	simd_float3 bHSV = {evenHueMod(h),s,v};
	
	// get the pure hue as an RGB colour
	simd_float3 hueRGB = saturate3(6.0f * simd_abs(simd_fract(bHSVZeroH + bHSV.x) - .5f) - 1.f);
	
	// emphasize the Yellow, Cyan, Magenta
	simd_float3 huePow = pow(hueRGB,2.0f);
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
	float hue = simd_fract(log2f(freqHz));
	simd_float3 rgb3 = bHSVToRGB(hue,1.0,0.5);
	rgb[0] = rgb3.x;
	rgb[1] = rgb3.y;
	rgb[2] = rgb3.z;
}
