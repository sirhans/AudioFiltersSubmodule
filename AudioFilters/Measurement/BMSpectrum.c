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
    this->fftLength = fftLength;
    this->fft_initialized = false;
    BMSpectrum_setupFFT(this, fftLength);
}





void BMSpectrum_setupFFT(BMSpectrum *This,size_t n){
    if(this->fft_initialized){
        vDSP_destroy_fftsetup(this->setup);
        
        free(this->fft_input_buffer1);
        free(this->fft_input_buffer2);
        free(this->fft_output_buffer1);
        free(this->fft_output_buffer2);
    }
    //FFT
    assert(isPowerOfTwo(n));
    
    const size_t log2n = log2i(n); // 2^10 = 1024
    printf("%zu %zu\n",log2n,n);
    
    //input
    this->fft_input_buffer1 = malloc((n/2)*sizeof(float)+15);
    this->fft_input.realp = (float*)(((uintptr_t)this->fft_input_buffer1+15) & ~ (uintptr_t)0x0F);
    this->fft_input_buffer2 = malloc((n/2)*sizeof(float)+15);
    this->fft_input.imagp = (float*)(((uintptr_t)this->fft_input_buffer2+15) & ~ (uintptr_t)0x0F);
    //output
    this->fft_output_buffer1 = malloc((n/2)*sizeof(float)+15);
    this->fft_output.realp = (float*)(((uintptr_t)this->fft_output_buffer1+15) & ~ (uintptr_t)0x0F);
    this->fft_output_buffer2 = malloc((n/2)*sizeof(float)+15);
    this->fft_output.imagp = (float*)(((uintptr_t)this->fft_output_buffer2+15) & ~ (uintptr_t)0x0F);
    //Buffer
    this->fft_buffer_buffer1 = malloc((n/2)*sizeof(float)+15);
    this->fft_buffer.realp = (float*)(((uintptr_t)this->fft_buffer_buffer1+15) & ~ (uintptr_t)0x0F);
    this->fft_buffer_buffer2 = malloc((n/2)*sizeof(float)+15);
    this->fft_buffer.imagp = (float*)(((uintptr_t)this->fft_buffer_buffer2+15) & ~ (uintptr_t)0x0F);
    
    // prepare the fft algo (you want to reuse the setup across fft calculations)
    this->setup = vDSP_create_fftsetup(log2n, kFFTRadix2);
    
    this->fft_initialized = true;
    
    this->windowData = malloc(sizeof(float)*n);
    vDSP_hamm_window(this->windowData, n, 0);
}





float* BMSpectrum_processData(BMSpectrum *This,float* inData,int inSize,int* outSize,float* nq){
    if(inSize>0){
        //Output size = half input size
        *outSize = inSize/2;
        
        const size_t log2n = log2i(inSize);
        
        //        assert(dataSize!=DesiredBufferLength);
        
        // apply a window function to fade the edges to zero
        vDSP_vmul(inData, 1, this->windowData, 1, inData, 1, inSize);
        
        // copy the input to the packed complex array that the fft algo uses
        vDSP_ctoz((DSPComplex *) inData, 2, &this->fft_input, 1, *outSize);
        
        // calculate the fft
        //        vDSP_fft_zrip(setup, &a, 1, log2n, FFT_FORWARD);
        vDSP_fft_zropt(this->setup, &this->fft_input, 1, &this->fft_output, 1, &this->fft_buffer, log2n, FFT_FORWARD);
        
        *nq = this->fft_output.imagp[0];
        this->fft_output.imagp[0] = 0;
        // give a nice name and reuse the buffer for output
        float* magSpectrum = this->fft_buffer.realp;
        vDSP_zvabs(&this->fft_output, 1, magSpectrum, 1, *outSize);
        
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


