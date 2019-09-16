//
//  BMIntegerMath.h
//  BMAudioFilters
//
//  Created by Hans on 14/9/17.
//  Anyone can use this file without restrictions
//

#ifdef __cplusplus
extern "C" {
#endif


#ifndef BMIntegerMath_h
#define BMIntegerMath_h

    
    
    /*
     * https://stackoverflow.com/questions/600293/how-to-check-if-a-number-is-a-power-of-2
     */
    static bool isPowerOfTwo(size_t x)
    {
        return (x & (x - 1)) == 0;
    }
    
    
    
    
    /*
     * Integer absolute value
     *
     * https://stackoverflow.com/questions/9772348/get-absolute-value-without-using-abs-function-nor-if-statement
     */
    static int32_t absi(int32_t x){
        return (x ^ (x >> 31)) - (x >> 31);
    }
    
    
    
    /*
     * integer log base 2
     * 
     * https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c
     */
    static uint32_t log2i(uint32_t x){
        // get index of the most significant bit
        return __builtin_ctz(x);
    }
    
    
    
    
    
    /*!
     *gcd_ui
     *
     * @returns the greatest common divisor of a and b
     */
    static size_t gcd_ui(size_t a, size_t b)
    {
        while (b != 0)
        {
            size_t t = b;
            b = a % b;
            a = t;
        }
        return a;
    }
    
    
    
    
    
    /*!
     *gcd_i
     *
     * @returns the greatest common divisor of a and b
     */
    static int gcd_i (int a, int b){
        int c;
        while ( a != 0 ) {
            c = a; a = b%a; b = c;
        }
        return b;
    }
    
    
    
    
    
    /*
     * a^b
     *
     * https://stackoverflow.com/questions/101439/the-most-efficient-way-to-implement-an-integer-based-power-function-powint-int
     */
    static int powi(int a, int b)
    {
        int result = 1;
        while (b)
        {
            if (b & 1)
                result *= a;
            b >>= 1;
            a *= a;
        }
        
        return result;
    }
    
    
    

#endif /* BMIntegerMath_h */

#ifdef __cplusplus
}
#endif
