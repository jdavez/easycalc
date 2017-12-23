/*   
 *   $Id: mathem.h,v 1.27 2006/09/12 19:40:55 tvoverbe Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000 Ondrej Palkovsky
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA  02110-1301, USA.
 * 
 *  You can contact me at 'ondrap@penguin.cz'.
*/

#ifndef _INTOPS_H
#define _INTOPS_H

#include "segment.h"

/* These are helper constants to allow one function serve
 * more types of operation. They needn't be unique, but must
 * be unique inside the function, that operates this type of
 * operation.
 */

/* The following about 20 constants are pointers to an array,
 * be sure you know what you do if you change them
 */
#define MATH_SIN   0
#define MATH_COS   1
#define MATH_TAN   2
#define MATH_SINH  3
#define MATH_COSH  4
#define MATH_TANH  5
/* */
#define MATH_ASIN  6
#define MATH_ACOS  7
#define MATH_ATAN  8
#define MATH_ASINH 9
#define MATH_ACOSH 10
#define MATH_ATANH 11
/* */
#define MATH_EXP   12
#define MATH_LN    13
#define MATH_LOG   14
#define MATH_SQRT  15
#define MATH_CBRT  16
#define MATH_ABS   17
#define MATH_LOG2  18
#define MATH_FPART 19
#define MATH_FLOOR 20
#define MATH_SIGN  21
#define MATH_CEIL  22
/* */

/****************/
#define MATH_REAL  3029
#define MATH_IMAG  3030
/* */
#define MATH_TRUNC 3031
#define MATH_ROUND 3032
/* */
#define MATH_FACT  3033
#define MATH_GAMMA 3034
/* Combinatorics */
#define MATH_PERM  3035
#define MATH_COMB  3036
/* */
#define MATH_FZERO 3037
/* */
#define MATH_HYPOT  3039
#define MATH_PTORX  3040
#define MATH_PTORY  3041
#define MATH_ATAN2  3042
#define MATH_RTOPD  3043
/* */
#define MATH_FSIMPS  3044
#define MATH_FDYDX1  3046
#define MATH_FMIN    3047
#define MATH_FMAX    3048
#define MATH_FVALUE  3049
#define MATH_ANGLE   3050
#define MATH_FDYDX2  3051
#define MATH_FROMBERG 3052
#define MATH_FINTERSECT 3053
/* */
#define MATH_SEED    3060
/* */
#define MATH_FINPV   3070
#define MATH_FINPMT  3071
#define MATH_FINFV   3072
#define MATH_FINN    3073

#ifdef SPECFUN_ENABLED

/* We'll reserve values 100 and up for other special functions */
#define MATH_BETA    100	/* Beta function */
#define MATH_IGAMMA  101	/* incomplete Gamma function */
#define MATH_IBETA   102	/* incomplete Beta function */
#define MATH_ERF     103	/* error function */
#define MATH_ERFC    104	/* complementary error function */

#define MATH_BESSELJ 110	/* Bessel functions of the first kind */
#define MATH_BESSELY 111	/* Bessel functions of the second kind */
#define MATH_BESSELI 112	/* Modified Bessel functions, first kind */
#define MATH_BESSELK 113	/* Modified Bessel functions, second kind */
#define MATH_AIRY    114	/* Airy function, Ai */
#define MATH_BIRY    115	/* Airy function, Bi */

#define MATH_ELLC1   120	/* Complete Elliptic Integral, 1st kind */
#define MATH_ELLC2   121	/* Complete Elliptic Integral, 2nd kind */
#define MATH_ELLI1   122	/* Incomplete Elliptic Integral, 1st kind */
#define MATH_ELLI2   123	/* Incomplete Elliptic Integral, 1st kind */
#define MATH_SN      124	/* Jacobian elliptic function sn */
#define MATH_CN      125	/* Jacobian elliptic function cn */
#define MATH_DN      126	/* Jacobian elliptic function dn */

#define MATH_HYPGG   130	/* Gaussian hypergeometric function F */
#define MATH_HYPGC   131	/* Confluent hypergeometric function M */

#define MATH_ALGNDRF 140	/* associated Legendre functions */
#define MATH_SPHHARM 141	/* spherical harmonics */

#endif

#define MATH_GCD       142
#define MATH_LCM       143
#define MATH_MODINV    144
#define MATH_MODPOW    145
#define MATH_CHINESE   146
#define MATH_PHI       147
#define MATH_ISPRIME   148
#define MATH_NEXTPRIME 149
#define MATH_PREVPRIME 150

/* Mathematical declarations */
#ifndef M_PIl
#define M_PIl 3.14159265358979323846
#endif
#define M_PI_SQRT 1.77245385091

CError math_cmpeq(Functype *func,CodeStack *stack) BASEFUNC;
CError math_cmp(Functype *func,CodeStack *stack) BASEFUNC;
CError math_oper(Functype *func,CodeStack *stack) BASEFUNC;
CError math_neg(Functype *func,CodeStack *stack) BASEFUNC;
CError math_trigo(Functype *func,CodeStack *stack) BASEFUNC;
CError math_arctrigo(Functype *func,CodeStack *stack) BASEFUNC;
CError math_math(Functype *func,CodeStack *stack) BASEFUNC;
CError math_bin(Functype *func,CodeStack *stack) BASEFUNC; 
CError math_fact(Functype *func,CodeStack *stack) BASEFUNC;
CError math_gamma(Functype *func,CodeStack *stack) BASEFUNC;
CError math_int(Functype *func,CodeStack *stack) BASEFUNC;
CError math_comb(Functype *func,CodeStack *stack) BASEFUNC;
CError math_convcoord(Functype *func,CodeStack *stack) BASEFUNC;
CError math_round(Functype *func,CodeStack *stack) BASEFUNC;
CError math_apostrophe(Functype *func,CodeStack *stack) BASEFUNC;
CError math_seed(Functype *func,CodeStack *stack) BASEFUNC;
CError math_financial(Functype *func,CodeStack *stack) BASEFUNC;

CError math_gcd(Functype *func,CodeStack *stack) BASEFUNC;
CError math_modinv(Functype *func,CodeStack *stack) BASEFUNC;
CError math_modpow(Functype *func,CodeStack *stack) BASEFUNC;
CError math_chinese(Functype *func,CodeStack *stack) BASEFUNC;
CError math_phi(Functype *func,CodeStack *stack) BASEFUNC;
CError math_primes(Functype *func,CodeStack *stack) BASEFUNC;

double math_rad_to_user(double angle) BASEFUNC;
double math_user_to_rad(double angle) BASEFUNC;

UInt8 factorize(Int32 value, Int32* dest) BASEFUNC;
UInt32 gcd(UInt32 a, UInt32 b) BASEFUNC;
UInt32 gcdex(UInt32 a, UInt32 b, Int32* coA, Int32* coB) BASEFUNC;

double sgn(double x) MLIB;
#endif
