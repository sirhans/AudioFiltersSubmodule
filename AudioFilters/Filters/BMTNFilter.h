//
//  BMTNFilter.h
//  BMAudioFilters
//
//  Uses an adaptive least mean squares filter to separate the input
//  signal into tonal and noisy components
//
//  Created by Hans on 25/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMTNFilter_h
#define BMTNFilter_h

#include <stddef.h>
#include <stdbool.h>

// set the number of samples between wrap operations in the X delay buffer
#define BMTNF_FILTER_WRAP_TIME 1024

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct BMTNFilter {
        size_t filterLength, delayTime, XMemLength, normBufferLength, normBufferMaxLength, delayBufferMaxLength;
        float mu, *Xmem, *delayLine, *normalizationBuffer, *W, *X, *Xlast, *dp, *Xstart, *delayLineEnd, norm, sampleRate;
        bool hasF0;
    } BMTNFilter;
    
    /*
     * Separates the mono input signal input into two channels of output:
     * one for the periodic component (tone) and the other for the random
     * component (noise) of the input signal.
     *
     * This class does not include a function for processing stereo signals.
     * If stereo processing is required, use two BMTNFilters in parallel.
     */
    void BMTNFilter_processBuffer(BMTNFilter* f, const float* input, float* toneOut, float* noiseOut, size_t numSamples);
    
    /*
     * This function is optional.  If you don't call it, the filter will
     * use reasonable default settings.
     *
     *
     * filterOrder - set the order of the FIR Least Mean Squares Filter used
     *               to do the separation.  Default = 48. Higher values work
     *               better but take more processing power so use the lowest
     *               value that still gives a clear separation.
     *               For best performance this should be a multiple of 4.
     *
     *
     * mu - rate of convergence, mu in (0,1]. Higher values acheive separation
     *      more quickly after the initial attack of each note, but with
     *      faster convergence rate, some noise remains in the tonal signal
     *      output. Lower values adapt more slowly to attacks and changes in
     *      tonal content, but remove more noise from the tonal signal. If the
     *      value is too low, there will be some tone left back in the noise
     *      signal output.
     *
     *
     * delayTime - delays the input signal by n=delayTime samples before
     *             estimating the tonal content. If n == 1, only white noise
     *             can be removed. For other types of noise, there is some
     *             correlation between adjacent samples. For example, in
     *             brown noise, low frequency components dominate so the kth
     *             sample is close in value to the k+1th sample. In this
     *             situation, a linear predictor will be able to predict the
     *             next sample in the noise signal with some accuracy. This
     *             will result in low frequency noise being classified as
     *             tone, since the LMS filter considers anything it can
     *             predict with a linear model to be tonal.  To avoid this,
     *             we delay the linear predictor input behind the main signal
     *             input by n samples, where n is sufficiently large to ensure
     *             that the kth sample of the noise is not correlated with the
     *             k+nth sample. Setting n too low results in a noise signal
     *             that lacks bass frequency noise and a tonal signal that
     *             contains some low-frequency noise. Setting delayTime = n
     *             means that the first n samples of every transient will not
     *             be separated at all and will go 100% to the noise output,
     *             leaving nothing in the tonal output.  Since most musical
     *             sounds begin with a noisy attack sound, this is usually
     *             acceptable for small values of n.
     *
     *
     * Suggested Settings
     *
     *   //for sustained musical tones:
     *   BMTNFilter_init(f, 64, 0.25, 150)
     *
     *   //for speech:
     *   BMTNFilter_init(f, 512, 0.2, 64)
     *
     */
    void BMTNFilter_init(BMTNFilter* f, size_t filterOrder, float mu, size_t delayTime);
    
    
    
    /*
     * Use this init when the input is a single note with known fundamental
     * frequency.
     */
    void BMTNFilter_initWithF0(BMTNFilter* f, size_t filterOrder, float minf0, float sampleRate, float mu);
    
    
     
     
    /*
     * Resets the filter coefficients to zero without re-allocating
     * memory.
     */
    void BMTNFilter_reset(BMTNFilter* f);
    
    
    
    /*
     * free memory used by the filter struct
     */
    void BMTNFilter_destroy(BMTNFilter*f);
    
#ifdef __cplusplus
}
#endif

#endif /* BMToneAndNoiseSeparationFilter_h */
