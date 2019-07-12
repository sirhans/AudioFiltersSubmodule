//
//  BMDownsampler.h
//  BMAudioFilters
//
//  Created by Hans on 13/9/17.
//  Anyone may use this file without restrictions
//


#ifdef __cplusplus
extern "C" {
#endif

#ifndef BMDownsampler_h
#define BMDownsampler_h
    
#include <MacTypes.h>

    typedef struct BMDownsampler {

        size_t IRLength;
    } BMDownsampler;
    
    
    
    
    /*
     * @param decimationFactor   set this to N to keep 1 in N samples
     */
    void BMDownsampler_init(BMDownsampler* This,
                            size_t decimationFactor);
    
    
    
    /*
     * length of input >= numSamplesOut*This->decimationFactor
     * length of output >= numSamplesOut
     */
    void BMDownsampler_process(BMDownsampler* This,
                               float* input,
                               float* output,
                               size_t numSamplesOut);
    

    void BMDownsampler_free(BMDownsampler* This);
    
    
    
    /*
     * Calling function must ensure that the length of IR is at least
     * This->IRLength
     */
    void BMDownsampler_impulseResponse(BMDownsampler* This, float* IR);
        

    
    
#endif /* BMDownsampler_h */

#ifdef __cplusplus
}
#endif
