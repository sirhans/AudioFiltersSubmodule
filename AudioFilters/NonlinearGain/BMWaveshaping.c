//
//  BMWaveshaping.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 4/7/19.
//  Anyone may use this file without restrictions of any kind
//

#include <Accelerate/Accelerate.h>
#include <simd/simd.h>

/*!
 *BMWaveshaper_processBufferBidirectional
 * @abstract clips the input to [-1,1] with a cubic soft clipping function
 */
void BMWaveshaper_processBufferBidirectional(const float* input, float* output, size_t numSamples){
    // Waveshaping function on [-1,1] with continuous derivatives at the boundaries
    // Mathematica:
    //   waveshaping[x_]:=-(1/2)Clip[x]^3 + (3/2)Clip[x]
    //
    // hard clip to [-1,1]
    float one = 1.0f;
    float negOne = -1.0f;
    vDSP_vclip(input, 1, &negOne, &one, output, 1, numSamples);
    //
    //
    // use third order polynomial to apply waveshaping
    // -(1/2)Clip[x]^3 + (3/2)Clip[x] = (x/2)(3 - x^2)
//    float* x = tempBuffer;
//    vDSP_vsq(x, 1, output, 1, numSamples); // x^2 in output
//    float neg3 = -3.0f;
//    vDSP_vsadd(output, 1, &neg3, output, 1, numSamples); // (x^2 - 3)
//    vDSP_vmul(x,1,output,1,output,1,numSamples); // x (x^2 - 3)
//    float negHalf = -0.5f;
//    vDSP_vsmul(output, 1, &negHalf, output, 1, numSamples); // (x/2)(3 - x^2)
    
    // apply the polynomial function -(1/2)Clip[x]^3 + (3/2)Clip[x]
    //float coefficients [4] = {-0.5f,0.0f,1.5f,0.0f};
    //vDSP_vpoly(coefficients, 1, output, 1, output, 1, numSamples, 3);
    
    // apply the polynomial function -(1/2) x^3 + (3/2) x
    while(numSamples >= 4){
        simd_float4* x = (simd_float4*)output;
        *x = *x * (*x * *x - 3.0f) * -0.5f;
        output+=4;
        numSamples -= 4;
    }
    // finish up the last n <= 3 samples if necessary
    while (numSamples > 0){
        *output = *output * (*output + *output - 3.0f) * -0.5f;
        output++;
        numSamples--;
    }
}



/*!
 *BMWaveshaper_processBufferPositive
 * @abstract clips the input to [0,1] with a cubic soft clipping function
 * @input    an array of values in the range [0,1].
 * @output   if the input is in [0,inf] then the output will be in [0,1]
 */
void BMWaveshaper_processBufferPositive(const float* input, float* output, size_t numSamples){
    // Mathematica:
    //     waveshapingPos[x_] := 2 Clip[x] - Clip[x]^2
    
    // clip values to
    float zero = 0.0f;
    float negOne = 1.0f;
    vDSP_vclip(input, 1, &zero, &negOne, output, 1, numSamples);
    
    // apply the polynomial function 2 x - x^2
    //float coefficients [3] = {-1.0f,2.0f,0.0f};
    //vDSP_vpoly(coefficients, 1, output, 1, output, 1, numSamples, 2);
    
    // apply the polynomial function 2 x - x^2
    simd_float4 two = 2.0f;
    while(numSamples >= 4){
        simd_float4* x = (simd_float4*)output;
        *x = *x * (two - *x);
        output+=4;
        numSamples -=4;
    }
    // finish up the last n <= 3 samples if necessary
    while (numSamples > 0){
        *output = *output * (2.0f - *output);
        output++;
        numSamples--;
    }
}



//
void BMWaveshaper_processBufferNegative(const float* input, float* output, size_t numSamples){
    // Mathematica:
    //     waveshapingPos[x_] := 2 Clip[x] + Clip[x]^2
    
    // clip values to [-1,0]
    float zero = 0.0f;
    float negOne = -1.0f;
    vDSP_vclip(input, 1, &negOne, &zero, output, 1, numSamples);
    
    // apply the polynomial function 2 x + x^2
    //float coefficients [3] = {1.0f,2.0f,0.0f};
    //vDSP_vpoly(coefficients, 1, output, 1, output, 1, numSamples, 2);
    
    // apply the polynomial function 2 x + x^2
    // in chunks of four samples
    simd_float4 two = 2.0f;
    while(numSamples >= 4){
        simd_float4* x = (simd_float4*)output;
        *x = *x * (two + *x);
        output+=4;
        numSamples -= 4;
    }
    // finish up the last n <= 3 samples if necessary
    while (numSamples > 0){
        *output = *output * (2.0f - *output);
        output++;
        numSamples--;
    }
}



#include "BMWaveshaping.h"
