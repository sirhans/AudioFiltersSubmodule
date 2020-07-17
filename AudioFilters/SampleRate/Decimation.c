//
//  Decimation.c
//  OscilloScope
//
//  Created by Anh Nguyen Duc on 5/26/20.
//  Copyright © 2020 Anh Nguyen Duc. All rights reserved.
//

#include "Decimation.h"
#include <Accelerate/Accelerate.h>


void maxDecimation(const float* input, float* output, size_t N, size_t outputLength){
    assert(input != output);
    
    if(N == 1)
        memcpy(output,input,sizeof(float)*outputLength);
    else {
        // take the max of the first two elements in each group of N elements
        vDSP_vmax(input, N, input+1, N, output, 1, outputLength);
        // take the max of the element in the output and the ith element in each group of N input elements
        for(size_t i=2; i<N; i++)
            vDSP_vmax(output, 1, input+i, N, output, 1, outputLength);
    }
}

void minDecimation(const float* input, float* output, size_t N, size_t outputLength){
    assert(input != output);
    
    if(N == 1)
        memcpy(output,input,sizeof(float)*outputLength);
    else {
        // take the max of the first two elements in each group of N elements
        vDSP_vmin(input, N, input+1, N, output, 1, outputLength);
        // take the max of the element in the output and the ith element in each group of N input elements
        for(size_t i=2; i<N; i++)
            vDSP_vmin(output, 1, input+i, N, output, 1, outputLength);
    }
}


void minMaxDecimation(const float *input, float *outputMin, float *outputMax, size_t N, size_t outputLength){
    // it does not make sense to decimate by a factor of 1 or 2 because the number of output samples in the two output arrays would be >= the number of input samples
    assert(N >= 2);
    
    maxDecimation(input, outputMax, N, outputLength);
    minDecimation(input, outputMin, N, outputLength);
}


void minMaxDecimationInterleaved(const float *input, float *output, float *temp1, float *temp2, size_t N, size_t halfOutputLength){
    // it does not make sense to decimate by a factor of 1 or 2 because the number of output samples in the two output arrays would be >= the number of input samples
    assert(N >= 2);
    
    minDecimation(input, temp1, N, halfOutputLength);
    maxDecimation(input, temp2, N, halfOutputLength);
	
	size_t inputIdx = 0;
	size_t tempIdx = 0;
	size_t outputIdx = 0;
	size_t nMinusOne = N - 1;
	while(tempIdx<halfOutputLength){
		// if the current section of the input is increasing, interleave min before max
		if(input[inputIdx+nMinusOne] > input[inputIdx]){
			output[outputIdx] = temp1[tempIdx];
			output[outputIdx+1] = temp2[tempIdx];
		}
		// if decreasing, max before min
		else {
			output[outputIdx] = temp2[tempIdx];
			output[outputIdx+1] = temp1[tempIdx];
		}
			
		// advance indices
		tempIdx++;
		outputIdx += 2;
		inputIdx += N;
	}
}


/*!
 *maxAbsDecimation
 *
 * copy 1 in every N elements from input to output, taking the largest absolute value.
 */
void maxAbsDecimation(float* input, float* output, size_t N, size_t outputLength){
    assert(input != output);
    
    if(N == 1)
        memcpy(output,input,sizeof(float)*outputLength);
    else {
        // take the max of abs of the first two elements in each group of N elements
        vDSP_vmaxmg(input, N, input+1, N, output, 1, outputLength);
        // take the max of abs of the element in the output and the ith element in each group of N input elements
        for(size_t i=2; i<N; i++)
            vDSP_vmaxmg(output, 1, input+i, N, output, 1, outputLength);
    }
}

void minAbsDecimation(float* input, float* output, size_t N, size_t outputLength){
    assert(input != output);
    
    if(N == 1)
        memcpy(output,input,sizeof(float)*outputLength);
    else {
        // take the max of abs of the first two elements in each group of N elements
        vDSP_vminmg(input, N, input+1, N, output, 1, outputLength);
        // take the max of abs of the element in the output and the ith element in each group of N input elements
        for(size_t i=2; i<N; i++)
            vDSP_vminmg(output, 1, input+i, N, output, 1, outputLength);
    }
}

/*!
 *signedMaxAbsDecimationVDSP
 *
 * copy 1 in every N elements from input to output, taking the signed value that has the largest absolute value.
 *
 * @param input input array of length outputLength*N
 * @param output length = outputLength
 * @param temp temporary buffer memory length = outputLength
 * @param N decimation factor
 * @param outputLength length of output
 */
void signedMaxAbsDecimationVDSP(float* input, float* output, float* temp, size_t N, size_t outputLength){
    assert(input != output);
    
    // decimate by a factor of N, taking the max value
    maxDecimation(input, output, N, outputLength);
    
    // decimate by a factor of N, taking the max abs value
    maxAbsDecimation(input, temp, N, outputLength);
    
    // generate signed max abs output
    for(size_t i=0; i<outputLength; i++)
        // if the signed max == the abs max then the abs max came from a positive-valued input; otherwise it's negative
        output[i] = (output[i] == temp[i]) ? temp[i] : -temp[i];
}




void signedMaxAbsDecimationVDSPStable(BMDecimation *This, float *input, float *output, float *temp, float *sourceLocation, size_t upsampleFactor, size_t N, size_t outputLength){
    
    // find out how many samples we advanced in the pre-upsampled source buffer
    // Note: this will be wrong the first time we do it but it will be right
    // every time after that.
    if (This->previousSourcePosition == NULL){
        This->previousSourcePosition = sourceLocation;
    }
    size_t progress = sourceLocation - This->previousSourcePosition;
    This->previousSourcePosition = sourceLocation;
    
    // account for upsampling
    progress *= upsampleFactor;
    
    // Find out where the first sample in the *input array is located relative
    // to the downsample groups. When downsampling by a factor of N, every N
    // samples is a downsample group. The goal of this function is to preserve the
    // start indices of the groups across subsequent function calls.
    size_t downsampleGroupIndexAtStart = (progress + This->previousDownsampleGroupIndex) % N;
    This->previousDownsampleGroupIndex = downsampleGroupIndexAtStart;
	
	// find out how many samples of the input must be calculated separately at the beginning
	size_t startSamples = downsampleGroupIndexAtStart;
	
	// calculate the signed max abs of the extra samples at the beginning
	signedMaxAbsDecimationVDSP(input, output, temp, startSamples, 1);
	
	// calculate the remaining portion of the samples
	signedMaxAbsDecimationVDSP(input+startSamples, output+1, temp, N, outputLength-1);
}




void signedMinAbsDecimationVDSP(float* input, float* output, float* temp, size_t N, size_t outputLength){
    assert(input != output);
    
    // decimate by a factor of N, taking the max value
    minDecimation(input, output, N, outputLength);
    
    // decimate by a factor of N, taking the max abs value
    minAbsDecimation(input, temp, N, outputLength);
    
    // generate signed max abs output
    for(size_t i=0; i<outputLength; i++)
        // if the signed max == the abs max then the abs max came from a positive-valued input; otherwise it's negative
        output[i] = output[i] == temp[i] ? temp[i] : -temp[i];
}

void DecimationVDSP(float* input, float* output, float* temp, size_t N, size_t outputLength){
    float zero = 0.0f;
    vDSP_vsadd(input, N, &zero, output, 1, outputLength);
}

/*!
 *signedMaxAbsDecimation
 *
 * copy 1 in every N elements from input to output, taking the signed value that has the largest absolute value.
 *
 * @param input input array of length outputLength*N
 * @param output length = outputLength
 * @param N decimation factor
 * @param outputLength length of output
 */
void signedMaxAbsDecimation(float* input, float* output, size_t N, size_t outputLength){
    assert(input != output);
    
    for(size_t i=0; i<outputLength; i += 1){
        size_t k = N * i;
        float signedMaxAbs = input[k];
        float maxAbs = fabsf(input[k]);
        for(size_t j=k+1; j<k+N; j++){
            float absInputJ = fabsf(input[j]);
            if (absInputJ > maxAbs){
                signedMaxAbs = input[j];
                maxAbs = absInputJ;
            }
        }
        output[i] = signedMaxAbs;
    }
}


void getResampleFactors(size_t sampleWidth, size_t targetSampleWidth, size_t *upsampleFactor, size_t *downsampleFactor){
    
    // more than 8x extra samples
    if (sampleWidth > 8 * targetSampleWidth){
        *upsampleFactor = 1;
		*downsampleFactor = floor((float)sampleWidth/(4.0f*(float)targetSampleWidth));
        return;
    }
    
//    // more than 3/2 extra samples
//    if (2*sampleWidth > 3*targetSampleWidth){
//        *upsampleFactor = 2;
//        *downsampleFactor = 3;
//        return;
//    }
    
//    // near target samples (between 2/3 and 3/2 of target)
//    if (3*sampleWidth > 2*targetSampleWidth){
//        *upsampleFactor = 1;
//        *downsampleFactor = 1;
//        return;
//    }
    
    // near target samples (greater than 1/2 target)
    if (2*sampleWidth >= targetSampleWidth){
        *upsampleFactor = 1;
        *downsampleFactor = 1;
        return;
    }
    
    // less than 1/2 of target samples
    if(2*sampleWidth < targetSampleWidth){
        *upsampleFactor = floor((float)targetSampleWidth/(float)sampleWidth);
        *downsampleFactor = 1;
        return;
    }
    
//    // between 1/2 and 2/3 of target
//    // else
//    *upsampleFactor = 3;
//    *downsampleFactor = 2;
}
