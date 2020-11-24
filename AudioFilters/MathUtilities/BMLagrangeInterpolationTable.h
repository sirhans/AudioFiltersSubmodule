//
//  BMLagrangeInterpolationTable.h
//  BMAudioFilters
//
//  Created by Hans on 9/9/16.
//
//  This file may be used, distributed and modified freely by anyone,
//  for any purpose, without restrictions.
//


#ifdef __cplusplus
extern "C" {
#endif
    

#ifndef BMLagrangeInterpolationTable_h
#define BMLagrangeInterpolationTable_h

#include <stdio.h>
    
    typedef struct BMLagrangeInterpolationTable {
        float** table;
        size_t lengthI, width, dCentreI;
        float dMin, dCentreF, dMax, lengthF;
    }BMLagrangeInterpolationTable;
    
    
    void BMLagrangeInterpolationTable_init(BMLagrangeInterpolationTable *This,
                                           size_t interpolationOrder,
                                           size_t length);
    
    
    void BMLagrangeInterpolationTable_destroy(BMLagrangeInterpolationTable *This);
    
    
    
    size_t BMLagrangInterpolationTable_getIndex(BMLagrangeInterpolationTable *This, float delta);

#endif /* BMLagrangeInterpolationTable_h */




#ifdef __cplusplus
}
#endif
