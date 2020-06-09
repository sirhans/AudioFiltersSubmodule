//
//  BMFastHadamard.h
//  BMAudioFilters
//
//  Performs a Fast Hadamard Transform
//  reference: https://en.wikipedia.org/wiki/Fast_Walsh–Hadamard_transform
//
//  Created by Hans on 15/9/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMFastHadamard_h
#define BMFastHadamard_h

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <simd/simd.h>
#include <Accelerate/Accelerate.h>

// forward declarations
static void BMFastHadamard16(const float* input, float* output, float* temp16);
static void BMFastHadamard8(const float* input, float* output, float* temp8);
static void BMFastHadamard4(const float* input, float* output, float* temp4);
static void BMFastHadamard2(const float* input, float* output);




// is x a power of 2?
// reference: http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/
static inline bool BMPowerOfTwoQ (size_t x)
{
	return ((x != 0) && ((x & (~x + 1)) == x));
}


static __inline__ __attribute__((always_inline)) void BMFastHadamardTransformProcessBlock(float** input,
                                                                                    float** output,
                                                                                    size_t inputSize,
                                                                                    size_t numSamples){
    size_t halfSize = inputSize/2;
    //block 1
    for(int i=0;i<halfSize;i++){
        vDSP_vadd(input[i], 1, output[i], 1, output[i], 1, numSamples);
        vDSP_vadd(input[i], 1, output[i+halfSize], 1, output[i+halfSize], 1, numSamples);
    }
    //block 2
    for(int i=0;i<halfSize;i++){
        vDSP_vadd(input[i+halfSize], 1, output[i], 1, output[i], 1, numSamples);
        vDSP_vsub(output[i+halfSize], 1, input[i+halfSize], 1, output[i+halfSize], 1, numSamples);
    }
    
}

static __inline__ __attribute__((always_inline)) void BMFastHadamardTransformBuffer(float** input,
                                                                            float** output,
                                                                            size_t inputSize,
                                                                            size_t numSamples){
    assert(input!=output);

    
    //Step1
    size_t blockSize = inputSize;
    while (blockSize>1) {
        //Set output = 0
        for(int i=0;i<inputSize;i++){
            vDSP_vclr(output[i], 1, numSamples);
        }
        
        for(int shift=0;shift<inputSize;shift+=blockSize){
            BMFastHadamardTransformProcessBlock(input+shift, output+shift, blockSize,numSamples);
        }
        
        //Copy output into input
        size_t bufferSize = sizeof(float)*numSamples;
        for(int i=0;i<inputSize;i++){
            memcpy(input[i], output[i], bufferSize);
        }
        
        //take half of the block size
        blockSize /= 2;
    }
    
    float normalizeVol = 1/sqrtf(inputSize);
    for(int i=0;i<inputSize;i++){
        vDSP_vsmul(output[i], 1, &normalizeVol, output[i], 1, numSamples);
    }
}



/*!
 * Fast Hadamard Transform
 *   works in place
 *
 * @param input   an array of floats with length = length
 * @param output  an array of floats with length = length
 * @param temp1   an array of floats with length = length (temp buffer)
 * @param temp2   an array of floats with length = length (temp buffer)
 * @param length  must be a power of 2, >= 16
 */

static __inline__ __attribute__((always_inline)) void BMFastHadamardTransform(float* input,
																			  float* output,
																			  float* temp1,
																			  float* temp2,
																			  size_t length){
	// length must be a power of 2
	assert(BMPowerOfTwoQ(length));
	// length must be at least 2
	assert(length >= 2);
	
	size_t partitionSize = length;
	float* tmpIn = input;
	float* tmpOut = temp1;
	bool tmpState = true;
	
	// iteratively calculated recursion
	while (partitionSize > 16) {
		size_t halfPartitionSize = partitionSize >> 1;
		for (size_t i = 0; i < length; i += partitionSize){
			// copy all the lower terms into place
			memcpy(tmpOut+i,tmpIn+i,sizeof(float)*halfPartitionSize);
			memcpy(tmpOut+i+halfPartitionSize,tmpIn+i,sizeof(float)*halfPartitionSize);
			// sum all the higher terms into place
			for (size_t j=i; j<halfPartitionSize+i; j++) {
				size_t idx2 = j+halfPartitionSize;
				tmpOut[j] += tmpIn[idx2];
				tmpOut[idx2] -= tmpIn[idx2];
			}
		}
		
		// swap temp buffers to avoid using the same memory for reading and writing.
		tmpIn = tmpOut;
		tmpState = !tmpState;
		if (tmpState) tmpOut = temp1;
		else tmpOut = temp2;
		partitionSize >>= 1;
	}
	
	// base cases
	if(partitionSize == 16) {
		for(size_t i=0; i<length; i+=16)
			BMFastHadamard16(tmpIn + i, output + i, tmpOut);
		return;
	}
	if(partitionSize == 8){
		BMFastHadamard8(tmpIn, output, tmpOut);
		return;
	}
	if(partitionSize == 4){
		BMFastHadamard4(tmpIn, output, tmpOut);
		return;
	}
	if(partitionSize == 2){
		BMFastHadamard2(tmpIn, output);
		return;
	}
}




/*
 * Fast Hadamard transform of length 16
 *   works in place.
 *
 * @param input   an array of 16 floats
 * @param output  an array of 16 floats
 * @param temp16  an array of 16 floats that will be over-written
 *
 */
static inline void BMFastHadamard16(const float* input, float* output, float* temp16){
	
	// TODO: is hard-coding faster than using for loops?
	//
	// here is the loop version of the code:
	//
	// size_t i;
	// // level 1
	// for(i=0; i<8; i++)   temp16[i] = input[i] + input[i+8];
	// for(i=8; i<16; i++)  temp16[i] = input[i-8] - input[i];
	//
	// // level 2
	// for(i=0; i<4; i++)   output[i] = temp16[i] + temp16[i+4];
	// for(i=4; i<8; i++)   output[i] = temp16[i-4] - temp16[i];
	// for(i=8; i<12; i++)  output[i] = temp16[i] + temp16[i+4];
	// for(i=12; i<16; i++) output[i] = temp16[i-4] - temp16[i];
	//
	// // level 3
	// for(i=0; i<16; i+=4) temp16[i] = output[i] + output[i+2];
	// for(i=1; i<16; i+=4) temp16[i] = output[i] + output[i+2];
	// for(i=2; i<16; i+=4) temp16[i] = output[i-2] - output[i];
	// for(i=3; i<16; i+=4) temp16[i] = output[i-2] - output[i];
	//
	// // level 4
	// for(i=0; i<16; i+=2) output[i] = temp16[i] + temp16[i+1];
	// for(i=1; i<16; i+=2) output[i] = temp16[i-1] - temp16[i];
	
	
	// hard-coded version:
	//
	// level 1
	// +
	temp16[0] = input[0] + input[8];
	temp16[1] = input[1] + input[9];
	temp16[2] = input[2] + input[10];
	temp16[3] = input[3] + input[11];
	temp16[4] = input[4] + input[12];
	temp16[5] = input[5] + input[13];
	temp16[6] = input[6] + input[14];
	temp16[7] = input[7] + input[15];
	// -
	temp16[8] = input[0] - input[8];
	temp16[9] = input[1] - input[9];
	temp16[10] = input[2] - input[10];
	temp16[11] = input[3] - input[11];
	temp16[12] = input[4] - input[12];
	temp16[13] = input[5] - input[13];
	temp16[14] = input[6] - input[14];
	temp16[15] = input[7] - input[15];
	
	
	// level 2
	//+
	output[0] = temp16[0] + temp16[4];
	output[1] = temp16[1] + temp16[5];
	output[2] = temp16[2] + temp16[6];
	output[3] = temp16[3] + temp16[7];
	//-
	output[4] = temp16[0] - temp16[4];
	output[5] = temp16[1] - temp16[5];
	output[6] = temp16[2] - temp16[6];
	output[7] = temp16[3] - temp16[7];
	
	//+
	output[8] = temp16[8] + temp16[12];
	output[9] = temp16[9] + temp16[13];
	output[10] = temp16[10] + temp16[14];
	output[11] = temp16[11] + temp16[15];
	//-
	output[12] = temp16[8] - temp16[12];
	output[13] = temp16[9] - temp16[13];
	output[14] = temp16[10] - temp16[14];
	output[15] = temp16[11] - temp16[15];
	
	
	
	// level 3
	// +
	temp16[0] = output[0] + output[2];
	temp16[1] = output[1] + output[3];
	// -
	temp16[2] = output[0] - output[2];
	temp16[3] = output[1] - output[3];
	
	// +
	temp16[4] = output[4] + output[6];
	temp16[5] = output[5] + output[7];
	// -
	temp16[6] = output[4] - output[6];
	temp16[7] = output[5] - output[7];
	
	// +
	temp16[8] = output[8] + output[10];
	temp16[9] = output[9] + output[11];
	// -
	temp16[10] = output[8] - output[10];
	temp16[11] = output[9] - output[11];
	
	// +
	temp16[12] = output[12] + output[14];
	temp16[13] = output[13] + output[15];
	// -
	temp16[14] = output[12] - output[14];
	temp16[15] = output[13] - output[15];
	
	
	// level 4
	output[0] = temp16[0] + temp16[1];
	output[1] = temp16[0] - temp16[1];
	output[2] = temp16[2] + temp16[3];
	output[3] = temp16[2] - temp16[3];
	
	output[4] = temp16[4] + temp16[5];
	output[5] = temp16[4] - temp16[5];
	output[6] = temp16[6] + temp16[7];
	output[7] = temp16[6] - temp16[7];
	
	output[8] = temp16[8] + temp16[9];
	output[9] = temp16[8] - temp16[9];
	output[10] = temp16[10] + temp16[11];
	output[11] = temp16[10] - temp16[11];
	
	output[12] = temp16[12] + temp16[13];
	output[13] = temp16[12] - temp16[13];
	output[14] = temp16[14] + temp16[15];
	output[15] = temp16[14] - temp16[15];
}








/*
 * Fast Hadamard transform of length 8
 *   works in place.
 *
 * @param input   an array of 8 floats
 * @param output  an array of 8 floats
 * @param temp8  an array of 8 floats that will be over-written
 *
 */
static inline void BMFastHadamard8(const float* input, float* output, float* temp8){
	
	// level 2
	//+
	output[0] = input[0] + input[4];
	output[1] = input[1] + input[5];
	output[2] = input[2] + input[6];
	output[3] = input[3] + input[7];
	//-
	output[4] = input[0] - input[4];
	output[5] = input[1] - input[5];
	output[6] = input[2] - input[6];
	output[7] = input[3] - input[7];
	
	
	// level 3
	// +
	temp8[0] = output[0] + output[2];
	temp8[1] = output[1] + output[3];
	// -
	temp8[2] = output[0] - output[2];
	temp8[3] = output[1] - output[3];
	
	// +
	temp8[4] = output[4] + output[6];
	temp8[5] = output[5] + output[7];
	// -
	temp8[6] = output[4] - output[6];
	temp8[7] = output[5] - output[7];
	
	
	// level 4
	output[0] = temp8[0] + temp8[1];
	output[1] = temp8[0] - temp8[1];
	output[2] = temp8[2] + temp8[3];
	output[3] = temp8[2] - temp8[3];
	
	output[4] = temp8[4] + temp8[5];
	output[5] = temp8[4] - temp8[5];
	output[6] = temp8[6] + temp8[7];
	output[7] = temp8[6] - temp8[7];
}






/*
 * Fast Hadamard transform of length 4
 *   works in place.
 *
 * @param input   an array of 4 floats
 * @param output  an array of 4 floats
 * @param temp4  an array of 4 floats that will be over-written
 *
 */
static inline void BMFastHadamard4(const float* input, float* output, float* temp4){
	
	// level 3
	// +
	temp4[0] = input[0] + input[2];
	temp4[1] = input[1] + input[3];
	// -
	temp4[2] = input[0] - input[2];
	temp4[3] = input[1] - input[3];
	
	
	
	// level 4
	output[0] = temp4[0] + temp4[1];
	output[1] = temp4[0] - temp4[1];
	output[2] = temp4[2] + temp4[3];
	output[3] = temp4[2] - temp4[3];
}





/*
 * Fast Hadamard transform of length 2
 *   works in place.
 *
 * @param input   an array of 2 floats
 * @param output  an array of 2 floats
 *
 */
static inline void BMFastHadamard2(const float* input, float* output){
	
	// level 4
	output[0] = input[0] + input[1];
	output[1] = input[0] - input[1];
}


static inline void BMFastHadamard1x4(const simd_float4 *input, simd_float4 *output){
	simd_float2 t;
	// level 3
	t.xy         = output[0].xy + output[0].zw;
	output[0].zw = output[0].xy - output[0].zw;
	output[0].xy = t.xy;
	// level 4
	t.x         = output[0].x + output[0].y;
	output[0].y = output[0].x - output[0].y;
	output[0].x = t.x;
	t.x         = output[0].z + output[0].w;
	output[0].w = output[0].z - output[0].w;
	output[0].z = t.x;
}


static inline void BMFastHadamard2x4(const simd_float4 *input, simd_float4 *output){
	simd_float4 t;
	// level 2
	t         = output[0] + output[1];
	output[1] = output[0] - output[1];
	output[0] = t;
	// level 3
	t.xy         = output[0].xy + output[0].zw;
	output[0].zw = output[0].xy - output[0].zw;
	output[0].xy = t.xy;
	
	t.xy         = output[1].xy + output[1].zw;
	output[1].zw = output[1].xy - output[1].zw;
	output[1].xy = t.xy;
	// level 4
	t.x         = output[0].x + output[0].y;
	output[0].y = output[0].x - output[0].y;
	output[0].x = t.x;
	t.z         = output[0].z + output[0].w;
	output[0].w = output[0].z - output[0].w;
	output[0].z = t.z;
	
	t.x         = output[1].x + output[1].y;
	output[1].y = output[1].x - output[1].y;
	output[1].x = t.x;
	t.z         = output[1].z + output[1].w;
	output[1].w = output[1].z - output[1].w;
	output[1].z = t.z;
}


static inline void BMFastHadamard4x4(const simd_float4 *input, simd_float4 *output){
	simd_float4 t;
	// level 1
	t 		  = input[0] + input[2];
	output[2] = input[0] - input[2];
	output[0] = t;
	t         = input[1] + input[3];
	output[3] = input[1] - input[3];
	output[1] = t;
	// level 2
	t         = output[0] + output[1];
	output[1] = output[0] - output[1];
	output[0] = t;
	t         = output[2] + output[3];
	output[3] = output[2] - output[3];
	output[2] = t;
	// level 3
	t.xy         = output[0].xy + output[0].zw;
	output[0].zw = output[0].xy - output[0].zw;
	output[0].xy = t.xy;
	
	t.xy         = output[1].xy + output[1].zw;
	output[1].zw = output[1].xy - output[1].zw;
	output[1].xy = t.xy;
	
	t.xy         = output[2].xy + output[2].zw;
	output[2].zw = output[2].xy - output[2].zw;
	output[2].xy = t.xy;
	
	t.xy         = output[3].xy + output[3].zw;
	output[3].zw = output[3].xy - output[3].zw;
	output[3].xy = t.xy;
	// level 4
	t.x         = output[0].x + output[0].y;
	output[0].y = output[0].x - output[0].y;
	output[0].x = t.x;
	t.z         = output[0].z + output[0].w;
	output[0].w = output[0].z - output[0].w;
	output[0].z = t.z;
	
	t.x         = output[1].x + output[1].y;
	output[1].y = output[1].x - output[1].y;
	output[1].x = t.x;
	t.z         = output[1].z + output[1].w;
	output[1].w = output[1].z - output[1].w;
	output[1].z = t.z;
	
	t.x         = output[2].x + output[2].y;
	output[2].y = output[2].x - output[2].y;
	output[2].x = t.x;
	t.z         = output[2].z + output[2].w;
	output[2].w = output[2].z - output[2].w;
	output[2].z = t.z;
	
	t.x         = output[3].x + output[3].y;
	output[3].y = output[3].x - output[3].y;
	output[3].x = t.x;
	t.z         = output[3].z + output[3].w;
	output[3].w = output[3].z - output[3].w;
	output[3].z = t.z;
}




/*!
 *BMRotateRight1
 *
 * @abstract shift all elements right one position, wrapping around to the beginning
 */
static inline void BMRotateRight1(const float* input, float* output, size_t numSamples){
	float lastElement = input[numSamples-1];
	memmove(1 + output, input, (numSamples - 1)*sizeof(float));
	output[0] = lastElement;
}





/*
* Mix the feedback signal
*
* The code below does the first two stages of a fast hadamard transform,
* followed by some swapping.
* Leaving the transform incomplete is equivalent to using a
* block-circulant mixing matrix. Typically, block circulant mixing is
* done using the last two stages of the fast hadamard transform. Here
* we use the first two stages instead because it permits us to do
* vectorised additions and subtractions with a stride of 1.
*
* Regarding block-circulant mixing, see: https://www.researchgate.net/publication/282252790_Flatter_Frequency_Response_from_Feedback_Delay_Network_Reverbs
*
*/
static inline void BMBlockCirculantMix4xN(simd_float4 *in,
										  simd_float4 *out,
										  size_t numBlocks){
	
	size_t halfNumBlocks = numBlocks/2;
	size_t fourthNumBlocks = numBlocks/4;
	simd_float4 *inHalf = in + halfNumBlocks;
	simd_float4 *outHalf = out + halfNumBlocks;
	simd_float4 *outFourth = out + fourthNumBlocks;
	simd_float4 *outThreeFourths = outHalf + fourthNumBlocks;
	simd_float4 matrixAttenuation = simd_make_float4(0.5f);
	for(size_t i=0; i<halfNumBlocks; i++){
		// first half = first half plus second half => buffer to temp variable
		simd_float4 t = matrixAttenuation * (in[i] + inHalf[i]);
		// second half = first half minus second half
		outHalf[i] = matrixAttenuation * (in[i] - inHalf[i]);
		// restore from temp variable
		out[i] = t;
	}
	for(size_t i=0; i<fourthNumBlocks; i++){
		// first fourth equals first forth + second fourth => buffer to temp
		simd_float4 t = out[i] + outFourth[i];
		// second fourth equals first forth - second fourth
		outFourth[i] = out[i] - outFourth[i];
		// restore from temp
		out[i]= t;
		// third fourth equals third fourth + fourth fourth => buffer to temp
		t = outHalf[i] + outThreeFourths[i];
		// fourth fourth equals third fourth - fourth fourth
		outThreeFourths[i] = outHalf[i] - outThreeFourths[i];
		// restore from temp
		outHalf[i] = t;
	}
	
	// rotate the output by one position so that the signal has to pass
	// numDelays/4 delays before completing the circuit
	BMRotateRight1((float*)out, (float*)out, 4*numBlocks);
}




/*!
 *BMBlockCirculantMix4x4
 *
 * @param in an array of 4 simd_float4 vectors
 * @param out an array of 4 simd_float4 vectors
 */
static inline void BMBlockCirculantMix4x4(const simd_float4 *in, simd_float4 *out){
	simd_float4 t;
	
	// level 4
		 t = (in[0] + in[2]);
	out[2] = (in[0] - in[2]);
	out[0] = t;
		 t = (in[1] + in[3]);
	out[3] = (in[1] - in[3]);
	out[1] = t;
	
	// level 3
	t      = out[0] + out[1];
	out[1] = out[0] - out[1];
	out[0] = t;
	t      = out[2] + out[3];
	out[3] = out[2] - out[3];
	out[2] = t;
	
	// rotate the output by one position so that the signal has to pass
	// numDelays/4 delays before completing the circuit
	BMRotateRight1((float*)out, (float*)out, 16);
}


#endif /* BMFastHadamard_h */


#ifdef __cplusplus
}
#endif
