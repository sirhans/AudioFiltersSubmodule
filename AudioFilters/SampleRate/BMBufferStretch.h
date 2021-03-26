//
//  BMBufferStretch.h
//  AudioFiltersXcodeProject
//
//  We use this to solve cases where there is a tiny sample rate mismatch between
//  the audio source and the audio player. For example, when source and destination
//  are both running at 44100 KHz but the clock speeds mismatch by 0.1%
//
//  It works by using simple quadratic interpolation to resize the buffer. This
//  results in a loss of audio quality. Therefore we recommend you implement in
//  such a way that you call the process function with inputLength = outputLength
//  as often as possible and only stretch the buffer when it's absolutely necessary to do so.
//
//  Created by Hans on 19/3/21.
//  We hereby release this file into the public domain without restrictions
//

#ifndef BMBufferStretch_h
#define BMBufferStretch_h

#include <stdio.h>

#endif /* BMBufferStretch_h */
