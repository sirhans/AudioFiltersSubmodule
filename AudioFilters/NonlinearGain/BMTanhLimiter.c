//
//  BMTanhLimiter.c
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/31/19.
//  Copyright © 2019 BlueMangoo. All rights reserved.
//

#include "BMTanhLimiter.h"
#include <Accelerate/Accelerate.h>
#include <assert.h>
#include <simd/simd.h>



//
//void BMTanhLimiter(const float* input, float* output, float softLimit, float hardLimit, size_t numSamples){
//
//    simd_float4* input4 = (simd_float4*)input;
//    simd_float4* output4 = (simd_float4*)output;
//    simd_float4 scaleUp = hardLimit - softLimit;
//    simd_float4 scaleDown = 1.0f / scaleUp;
//    simd_float4 shiftDown = -softLimit * scaleDown;
//    simd_float4 softLimit4 = softLimit;
//
//    while(numSamples >= 4){
//        // save the sign of the input
//        simd_float4 sign = simd_sign(*input4);
//
//        // make the input positive so we can use the positive side limiter code
//        simd_float4 t = simd_abs(*input4);
//
//        // (x/(hardLimit-softLimit)-softLimit/(hardLimit-softLimit))
//        // scale and shift down
//        t = simd_muladd(*input4, scaleDown, shiftDown);
//
//        // apply the tanh function
//        t = _simd_tanh_f4(t);
//
//        // scale and shift back into place
//        t = simd_muladd(t, scaleUp, softLimit4);
//
//        // take the min of input and output
//        t = simd_min(t, *input4);
//
//        // restore the sign and output
//        *output4 = t * sign;
//
//        input4++;
//        output4++;
//        numSamples -= 4;
//    }
//
//    // finish up if there are any samples left not divisible by 4
//    output = (float*)output4;
//    input = (float*)input4;
//    while(numSamples > 0){
//        float sign = simd_sign(*input);
//        float t = fabsf(*input);
//        t = simd_muladd(*input,scaleDown.x,shiftDown.x);
//        t = tanhf(t);
//        t = simd_muladd(t, scaleUp.x, softLimit);
//        t = simd_min(t, *input);
//        *output = sign * t;
//
//        input++;
//        output++;
//        numSamples--;
//    }
//}




void BMTanhLimiterUpper(const float* input, float* output, float softLimit, float hardLimit, size_t numSamples){
    assert(hardLimit > softLimit);
    assert(input != output);
    
    // Mathematica prototype:
    //
    // TanhLimitUpper[x_,softLimit_,hardLimit_] :=
    //   Min[(hardLimit-softLimit)Tanh[(x/(hardLimit-softLimit)-softLimit/(hardLimit-softLimit))]+softLimit,x]

    //     (x/(hardLimit-softLimit)-softLimit/(hardLimit-softLimit))
    float scaleUp = (hardLimit-softLimit);
    float scaleDown = 1.0f / scaleUp;
    float shiftDown = -softLimit * scaleDown;
    vDSP_vsmsa(input, 1, &scaleDown, &shiftDown, output, 1, numSamples);
    
    // apply the tanh function
    int numSamplesInt = (int)numSamples;
    vvtanhf(output,output,&numSamplesInt);
    
    // shift and scale back into place
    // (hardLimit-softLimit)output + softLimit
    vDSP_vsmsa(output, 1, &scaleUp, &softLimit, output, 1, numSamples);
    
    // The positive side of the tanh function applies a gain reduction, so
    // wherever the output of the tanh function is greater than the input, we
    // are on the wrong side of the tanh, and should copy the input to the output
    // directly.
    vDSP_vmin(input, 1, output, 1, output, 1, numSamples);
}


//void BMTanhLimiterUpperSimd(const float* input, float* output, float softLimit, float hardLimit, size_t numSamples){
//    simd_float4* input4 = (simd_float4*)input;
//    simd_float4* output4 = (simd_float4*)output;
//    simd_float4 scaleUp = hardLimit - softLimit;
//    simd_float4 scaleDown = 1.0f / scaleUp;
//    simd_float4 shiftDown = -softLimit * scaleDown;
//    simd_float4 softLimit4 = softLimit;
//
//    while(numSamples >= 4){
//        // (x/(hardLimit-softLimit)-softLimit/(hardLimit-softLimit))
//        // scale and shift down
//        simd_float4 t = simd_muladd(*input4, scaleDown, shiftDown);
//
//        // apply the tanh function
//        t = _simd_tanh_f4(t);
//
//        // scale and shift back into place
//        t = simd_muladd(t, scaleUp, softLimit4);
//
//        // take the min of input and output
//        *output4 = simd_min(t, *input4);
//
//        input4++;
//        output4++;
//        numSamples -= 4;
//    }
//
//    // finish up if there are any samples left not divisible by 4
//    output = (float*)output4;
//    input = (float*)input4;
//    while(numSamples > 0){
//        float t = simd_muladd(*input,scaleDown.x,shiftDown.x);
//        t = tanhf(t);
//        t = simd_muladd(t, scaleUp.x, softLimit);
//        *output = simd_min(t, *input);
//
//        input++;
//        output++;
//        numSamples--;
//    }
//}



//void BMTanhLimiterLowerSimd(const float* input, float* output, float softLimit, float hardLimit, size_t numSamples){
//    simd_float4* input4 = (simd_float4*)input;
//    simd_float4* output4 = (simd_float4*)output;
//    simd_float4 scaleUp = hardLimit - softLimit;
//    simd_float4 scaleDown = 1.0f / scaleUp;
//    simd_float4 shiftUp = softLimit * scaleDown;
//    simd_float4 negSoftLimit4 = -softLimit;
//    
//    while(numSamples >= 4){
//        // (x/(hardLimit-softLimit)-softLimit/(hardLimit-softLimit))
//        // scale and shift down
//        simd_float4 t = simd_muladd(*input4, scaleDown, shiftUp);
//        
//        // apply the tanh function
//        t = _simd_tanh_f4(t);
//        
//        // scale and shift back into place
//        t = simd_muladd(t, scaleUp, negSoftLimit4);
//        
//        // take the max of input and output
//        *output4 = simd_max(t, *input4);
//        
//        input4++;
//        output4++;
//        numSamples -= 4;
//    }
//    
//    // finish up if there are any samples left not divisible by 4
//    output = (float*)output4;
//    input = (float*)input4;
//    while(numSamples > 0){
//        float t = simd_muladd(*input,scaleDown.x,shiftUp.x);
//        t = tanhf(t);
//        t = simd_muladd(t, scaleUp.x, negSoftLimit4.x);
//        *output = simd_max(t, *input);
//        
//        input++;
//        output++;
//        numSamples--;
//    }
//}


void BMTanhLimiterLower(const float* input, float* output, float softLimit, float hardLimit, size_t numSamples){
    assert(hardLimit > softLimit);
    assert(input != output);
    
    // Mathematica prototype:
    //
    // TanhLimitLower[x_,softLimit_,hardLimit_] :=
    //    Max[(hardLimit-softLimit)Tanh[(x/(hardLimit-softLimit)+softLimit/(hardLimit-softLimit))]-softLimit,x]

    //     (x/(hardLimit-softLimit)+softLimit/(hardLimit-softLimit))
    float scaleUp = (hardLimit-softLimit);
    float scaleDown = 1.0f / scaleUp;
    float shiftUp = softLimit * scaleDown;
    vDSP_vsmsa(input, 1, &scaleDown, &shiftUp, output, 1, numSamples);
    
    // apply the tanh function
    int numSamplesInt = (int)numSamples;
    vvtanhf(output,output,&numSamplesInt);
    
    // shift and scale back into place
    // (hardLimit-softLimit)output - softLimit
    float shiftDown = -softLimit;
    vDSP_vsmsa(output, 1, &scaleUp, &shiftDown, output, 1, numSamples);
    
    // The negative side of the tanh function takes the gain closer to zero, so
    // wherever the output of the tanh function is less than the input, we
    // are on the wrong side of the tanh, and should copy the input to the output
    // directly.
    vDSP_vmax(input, 1, output, 1, output, 1, numSamples);
}
