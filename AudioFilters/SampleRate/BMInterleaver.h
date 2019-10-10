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
 *BMDeInterleave4
 *
 * @abstract split an interleaved array into four non-interleaved arrays
 *
 * @param input interleaved input with length = numSamplesIn
 * @param out1  empty array for storing the even samples of input (length / numSamplesIn/4)
 * @param out2  empty array for storing the odd samples of input (length / numSamplesIn/4)
 * @param out3  empty array for storing the even samples of input (length / numSamplesIn/4)
 * @param out4  empty array for storing the odd samples of input (length / numSamplesIn/4)
 * @param buffer array for temp storage with length = numSamplesIn (This can point to input)
 */
static void BMDeInterleave4(const float* input,
                            float* out1, float* out2, float* out3, float* out4,
                            float* buffer,
                            size_t numSamplesIn){
    
    // package the two output buffers as if they were real and imaginary components
    // of complex numbers. This is a misuse of notation, but computationally
    // speaking, the vDSP_ctoz function does exactly what we need here, except
    // that its syntax is designed for splitting an interleaved complex array
    // into separate real and imaginary arrays.
    float* temp13 = buffer;
    float* temp24 = buffer + numSamplesIn/2;
    DSPSplitComplex outputEvenOdd = {temp13, temp24};
    
    // split the even and odd samples into two separate vectors.
    vDSP_ctoz((const DSPComplex *)input, 2, &outputEvenOdd, 1, numSamplesIn/2);
    
    DSPSplitComplex output13 = {out1, out3};
    DSPSplitComplex output24 = {out2, out4};
    vDSP_ctoz((const DSPComplex *)temp13, 2, &output13, 1, numSamplesIn/4);
    vDSP_ctoz((const DSPComplex *)temp24, 2, &output24, 1, numSamplesIn/4);
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

    // combine the even and odd samples into one vector
    vDSP_ztoc(&input, 1, (DSPComplex*)output, 2, numSamplesIn);
}


/*!
 *BMInterleave4
 *
 * @abstract join four arrays into a single interleaved array
 *
 * @param input1 input with length = numSamplesIn
 * @param input2 input with length = numSamplesIn
 * @param input3 input with length = numSamplesIn
 * @param input4 input with length = numSamplesIn
 * @param output empty array for storing the output with length = 4*numSamplesIn
 */
static void BMInterleave4(float*  input1, float* input2,
                          float*  input3, float* input4,
                          float*  output, size_t numSamplesIn){

    // package the two input buffers as if they were real and imaginary components
    // of complex numbers. This is a misuse of notation, but computationally
    // speaking, the vDSP_ztoc function does exactly what we need here, except
    // that its syntax is designed for joining separate arrays of real and
    // imaginary numbers into a single array of interleaved real and imaginary
    // values.
    DSPSplitComplex input13 = {input1, input3};
    DSPSplitComplex input24 = {input2, input4};

    // combine 1 and 3 samples and combine 2 and 4 samples
    float* odd = output;
    float* even = output + 2*numSamplesIn;
    vDSP_ztoc(&input13, 1, (DSPComplex*)odd, 2, numSamplesIn);
    vDSP_ztoc(&input24, 1, (DSPComplex*)odd, 2, numSamplesIn);
    
    // combine the odd and even samples into one output
    DSPSplitComplex inputAll = {odd,even};
    vDSP_ztoc(&inputAll, 1, (DSPComplex*)output, 2, 2*numSamplesIn);
}

#endif /* BMInterleaver_h */
