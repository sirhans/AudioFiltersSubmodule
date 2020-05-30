//
//  BMSincDownsampler.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 28/5/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMSincDownsampler.h"
#include <Accelerate/Accelerate.h>

void BMSincDownsampler_genKernel(BMSincDownsampler *This);

void BMSincDownsampler_init(BMSincDownsampler *This, size_t kernelLength, size_t downsampleFactor){
    // the kernel length should be an odd whole-number greater than zero
    assert(kernelLength != 0 && kernelLength % 2 == 1);
    // the downsample factor can not be zero
    assert(downsampleFactor != 0);

    // if we are not downsampling, ignore the kernel length
    if(downsampleFactor == 1)
        kernelLength = 1;
    
    This->kernelLength = kernelLength;
    This->downsampleFactor = downsampleFactor;
    This->paddingLeft = (kernelLength - 1) / 2;
    This->paddingRight = This->paddingLeft;

    if(kernelLength > 1 && downsampleFactor > 1){
        // If the padding is divisible by the downsample factor then then first
        // and last samples in the kernel are zero, which wastes processing time.
        // Therefore in this case we reduce the kernel length by two
        if(This->paddingLeft % downsampleFactor == 0){
            printf("WARNING: Sinc downsampler kernel starts and ends with zeros. The kenernel length will be reduced by 2 to save processing time.\n");
            kernelLength -= 2;
            This->kernelLength = kernelLength;
            This->downsampleFactor = downsampleFactor;
            This->paddingLeft = (kernelLength - 1) / 2;
            This->paddingRight = This->paddingLeft;
        }

        This->kernel = malloc(sizeof(float)*kernelLength);

        // generate the sinc lowpass filter kernel
        BMSincDownsampler_genKernel(This);
    }
}


void BMSincDownsampler_free(BMSincDownsampler *This){
    if(This->kernelLength > 1 && This->downsampleFactor > 1){
        free(This->kernel);
        This->kernel = NULL;
    }
}


float BMSinc(float t){
    // avoid divide by zero error
    if (fabsf(t)<=FLT_EPSILON)
        return 1.0f;

    // else return sinc(t)
    return sinf(t)/t;
}


void BMSincDownsampler_genKernel(BMSincDownsampler *This){
    for(size_t i=0; i<This->kernelLength; i++){
        // t is the index i shifted so the centre of the kernel is at 0
        float t = (float)i - (float)This->paddingLeft;
        // p is the phase of the sine wave in the sinc function
        float p = M_PI * t / (float)This->downsampleFactor;
        // this places the cutoff frequency of the filter at nyquist/downsampleFactor
        This->kernel[i] = BMSinc(p);
    }

    // apply a Hamming window function to the filter kernel. Hamming is the
    // window function that doesn't go all the way down to zero at its edges.
    // It has steeper cutoff and less flat frequency response.
    float *window = malloc(sizeof(float)*This->kernelLength);
    int useHalfWindow = false;
    vDSP_hamm_window(window,This->kernelLength,useHalfWindow);
    vDSP_vmul(window,1,This->kernel,1,This->kernel,1,This->kernelLength);
    free(window);
}



void BMSincDownsampler_process(BMSincDownsampler *This, float *input, float *output, size_t outputLength){
    // no downsample if the downsample factor is one
    if(This->downsampleFactor == 1){
        // no copy if input and output point to the same data
        if(output != input)
            memcpy(output, input, sizeof(float)*outputLength);
    } else {
        // we can not do downsampling in place
        assert(input != output);

        vDSP_desamp(input, This->downsampleFactor, This->kernel, output, outputLength, This->kernelLength);
    }
}
