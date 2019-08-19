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
void BMWaveshaper_processBufferBidirectional(const simd_float4* input, simd_float4* output, size_t numSamples){
    simd_float4 negOne = -1.0f;
    simd_float4 one = 1.0f;
    while(numSamples >= 4){
        *output = simd_smoothstep(negOne, one, *input);
        *output = (*output * 2.0f) - 1.0f;
        output++;
        input++;
        numSamples -= 4;
    }
    
    // finish up the last values if numSamples wasn't a multiple of 4
    float* outputSingle = (float*)output;
    float* inputSingle = (float*)input;
    float negOneSingle = -1.0f;
    float oneSingle = 1.0f;
    while(numSamples > 0){
        *outputSingle = simd_smoothstep(negOneSingle, oneSingle, *inputSingle);
        outputSingle++;
        inputSingle++;
        numSamples--;
    }
//    // Waveshaping function on [-1,1] with continuous derivatives at the boundaries
//    // Mathematica:
//    //   waveshaping[x_]:=-(1/2)Clip[x]^3 + (3/2)Clip[x]
//    float one = 1.0f;
//    float negOne = -1.0f;
//
//    // hard clip to [-1,1]
//    vDSP_vclip(input, 1, &negOne, &one, tempBuffer, 1, numSamples);
//
//    // use third order polynomial to apply waveshaping
//    // -(1/2)Clip[x]^3 + (3/2)Clip[x] = (x/2)(3 - x^2)
//    float* x = tempBuffer;
//    vDSP_vsq(x, 1, output, 1, numSamples); // x^2 in output
//    float neg3 = -3.0f;
//    vDSP_vsadd(output, 1, &neg3, output, 1, numSamples); // (x^2 - 3)
//    vDSP_vmul(x,1,output,1,output,1,numSamples); // x (x^2 - 3)
//    float negHalf = -0.5f;
//    vDSP_vsmul(output, 1, &negHalf, output, 1, numSamples); // (x/2)(3 - x^2)
}






#include "BMWaveshaping.h"
