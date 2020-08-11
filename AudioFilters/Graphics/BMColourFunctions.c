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
