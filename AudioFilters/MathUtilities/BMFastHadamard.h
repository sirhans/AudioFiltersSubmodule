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

    
    // forward declarations
    void BMFastHadamard16(const float* input, float* output, float* temp16);
    bool BMPowerOfTwoQ (size_t x);
    
    
    
    
    
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
        // length must be at least 16 because we hard-code the
        // base case at order 16.
        assert(length >= 16);
        
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
        
        // base case
        for (size_t i = 0; i < length; i += 16)
            BMFastHadamard16(tmpIn + i, output + i, tmpOut);
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
    inline void BMFastHadamard16(const float* input, float* output, float* temp16){
        
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

    
    
    // is x a power of 2?
    // reference: http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/
    inline bool BMPowerOfTwoQ (size_t x)
    {
        return ((x != 0) && ((x & (~x + 1)) == x));
    }

    
    
    
#endif /* BMFastHadamard_h */


#ifdef __cplusplus
}
#endif
