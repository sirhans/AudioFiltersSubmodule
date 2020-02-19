//
//  BMEarlyReflections.c
//  BMAudioFilters
//
//  Created by Hans on 1/6/17.
//
//  The author releases this file into the public domain with no restrictions
//  of any kind.
//

#ifdef __cplusplus
extern "C" {
#endif

#include "BMEarlyReflections.h"
#include "BMVelvetNoise.h"
#include <stdlib.h>
    
    /*
     * Initialises a stereo early reflections simulation with the first
     * reflection starting after startTimeMS and last reflection ending
     * before endTimeMS.  Between startTimeMS and endTimeMS, each channel
     * has numTapsPerChannel output taps.
     *
     * Tap times are set to produce "Velvet Noise"
     * (See: "Reverberation modeling using velvet noise" by M. Karjalainen
     * and Hanna Järveläinen.
     * https://www.researchgate.net/publication/283247924_Reverberation_modeling_using_velvet_noise
     *
     * @param This         pointer to the struct to initialise
     * @param startTimeMS  start time in milliseconds after the input impulse
     * @param endTimeMS    end time in milliseconds after the impulse
     * @param sampleRate   sample rate of audio system in samples per second
     * @param wetAmount    wet^2 + dry^2 == 1
     * @param numTapsPerChannel  output taps on each channel L and R
     */
    void BMEarlyReflections_init(BMEarlyReflections *This,
                                 float startTimeMS,
                                 float endTimeMS,
                                 float sampleRate,
                                 float wetAmount,
                                 size_t numTapsPerChannel){
        
        BMEarlyReflectionsSetting* setting = &This->setting;
        
        
        // this is strictly stereo for now
        bool stereo = true;
        setting->numberOfChannel = stereo? 2:1;
        setting->numberTaps = numTapsPerChannel;
        setting->sampleRate = sampleRate;
        setting->startTimeMS = startTimeMS;
        setting->endTimeMS = endTimeMS;
        
        
        // allocate memory for indices, gains and temp specification
        setting->indices = malloc(sizeof(size_t*) * setting->numberOfChannel);
        setting->gain = malloc(sizeof(float*) * setting->numberOfChannel);
        
        This->gain = malloc(sizeof(float*) * setting->numberOfChannel);
        
        for (int i=0; i<setting->numberOfChannel; i++) {
            //allocate memory for each channel
            //we + 1 to the numTap because we want to use the slot 0 for the dry signal
            setting->indices[i] = malloc(sizeof(size_t) * (setting->numberTaps + 1));
            setting->gain[i] = malloc(sizeof(float) * (setting->numberTaps + 1));
            
            This->gain[i] = malloc(sizeof(float) * (setting->numberTaps + 1));
        }
        
        // seed the random number generator for consistent results
        srand(0);
        BMEarlyReflection_RegenIndicesAndGain(This);
        
        //last element = max index
		size_t maxIndex = (size_t)((sampleRate * endTimeMS)/1000.0);
        setting->indices[0][setting->numberTaps] = maxIndex;
        
        // initialise the multi-tap delay
        BMMultiTapDelay_Init(&This->delay, stereo,
                             setting->indices[0], setting->indices[1],
							 maxIndex,
                             setting->gain[0], setting->gain[1],
                             numTapsPerChannel + 1,
							 numTapsPerChannel + 1);
        
        // set wet and dry amounts
        BMEarlyReflections_setWetAmount(This,wetAmount);
    }
    
    void BMEarlyReflection_RegenIndicesAndGain(BMEarlyReflections *This){
        BMEarlyReflectionsSetting* setting = &This->setting;

        float gainScale = 1.0 / sqrt((float)setting->numberTaps);
        
        for (int i=0; i<setting->numberOfChannel; i++) {
            
            // use velvet noise to set output tap times
            //we + 1 because we want to start from index 1, NOT 0
            BMVelvetNoise_setTapIndices(setting->startTimeMS, setting->endTimeMS,
                                          setting->indices[i] + 1,
                                          setting->sampleRate, setting->numberTaps);
            setting->indices[i][0] = 0;
            
            // set random tap gains with equal number of + and -
            BMVelvetNoise_setTapSigns(setting->gain[i] + 1, setting->numberTaps);
            
            // scale down the gain of output taps to make total
            // energy input equal to energy output
            vDSP_vsmul(setting->gain[i] + 1, 1, &gainScale, setting->gain[i] + 1, 1, setting->numberTaps);
            setting->gain[i][0] = 0;
        }
    }
    
    
    /*
     * free memory used by the struct at *This
     */
    void BMEarlyReflections_destroy(BMEarlyReflections *This){
        BMMultiTapDelay_free(&This->delay);
        
        BMEarlyReflectionsSetting* setting = &This->setting;
        
        for (int i=0; i<setting->numberOfChannel ; i++) {
            free(setting->indices[i]);
            free(setting->gain[i]);
            setting->indices[i] = NULL;
            setting->gain[i] = NULL;
            
            free(This->gain[i]);
            This->gain = NULL;
        }
    }
    
    
    /*
      *This can be done faster by calling the multi-tap delay process
     * function directly.
     */
    void BMEarlyReflections_processBuffer(BMEarlyReflections *This,
                                          const float* inputL, const float* inputR,
                                          float* outputL, float* outputR,
                                          size_t numSamples){
        

            BMMultiTapDelay_processBufferStereo(&This->delay,
                                                inputL, inputR,
                                                outputL, outputR,
                                                numSamples);
    }
    
    
    /*
     * set the portion of wet signal to mix with output. This also
     * sets dry gain so that wet^2 + dry^2 == 1.0
     *
     * @param wetAmount  gain of wet signal; must be in [0.0,1.0]
     */
    void BMEarlyReflections_setWetAmount(BMEarlyReflections *This,
                                         float wetAmount){
        assert(wetAmount <= 1.0f);
        assert(wetAmount >= 0.0f);
        BMEarlyReflectionsSetting* setting = &This->setting;
        
        setting->wet = wetAmount;
        setting->dry = sqrt(1.0f - wetAmount*wetAmount);
        
        //apply
        for (int i=0; i<setting->numberOfChannel; i++) {
            setting->indices[i][0] = 0;
            This->gain[i][0] = setting->dry;
            
            for (int j=1; j<=setting->numberTaps; j++) {
                This->gain[i][j] = setting->gain[i][j] * wetAmount;
            }
        }
        
        //apply to multitap delay
        BMMultiTapDelay_setDelayTimes(&This->delay, setting->indices[0], setting->indices[1]);
        BMMultiTapDelay_setGains(&This->delay, This->gain[0], This->gain[1]);
    }
    
    
    void BMEarlyReflections_impulseResponse(BMEarlyReflections *This){
        //BMSimpleDelay_impulseResponse(&This->delay);
    }
    
    
#ifdef __cplusplus
}
#endif
