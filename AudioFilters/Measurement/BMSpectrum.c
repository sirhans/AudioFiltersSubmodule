//
//  BMSpectrum.c
//  Saturator
//
//  Created by Nguyen Minh Tien on 1/31/18.
//  Anyone may use this file without restrictions
//

#include "BMSpectrum.h"
#include "BMIntegerMath.h"

// forward declarations
void BMSpectrum_setupFFT(BMSpectrum *This,size_t n);



void BMSpectrum_init(BMSpectrum *This, size_t fftLength){
    This->fftLength = fftLength;
    This->fft_initialized = false;
    BMSpectrum_setupFFT(This, fftLength);
}





void BMSpectrum_setupFFT(BMSpectrum *This,size_t n){
    if(This->fft_initialized){
        vDSP_destroy_fftsetup(This->setup);
        
        free(This->fft_input_buffer1);
        free(This->fft_input_buffer2);
        free(This->fft_output_buffer1);
        free(This->fft_output_buffer2);
    }
    //FFT
    assert(isPowerOfTwo(n));
    
    const size_t log2n = log2i(n); // 2^10 = 1024
    printf("%zu %zu\n",log2n,n);
    
    //input
    This->fft_input_buffer1 = malloc((n/2)*sizeof(float)+15);
    This->fft_input.realp = (float*)(((uintptr_t)This->fft_input_buffer1+15) & ~ (uintptr_t)0x0F);
    This->fft_input_buffer2 = malloc((n/2)*sizeof(float)+15);
    This->fft_input.imagp = (float*)(((uintptr_t)This->fft_input_buffer2+15) & ~ (uintptr_t)0x0F);
    //output
    This->fft_output_buffer1 = malloc((n/2)*sizeof(float)+15);
    This->fft_output.realp = (float*)(((uintptr_t)This->fft_output_buffer1+15) & ~ (uintptr_t)0x0F);
    This->fft_output_buffer2 = malloc((n/2)*sizeof(float)+15);
    This->fft_output.imagp = (float*)(((uintptr_t)This->fft_output_buffer2+15) & ~ (uintptr_t)0x0F);
    //Buffer
    This->fft_buffer_buffer1 = malloc((n/2)*sizeof(float)+15);
    This->fft_buffer.realp = (float*)(((uintptr_t)This->fft_buffer_buffer1+15) & ~ (uintptr_t)0x0F);
    This->fft_buffer_buffer2 = malloc((n/2)*sizeof(float)+15);
    This->fft_buffer.imagp = (float*)(((uintptr_t)This->fft_buffer_buffer2+15) & ~ (uintptr_t)0x0F);
    
    // prepare the fft algo (you want to reuse the setup across fft calculations)
    This->setup = vDSP_create_fftsetup(log2n, kFFTRadix2);
    
    This->fft_initialized = true;
    
    This->windowData = malloc(sizeof(float)*n);
    vDSP_hamm_window(This->windowData, n, 0);
}





float* BMSpectrum_processData(BMSpectrum *This,float* inData,int inSize,int* outSize,float* nq){
    if(inSize>0){
        //Output size = half input size
        *outSize = inSize/2;
        
        const size_t log2n = log2i(inSize);
        
        //        assert(dataSize!=DesiredBufferLength);
        
        // apply a window function to fade the edges to zero
        vDSP_vmul(inData, 1, This->windowData, 1, inData, 1, inSize);
        
        // copy the input to the packed complex array that the fft algo uses
        vDSP_ctoz((DSPComplex *) inData, 2, &This->fft_input, 1, *outSize);
        
        // calculate the fft
        //        vDSP_fft_zrip(setup, &a, 1, log2n, FFT_FORWARD);
        vDSP_fft_zropt(This->setup, &This->fft_input, 1, &This->fft_output, 1, &This->fft_buffer, log2n, FFT_FORWARD);
        
        *nq = This->fft_output.imagp[0];
        This->fft_output.imagp[0] = 0;
        // give a nice name and reuse the buffer for output
        float* magSpectrum = This->fft_buffer.realp;
        vDSP_zvabs(&This->fft_output, 1, magSpectrum, 1, *outSize);
        
        //Limit the point
        float minValue = 0.0001;//-40db
        float maxValue = 1000;
        vDSP_vclip(magSpectrum, 1, &minValue, &maxValue, magSpectrum, 1, *outSize);
        float b = 0.01;
        vDSP_vdbcon(magSpectrum, 1, &b, magSpectrum, 1, *outSize, 1);
        
        *nq = 20* log10f(*nq/b);
        return magSpectrum;
    }
    return nil;
}


