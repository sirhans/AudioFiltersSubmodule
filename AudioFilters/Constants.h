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

#endif /* Constants_h */
