//
//  BMGetOSVersion.h
//  VelocityFilter
//
//  Created by Hans on 21/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#ifndef BMGetOSVersion_h
#define BMGetOSVersion_h

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * To translate from build numbers to commercialized version numbers, see
 * the following references
 *
 * see https://en.wikipedia.org/wiki/IOS_version_history (iOS)
 *
 * and https://support.apple.com/en-sg/HT201260 (OS X)
 *
 */
int BM_getOSMajorBuildNumber(void);

// returns true if running on OS X Mac
bool BM_isMacOS(void);

// returns true if running on iOS
bool BM_isiOS(void);

#ifdef __cplusplus
}
#endif

#endif /* BMGetOSVersion_h */
