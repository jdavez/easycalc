/*
 *   $Id: elliptic.c,v 1.4 2006/09/12 19:40:56 tvoverbe Exp $
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

#include <PalmOS.h>
#include "MathLib.h"
#include "konvert.h"
#include "funcs.h"
#include "mathem.h"
#include "stack.h"
#include "prefs.h"
#include "display.h"
#include "specfun.h"

#define PIO2 (M_PIl*0.5)
#define C1 1.3862943611198906188E0

/***********************************************************************
 *
 * FUNCTION:     ellc1
 * 
 * DESCRIPTION:  Compute complete elliptic integral of the first kind
 *
 * PARAMETERS:   eccentricity m
 *
 * RETURN:       K(m)
 *      
 ***********************************************************************/
static double ellc1(double x) SPECFUN;
static double ellc1(double x)
{
  static double P[] =
  {
    1.38629436111989062502E0,
    9.65735902811690126535E-2,
    3.08851465246711995998E-2,
    1.49380448916805252718E-2,
    8.79078273952743772254E-3,
    6.18901033637687613229E-3,
    6.87489687449949877925E-3,
    9.85821379021226008714E-3,
    7.97404013220415179367E-3,
    2.28025724005875567385E-3,
    1.37982864606273237150E-4
  };

  static double Q[] =
  {
    4.99999999999999999821E-1,
    1.24999999999870820058E-1,
    7.03124996963957469739E-2,
    4.88280347570998239232E-2,
    3.73774314173823228969E-2,
    3.01204715227604046988E-2,
    2.39089602715924892727E-2,
    1.54850516649762399335E-2,
    5.94058303753167793257E-3,
    2.94078955048598507511E-5,
  };

  if (x > EPSILON)
    return(polevl(x, P, 10) - log(x)*polevl(x, Q, 10));
  return(C1 - 0.5*log(x));
}

/***********************************************************************
 *
 * FUNCTION:     elli1
 * 
 * DESCRIPTION:  Compute incomplete elliptic integral of the first kind
 *
 * PARAMETERS:   eccentricity m and modulus phi
 *
 * RETURN:       F(m, phi)
 *      
 ***********************************************************************/
static double elli1(double phi, double m) SPECFUN;
static double elli1(double phi, double m)
{
  double a, b, c, e, temp, t, K;
  int d, mod, sign, npio2;

  if( m == 0.0 )
    return( phi );
  a = 1.0 - m;
  if( a == 0.0 ) {
    return(log(tan((PIO2 + phi)/2.0)));
  }
  npio2 = floor(phi/PIO2);
  if(npio2 & 1)
    npio2 += 1;
  if(npio2) {
    K = ellc1(a);
    phi = phi - npio2 * PIO2;
  } else
    K = 0.0;

  if( phi < 0.0 ) {
    phi = -phi;
    sign = -1;
  } else
    sign = 0;
  b = sqrt(a);
  t = tan( phi );
  if( fabs(t) > 10.0 ) {
    /* Transform the amplitude */
    e = 1.0/(b*t);
    /* ... but avoid multiple recursions.  */
    if( fabs(e) < 10.0 ) {
      e = atan(e);
      if(npio2 == 0)
	K = ellc1(a);
      temp = K - elli1(e, m);
      goto done;
    }
  }
  a = 1.0;
  c = sqrt(m);
  d = 1;
  mod = 0;

  while(fabs(c/a) > EPSILON) {
    temp = b/a;
    phi = phi + atan(t*temp) + mod * M_PIl;
    mod = (phi + PIO2)/M_PIl;
    t = t * ( 1.0 + temp )/( 1.0 - temp * t * t );
    c = ( a - b )/2.0;
    temp = sqrt( a * b );
    a = ( a + b )/2.0;
    b = temp;
    d += d;
  }

  temp = (atan(t) + mod * M_PIl)/(d * a);

 done:
  if( sign < 0 )
    temp = -temp;
  temp += npio2 * K;
  return(temp);
}

/***********************************************************************
 *
 * FUNCTION:     ellc2
 * 
 * DESCRIPTION:  Compute complete elliptic integral of the second kind
 *
 * PARAMETERS:   eccentricity m
 *
 * RETURN:       E(m)
 *      
 ***********************************************************************/
static double ellc2(double m) SPECFUN;
static double ellc2(double m)
{
  static double P[] = {
    1.00000000000000000299E0,
    4.43147180560990850618E-1,
    5.68051945617860553470E-2,
    2.18317996015557253103E-2,
    1.15688436810574127319E-2,
    7.58395289413514708519E-3,
    7.77395492516787092951E-3,
    1.07350949056076193403E-2,
    8.68786816565889628429E-3,
    2.50888492163602060990E-3,
    1.53552577301013293365E-4,
  };

  static double Q[] = {
    2.49999999999888314361E-1,
    9.37499997197644278445E-2,
    5.85936634471101055642E-2,
    4.27180926518931511717E-2,
    3.34833904888224918614E-2,
    2.61769742454493659583E-2,
    1.68862163993311317300E-2,
    6.50609489976927491433E-3,
    1.00962792679356715133E-3,
    3.27954898576485872656E-5,
  };

  if (m == 0)
    return(1.0);
  return(polevl(m, P, 10) - log(m)*(m*polevl(m, Q, 9)));
}

/***********************************************************************
 *
 * FUNCTION:     elli2
 * 
 * DESCRIPTION:  Compute incomplete elliptic integral of the second kind
 *
 * PARAMETERS:   eccentricity m and modulus phi
 *
 * RETURN:       F(m, phi)
 *      
 ***********************************************************************/
static double elli2(double phi, double m) SPECFUN;
static double elli2(double phi, double m)
{
  double a, b, c, e, temp;
  double lphi, t, E;
  int d, mod, npio2, sign;

  if( m == 0.0 )
    return( phi );
  lphi = phi;
  npio2 = floor( lphi/PIO2 );
  if( npio2 & 1 )
    npio2 += 1;
  lphi = lphi - npio2 * PIO2;
  if( lphi < 0.0 ) {
    lphi = -lphi;
    sign = -1;
  } else {
    sign = 1;
  }
  a = 1.0 - m;
  E = ellc2(a);
  if( a == 0.0 ) {
    temp = sin(lphi);
    goto done;
  }
  t = tan(lphi);
  b = sqrt(a);
  if( fabs(t) > 10.0 ) {
    /* Transform the amplitude */
    e = 1.0/(b*t);
    /* ... but avoid multiple recursions.  */
    if( fabs(e) < 10.0 ) {
      e = atan(e);
      temp = E + m * sin( lphi ) * sin( e ) - elli2( e, m );
      goto done;
    }
  }
  c = sqrt(m);
  a = 1.0;
  d = 1;
  e = 0.0;
  mod = 0;

  while( fabs(c/a) > EPSILON ) {
    temp = b/a;
    lphi = lphi + atan(t*temp) + mod * M_PIl;
    mod = (lphi + PIO2)/M_PIl;
    t = t * ( 1.0 + temp )/( 1.0 - temp * t * t );
    c = ( a - b )/2.0;
    temp = sqrt( a * b );
    a = ( a + b )/2.0;
    b = temp;
    d += d;
    e += c * sin(lphi);
  }

  temp = E / ellc1( 1.0 - m );
  temp *= (atan(t) + mod * M_PIl)/(d * a);
  temp += e;

 done:

  if( sign < 0 )
    temp = -temp;
  temp += npio2 * E;
  return( temp );
}

/***********************************************************************
 *
 * FUNCTION:     ell_jacobi
 * 
 * DESCRIPTION:  Compute Jacobian elliptic functions, sn, cn, dn.
 *
 * PARAMETERS:   eccentricity m and argument u
 *
 * RETURN:       sn(u|m), cn(u|m), and dn(u|m)
 * 
 ***********************************************************************/
static int ell_jacobi(double u, double m, double *sn, double *cn, double *dn,
		      double *ph) SPECFUN;
static int ell_jacobi(double u, double m, double *sn, double *cn, double *dn,
		      double *ph)
{
  double ai, b, phi, t, twon;
  double a[9], c[9];
  int i;

  /* Check for special cases */

  if( m < 1.0e-9 ) {
    t = sin(u);
    b = cos(u);
    ai = 0.25 * m * (u - t*b);
    *sn = t - ai*b;
    *cn = b + ai*t;
    *ph = u - ai;
    *dn = 1.0 - 0.5*m*t*t;
    return(0);
  }

  if( m >= 0.9999999999 ) {
    ai = 0.25 * (1.0-m);
    b = cosh(u);
    t = tanh(u);
    phi = 1.0/b;
    twon = b * sinh(u);
    *sn = t + ai * (twon - u)/(b*b);
    *ph = 2.0*atan(exp(u)) - PIO2 + ai*(twon - u)/b;
    ai *= t * phi;
    *cn = phi - ai * (twon - u);
    *dn = phi + ai * (twon + u);
    return(0);
  }


  /*	A. G. M. scale		*/
  a[0] = 1.0;
  b = sqrt(1.0 - m);
  c[0] = sqrt(m);
  twon = 1.0;
  i = 0;

  while( fabs(c[i]/a[i]) > EPSILON ) {
    if( i > 7 ) {
      return(-1);
    }
    ai = a[i];
    ++i;
    c[i] = ( ai - b )/2.0;
    t = sqrt( ai * b );
    a[i] = ( ai + b )/2.0;
    b = t;
    twon *= 2.0;
  }

  /* backward recurrence */
  phi = twon * a[i] * u;
  do {
    t = c[i] * sin(phi) / a[i];
    b = phi;
    phi = (asin(t) + phi)/2.0;
  } while( --i );

  *sn = sin(phi);
  t = cos(phi);
  *cn = t;
  *dn = t/cos(phi-b);
  *ph = phi;
  return(0);
}

/***********************************************************************
 *
 * FUNCTION:     math_elliptic
 * 
 * DESCRIPTION:  Compute elliptic integrals and elliptic functions
 *
 * PARAMETERS:   On stack - 1 or 2 numbers, depending on function
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError
math_elliptic(Functype *func, CodeStack *stack)
{
  double m, u, res, sn, cn, dn, ph;
  CError err;

  err = stack_get_val(stack, &m, real);
  if (err)
    return(err);

  switch (func->num) {
  case MATH_ELLC1:
  case MATH_ELLC2:
    /* check domain */
    m = 1 - m;
    if (m > 1.0 || m < 0.0 || (func->num == MATH_ELLC1 && m == 0.0))
      return(c_badarg);
    if (func->num == MATH_ELLC2)
    res = (func->num == MATH_ELLC1) ? ellc1(m) : ellc2(m);
    break;
  case MATH_ELLI1:
  case MATH_ELLI2:
    err = stack_get_val(stack, &u, real);
    if (err)
      return(err);
    if (m > 1.0 || m < 0.0 || (func->num == MATH_ELLI1 && m==1.0 &&
			       fabs(u) > PIO2))
      return(c_badarg);
    res = (func->num == MATH_ELLI1) ? elli1(m, u) : elli2(m, u);
    break;
  case MATH_SN:
  case MATH_CN:
  case MATH_DN:
    /* note that u and m are reversed! */
    err = stack_get_val(stack, &u, real);
    if (err)
      return(err);
    if (u > 1.0 || u < 0.0)
      return(c_badarg);
    if (ell_jacobi(m, u, &sn, &cn, &dn, &ph) < 0)
      return(c_badarg);
    res = (func->num == MATH_SN) ? sn : ((func->num == MATH_CN) ? cn : dn);
    break;
  default:
    return(c_internal);
  }
  return stack_add_val(stack, &res, real);
}
