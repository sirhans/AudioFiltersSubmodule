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
    static inline bool isPowerOfTwo(size_t x)
    {
        return (x & (x - 1)) == 0;
    }
    
    
    
    
    /*
     * Integer absolute value
     *
     * https://stackoverflow.com/questions/9772348/get-absolute-value-without-using-abs-function-nor-if-statement
     */
    static inline int32_t absi(int32_t x){
        return (x ^ (x >> 31)) - (x >> 31);
    }
    
    
    
    /*
     * integer log base 2
     * 
     * https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c
     */
    static inline uint32_t log2i(uint32_t x){
        // get index of the most significant bit
        return __builtin_ctz(x);
    }
    
    
    
    
    
    /*!
     *gcd_ui
     *
     * @returns the greatest common divisor of a and b
     */
    static inline size_t gcd_ui(size_t a, size_t b)
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
    static inline int gcd_i (int a, int b){
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
    static inline int powi(int a, int b)
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
    
    
    
    
    // calcul a^n%mod
    // https://rosettacode.org/wiki/Miller–Rabin_primality_test#Deterministic_up_to_341.2C550.2C071.2C728.2C321
    static inline size_t powerMod(size_t a, size_t n, size_t mod)
    {
        size_t power = a;
        size_t result = 1;
        
        while (n)
        {
            if (n & 1)
                result = (result * power) % mod;
            power = (power * power) % mod;
            n >>= 1;
        }
        return result;
    }
    
    
    
    // REQUIRED FOR MILLER-RABIN PRIMALITY TEST
    // n−1 = 2^s * d with d odd by factoring powers of 2 from n−1
    // https://rosettacode.org/wiki/Miller–Rabin_primality_test#Deterministic_up_to_341.2C550.2C071.2C728.2C321
    static inline bool witness(size_t n, size_t s, size_t d, size_t a)
    {
        size_t x = powerMod(a, d, n);
        size_t y = 0;
        
        while (s) {
            y = (x * x) % n;
            if (y == 1 && x != 1 && x != n-1)
                return false;
            x = y;
            --s;
        }
        if (y != 1)
            return false;
        return true;
    }
    
    
    /*
     * MILLER-RABIN PRIMALITY TEST
     *
     * if n < 1,373,653, it is enough to test a = 2 and 3;
     * if n < 9,080,191, it is enough to test a = 31 and 73;
     * if n < 4,759,123,141, it is enough to test a = 2, 7, and 61;
     * if n < 1,122,004,669,633, it is enough to test a = 2, 13, 23, and 1662803;
     * if n < 2,152,302,898,747, it is enough to test a = 2, 3, 5, 7, and 11;
     * if n < 3,474,749,660,383, it is enough to test a = 2, 3, 5, 7, 11, and 13;
     * if n < 341,550,071,728,321, it is enough to test a = 2, 3, 5, 7, 11, 13, and 17.
     */
    // https://rosettacode.org/wiki/Miller–Rabin_primality_test#Deterministic_up_to_341.2C550.2C071.2C728.2C321
    static inline bool isPrimeMr(size_t n)
    {
        if (((!(n & 1)) && n != 2 ) || (n < 2) || (n % 3 == 0 && n != 3))
            return false;
        if (n <= 3)
            return true;
        
        size_t d = n / 2;
        size_t s = 1;
        while (!(d & 1)) {
            d /= 2;
            ++s;
        }
        
        if (n < 1373653)
            return witness(n, s, d, 2) && witness(n, s, d, 3);
        if (n < 9080191)
            return witness(n, s, d, 31) && witness(n, s, d, 73);
        if (n < 4759123141)
            return witness(n, s, d, 2) && witness(n, s, d, 7) && witness(n, s, d, 61);
        if (n < 1122004669633)
            return witness(n, s, d, 2) && witness(n, s, d, 13) && witness(n, s, d, 23) && witness(n, s, d, 1662803);
        if (n < 2152302898747)
            return witness(n, s, d, 2) && witness(n, s, d, 3) && witness(n, s, d, 5) && witness(n, s, d, 7) && witness(n, s, d, 11);
        if (n < 3474749660383)
            return witness(n, s, d, 2) && witness(n, s, d, 3) && witness(n, s, d, 5) && witness(n, s, d, 7) && witness(n, s, d, 11) && witness(n, s, d, 13);
        return witness(n, s, d, 2) && witness(n, s, d, 3) && witness(n, s, d, 5) && witness(n, s, d, 7) && witness(n, s, d, 11) && witness(n, s, d, 13) && witness(n, s, d, 17);
    }
    
    
    

#endif /* BMIntegerMath_h */

#ifdef __cplusplus
}
#endif
