//
//  Constants.h
//  VelocityFilter
//
//  Created by Hans on 14/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef Constants_h
#define Constants_h

#define BM_DB_TO_GAIN(db) pow(10.0,db/20.0)
#define BM_GAIN_TO_DB(gain) log10f(gain)*20.0
#define BM_BUFFER_CHUNK_SIZE 512

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef BM_MIN
#define BM_MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef BM_MAX
#define BM_MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

#ifndef is_aligned
#define is_aligned(POINTER, BYTE_COUNT) (((uintptr_t)(const void *)(POINTER)) % (BYTE_COUNT) == 0)
#endif
    
///*
// * This macro handles chunked processing of an array for a process function
// * that has limited length buffer memory.
// *
// * Example:
// *   We want to process a function BMDelay_process(BMDelay *This, const float* input, float *output, size_t length). We can do it as follows
// *
// *   BM_CHUNK_PROCESS_MONO(BMDelay_process, This, input, output, BM_BUFFER_CHUNK_SIZE, length);
// */
//#ifndef BM_CHUNK_PROCESS_MONO
//#define BM_CHUNK_PROCESS_MONO(f,s,i,o,bcs,len) ({ \
//		__typeof__ (f) _f = (f); \
//		__typeof__ (s) _s = (s); \
//		__typeof__ (i) _i = (i); \
//		__typeof__ (o) _o = (o); \
//		__typeof__ (len) _len = (len); \
//		__typeof__ (bcs) _bcs = (bcs); \
//		size_t rem = _len; \
//		size_t j = 0; \
//		while(rem > 0) { \
//			size_t proc = BM_MIN(rem,bcs); \
//			f(s,i+j,o+j,proc); \
//			j+=proc; \
//			rem-=proc; \
//		} \
//})
//#endif

static void BMChunkProcessMono(void (*procFunction)(void*,const float*, float*, size_t),
							   void *This,
							   const float *input,
							   float *output,
							   size_t length,
							   size_t maxChunkSize){
	size_t samplesRemaining = length;
	size_t i=0;
	while(samplesRemaining > 0){
		size_t samplesProcessing = BM_MIN(maxChunkSize,samplesRemaining);
		
		procFunction(This, input, output, samplesProcessing);
		
		i+= samplesProcessing;
		samplesRemaining -= samplesProcessing;
	}
}


#endif /* Constants_h */
