//
//  BMCepstrum.c
//  AudioFiltersXcodeProject
//
//  Created by Hans on 11/3/20.
//  Copyright © 2020 BlueMangoo. All rights reserved.
//

#include "BMCepstrum.h"
#include "Constants.h"



void BMCepstrum_init(BMCepstrum *This, size_t maxInputLength){
    BMSpectrum_initWithLength(&This->spectrum1, maxInputLength);
    BMSpectrum_initWithLength(&This->spectrum2, maxInputLength/2);
    This->maxInputLength = maxInputLength;
    This->buffer = malloc(sizeof(float)*maxInputLength);
}



void BMCepstrum_free(BMCepstrum *This){
    BMSpectrum_free(&This->spectrum1);
    BMSpectrum_free(&This->spectrum2);
    free(This->buffer);
    This->buffer = NULL;
}



void BMCepstrum_getCepstrum(BMCepstrum *This,
                            const float *input,
                            float *output,
                            bool logSpectrum,
                            size_t inputLength){
    
    // spectrum[x_] := Abs[Take[Fourier[window[Length[x]]*x], Length[x]/2]]
    bool applyWindow = true;
    BMSpectrum_processDataBasic(&This->spectrum1, input, This->buffer, applyWindow, inputLength);
    
    // take the log if required
    if(logSpectrum){
        int lengthI = (int)inputLength / 2;
        vvlog2f(This->buffer, This->buffer, &lengthI);
    }
    
    // cepstrum[x_] := spectrum[spectrum[x]]
    applyWindow = false;
    BMSpectrum_processDataBasic(&This->spectrum2, This->buffer, output, applyWindow, inputLength / 2);
}
