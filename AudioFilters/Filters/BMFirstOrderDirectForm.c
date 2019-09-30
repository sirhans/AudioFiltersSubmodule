//
//  BMFirstOrderDirectForm.c
//  SaturatorAU
//
//  Created by hans anderson on 9/29/19.
//  Anyone may use this file without restrictions of any kind
//

#include "BMFirstOrderDirectForm.h"
#include <assert.h>


/*!
 *BMFirstOrderDirectForm_process
 */
void BMFirstOrderDirectForm_process(BMFirstOrderDirectForm* This,
                                    const float** input,
                                    float** output,
                                    size_t numChannels,
                                    size_t numSamples){
    assert(numChannels <= 4 && numChannels != 3);
    
    if(numChannels == 1){
        for(size_t i=0; i<numSamples; i++){
            output[0][i] = input[0][i] * This->b0_f + This->az_f * This->b1_f + This->z1_f * This->a1_f;
            This->z1_f = output[0][i];
            This->az_f = input[0][i];
        }
    }
    if(numChannels == 2){
        for(size_t i=1; i<numSamples; i++){
            simd_float2 input2 = {input[0][i],input[1][i]};
            This->z1_f2 = input2 * This->b0_f2 + This->az_f2 * This->b1_f2 + This->z1_f2 * This->a1_f2;
            This->az_f2 = input2;
            output[0][i] = This->z1_f2.x;
            output[1][i] = This->z1_f2.y;
        }
    }
    if(numChannels == 4){
        for(size_t i=1; i<numSamples; i++){
            simd_float4 input4 = {input[0][i],input[1][i],input[2][i],input[3][i]};
            This->z1_f4 = input4 * This->b0_f4 + This->az_f4 * This->b1_f4 + This->z1_f4 * This->a1_f4;
            This->az_f4 = input4;
            output[0][i] = This->z1_f4.x;
            output[1][i] = This->z1_f4.y;
            output[2][i] = This->z1_f4.z;
            output[3][i] = This->z1_f4.w;
        }
    }
}



/*!
*BMFirstOrderDirectForm_init
*/
void BMFirstOrderDirectForm_init(BMFirstOrderDirectForm* This, float sampleRate){
    This->z1_f = 0.0f;
    This->z1_f2 = 0.0f;
    This->z1_f4 = 0.0f;
    
    This->az_f = 0.0f;
    This->az_f2 = 0.0f;
    This->az_f4 = 0.0f;
    
    This->sampleRate = sampleRate;
}



/*!
*BMFirstOrderDirectForm_setLowpass
*/
void BMFirstOrderDirectForm_setLowpass(BMFirstOrderDirectForm* This, float fc){
        
        double gamma = tan(M_PI * fc / This->sampleRate);
        double one_over_denominator = 1.0 / (gamma + 1.0);
        
        This->b0_f = gamma * one_over_denominator;
        This->b1_f = This->b0_f;
        
        This->a1_f = (gamma - 1.0) * one_over_denominator;
    
    // float2
    This->b0_f2 = This->b0_f;
    This->b1_f2 = This->b1_f;
    This->a1_f2 = This->a1_f;
    
    // float 4
    This->b0_f4 = This->b0_f;
    This->b1_f4 = This->b1_f;
    This->a1_f4 = This->a1_f;
}
