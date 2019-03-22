//
//  BMLagrangeInterpolationTable.c
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

#include <stdlib.h>
#include <assert.h>
#include "BMLagrangeInterpolationTable.h"


    /*!
     * computes the N+1 lagrange interpolation coefficients for order N 
     * interpolation at delta.
     *
     * ref: https://ccrma.stanford.edu/~jos/pasp/Explicit_Lagrange_Coefficient_Formulas.html
     *
     * @param N      interpolation order
     * @param delta  fractional delay (see the link above)
     * @param cfts   the coefficients output array, with length >= N+1
     */
    void lagrangeInterpolationCoefficients(size_t N, float delta, float* cfts){
        for(size_t n=0; n<=N; n++)
            for(size_t k=0; k<=N; k++)
                if(n != k)
                    cfts[n] = (delta - (float)k) / ((float)n - (float)k);
    }
    
    
    void BMLagrangeInterpolationTable_init(BMLagrangeInterpolationTable* This,
                                           size_t interpolationOrder,
                                           size_t length){
        // rename the input for convenience of notation
        size_t N = interpolationOrder;
        
        // allocate memory for the first dimension of the table
        This->table = malloc(sizeof(float*) * length);
        
        // we need N + 1 coefficients in each row of the table
        This->width = N + 1;
        This->lengthI = length;
        This->lengthF = (float)length;
        
        // allocate memory for the second dimension of the table
        for(size_t i=0; i<length; i++)
            This->table[i] = malloc(sizeof(float) * This->width);
        
        
        /*
         * populate the table for #=length values of delta in [dMin,dMax), 
         * where dMin and dMax are calculated as shown below.
         *
         * N even:
         *  dMin = N/2 - 1/2
         *  dMax = N/2 + 1/2
         *
         * N odd:
         *  dMin = (N+1)/2 - 1/2
         *  dMax = (N+2)/2 + 1/2
         *
         * ref: https://ccrma.stanford.edu/~jos/pasp/Avoiding_Discontinuities_When_Changing.html
         */
        
        // find dMin and dMax as shown in the preceeding comment
        if(N % 2 == 0){ // N even
            This->dMin = (float)(N-1) / 2.0f;
            This->dCentreF = (float)N  / 2.0f;
            This->dMax = (float)(N+1) / 2.0f;
            This->dCentreI = N / 2;
        } else { // N odd
            This->dMin = (float)(N) / 2.0f;
            This->dCentreF = (float)(N+1) / 2.0f;
            This->dMax = (float)(N+2) / 2.0f;
            This->dCentreI = (N+1) / 2;
        }
        
        // populate the table for values of delta in [dMin,dMax)
        float deltaDiff = 1.0 / (float)length;
        for(size_t i=0; i<length; i++){
            lagrangeInterpolationCoefficients(N,
                                              This->dMin + (float)i * deltaDiff,
                                              This->table[i]);
        }
    }
    
    
    
    size_t BMLagrangInterpolationTable_getIndex(BMLagrangeInterpolationTable* This, float delta){
        // confirm that the input is valid
        assert(delta >= This->dMin && delta < This->dMax);

        // get the index corresponding to delta (by truncation)
        return (size_t) ((delta - This->dMin) * This->lengthF);
    }
    
    
    void BMLagrangeInterpolationTable_destroy(BMLagrangeInterpolationTable* This){
        
        // free the inner dimension of the table
        for(size_t i=0; i<This->lengthI; i++)
            free(This->table[i]);
        
        // free the outer dimension
        free(This->table);
    }


#ifdef __cplusplus
}
#endif
