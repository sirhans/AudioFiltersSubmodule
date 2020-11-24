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
void BMDeInterleave(const float* input, float* evenOut, float* oddOut, size_t numSamplesIn);



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
void BMDeInterleave4(const float* input,
					 float* out1, float* out2, float* out3, float* out4,
					 float* buffer,
					 size_t numSamplesIn);


/*!
 *BMInterleave
 *
 * @abstract join two arrays into a single interleaved array
 *
 * @param evenInput input with length = numSamplesIn
 * @param oddInput  input with length = numSamplesIn
 * @param output    empty array for storing the output with length = 2*numSamplesIn
 */
void BMInterleave(float* evenInput, float* oddInput, float* output, size_t numSamplesIn);


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
void BMInterleave4(float*  input1, float* input2,
				   float*  input3, float* input4,
				   float*  output, size_t numSamplesIn);

#endif /* BMInterleaver_h */
