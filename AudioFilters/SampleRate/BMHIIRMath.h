//
//  BMHIIRMath.h
//  AudioFiltersXcodeProject
//
//  Ported from fnc.hpp from the HIIR library by
//  Laurend De Soras
//  http://ldesoras.free.fr/prod.html
//
//  Created by hans anderson on 7/12/19.
//  Anyone may use this file without restrictions of any kind
//

#ifndef BMHIIRMath_h
#define BMHIIRMath_h

#include <math.h>

static inline int hiir_ceil_int (double x)
{
    return (int)ceil(x);
}


//template <class T>
//T    ipowp (T x, long n)
//{
//    assert (n >= 0);
//
//    T              z (1);
//    while (n != 0)
//    {
//        if ((n & 1) != 0)
//        {
//            z *= x;
//        }
//        n >>= 1;
//        x *= x;
//    }
//
//    return z;
//}
double hiir_ipowp (double x, long n)
{
    assert (n >= 0);
    
    double z = 1.0;
    while (n != 0)
    {
        if ((n & 1) != 0)
        {
            z *= x;
        }
        n >>= 1;
        x *= x;
    }
    
    return z;
}

#endif /* BMHIIRMath_h */
