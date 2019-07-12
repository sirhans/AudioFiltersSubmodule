//
//  BMPolyphaseIIR2Designer.c
//  AudioFiltersXcodeProject
//
//  Ported from PolyphaseIIR2Designer.cpp from the HIIR library by
//  Laurend De Soras
//  http://ldesoras.free.fr/prod.html
//
//  Created by hans anderson on 7/12/19.
//  Anyone may use this file without restrictions of any kind.
//

#include <assert.h>
#include <math.h>
#include "BMPolyphaseIIR2Designer.h"
#include "BMHIIRMath.h"



/* private functions: forward declarations */
void    compute_transition_param (double* k, double* q, double transition);
int     compute_order (double attenuation, double q);
double  compute_atten (double q, int order);
double  compute_coef (int index, double k, double q, int order);
double  compute_acc_num (double q, int order, int c);
double  compute_acc_den (double q, int order, int c);




/*
 ==============================================================================
 Name: compute_nbr_coefs_from_proto
 Description:
 Finds the minimum number of coefficients for a given filter specification
 Input parameters:
 - attenuation: stopband attenuation, dB. > 0.
 - transition: normalized transition bandwith. Range ]0 ; 1/2[
 Returns: Number of coefficients, > 0
 Throws: Nothing
 ==============================================================================
 */

int BMPolyphaseIIR2Designer_computeNbrCoefsFromProto(double attenuation, double transition)
{
    assert (attenuation > 0);
    assert (transition > 0);
    assert (transition < 0.5);
    
    double         k;
    double         q;
    compute_transition_param (&k, &q, transition);
    const int      order     = compute_order (attenuation, q);
    const int      nbr_coefs = (order - 1) / 2;
    
    return nbr_coefs;
}



/*
 ==============================================================================
 Name: compute_atten_from_order_tbw
 Description:
 Compute the attenuation correspounding to a given number of coefficients
 and the transition bandwith.
 Input parameters:
 - nbr_coefs: Number of desired coefficients. > 0.
 - transition: normalized transition bandwith. Range ]0 ; 1/2[
 Returns: stopband attenuation, dB. > 0.
 Throws: Nothing
 ==============================================================================
 */

double  BMPolyphaseIIR2Designer_computeAttenFromOrderTbw(int nbr_coefs, double transition)
{
    assert (nbr_coefs > 0);
    assert (transition > 0);
    assert (transition < 0.5);
    
    double         k;
    double         q;
    compute_transition_param (&k, &q, transition);
    const int      order       = nbr_coefs * 2 + 1;
    const double   attenuation = compute_atten (q, order);
    
    return attenuation;
}



/*
 ==============================================================================
 Name: compute_coefs
 Description:
 Computes coefficients for a half-band polyphase IIR filter, function of a
 given stopband gain / transition bandwidth specification.
 Order is automatically calculated.
 Input parameters:
 - attenuation: stopband attenuation, dB. > 0.
 - transition: normalized transition bandwith. Range ]0 ; 1/2[
 Output parameters:
 - coef_arr: Coefficient list, must be large enough to store all the
 coefficients. Filter order = nbr_coefs * 2 + 1
 Returns: number of coefficients
 Throws: Nothing
 ==============================================================================
 */

int    BMPolyphaseIIR2Designer_computeCoefs(double coef_arr [], double attenuation, double transition)
{
    assert (attenuation > 0);
    assert (transition > 0);
    assert (transition < 0.5);
    
    double         k;
    double         q;
    compute_transition_param (&k, &q, transition);
    
    // Computes number of required coefficients
    const int      order     = compute_order (attenuation, q);
    const int      nbr_coefs = (order - 1) / 2;
    
    // Coefficient calculation
    for (int index = 0; index < nbr_coefs; ++index)
    {
        coef_arr [index] = compute_coef (index, k, q, order);
    }
    
    return nbr_coefs;
}



/*
 ==============================================================================
 Name: compute_coefs_spec_order_tbw
 Description:
 Computes coefficients for a half-band polyphase IIR filter, function of a
 given transition bandwidth and desired filter order. Bandstop attenuation
 is set to the maximum value for these constraints.
 Input parameters:
 - nbr_coefs: Number of desired coefficients. > 0.
 - transition: normalized transition bandwith. Range ]0 ; 1/2[
 Output parameters:
 - coef_arr: Coefficient list, must be large enough to store all the
 coefficients.
 Throws: Nothing
 ==============================================================================
 */

void    BMPolyphaseIIR2Designer_computeCoefsSpecOrderTbw (double coef_arr [], int nbr_coefs, double transition)
{
    assert (nbr_coefs > 0);
    assert (transition > 0);
    assert (transition < 0.5);
    
    double         k;
    double         q;
    compute_transition_param (&k, &q, transition);
    const int      order = nbr_coefs * 2 + 1;
    
    // Coefficient calculation
    for (int index = 0; index < nbr_coefs; ++index)
    {
        coef_arr [index] = compute_coef (index, k, q, order);
    }
}



/*
 ==============================================================================
 Name: compute_phase_delay
 Description:
 Computes the phase delay introduced by a single filtering unit at a
 specified frequency.
 The delay is given for a constant sampling rate between input and output.
 Input parameters:
 - a: coefficient for the cell, [0 ; 1]
 - f_fs: frequency relative to the sampling rate, [0 ; 0.5].
 Returns:
 The phase delay in samples, >= 0.
 Throws: Nothing
 ==============================================================================
 */

double    BMPolyphaseIIR2Designer_computePhaseDelay (double a, double f_fs)
{
    assert (a >= 0);
    assert (a <= 1);
    assert (f_fs >= 0);
    assert (f_fs < 0.5);
    
    const double   w  = 2 * M_PI * f_fs;
    const double   c  = cos (w);
    const double   s  = sin (w);
    const double   x  = a + c + a * (c * (a + c) + s * s);
    const double   y  = a * a * s - s;
    double         ph = atan2 (y, x);
    if (ph < 0)
    {
        ph += 2 * M_PI;
    }
    const double   dly = ph / w;
    
    return dly;
}



/*
 ==============================================================================
 Name: compute_group_delay
 Description:
 Computes the group delay introduced by a single filtering unit at a
 specified frequency.
 The delay is given for a constant sampling rate between input and output.
 To compute the group delay of a complete filter, add the group delays
 of all the units in A0 (z).
 Input parameters:
 - a: coefficient for the cell, [0 ; 1]
 - f_fs: frequency relative to the sampling rate, [0 ; 0.5].
 - ph_flag: set if filtering unit is used in pi/2-phaser mode, in the form
 (a - z^-2) / (1 - az^-2)
 Returns:
 The group delay in samples, >= 0.
 Throws: Nothing
 ==============================================================================
 */

double    BMPolyphaseIIR2Designer_computeGroupDelay (double a, double f_fs, bool ph_flag)
{
    assert (a >= 0);
    assert (a <= 1);
    assert (f_fs >= 0);
    assert (f_fs < 0.5);
    
    const double   w   = 2 * M_PI * f_fs;
    const double   a2  = a * a;
    const double   sig = (ph_flag) ? -2 : 2;
    const double   dly = 2 * (1 - a2) / (a2 + sig * a * cos (2 * w) + 1);
    
    return dly;
}



/*
 ==============================================================================
 Name: compute_group_delay
 Description:
 Computes the group delay introduced by a complete filter at a specified
 frequency.
 The delay is given for a constant sampling rate between input and output.
 Input parameters:
 - coef_arr: filter coefficient, as given by the designing functions
 - nbr_coefs: Number of filter coefficients. > 0.
 - f_fs: frequency relative to the sampling rate, [0 ; 0.5].
 - ph_flag: set if filter is used in pi/2-phaser mode, in the form
 (a - z^-2) / (1 - az^-2)
 Returns:
 The group delay in samples, >= 0.
 Throws: Nothing
 ==============================================================================
 */

double    BMPolyphaseIIR2Designer_computeGroupDelayCascade (const double coef_arr [], int nbr_coefs, double f_fs, bool ph_flag)
{
    assert (nbr_coefs > 0);
    assert (f_fs >= 0);
    assert (f_fs < 0.5);
    
    double         dly_total = 0;
    for (int k = 0; k < nbr_coefs; ++k)
    {
        const double   dly = BMPolyphaseIIR2Designer_computeGroupDelay (coef_arr [k], f_fs, ph_flag);
        dly_total += dly;
    }
    
    return dly_total;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void    compute_transition_param (double* k, double* q, double transition)
{
    assert (transition > 0);
    assert (transition < 0.5);
    
    *k  = tan ((1 - transition * 2) * M_PI / 4);
    *k *= *k;
    assert (*k < 1);
    assert (*k > 0);
    double         kksqrt = pow (1 - *k * *k, 0.25);
    const double   e = 0.5 * (1 - kksqrt) / (1 + kksqrt);
    const double   e2 = e * e;
    const double   e4 = e2 * e2;
    *q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4)));
    assert (q > 0);
}



int    compute_order (double attenuation, double q)
{
    assert (attenuation > 0);
    assert (q > 0);
    
    const double   attn_p2 = pow (10.0, -attenuation / 10);
    const double   a       = attn_p2 / (1 - attn_p2);
    int            order   = hiir_ceil_int (log (a * a / 16) / log (q));
    if ((order & 1) == 0)
    {
        ++ order;
    }
    if (order == 1)
    {
        order = 3;
    }
    
    return order;
}



double    compute_atten (double q, int order)
{
    assert (q > 0);
    assert (order > 0);
    assert ((order & 1) == 1);
    
    const double   a           = 4 * exp (order * 0.5 * log (q));
    assert (a != -1.0);
    const double   attn_p2     = a / (1 + a);
    const double   attenuation = -10 * log10 (attn_p2);
    assert (attenuation > 0);
    
    return attenuation;
}



double    compute_coef (int index, double k, double q, int order)
{
    assert (index >= 0);
    assert (index * 2 < order);
    
    const int      c    = index + 1;
    const double   num  = compute_acc_num (q, order, c) * pow (q, 0.25);
    const double   den  = compute_acc_den (q, order, c) + 0.5;
    const double   ww   = num / den;
    const double   wwsq = ww * ww;
    
    const double   x    = sqrt ((1 - wwsq * k) * (1 - wwsq / k)) / (1 + wwsq);
    const double   coef = (1 - x) / (1 + x);
    
    return coef;
}



double    compute_acc_num (double q, int order, int c)
{
    assert (c >= 1);
    assert (c < order * 2);
    
    int            i   = 0;
    int            j   = 1;
    double         acc = 0;
    double         q_ii1;
    do
    {
        q_ii1  = hiir_ipowp (q, i * (i + 1));
        q_ii1 *= sin ((i * 2 + 1) * c * M_PI / order) * j;
        acc   += q_ii1;
        
        j = -j;
        ++i;
    }
    while (fabs (q_ii1) > 1e-100);
    
    return acc;
}



double    compute_acc_den (double q, int order, int c)
{
    assert (c >= 1);
    assert (c < order * 2);
    
    int            i = 1;
    int            j = -1;
    double         acc = 0;
    double         q_i2;
    do
    {
        q_i2  = hiir_ipowp (q, i * i);
        q_i2 *= cos (i * 2 * c * M_PI / order) * j;
        acc  += q_i2;
        
        j = -j;
        ++i;
    }
    while (fabs (q_i2) > 1e-100);
    
    return acc;
}
