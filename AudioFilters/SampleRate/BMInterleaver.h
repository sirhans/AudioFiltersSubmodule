//
//  BMInterleaver.h
//  AudioFiltersXcodeProject
//
//  Created by hans anderson on 7/19/19.
//  Anyone may use this file without restrictions of any kind.
//

#ifndef BMInterleaver_h
#define BMInterleaver_h

#include <stdio.h>


/*!
 *BMDeInterleave
 *
 * @abstract split an interleaved array into two non-interleaved arrays
 *
 * @param input   interleaved input with length = numSamplesIn
 * @param evenOut empty array for storing the even samples of input (length / numSamplesIn/2)
 * @param oddOut  empty array for storing the odd samples of input (length / numSamplesIn/2)
 */
static void BMDeInterleave(const float* input, float* evenOut, float* oddOut, size_t numSamplesIn){
    
    // package the two output buffers as if they were real and imaginary components
    // of complex numbers. This is a misuse of notation, but computationally
    // speaking, the vDSP_ctoz function does exactly what we need here, except
    // that its syntax is designed for splitting an interleaved complex array
    // into separate real and imaginary arrays.
    DSPSplitComplex output = {evenOut, oddOut};
    
    // split the even and odd samples into two separate vectors.
    vDSP_ctoz((const DSPComplex *)input, 2, &output, 1, numSamplesIn/2);
}




/*!
 *BMInterleave
 *
 * @abstract join two arrays into a single interleaved array
 *
 * @param evenInput input with length = numSamplesIn
 * @param oddInput  input with length = numSamplesIn
 * @param output    empty array for storing the output with length = 2*numSamplesIn
 */
static void BMInterleave(float* evenInput, float* oddInput, float* output, size_t numSamplesIn){

    // package the two input buffers as if they were real and imaginary components
    // of complex numbers. This is a misuse of notation, but computationally
    // speaking, the vDSP_ztoc function does exactly what we need here, except
    // that its syntax is designed for joining separate arrays of real and
    // imaginary numbers into a single array of interleaved real and imaginary
    // values.
    DSPSplitComplex input = {evenInput, oddInput};

    // split the even and odd samples into two separate vectors.
    vDSP_ztoc(&input, 1, (DSPComplex*)output, 2, numSamplesIn);
}



#endif /* BMInterleaver_h */
