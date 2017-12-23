/*
 *   $Id: bessels.cpp,v 1.1 2009/10/17 13:48:34 mapibid Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000 Ondrej Palkovsky
 *   This file Copyright (C) 2000 Rafael R. Sevilla
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
 *  You can contact Ondrej at 'ondrap@penguin.cz'.
 *  You can contact me at 'dido@pacific.net.ph'.
 *
 *  Portions of this code are from the Cephes Math Library, Release 2.8
 *  Copyright (C) 1984, 1987, 1989, 2000 by Stephen L. Moshier
 */
#include "StdAfx.h"

#include "compat/PalmOS.h"
#include "compat/MathLib.h"
#include "konvert.h"
#include "funcs.h"
#include "mathem.h"
#include "stack.h"
#include "core/prefs.h"
#include "display.h"
#include "specfun.h"

static double p0[5] = {
   1.0000000000e0,
  -0.1098628627e-2,
   0.2734510407e-4,
  -0.2073370639e-5,
   0.2093887211e-6
}
;
static double q0[5] = {
  -0.1562499995e-1,
   0.1430488765e-3,
  -0.6911147651e-5, 
   0.7621095161e-6,
   0.9349451520e-7
};

static double p1[5] = {
   1.0000000000e0,
   0.1831050000e-2,
  -0.3516396496e-4,
   0.2457520174e-5,
  -0.2403370190e-6
};

static double q1[5] = {
   0.04687499995e0, 
  -0.20026908730e-3,
   0.84491990960e-5,
  -0.88228987000e-6,
   0.10578741200e-6
};

#define TWOOVRPI 0.63661977236758134307
#define PIOVR4   0.78539816339744830962
#define THRPIOV4 2.35619449019234492885

/***********************************************************************
 *
 * FUNCTION:     besselj01
 * 
 * DESCRIPTION:  Compute Bessel function of the first kind, order 0 or 1
 *
 * PARAMETERS:   n (either 0 or 1), x
 *
 * RETURN:       J_0(x) or J_1(x)
 *      
 ***********************************************************************/
static double besselj01(int n, double x) SPECFUN;
static double besselj01(int n, double x)
{
  double y, fact, pifact, ax, z, xx, rv;
  double *p, *q, *r, *s;
  static double r0[6] = { 57568490574.0, -13362590354.0, 651619640.7,
			 -11214424.18, 77392.33017, -184.9052456 };
  static double s0[6] = { 57568490411.0, 1029532985.0, 9494680.718,
			 59272.64853, 267.8532712, 1.0 };
  static double r1[6] = { 72362614232.0, -7895059235.0, 242396853.1,
			  -2972611.439, 15704.48260, -30.16036606 };
  static double s1[6] = { 144725228442.0, 2300535178.0, 18583304.74,
			  99447.43394, 376.9991397, 1.0 };

  if (n == 0) {
    p = p0;
    q = q0;
    r = r0;
    s = s0;
    fact = 1.0;
    pifact = PIOVR4; /* pi/4 */
  } else {
    p = p1;
    q = q1;
    r = r1;
    s = s1;
    fact = x;
    pifact = THRPIOV4; /* 3*pi/4 */
  }

  if (fabs(x) < 8) {
    y = x*x;
    return(fact*polevl(y, r, 6)/polevl(y, s, 6));
  }
  ax = fabs(x);
  z = 8/ax;
  y = z*z;
  xx = ax - pifact;
  rv = sqrt(TWOOVRPI/ax)*
    (cos(xx)*polevl(y, p, 5) - z*sin(xx)*polevl(y, q, 5));
  rv = (x < 0 && n == 1) ? -rv : rv;
  return(rv);
}

/***********************************************************************
 *
 * FUNCTION:     bessely01
 * 
 * DESCRIPTION:  Compute Bessel function of the second kind, order 0 or 1
 *
 * PARAMETERS:   n (either 0 or 1), x
 *
 * RETURN:       Y_0(x) or Y_1(x)
 *      
 ***********************************************************************/
static double bessely01(int n, double x) SPECFUN;
static double bessely01(int n, double x)
{
  double y, fact, pifact, onovx, z, xx, rv;
  double *p, *q, *r, *s;
  int sdeg;
  static double r0[6] = { -2957821389.0, 7062834065.0, -512359803.6,
			  10879881.29, -86327.92757, 228.4622733 };
  static double s0[6] = { 40076544269.0, 745249964.8, 7189466.438,
			  47447.26470, 226.1030244, 1.0 };
  static double r1[6] = { -0.4900604943e13, 0.1275274390e13, 
			  -0.5153438139e11, 0.7349264551e9,
			  -0.4237922726e7, 0.8511937935e4 };
  static double s1[7] = { 0.2499580570e14, 0.4244419664e12,
			  0.3733650367e10, 0.2245904002e8,
			  0.1020426050e6, 0.3549632885e3, 1.0 };
  if (n == 0) {
    p = p0;
    q = q0;
    r = r0;
    s = s0;
    fact = 1.0;
    sdeg = 6;
    pifact = PIOVR4; /* pi/4 */
    onovx = 0;
  } else {
    p = p1;
    q = q1;
    r = r1;
    s = s1;
    fact = x;
    sdeg= 7;
    pifact = THRPIOV4; /* 3*pi/4 */
    onovx = 1/x;
  }

  if (x < 8) {
    y = x*x;
    return(fact*polevl(y, r, 6)/polevl(y, s, sdeg) +
	   TWOOVRPI*(besselj01(n, x)*log(x) - onovx));
  }
  z = 8/x;
  y = z*z;
  xx = x - pifact;
  rv = sqrt(TWOOVRPI/x)*
    (sin(xx)*polevl(y, p, 5) + z*cos(xx)*polevl(y, q, 5));
  return(rv);
}

/***********************************************************************
 *
 * FUNCTION:     besseljn
 * 
 * DESCRIPTION:  Compute Bessel function of
 *               the first kind, order n
 *
 * PARAMETERS:   n, x, j01
 *
 * RETURN:       J_n(x)
 *      
 ***********************************************************************/
static double besseljn(int n, double x,
		       double (*j01)(int n, double x)) SPECFUN;
static double besseljn(int n, double x, double (*j01)(int n, double x))
{
  double pkm2, pkm1, pk, xk, r, ans;
  int k, sign;

  if( n < 0 ) {
    n = -n;
    if( (n & 1) == 0 )	/* -1**n */
      sign = 1;
    else
      sign = -1;
  } else
    sign = 1;

  if( x < 0.0 ) {
    if( n & 1 )
      sign = -sign;
    x = -x;
  }

  if( n == 0 )
    return( sign * j01(0, x) );
  if( n == 1 )
    return( sign * j01(1, x) );
  if( n == 2 )
    return( sign * (2.0 * j01(1, x) / x  -  j01(0, x)) );

  if( x < EPSILON )
    return( 0.0 );

  k = 53;

  pk = 2 * (n + k);
  ans = pk;
  xk = x * x;

  do {
    pk -= 2.0;
    ans = pk - (xk/ans);
  } while( --k > 0 );
  ans = x/ans;

  /* backward recurrence */

  pk = 1.0;
  pkm1 = 1.0/ans;
  k = n-1;
  r = 2 * k;

  do {
    pkm2 = (pkm1 * r  -  pk * x) / x;
    pk = pkm1;
    pkm1 = pkm2;
    r -= 2.0;
  } while( --k > 0 );

  if( fabs(pk) > fabs(pkm1) )
    ans = j01(1, x)/pk;
  else
    ans = j01(0, x)/pkm1;
  return( sign * ans );
}

/***********************************************************************
 *
 * FUNCTION:     besselyn
 * 
 * DESCRIPTION:  Compute Bessel function of
 *               the second kind, order n
 *
 * PARAMETERS:   n, x, y01
 *
 * RETURN:       Y_n(x)
 *      
 ***********************************************************************/
static double besselyn(int n, double x,
		       double (*y01)(int n, double x)) SPECFUN;
static double besselyn(int n, double x, double (*y01)(int n, double x))
{
  double an, anm1, anm2, r;
  int k, sign;

  if( n < 0 ) {
    n = -n;
    if( (n & 1) == 0 )	/* -1**n */
      sign = 1;
    else
      sign = -1;
  } else
    sign = 1;

  if( n == 0 )
    return( sign * y01(0, x) );
  if( n == 1 )
    return( sign * y01(1, x) );

  /* forward recurrence on n */

  anm2 = y01(0, x);
  anm1 = y01(1, x);
  k = 1;
  r = 2 * k;
  do {
    an = r * anm1 / x  -  anm2;
    anm2 = anm1;
    anm1 = an;
	r += 2.0;
	++k;
  } while( k < n );

  return( sign * an );
}

/***********************************************************************
 *
 * FUNCTION:     besseli01
 * 
 * DESCRIPTION:  Compute modified Bessel function of the first kind,
 *               order 0 or 1
 *
 * PARAMETERS:   n (0 or 1 only), x
 *
 * RETURN:       I_0(x) or I_1(x)
 *      
 ***********************************************************************/
static double besseli01(int n, double x) SPECFUN;
static double besseli01(int n, double x)
{
  double y, ax, fact;
  double *p, *q;
  static double p0[7] = { 1.0, 3.5156229, 3.0899424, 1.2067492, 0.2659732,
			0.360768e-1, 0.45813e-2 };
  static double q0[9] = { 0.39894228, 0.1328592e-1, 0.225319e-2, -0.157565e-2,
			0.916281e-2, -0.2057706e-1, 0.2635537e-1,
			-0.1647633e-1, 0.392377e-2 };
  static double p1[7] = { 0.5, 0.87890594, 0.51498869, 0.15084934,
			  0.2658733e-1, 0.301532e-2, 0.32411e-3 };
  static double q1[9] = { 0.39894228, -0.3988024e-1, -0.362018e-2, 0.163801e-2,
			 -0.1031555e-1, 0.2282967e-1, -0.2895312e-1,
			 0.1787654e-1, -0.420059e-2 };

  p = (n == 0) ? p0 : p1;
  q = (n == 0) ? q0 : q1;

  if (fabs(x) < 3.75) {
    fact = (n == 0) ? 1 : x;
    y = (x*x/14.0625);
    return(fact*polevl(y, p, 7));
  }
  ax = fabs(x);
  y = 3.75/ax;
  return((exp(ax)/sqrt(ax))*polevl(y, q, 9));
}

/***********************************************************************
 *
 * FUNCTION:     besselk01
 * 
 * DESCRIPTION:  Compute modified Bessel function of the 2nd kind,
 *               order 0 or 1.
 *
 * PARAMETERS:   n (either 0 or 1 only), x
 *
 * RETURN:       K_0(x) or K_1(x)
 *      
 ***********************************************************************/
static double besselk01(int n, double x) SPECFUN;
static double besselk01(int n, double x)
{
  double y, fact;
  double *p, *q;
  double p0[7] = { -0.57721566, 0.42278420, 0.23069756, 0.3488590e-1,
		  0.262698e-2, 0.10750e-3, 0.74e-5 };
  double q0[7] = { 1.25331414, -0.7832358e-1, 0.2189568e-1, -0.1062446e-1,
		  0.587872e-2, -0.251540e-2, 0.53208e-3 };
  double p1[7] = { 1.0, 0.15443144, -0.67278579, -0.18156897, -0.1919402e-1,
		  -0.110404e-2, -0.4686e-4 };
  double q1[7] = { 1.25331414, 0.23498619, -0.3655620e-1, 0.1504268e-1,
		  -0.780353e-2, 0.325614e-2, -0.68245e-3 };
  int sign;

  if (n == 0) {
    p = p0;
    q = q0;
    sign = -1;
    fact = 1.0;
  } else {
    p = p1;
    q = q1;
    sign = 1;
    fact = 1.0/x;
  }
  if (x <= 2.0) {
    y = x*x/4.0;
    return((sign*log(x/2.0)*besseli01(n, x))+
	   fact*polevl(y, p, 7));
  }
  y = 2.0/x;
  return((exp(-x)/sqrt(x))*polevl(y, q, 7));
}

/***********************************************************************
 *
 * FUNCTION:     besselin
 * 
 * DESCRIPTION:  Compute modified Bessel function of the first kind,
 *               order n
 *
 * PARAMETERS:   n, x
 *
 * RETURN:       I_n(x)
 *      
 ***********************************************************************/
#define IACC 40
#define BIGNO 1e10
#define BIGNI 1e-10
static double besselin(int n, double x,
		       double (*i01)(int n, double x)) SPECFUN;
static double besselin(int n, double x, double (*i01)(int n, double x))
{
  double tox, bip, bi, bim, retval;
  int j, m;

  if (n < 0)
    n = -n;
  if (n == 0 || n == 1)
    return(i01(n, x));
  if (x == 0)
    return(0);
  tox = 2.0/fabs(x);
  bip = 0.0;
  bi = 1.0;
  retval = 0.0;
  m = 2*(n+(int)(sqrt(IACC*n)));
  for (j=m; j>=1; j--) {
    bim = bip + j*tox*bi;
    bip = bi;
    bi = bim;
    if (fabs(bi) > BIGNO) {
      retval *= BIGNI;
      bi *= BIGNI;
      bip *= BIGNI;
    }
    if (j == n)
      retval = bip;
  }
  retval *= i01(0, x)/bi;
  if (x < 0 && (n & 1))
    return(-retval);
  return(retval);
}

/***********************************************************************
 *
 * FUNCTION:     besselkn
 * 
 * DESCRIPTION:  Compute modified Bessel function of the second kind,
 *               order n
 *
 * PARAMETERS:   n, x
 *
 * RETURN:       K_n(x)
 *      
 ***********************************************************************/
static double besselkn(int n, double x,
		       double (*k01)(int n, double x)) SPECFUN;
static double besselkn(int n, double x, double (*k01)(int n, double x))
{
  double tox, bkm, bkp, bk;
  int j;

  if (n < 0)
    n = -n;

  if (n == 0 || n == 1)
    return(k01(n, x));
  tox = 2.0/x;
  bkm = k01(0, x);
  bk = k01(1, x);
  for (j=1; j<n; j++) {
    bkp = bkm + j*tox*bk;
    bkm = bk;
    bk = bkp;
  }
  return(bk);
}

/***********************************************************************
 *
 * FUNCTION:     math_bessels
 * 
 * DESCRIPTION:  Compute Bessel and related functions
 *
 * PARAMETERS:   On stack - 2 numbers
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError
math_bessels(Functype *func, CodeStack *stack)
{

  double x, n, res;
  CError err;

  err = stack_get_val(stack, &x, real);
  if (err)
    return(err);

  switch (func->num) {
  case MATH_BESSELJ:
  case MATH_BESSELY:
    err = stack_get_val(stack, &n, real);
    if (err)
      return(err);
    /* integer bessels only */
    if (!IS_ZERO(n - round(n)) ||
	(IS_ZERO(x) && (func->num == MATH_BESSELY)))
      return(c_badarg);
    res = (func->num == MATH_BESSELJ) ? besseljn((int) n, x, besselj01) : 
      besselyn((int) n, x, bessely01);
    break;
  case MATH_BESSELI:
  case MATH_BESSELK:
    err = stack_get_val(stack, &n, real);
    if (err)
      return(err);
    /* integer bessels only */
    if (!IS_ZERO(n - round(n)) ||
	(IS_ZERO(x) && (func->num == MATH_BESSELK)))
      return(c_badarg);
    res = (func->num == MATH_BESSELI) ? besselin((int) n, x, besseli01) :
      besselkn((int) n, x, besselk01);
    break;
  case MATH_AIRY:
  case MATH_BIRY:
  default:
    return(c_internal);
  }
  return stack_add_val(stack, &res, real);
}

