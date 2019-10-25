//
//  BMBiquadArray.h
//  BMAudioFilters
//
//  This is useful for situations where we need to filter a large number
//  of audio channels in parallel. For example, for filtering inside a FDN
//  reverberator
//
//  To use this in an FDN, initialise with FDNHighShelfInit or FDNLowShelfInit.
//  If you need low, mid, and high frequency decay time settings, use two
//  filter arrays. One for low, one for high. Mid will be controlled by the
//  decay rate of the unfiltered part of the spectrum between low and high.
//  You must set cutoff frequencies to ensure that an unfiltered part does
//  exist between low and high. Remember that these filters have gradual
//  cutoff so you need to leave a wide space in the middle to avoid overlap.
//
//  To actually process audio in an FDN, pass an array* of input and another
//  array for output and call the processSample function (so called because
//  it processes a single sample of input and output on all N channels).
//
//  Created by hans anderson on 1/1/18.
//  Anyone may use this file without restrictions
//

#ifndef BMBiquadArray_h
#define BMBiquadArray_h

#include <stdio.h>
#include <Accelerate/Accelerate.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct BMBiquadArray {
        float *x1, *x2, *y1, *y2;
        float *a1, *a2, *b0, *b1, *b2;
        float sampleRate;
        size_t numChannels;
    } BMBiquadArray;
    
    
    /*
      *This function initialises memory and puts all the filters
     * into bypass mode.
     */
    void BMBiquadArray_init(BMBiquadArray *This, size_t numChannels, float sampleRate);


    /*
     * Initialize a filter array for use in controlling high frequency decay time in a feedback Delay network
     *
     * @param delayTimes          list of delay times in seconds for the FDN
     * @param fc                  cutoff frequency of the shelf filters (Hz)
     * @param unfilteredRT60      the decay time below fc
     * @param filteredRT60        the decay time above fc
     * @param numChannels         size of FDN, length of delayTimes
     * @param sampleRate          sample rate in Hz
     */
//    void BMBiquadArray_FDNHighShelfInit(BMBiquadArray *This, float* delayTimes, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels, float sampleRate);
    
    /*
     * Set high frequency decay for High shelf filter array in an FDN
     *
     * @param delayTimeSeconds  the length in seconds of each delay in the FDN
     * @param fc                cutoff frequency
     * @param unfilteredRT60    decay time below fc (seconds)
     * @param filteredRT60      decay time at nyquist (seconds)
     * @param numChannels       size of FDN
     */
    void BMBiquadArray_setHighDecayFDN(BMBiquadArray *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    
    
    
    
    /*
     * Initialize a filter array for use in controlling low frequency decay time in a feedback Delay network
     *
     * @param delayTimes          list of delay times in seconds for the FDN
     * @param fc                  cutoff frequency of the shelf filters (Hz)
     * @param unfilteredRT60      the decay time above fc
     * @param filteredRT60        the decay time below fc
     * @param numChannels         size of FDN, length of delayTimes
     * @param sampleRate          sample rate in Hz
     */
//    void BMBiquadArray_FDNLowShelfInit(BMBiquadArray *This, float* delayTimes, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels, float sampleRate);
    
    
    /*
     * Set low frequency decay for High shelf filter array in an FDN
     *
     * @param delayTimeSeconds  the length in seconds of each delay in the FDN
     * @param fc                cutoff frequency
     * @param unfilteredRT60    decay time above fc (seconds)
     * @param filteredRT60      decay time at DC (seconds)
     * @param numChannels       size of FDN
     */
    void BMBiquadArray_setLowDecayFDN(BMBiquadArray *This, float* delayTimesSeconds, float fc, float unfilteredRT60, float filteredRT60, size_t numChannels);
    
    
    
    /*
     * process one sample of audio on the input array of numChannels channels
     *
     * length of input == length of output == numChannels
     * (works in place)
     */
    
    static __inline__ __attribute__((always_inline)) void BMBiquadArray_processSample(BMBiquadArray *This, float* input, float* output, size_t numChannels){
        assert(numChannels <= This->numChannels);
        
        /*
          *This is the biquad difference equation that we want to compute:
         *
         * y0 = b0 * x0 + b1 * x1 + b2 * x2 - (a1 * y1 + a2 * y2);
         *
         * below are the steps we use to vectorise that computation
         */
        
        // rename input to x0 and output to y0;
        float* x0 = input;
        float* y0 = output;
        
        // rename x2 and y2 for use as temp storage arrays
        float* tx = This->x2;
        float* ty = This->y2;
        float* tt = ty;
        
        // tx = b1 * x1 + b2 * x2
        vDSP_vmma(This->b1, 1, This->x1, 1,
                  This->b2, 1, This->x2, 1,
                  tx, 1, numChannels);
        
        // ty = a1 * y1 + a2 * y2
        vDSP_vmma(This->a1, 1, This->y1, 1,
                  This->a2, 1, This->y2, 1,
                  ty, 1, numChannels);
        
        // tt = tx - ty
        vDSP_vsub(ty, 1, tx, 1, tt, 1, numChannels);
        
        // y0 = (b0 * x0) + tt
        vDSP_vma(This->b0, 1, x0, 1, tt, 1, y0, 1, numChannels);
        
        
        
        /*
         * advance the memory
         */
        // swap x1, x2
        float* temp = This->x2;
        This->x2 = This->x1;
        This->x1 = temp;
        
        // copy x0 to x1
        memcpy(This->x1, x0, numChannels*sizeof(float));
        
        // swap y1, y2
        temp = This->y2;
        This->y2 = This->y1;
        This->y1 = temp;
        
        // copy y0 to y1
        memcpy(This->y1, y0, numChannels*sizeof(float));
    }
    
    
    
    
    
    
    /*
     * Free memory and clear
     */
    void BMBiquadArray_free(BMBiquadArray *This);
    
    
    
    /*
     * Print the impulse response of the first filter in the array
     */
    void BMBiquadArray_impulseResponse(BMBiquadArray *This);
    

#ifdef __cplusplus
}
#endif
    
#endif /* BMBiquadArray_h */
