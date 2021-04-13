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
    
    


#endif /* Constants_h */
