//
//  BMGetOSVersion.c
//  VelocityFilter
//
//  Created by Hans on 21/3/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//

#include "BMGetOSVersion.h"
#include <stdlib.h>
#include <sys/sysctl.h>
#import <TargetConditionals.h>

#ifdef __cplusplus
extern "C" {
#endif

bool BM_isMacOS(){
    #if TARGET_OS_MAC
        #if !(TARGET_OS_IOS || TARGET_OS_WATCH || TARGET_OS_TV)
            return true;
        #endif
    #endif
    return false;
}

bool BM_isiOS(){
    #if TARGET_OS_IOS
    return true;
    #else
    return false;
    #endif
}

int BM_getOSMajorBuildNumber(){
    int ctlCmd[2];
    // the info we will be requesting from sysctl is in the kernel category
    ctlCmd[0] = CTL_KERN;
    // precisely, we will request the OSRELEASE
    ctlCmd[1] = KERN_OSRELEASE;
    // osRelease is a string
    char* osRelease;
    
    // call sysctl with a null pointer to get the buffer size
    size_t bufferSize;
    sysctl(ctlCmd, 2, NULL, &bufferSize, NULL, 0);
    
    // allocate enough memory for the buffer
    osRelease = malloc(bufferSize*sizeof(char));
    
    // call sysctl again with a pointer to osRelease to get the build number
    sysctl(ctlCmd, 2, osRelease, &bufferSize, NULL, 0);

    
    // terminate the string osRelease at the first '.' by replacing it with
    // the null character. We do this so that only the first part of the
    // string will be converted to an integer in the next step.
    for (size_t i=0; i<bufferSize; i++)
        if(osRelease[i] == '.') osRelease[i]='\0';
    
    // convert the first part of the string to an int
    int majorBuildNumber = atoi(osRelease);
    
    free(osRelease);
    
    return majorBuildNumber;
}

#ifdef __cplusplus
}
#endif
