//
//  BMRoundRobin.c
//  ETSax
//
//  Created by Duc Anh on 6/22/17.
//  Anyone may use this file without restrictions
//

#include "BMRoundRobin.h"
#include "BMVelvetNoise.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void BMRoundRobin_init(BMRoundRobin* This,
                       float sampleRate,
                       float stableFrequencyWish,
                       size_t numTapsPerChannel){
    
    BMRoundRobinSetting* setting = &This->setting;
    
    bool stereo = false;
    setting->numberOfChannel = stereo? 2:1;
    setting->numberTaps = numTapsPerChannel;
    setting->sampleRate = sampleRate;
    setting->stableFrequencyWish = stableFrequencyWish;
    setting->bufferLengthInFrames = sampleRate/stableFrequencyWish;
    setting->bufferLengthInMs = 1000.0 * (setting->bufferLengthInFrames / sampleRate);
    
    // allocate memory for indices, gains and temp specification
    setting->indices = malloc(sizeof(size_t) * (setting->numberTaps + 1));
    setting->gain = malloc(sizeof(float) * (setting->numberTaps + 1));

    BMRoundRobin_RegenIndicesAndGain(This);
    setting->indices[setting->numberTaps] = setting->bufferLengthInFrames;
    
    // initialise the multi-tap delay
    BMMultiTapDelay_Init(&This->delay, stereo,
                         setting->indices, setting->indices,
                         setting->gain, setting->gain,
                         numTapsPerChannel + 1,numTapsPerChannel + 1, sampleRate);

}


/*
 * This function sets indices and gain so that the multi-tap delay at the
 * heart of this class has an impulse response with the following 
 * characteristics:
 *
 * 1. There is an output tap at index 0, representing the dry signal
 * 2. There are numberTaps additional delayed output taps, with timings
 *    randomised between 0 and the maximum delay time, using the velvet
 *    noise algorithm for randomisation.
 * 3. The gains of the output taps are of the form {a, -b, -b,..., -b, b}
 *    where a is the dry gain, and a and b have the same sign.
 * 4. the values a and b are calculated to ensure that gain at the DC
 *    frequency (zero frequency) is unitary and the average gain over the
 *    entire frequency range is also unitary, although the gain at each
 *    specific nonzero frequency is random.
 * 5. the deviation from unity near zero is small, so that the
 *    randomisation of the spectrum occurs mainly above a specified 
 *    threshold frequency.
 */
void BMRoundRobin_RegenIndicesAndGain(BMRoundRobin* This){
    BMRoundRobinSetting* setting = &This->setting;
    
    //use velvet noise to set output tap times
    //we + 1 because we want to start from index 1, NOT 0
    float startTime = 0.0f;
    float endTime = setting->bufferLengthInMs;
    BMVelvetNoise_setTapIndices(startTime, endTime,
								setting->indices + 1,
								setting->sampleRate,
								setting->numberTaps);

    float n = (float)setting->numberTaps;
    float m = 1.0; // number of delay taps with unchanged sign
    
    /*
     * The formulae for dry and output tap gains emerge when we 
     * solve for x=dryGain and y=delayTapGain under the following 
     * two constraints:
     *
     * 1. filter gain at DC is unity
     * 2. total broad spectrum gain is unity
     *
     * dryGain = (-4 m^2 + n + 4 m n - n^2)/(4 m^2 + n - 4 m n + n^2)
     * delayTapGain = (2 (2 m - n))/(4 m^2 + n - 4 m n + n^2)
     *
     *
     * For the record, the system of equations we solved to find those 
     * formulae is as follows:
     *
     *  Solve[{x + y m - y (n - m) == 1, Sqrt[x^2 + n y^2] == 1}, {x, y}]
     *
     * ...where x is the dryGain, y is the delayTap gain, n is the total
     * number of delay taps, excluding the dry signal tap, and m is the
     * total number of delay taps for which the sign of delayTapGain
     * is NOT inverted.
     *
     * The purpose of setting signs this way is to have the first tap
     * (dry signal) and last delay tap be the same sign, but all the 
     * taps in between have opposite sign. The result of this is an 
     * impulse response that correlates in a predictable way with the
     * first cosine term of the Fourier series.
     *
     * (recall that in the first cosine term of the Fourier series, 
     * the ends (f=0 and f=2*PI) have opposite sign from
     * the middle region (near f=PI)). 
     *
     * Having predictable correlation with the first term of the Fourier
     * series means that we expect less variation from the filter in
     * the frequency bin just above 0. This particular system, we know
     * from trial and error, gives the first term of the Fourier series 
     * very near unity. That helps us to use this filter to add variation
     * to the upper part of the spectrum while leaving the lower part
     * unaffected. To control the frequency range where it is near unity
     * gain change, we simply shorten or lengthen the filter kernel.
     */
    float drySignalGain = (-4.*m*m + n + 4.*m*n - n*n)/(4.*m*m + n - 4.*m*n + n*n);
    float wetTapGain = (2 * (2*m - n))/(4*m*m + n - 4*m*n + n*n);
    
    setting->indices[0] = 0;
    setting->gain[0] = drySignalGain;
    for (int i=1; i<setting->numberTaps; i++) {
        setting->gain[i] = -wetTapGain;
    }
    setting->gain[setting->numberTaps] = wetTapGain;
}



void BMRoundRobin_NewNote(BMRoundRobin* This){
    //reset indices and gain;
    //set to multitap delay
    BMRoundRobinSetting* setting = &This->setting;
    
    BMMultiTapDelay_clearBuffers(&This->delay);
    BMRoundRobin_RegenIndicesAndGain(This);
    BMMultiTapDelay_setGains(&This->delay, setting->gain, setting->gain);
    BMMultiTapDelay_setDelayTimes(&This->delay, setting->indices, setting->indices);
}





/*
 * free memory used by the struct at *This
 */
void BMRoundRobin_destroy(BMRoundRobin* This){
    BMMultiTapDelay_free(&This->delay);
    free(This->setting.indices);
    This->setting.indices = NULL;
    free(This->setting.gain);
    This->setting.gain = NULL;
}






/*
 * Heads up: we could save a function call by calling the process buffer
 * function of multiTapDelay directly
 */
void BMRoundRobin_processBufferStereo(BMRoundRobin* This,
                                      float* inputL, float* inputR,
                                      float* outputL, float* outputR,
                                      size_t numSamples){

        BMMultiTapDelay_ProcessBufferStereo(&This->delay,
                                            inputL, inputR,
                                            outputL, outputR,
                                            numSamples);
}

void BMRoundRobin_processBufferMono(BMRoundRobin* This,
                                    float* input, float* output,
                                    size_t numSamples){
    BMMultiTapDelay_ProcessBufferMono(&This->delay,
                                        input, output,
                                        numSamples);
}

void BMRoundRobin_testImpulseResponse(BMRoundRobin* This){
    BMMultiTapDelay_impulseResponse(&This->delay);
}


#ifdef __cplusplus
}
#endif

