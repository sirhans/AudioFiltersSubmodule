//
//  BMCrossover.c
//  BMAudioFilters
//
//  Created by Hans on 17/2/17.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifdef __cplusplus
extern "C" {
#endif
    
#include <Accelerate/Accelerate.h>
#include "BMCrossover.h"
#include <assert.h>
    
    /*!
     *BMCrossover_init
     * @abstract This function must be called prior to use
     *
     * @param cutoff      - cutoff frequency in hz
     * @param sampleRate  - sample rate in hz
     * @param fourthOrder - true: 4th order, false: 2nd order
     * @param stereo      - true: stereo, false: mono
     */
    void BMCrossover_init(BMCrossover* This,
                          float cutoff,
                          float sampleRate,
                          bool fourthOrder,
                          bool stereo){
        
        This->fourthOrder = fourthOrder;
        This->stereo      = stereo;
        
        
        // we need one biquad section for 2nd order; two for 4th
        size_t numLevels;
        numLevels = fourthOrder ? 2:1;
        
        
        // initialise the filters
        BMMultiLevelBiquad_init(&This->lp,
                                numLevels,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->hp,
                                numLevels,
                                sampleRate,
                                stereo,
                                false,false);
        
        
        BMCrossover_setCutoff(This, cutoff);
    }
    
    
    
    
    
    
    /*
     * Free memory used by the filters
     */
    void BMCrossover_free(BMCrossover* This){
        BMMultiLevelBiquad_destroy(&This->lp);
        BMMultiLevelBiquad_destroy(&This->hp);
    }
    
    
    
    
    
    
    /*
     * @param cutoff - cutoff frequency in Hz
     */
    void BMCrossover_setCutoff(BMCrossover* This, float cutoff){
        
        if(This->fourthOrder){
            // the fourth order crossover uses pairs of
            // 2nd order butterworth filters
            //
            // lowpass 4th order
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->lp,
                                                          cutoff,
                                                          0);
            // highpass fourth order
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->hp,
                                                          cutoff,
                                                          0);
        } else {
            // the second order crossover uses 2nd order linkwitz-riley
            // filters. The transfer function of these filters is
            // equivalent to a cascade of first order butterworth
            // filters. However, the vDSP library only has machine
            // optimised code for biquads so don't want to implement
            // this as a cascade of first-order filters
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->lp,
                                                  cutoff,
                                                  0);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->hp,
                                                  cutoff,
                                                  0);
        }
    }
    
    
    
    /*
     * Apply the crossover to stereo audio
     *
     * @param inL        - array of left channel audio input
     * @param inR        - array of right channel audio input
     * @param lowpassL   - lowpass output left
     * @param lowpassR   - lowpass output right
     * @param highpassL  - highpass output left
     * @param highpassR  - highpass output right
     * @param numSamples - number of samples to process. all arrays must have at least this length
     */
    void BMCrossover_processStereo(BMCrossover* This,
                                   const float* inL, const float* inR,
                                   float* lowpassL, float* lowpassR,
                                   float* highpassL, float* highpassR,
                                   size_t numSamples){
        assert(This->stereo);
        
        BMMultiLevelBiquad_processBufferStereo(&This->lp,
                                               inL, inR,
                                               lowpassL, lowpassR,
                                               numSamples);
        BMMultiLevelBiquad_processBufferStereo(&This->hp,
                                               inL, inR,
                                               highpassL, highpassR,
                                               numSamples);
    }
    
    
    
    
    
    
    
    /*
     * Apply the crossover to mono audio
     *
     * @param input      - array of audio input
     * @param lowpass    - lowpass output
     * @param highpass   - highpass output
     * @param numSamples - number of samples to process. all arrays must have at least this length
     */
    void BMCrossover_processMono(BMCrossover* This,
                                 float* input,
                                 float* lowpass,
                                 float* highpass,
                                 size_t numSamples){
        assert(!This->stereo);
        
        
        BMMultiLevelBiquad_processBufferMono(&This->lp,
                                             input,
                                             lowpass,
                                             numSamples);
        BMMultiLevelBiquad_processBufferMono(&This->hp,
                                             input,
                                             highpass,
                                             numSamples);
        
        
        // if the filter is second order, one of the two outputs must be
        // inverted so that the sum of the two have unity gain transfer
        // function when we sum the lowpass and highpass outputs back
        // together.
        if(!This->fourthOrder){
            // negate the highpass outputs
            vDSP_vneg(highpass, 1, highpass, 1, numSamples);
        }
    }
    
    
    
    
    
    /*!
     *BMCrossover_tfMagVectors
     *
     * @abstract get points to plot a graph of each of the two bands
     *
     * @param This        pointer to an initialized struct
     * @param frequencies input vector containing the x-axis coordinates of the plot
     * @param magLow      output vector containing the y-coordinates for low band
     * @param magHigh     output vector containing the y-coordinates for high band
     * @param length      length of the input and output vectors
     */
    void BMCrossover_tfMagVectors(BMCrossover *This,
                                      const float* frequencies,
                                      float* magLow,
                                      float* magHigh,
                                  size_t length){
        BMMultiLevelBiquad_tfMagVector(&This->lp, frequencies, magLow, length);
        BMMultiLevelBiquad_tfMagVector(&This->hp, frequencies, magHigh, length);
    }
    
    
    
    
    
    
    /*!
     *BMCrossover3way_init
     * @abstract This function must be called prior to use
     *
     * @param cutoff1     - cutoff frequency between bands 1 and 2 in hz
     * @param cutoff2     - cutoff frequency between bands 2 and 3 in hz
     * @param sampleRate  - sample rate in hz
     * @param fourthOrder - true: 4th order, false: 2nd order
     * @param stereo      - true: stereo, false: mono
     */
    void BMCrossover3way_init(BMCrossover3way* This,
                              float cutoff1,
                              float cutoff2,
                              float sampleRate,
                              bool fourthOrder,
                              bool stereo){
        
        This->fourthOrder = fourthOrder;
        This->stereo      = stereo;
        
        
        // we need one biquad section for 2nd order; two for 4th
        size_t levelsPerFilter = fourthOrder ? 2:1;
        
        
        // initialise the filters
        BMMultiLevelBiquad_init(&This->low,
                                2*levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->midAndHigh,
                                levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->mid,
                                levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->high,
                                levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        
        // init filters for plotting
        BMMultiLevelBiquad_init(&This->plotFilters[0],
                                levelsPerFilter,
                                sampleRate,
                                false, false, false);
        BMMultiLevelBiquad_init(&This->plotFilters[1],
                                2*levelsPerFilter,
                                sampleRate,
                                false, false, false);
        BMMultiLevelBiquad_init(&This->plotFilters[2],
                                levelsPerFilter,
                                sampleRate,
                                false, false, false);
        
        
        BMCrossover3way_setCutoff1(This, cutoff1);
        BMCrossover3way_setCutoff2(This, cutoff2);
    }
    
    
    
    /*!
     *BMCrossover3way_free
     */
    void BMCrossover3way_free(BMCrossover3way* This){
        // audio filters
        BMMultiLevelBiquad_destroy(&This->low);
        BMMultiLevelBiquad_destroy(&This->midAndHigh);
        BMMultiLevelBiquad_destroy(&This->mid);
        BMMultiLevelBiquad_destroy(&This->high);
        
        // plot filters
        for(size_t i=0; i<3; i++)
            BMMultiLevelBiquad_destroy(&This->plotFilters[i]);
    }
    
    
    
    
    void BMCrossover3way_setCutoff1(BMCrossover3way *This, float fc){
        if(This->fourthOrder){
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->low, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->midAndHigh, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->plotFilters[0], fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->plotFilters[1], fc, 0);
        }
        else {
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->low, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->midAndHigh, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->plotFilters[0], fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->plotFilters[1], fc, 0);
        }
    }
    
    
	
	
    void BMCrossover3way_setCutoff2(BMCrossover3way *This, float fc){
        
        // here we set the lowpass filter on both the mid and the low
        // frequencies so that the lowpass filter will have the same phase shift
        // as the mid and high frequencies when we add it all back together
        if(This->fourthOrder){
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->mid, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->low, fc, 2);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->high, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->plotFilters[1], fc, 2);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->plotFilters[2], fc, 0);
        }
        else {
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->mid, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->low, fc, 1);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->high, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->plotFilters[1], fc, 1);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->plotFilters[2], fc, 0);
        }
    }
    
    
    void BMCrossover3way_processStereo(BMCrossover3way *This,
                                       const float* inL, const float* inR,
                                       float* lowL, float* lowR,
                                       float* midL, float* midR,
                                       float* highL, float* highR,
                                       size_t numSamples){
        assert(This->stereo);
        
        // split the low part of the signal off, processing it through
        // both crossovers to preserve phase
        BMMultiLevelBiquad_processBufferStereo(&This->low,
                                               inL, inR,
                                               lowL, lowR,
                                               numSamples);
        
        // split the mid and high, buffer to mid
        BMMultiLevelBiquad_processBufferStereo(&This->midAndHigh,
                                               inL, inR,
                                               midL, midR,
                                               numSamples);
        
        // split the high from the mid
        BMMultiLevelBiquad_processBufferStereo(&This->high,
                                               midL, midR,
                                               highL, highR,
                                               numSamples);
        
        // remove the high from the mid
        BMMultiLevelBiquad_processBufferStereo(&This->mid,
                                               midL, midR,
                                               midL, midR,
                                               numSamples);
    }
    
    
    
    
    
    /*!
     *BMCrossover3way_tfMagVectors
     *
     * @abstract get points to plot a graph of each of the four bands
     *
     * @param This        pointer to an initialized struct
     * @param frequencies input vector containing the x-axis coordinates of the plot
     * @param magLow      output vector containing the y-coordinates for low band
     * @param magMid      output vector containing the y-coordinates for mid band
     * @param magHigh     output vector containing the y-coordinates for high band
     * @param length      length of the input and output vectors
     */
    void BMCrossover3way_tfMagVectors(BMCrossover3way *This,
                                      const float* frequencies,
                                      float* magLow,
                                      float* magMid,
                                      float* magHigh,
                                      size_t length){
        BMMultiLevelBiquad_tfMagVector(&This->plotFilters[0], frequencies, magLow, length);
        BMMultiLevelBiquad_tfMagVector(&This->plotFilters[1], frequencies, magMid, length);
        BMMultiLevelBiquad_tfMagVector(&This->plotFilters[2], frequencies, magHigh, length);
    }
    
    
    
    
    
    /*!
     *BMCrossover4way_init
     * @abstract This function must be called prior to use
     *
     * @param cutoff1     - cutoff frequency between bands 1 and 2 in hz
     * @param cutoff2     - cutoff frequency between bands 2 and 3 in hz
     * @param cutoff3     - cutoff frequency between bands 2 and 3 in hz
     * @param sampleRate  - sample rate in hz
     * @param fourthOrder - true: 4th order, false: 2nd order
     * @param stereo      - true: stereo, false: mono
     */
    void BMCrossover4way_init(BMCrossover4way* This,
                              float cutoff1,
                              float cutoff2,
                              float cutoff3,
                              float sampleRate,
                              bool fourthOrder,
                              bool stereo){
        
        This->fourthOrder = fourthOrder;
        This->stereo      = stereo;
        
        
        // we need one biquad section for 2nd order; two for 4th
        size_t levelsPerFilter = fourthOrder ? 2:1;
        
        
        // initialise the filters
        BMMultiLevelBiquad_init(&This->band1,
                                3*levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->bands2to4,
                                levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->band2,
                                2*levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->bands3to4,
                                levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->band3,
                                levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        BMMultiLevelBiquad_init(&This->band4,
                                levelsPerFilter,
                                sampleRate,
                                stereo,
                                false,false);
        
        // init filters for plotting
        BMMultiLevelBiquad_init(&This->plotFilters[0],
                                levelsPerFilter,
                                sampleRate,
                                false, false, false);
        BMMultiLevelBiquad_init(&This->plotFilters[1],
                                2*levelsPerFilter,
                                sampleRate,
                                false, false, false);
        BMMultiLevelBiquad_init(&This->plotFilters[2],
                                2*levelsPerFilter,
                                sampleRate,
                                false, false, false);
        BMMultiLevelBiquad_init(&This->plotFilters[3],
                                levelsPerFilter,
                                sampleRate,
                                false, false, false);
        
        
        BMCrossover4way_setCutoff1(This, cutoff1);
        BMCrossover4way_setCutoff2(This, cutoff2);
        BMCrossover4way_setCutoff3(This, cutoff3);
    }
    
    
    /*!
     *BMCrossover4way_free
     */
    void BMCrossover4way_free(BMCrossover4way* This){
        BMMultiLevelBiquad_destroy(&This->band1);
        BMMultiLevelBiquad_destroy(&This->band2);
        BMMultiLevelBiquad_destroy(&This->band3);
        BMMultiLevelBiquad_destroy(&This->band4);
        BMMultiLevelBiquad_destroy(&This->bands2to4);
        BMMultiLevelBiquad_destroy(&This->bands3to4);
        
        for(size_t i=0; i<4; i++)
            BMMultiLevelBiquad_destroy(&This->plotFilters[i]);
    }
    
    
    void BMCrossover4way_setCutoff1(BMCrossover4way *This, float fc){
        if(This->fourthOrder){
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->band1, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->bands2to4, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->plotFilters[0], fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->plotFilters[1], fc, 0);
        }
        else {
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->band1, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->bands2to4, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->plotFilters[0], fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->plotFilters[1], fc, 0);
        }
    }
    
    
    
    void BMCrossover4way_setCutoff2(BMCrossover4way *This, float fc){
        
        // here we set the lowpass filter on both the mid and the low
        // frequencies so that the lowpass filter will have the same phase shift
        // as the mid and high frequencies when we add it all back together
        if(This->fourthOrder){
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->band2, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->band1, fc, 2);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->bands3to4, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->plotFilters[1], fc, 2);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->plotFilters[2], fc, 0);
        }
        else {
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->band2, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->band1, fc, 1);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->bands3to4, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->plotFilters[1], fc, 1);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->plotFilters[2], fc, 0);
        }
    }
    
    
    
    void BMCrossover4way_setCutoff3(BMCrossover4way *This, float fc){
        
        // here we set the lowpass filter on both the mid and the low
        // frequencies so that the lowpass filter will have the same phase shift
        // as the mid and high frequencies when we add it all back together
        if(This->fourthOrder){
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->band3, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->band2, fc, 2);
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->band1, fc, 4);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->band4, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP4thOrder(&This->plotFilters[2], fc, 2);
            BMMultiLevelBiquad_setLinkwitzRileyHP4thOrder(&This->plotFilters[3], fc, 0);
        }
        else {
            // for audio
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->band3, fc, 0);
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->band2, fc, 1);
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->band1, fc, 2);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->band4, fc, 0);
            
            // for plotting
            BMMultiLevelBiquad_setLinkwitzRileyLP(&This->plotFilters[2], fc, 1);
            BMMultiLevelBiquad_setLinkwitzRileyHP(&This->plotFilters[3], fc, 0);
        }
    }
    
    
    
    
    
    void BMCrossover4way_processStereo(BMCrossover4way *This,
                                       const float* inL, const float* inR,
                                       float* band1L, float* band1R,
                                       float* band2L, float* band2R,
                                       float* band3L, float* band3R,
                                       float* band4L, float* band4R,
                                       size_t numSamples){
        assert(This->stereo);
        
        // split the low part of the signal off, processing it through
        // all three crossovers to preserve phase
        BMMultiLevelBiquad_processBufferStereo(&This->band1,
                                               inL, inR,
                                               band1L, band1R,
                                               numSamples);
        
        // split bands 2-4, buffering into 2
        BMMultiLevelBiquad_processBufferStereo(&This->bands2to4,
                                               inL, inR,
                                               band2L, band2R,
                                               numSamples);
        
        // split bands 3-4 off from band 2
        BMMultiLevelBiquad_processBufferStereo(&This->bands3to4,
                                               band2L, band2R,
                                               band3L, band3R,
                                               numSamples);
        
        // remove bands 3-4 from band 2
        BMMultiLevelBiquad_processBufferStereo(&This->band2,
                                               band2L, band2L,
                                               band2L, band2L,
                                               numSamples);
        
        // split band 4 off from band 3
        BMMultiLevelBiquad_processBufferStereo(&This->band4,
                                               band3L, band3R,
                                               band4L, band4R,
                                               numSamples);
        
        // remove band 4 from band 3
        BMMultiLevelBiquad_processBufferStereo(&This->band3,
                                               band3L, band3L,
                                               band3L, band3L,
                                               numSamples);
    }
	
	
	
	
	

	/*!
	 *BMCrossover3way_recombine
	 */
	void BMCrossover3way_recombine(const float* bassL, const float* bassR,
								   const float* midL, const float* midR,
								   const float* trebleL, const float* trebleR,
								   float* outL, float* outR,
								   size_t numSamples){
		vDSP_vadd(bassL, 1, midL, 1, outL, 1, numSamples);
		vDSP_vadd(bassR, 1, midR, 1, outR, 1, numSamples);
		vDSP_vadd(trebleL,1,outL,1,outL,1,numSamples);
		vDSP_vadd(trebleR,1,outR,1,outR,1,numSamples);
	}
	
	
	
	

	/*!
	 *BMCrossover4way_recombine
	 */
	void BMCrossover4way_recombine(const float* band1L, const float* band1R,
								   const float* band2L, const float* band2R,
								   const float* band3L, const float* band3R,
								   const float* band4L, const float* band4R,
								   float* outL, 		float* outR,
								   size_t numSamples){
		
		vDSP_vadd(band1L,1,band2L,1,outL,1,numSamples);
		vDSP_vadd(band3L,1,outL,1,outL,1,numSamples);
		vDSP_vadd(band4L,1,outL,1,outL,1,numSamples);

	}
    
    
    
    
    

    //void BMMultiLevelBiquad_tfMagVector(BMMultiLevelBiquad* bqf, const float *frequency, float *magnitude, size_t length);
    /*!
     *BMCrossover4way_tfMagVectors
     *
     * @abstract get points to plot a graph of each of the four bands
     *
     * @param This        pointer to an initialized struct
     * @param frequencies input vector containing the x-axis coordinates of the plot
     * @param magBand1    output vector containing the y-coordinates for band 1
     * @param magBand2    output vector containing the y-coordinates for band 2
     * @param magBand3    output vector containing the y-coordinates for band 3
     * @param magBand4    output vector containing the y-coordinates for band 4
     * @param length      length of the input and output vectors
     */
    void BMCrossover4way_tfMagVectors(BMCrossover4way *This,
                                      const float* frequencies,
                                      float* magBand1,
                                      float* magBand2,
                                      float* magBand3,
                                      float* magBand4,
                                      size_t length){
        BMMultiLevelBiquad_tfMagVector(&This->plotFilters[0], frequencies, magBand1, length);
        BMMultiLevelBiquad_tfMagVector(&This->plotFilters[1], frequencies, magBand2, length);
        BMMultiLevelBiquad_tfMagVector(&This->plotFilters[2], frequencies, magBand3, length);
        BMMultiLevelBiquad_tfMagVector(&This->plotFilters[3], frequencies, magBand4, length);
    }

    
    

	
	/*!
	 *BMCrossover_impulseResponse
	 */
	void BMCrossover_impulseResponse(BMCrossover *This, float* IRL, float* IRR, size_t numSamples);

	/*!
	 *BMCrossover3way_impulseResponse
	 */
	void BMCrossover3way_impulseResponse(BMCrossover3way *This, float* IRL, float* IRR, size_t numSamples);

	/*!
	 *BMCrossover4way_impulseResponse
	 */
	void BMCrossover4way_impulseResponse(BMCrossover4way *This, float* IRL, float* IRR, size_t numSamples);

	
    
	
	
#ifdef __cplusplus
}
#endif
