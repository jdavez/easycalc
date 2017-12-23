/*
 *   $Id: specfun.cpp,v 1.2 2009/11/02 17:24:39 mapibid Exp $
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

extern double euler_gamma(double z);

/* Arrays are indexed from 1, that's why the 0.0 in the beginning */
/* Coefficients of rational functions for ln(gamma) approxiamtion*/
static const double r1[] = {0.0, -2.66685511495E0, -2.44387534237E1,
		      -2.19698958928E1, 1.11667541262E1,
		      3.13060547623E0, 6.07771387771E-1,
		      1.19400905721E1, 3.14690115749E1,
		      1.52346874070E1};

static const double r2[] = {0.0, -7.83359299449E1, -1.42046296688E2,
		      1.37519416416E2, 7.86994924154E1,
		      4.16438922228E0, 4.70668766060E1,
		      3.13399215894E2,  2.63505074721E2,
		      4.33400022514E1};

static const double r3[] = {0.0, -2.12159572323E5, 2.30661510616E5,
		      2.74647644705E4, -4.02621119975E4,
		      -2.29660729780E3, -1.16328495004E5,
		      -1.46025937511E5, -2.42357409629E4,
		      -5.70691009324E2};

static const double r4[] = {0.0, 2.79195317918525E-1, 4.917317610505968E-1,
		      6.92910599291889E-2, 3.350343815022304E0,
		      6.012459259764103E0};

static const double alr2pi = 9.18938533204673E-1;
static const double xlge = 5.10E6;

/***********************************************************************
 *
 * FUNCTION:     aln_gamma
 * 
 * DESCRIPTION:  Compute ln(gamma(x)) using the AS 245 algortithm
 *
 * PARAMETERS:   x
 *
 * RETURN:       ln(gamma(x))
 *      
 ***********************************************************************/
static double aln_gamma(double x) SPECFUN;
static double aln_gamma(double x)
{
	double alngam,y,x1,x2;

	if (x >= 1E305)
		return NaN;
	if (x <= 0)
		return NaN;
	
	alngam = 0.0;

	/* Calculation for 0 < X < 0.5 and 0.5 <= X < 1.5 combined */
	if (x < 1.5) {
		if (x < 0.5) {
			alngam = -log(x);
			y = x + 1.0;
			
			/* Test whether X < machine epsilon */
			if (y == 1.0)
				return alngam;
		} else {
			alngam = 0.0;
			y = x;
			x = (x - 0.5) - 0.5;
		}
		alngam = alngam + x * ((((r1[5]*y + r1[4])*y + r1[3])*y
				+ r1[2])*y + r1[1]) / ((((y+r1[9])*y 
				 + r1[8])*y + r1[7])*y+r1[6]);
		return alngam;
	}
	/* Calculation for 1.5 <= X < 4.0 */
	if (x < 4.0) {
		y = (x - 1.0) - 1.0;
		alngam = y * ((((r2[5]*x + r2[4])*x + r2[3])*x + r2[2])*x
			      + r2[1]) / ((((x + r2[9])*x + r2[8])*x 
					   + r2[7])*x + r2[6]);
		return alngam;
	}
	/* Calculation for 4.0 <= X < 12.0 */
	if (x < 12.0) {
		alngam = ((((r3[5]*x + r3[4])*x + r3[3])*x + r3[2])*x + r3[1]) /
			((((x + r3[9])*x + r3[8])*x + r3[7])*x + r3[6]);
		return alngam;
	}
	/* Calculation for X >= 12.0 */
	y = log(x);
	alngam = x * (y - 1.0) - y/2.0 + alr2pi;
	if (x > xlge)
		return alngam;
	x1 = 1.0 / x;
	x2 = x1 * x1;
	alngam = alngam + x1 * ((r4[3]*x2 + r4[2])*x2 + r4[1]) /
		((x2 + r4[5])*x2 + r4[4]);
	return alngam;
}

#define MAX_ITER 200

/***********************************************************************
 *
 * FUNCTION:     gser
 * 
 * DESCRIPTION:  Compute incomplete gamma function, power series
 *
 * PARAMETERS:   a, x
 *
 * RETURN:       P(a,x)
 *      
 ***********************************************************************/
static double gser(double a, double x) SPECFUN;
static double gser(double a, double x)
{
  double gln, ap, sum, del;
  int n;

  gln = aln_gamma(a);
  if (x == 0)
    return(0);
  ap = a;
  sum = 1.0/a;
  del = sum;
  for (n=0; n<MAX_ITER; n++) {
    ap++;
    del*=x/ap;
    sum+=del;
    if (fabs(del < fabs(sum)*EPSILON))
      break;
  }
  /* Screw it if the series failed to converge after the maximum number
     of iterations.  We need to determine a way of signalling to the user
     that the iteration failed to converge--next version maybe. */
  return(sum*exp(-x + a*log(x) - gln));
}

/***********************************************************************
 *
 * FUNCTION:     gcf
 * 
 * DESCRIPTION:  Compute complementary incomplete gamma function,
 *               continued fraction rep.
 *
 * PARAMETERS:   a, x
 *
 * RETURN:       Q(a,x)
 *      
 ***********************************************************************/
static double gcf(double a, double x) SPECFUN;
static double gcf(double a, double x)
{
  double gln, g_old, g, a0, a1, b0, b1, fac, ana, anf;
  int n;

  gln = aln_gamma(a);
  g_old = 0;
  a0 = 1;
  a1 = x;
  b0 = 0;
  b1 = 1;
  fac = 1;
  g = 1;
  for (n=1; n<=MAX_ITER; n++) {
    ana = (double)n - a;
    a0 = (a1 + a0*ana)*fac;
    b0 = (b1 + b0*ana)*fac;
    anf = n*fac;
    a1 = x*a0+anf*a1;
    b1 = x*b0+anf*b1;
    if (a1 != 0.0) {
      fac = 1.0/a1;
      g = b1*fac;
      if (fabs((g-g_old)/g) < EPSILON)
	break;
      g_old = g;
    }
  }
  return(exp(-x + a*log(x) - gln)*g);
}

/***********************************************************************
 *
 * FUNCTION:     incgamma
 * 
 * DESCRIPTION:  Compute incomplete gamma function
 *
 * PARAMETERS:   a, x
 *
 * RETURN:       P(a,x)
 *      
 ***********************************************************************/
static double incgamma(double a, double x) SPECFUN;
static double incgamma(double a, double x)
{
  if (x < a+1)
    return(gser(a, x));
  return(1.0-gcf(a, x));
}

/***********************************************************************
 *
 * FUNCTION:     erf
 * 
 * DESCRIPTION:  Compute error function
 *
 * PARAMETERS:   x
 *
 * RETURN:       erf(x)
 *      
 ***********************************************************************/
static double erf(double x) SPECFUN;
static double erf(double x)
{
  if (x < 0)
    return(-incgamma(0.5, x*x));
  return(incgamma(0.5, x*x));
}

/***********************************************************************
 *
 * FUNCTION:     erf
 * 
 * DESCRIPTION:  Compute complementary error function
 *
 * PARAMETERS:   x
 *
 * RETURN:       erfc(x)
 *      
 ***********************************************************************/
static double erfc(double x) SPECFUN;
static double erfc(double x)
{
	double xx;
	
	xx = x*x;
	if (x < 0) {
		if (xx < 1.5)
			return 1.0 + gser(0.5, xx);
		else
			return 2.0 - gcf(0.5, xx);
	} else {
		if (xx < 1.5)
			return 1.0 - gser(0.5, xx);
		else
			return gcf(0.5, xx);
	}
}

/***********************************************************************
 *
 * FUNCTION:     betacf
 * 
 * DESCRIPTION:  Compute incomplete beta function, continued fraction
 *
 * PARAMETERS:   a, b, x
 *
 * RETURN:       ibeta(a, b, x)
 *      
 ***********************************************************************/
static double betacf(double a, double b, double x) SPECFUN;
static double betacf(double a, double b, double x)
{
  double am, bm, az, qab, qap, qam, bz, em, tem, ap, bp, d, app, bpp, aold;
  int m;

  am = bm = az = 1;
  qab = a+b;
  qap = a+1;
  qam = a-1;
  bz = 1-qab*x/qap;
  for (m=1; m<=MAX_ITER; m++) {
    em = m;
    tem = em + em;
    d = em*(b-m)*x/((qam+tem)*(a+tem));
    ap = az + d*am;
    bp = bz + d*bm;
    d = -(a+em)*(qab+em)*x/((a+tem)*(qap+tem));
    app = ap + d*az;
    bpp = bp + d*bz;
    aold = az;
    am = ap/bpp;
    bm = bp/bpp;
    az = app/bpp;
    bz = 1.0;
    if (fabs(az - aold) < EPSILON*fabs(az))
      break;
  }
  return(az);
}

/***********************************************************************
 *
 * FUNCTION:     ibeta
 * 
 * DESCRIPTION:  Compute incomplete beta function
 *
 * PARAMETERS:   a, b, x
 *
 * RETURN:       ibeta(a, b, x)
 *      
 ***********************************************************************/
static double incbeta(double a, double b, double x) SPECFUN;
static double incbeta(double a, double b, double x)
{
  double bt;

  bt = (x == 0 || x == 1) ? 0 :
    exp(aln_gamma(a+b) - aln_gamma(a) -
	    aln_gamma(b) + a*log(x) + b*log(1-x));
  if (x < (a+1)/(a+b+2))		/* use continued fraction directly */
    return(bt*betacf(a, b, x)/a);
  /* use continued fraction after making symmetry transform */
  return(1-bt*betacf(b, a, 1-x)/b);
}

/***********************************************************************
 *
 * FUNCTION:     math_gambeta
 * 
 * DESCRIPTION:  Compute Gamma, Beta, and related functions
 *
 * PARAMETERS:   On stack - up to three numbers, depending on function
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError
math_gambeta(Functype *func, CodeStack *stack)
{
  double z, w, v, res;
  CError err;

  err = stack_get_val(stack, &z, real);
  if (err)
    return(err);

  switch (func->num) {
  case MATH_BETA:
    err=stack_get_val(stack, &w, real);
    if (err)
      return(err);
    if (z < 0 || w < 0)
      return(c_badarg);
    v = z+w;
    res = exp(aln_gamma(z) + aln_gamma(w) - aln_gamma(z+w));
    break;
  case MATH_IGAMMA:
    err=stack_get_val(stack, &w, real);
    if (err)
      return(err);
    if (z < 0 || w <= 0)
      return(c_badarg);
    res = incgamma(w, z);
    break;
  case MATH_ERF:
    res = erf(z);
    break;
  case MATH_ERFC:
    res = erfc(z);
    break;
  case MATH_IBETA:
    err = stack_get_val2(stack, &w, &v, real);
    if (err)
      return(err);
    if (z < 0 || z > 1 || w <= 0 || v <= 0)
      return(c_badarg);
    res = incbeta(w, v, z);
    break;
  default:
    return(c_internal);
  }
  return stack_add_val(stack, &res, real);
}

/***********************************************************************
 *
 * FUNCTION:     polevl
 * 
 * DESCRIPTION:  Evaluate a polynomial given an array of its coefficients

 * PARAMETERS:   x, coeffs, degree N
 *
 * RETURN:       value of polynomial at x
 *      
 ***********************************************************************/
double polevl(double x, double *coef, int N )
{
  double p;
  int i;

  p = coef[N-1];
  for (i=N-2; i>=0; i--)
    p = p*x + coef[i];
  return(p);
}
