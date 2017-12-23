/*
 *   $Id: complex.cpp,v 1.2 2009/10/25 17:52:34 mapibid Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 2000 Ondrej Palkovsky
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
 *
 *  2000-10-09 - Rafael R. Sevilla <dido@pacific.net.ph>
 *               Modified the sine and cosine code to use a more sensible
 *               approximation.
 *  2000-10-13 - Rafael R. Sevilla <dido@pacific.net.ph>
 *               Added complex hyperbolic functions, and inverse trig. and
 *               hyperbolics.
 */
#include "stdafx.h"

#include "compat/PalmOS.h"

#include "compat/MathLib.h"
#include "konvert.h"
#include "stack.h"
#include "funcs.h"
#include "mathem.h"
#include "complex.h"
#include "defuns.h"
#include "core/prefs.h"

#define ALLOWED_ERROR 1E-15

/***********************************************************************
 *
 * FUNCTION:     cplx_to_gon
 * 
 * DESCRIPTION:  Convert an (a+bi) number to xe^(iy)
 *
 * PARAMETERS:   x - number in (a+bi) format
 *
 * RETURN:       number in r*e^(i*angle)
 *      
 ***********************************************************************/
Complex_gon
cplx_to_gon(Complex x)
{
	Complex_gon result;
	
	result.r = hypot(x.real,x.imag);
	result.angle = atan2(x.imag,x.real);

	return result;
}

/***********************************************************************
 *
 * FUNCTION:     cplx_multiply
 * 
 * DESCRIPTION:  Multiply complex numbers 
 *               (a+bi)*(c+di) = (a*c-b*d) + (a*d+b*c)i
 * PARAMETERS:   2 complex numbers
 *
 * RETURN:       result
 *      
 ***********************************************************************/
Complex
cplx_multiply(Complex arg1,Complex arg2)
{
	Complex result;
	
	result.real = arg1.real*arg2.real-arg1.imag*arg2.imag;
	result.imag = arg1.real*arg2.imag+arg1.imag*arg2.real;
	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     cplx_div
 * 
 * DESCRIPTION:  Division in complex numbers
 *              ! doesn't check for division by zero
 *
 * PARAMETERS:   arg1, arg2
 *
 * RETURN:       arg1/arg2
 *      
 ***********************************************************************/
Complex
cplx_div(Complex arg1,Complex arg2)
{
	Complex result;	
	double tmp;
	
	tmp = arg2.real*arg2.real + arg2.imag*arg2.imag;
	result.real = (arg1.real*arg2.real+arg1.imag*arg2.imag)/tmp;
	result.imag = (arg1.imag*arg2.real-arg1.real*arg2.imag)/tmp;
	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     cplx_power
 * 
 * DESCRIPTION:  base^expon computation, base & expon are complex
 *               z = xe^(iy)
 *               z^w = (xe^(iy))^w = x^w*e^(iyw) =
 *               = e^(w*ln(x))*e^(iyw) = e^(w*(ln(x)+yi))
 *
 * PARAMETERS:   2 complex numbers
 *
 * RETURN:       complex number
 *      
 ***********************************************************************/
static Complex
cplx_power(Complex base,Complex expon) MLIB;
static Complex
cplx_power(Complex base,Complex expon)
{
	Complex_gon gtmp;
	Complex tmp,result;
	
    // If expon is integral and is within some bounds, use another computation method
    // which will be more precise. This is coming from free42.
    if (FlpIsZero(expon.imag) && (expon.real == floor(expon.real))
        && (expon.real >= -2147483647.0) && (expon.real <= 2147483647.0)) {
        int w = (int) (expon.real);
        if (FlpIsZero(base.real) && FlpIsZero(base.imag)) {
            if (w < 0) {
                double tmp = 1.0;
                result.real = tmp / 0.0; // +infinity
                result.imag = 0.0;
                return (result);
            } else if (w > 0) {
                result.real = 0.0;
                result.imag = 0.0;
                return (result);
            }
        }
        if (w == 0) {
            result.real = 1.0;
            result.imag = 0.0;
            return (result);
        }

        double zr = base.real;
        double zi = base.imag;
        if (w < 0) {
            double h = _hypot(zr, zi);
            zr = zr / h / h;
            zi = (-zi) / h / h;
            w = -w;
        }

		double res_r = 1;
		double res_i = 0;
        double tmp;
		while (1) {
		    if ((w & 1) != 0) {
			    tmp = res_r * zr - res_i * zi;
                res_i = res_r * zi + res_i * zr;
                res_r = tmp;
                /* TODO: can one component be infinite while
                 * the other is zero? If yes, how do we handle
                 * that?
                 */
                if (isinf(res_r) && isinf(res_i))
                    break;
                if ((res_r == 0.0) && (res_i == 0.0))
                    break;
		    }
		    w >>= 1;
            if (w == 0)
                break;
            tmp = zr * zr - zi * zi;
            zi = 2 * zr * zi;
            zr = tmp;
        }
        result.real = res_r;
        result.imag = res_i;
    } else {
        gtmp=cplx_to_gon(base);
        tmp.real = log(gtmp.r);
        tmp.imag = gtmp.angle;

        tmp = cplx_multiply(expon,tmp);

        result.real = exp(tmp.real) * cos(tmp.imag);
        result.imag = exp(tmp.real) * sin(tmp.imag);
    }

    return result;
}

/***********************************************************************
 *
 * FUNCTION:     cplx_add
 * 
 * DESCRIPTION:  the '+' with complex numbers
 *
 * PARAMETERS:   arg1, arg2
 *
 * RETURN:       arg1 + arg2
 *      
 ***********************************************************************/
Complex
cplx_add(Complex arg1,Complex arg2)
{
	Complex result;
	
	result.real = arg1.real + arg2.real;
	result.imag = arg1.imag + arg2.imag;
	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     cplx_sub
 * 
 * DESCRIPTION:  the '-' with complex numbers
 *
 * PARAMETERS:   arg1, arg2
 *
 * RETURN:       arg1 - arg2
 *      
 ***********************************************************************/
Complex
cplx_sub(Complex arg1,Complex arg2)
{
	Complex result;

	result.real = arg1.real - arg2.real;
	result.imag = arg1.imag - arg2.imag;

	return result;
}

/***********************************************************************
 *
 * FUNCTION:     cplx_exp
 * 
 * DESCRIPTION:  Complex Exponential
 * PARAMETERS:   arg
 *
 * RETURN:       exp(arg)
 *      
 ***********************************************************************/
Complex
cplx_exp(Complex carg)
{
        Complex result;
	
        result.real = exp(carg.real) * cos(carg.imag);
	result.imag = exp(carg.real) * sin(carg.imag);
	return(result);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_log
 * 
 * DESCRIPTION:  Complex Logarithm
 * PARAMETERS:   arg
 *
 * RETURN:       log(arg)
 *
 ***********************************************************************/
static Complex cplx_log(Complex carg) MLIB;
static Complex
cplx_log(Complex carg)
{
        Complex_gon tmp;
	Complex result;

	tmp = cplx_to_gon(carg);
	result.real = log(tmp.r);
	result.imag = tmp.angle;
	
	return(result);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_sqrt
 * 
 * DESCRIPTION:  Complex square root
 *
 * PARAMETERS:   arg
 *
 * RETURN:       sqrt(arg)
 *      
 ***********************************************************************/
Complex cplx_sqrt(Complex arg) MLIB;
Complex
cplx_sqrt(Complex arg)
{
    Complex result;

    if (FlpIsZero(arg.imag)) {
        if (signbit(arg.real)) {
            result.real = 0.0;
            result.imag = sqrt(-arg.real);
        } else {
            result.real = sqrt(arg.real);
            result.imag = 0.0;
        }
    } else {
       Complex_gon tmp;

       tmp=cplx_to_gon(arg);
       result.real = sqrt(tmp.r)*cos(tmp.angle/2);
       result.imag = sqrt(tmp.r)*sin(tmp.angle/2);
    }
    return(result);
}

/* a complex 1.0, used throughout for calculations */
static const Complex cplx_one = { 1.0, 0.0 };
/***********************************************************************
 *
 * FUNCTION:     cplx_cosh
 * 
 * DESCRIPTION:  Complex Hyperbolic Cosine, uses exponential definition:
 *               cosh(z) = (exp(z) + exp(-z))/2
 * PARAMETERS:   arg
 *
 * RETURN:       cosh(arg)
 *      
 ***********************************************************************/
static Complex cplx_cosh(Complex arg) MLIB;
static Complex
cplx_cosh(Complex arg)
{
        Complex r1, r2, result;

	r1 = r2 = cplx_exp(arg);
	r1 = cplx_div(cplx_one, r1);
	result = cplx_add(r1, r2);
	result.real *= 0.5;
	result.imag *= 0.5;
	return(result);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_sinh
 * 
 * DESCRIPTION:  Complex Hyperbolic Sine, uses exponential definition:
 *               sinh(z) = (exp(z) - exp(-z))/2
 * PARAMETERS:   arg
 *
 * RETURN:       sinh(arg)
 *      
 ***********************************************************************/
static Complex cplx_sinh(Complex arg) MLIB;
static Complex
cplx_sinh(Complex arg)
{
        Complex r1, r2, result;
        r1 = arg;

	r1 = r2 = cplx_exp(arg);
	r1 = cplx_div(cplx_one, r1);
	r2 = cplx_exp(arg);
	result = cplx_sub(r2, r1);
	result.real *= 0.5;
	result.imag *= 0.5;
	return(result);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_cos
 * 
 * DESCRIPTION:  Complex Cosine, uses exponential definition:
 *               cos(x+iy) = (exp(-jx)*exp(y) + exp(jx)*exp(-y))*0.5
 * PARAMETERS:   arg
 *
 * RETURN:       cos(arg)
 *      
 ***********************************************************************/
static Complex cplx_cos(Complex arg) MLIB;
static Complex
cplx_cos(Complex arg)
{
        Complex r1, r2, result;
	double expim;

	r1.real = r2.real = cos(arg.real);
	r1.imag = sin(arg.real);
	r2.imag = -r1.imag;
	expim = exp(arg.imag);
	r1.real /= expim;
	r1.imag /= expim;
	r2.real *= expim;
	r2.imag *= expim;
	result = cplx_add(r1, r2);
	result.real *= 0.5;
	result.imag *= 0.5;
	return(result);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_sin
 * 
 * DESCRIPTION:  complex sine, uses exponential definition:
 *               sin(x+iy) = (exp(-jx)exp(y) - exp(jx)*exp(-y))*0.5j
 *
 * PARAMETERS:   arg
 *
 * RETURN:       sin(arg)
 *      
 ***********************************************************************/
static Complex cplx_sin(Complex arg) MLIB;
static Complex
cplx_sin(Complex arg)
{
        Complex r1, r2, result;
	double expim, tmp;

	r1.real = r2.real = cos(arg.real);
	r1.imag = sin(arg.real);
	r2.imag = -r1.imag;
	expim = exp(arg.imag);
	r1.real /= -expim;
	r1.imag /= -expim;
	r2.real *= expim;
	r2.imag *= expim;
	result = cplx_add(r1, r2);
	tmp = result.real * 0.5;
	result.real = -result.imag * 0.5;
	result.imag = tmp;
	return(result);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_asin
 * 
 * DESCRIPTION:  complex arcsine, returns only value of principal branch:
 *               asin(z) = -i*ln(iz + sqrt(1 - z^2))
 *
 * PARAMETERS:   arg
 *
 * RETURN:       asin(arg)
 *      
 ***********************************************************************/
static Complex cplx_asin(Complex arg) MLIB;
static Complex
cplx_asin(Complex arg)
{
  Complex a, b;

  a = cplx_multiply(arg, arg);
  a = cplx_sub(cplx_one, a);
  a = cplx_sqrt(a);
  b.real = -arg.imag;
  b.imag = arg.real;
  a = cplx_add(a, b);
  a = cplx_log(a);
  b.real = a.imag;
  b.imag = -a.real;
  return(b);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_acos
 * 
 * DESCRIPTION:  complex arccosine, returns only value of principal branch:
 *               acos(z) = -i*ln(z + sqrt(z^2 - 1))
 *
 * PARAMETERS:   arg
 *
 * RETURN:       acos(arg)
 *      
 ***********************************************************************/
static Complex cplx_acos(Complex arg) MLIB;
static Complex
cplx_acos(Complex arg)
{
  Complex a, b;

  a = cplx_multiply(arg, arg);
  a = cplx_sub(a, cplx_one);
  a = cplx_sqrt(a);
  a = cplx_add(a, arg);
  a = cplx_log(a);
  b.real = a.imag;
  b.imag = -a.real;
  return(b);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_atan
 * 
 * DESCRIPTION:  complex arctangent, returns only value of principal branch:
 *               atan(z) = -0.5*i*ln((1+i*z)/(1-i*z))
 *
 * PARAMETERS:   arg
 *
 * RETURN:       atan(arg)
 *      
 ***********************************************************************/
static Complex cplx_atan(Complex arg) MLIB;
static Complex
cplx_atan(Complex arg)
{
  Complex a, b;

  a.real = -arg.imag;
  a.imag = arg.real;
  b = cplx_sub(cplx_one, a);
  a = cplx_add(cplx_one, a);
  b = cplx_div(a, b);
  a = cplx_log(b);
  b.real = 0.5*a.imag;
  b.imag = -0.5*a.real;
  return(b);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_asinh
 * 
 * DESCRIPTION:  complex hyperbolic arcsine
 *               asin(z) = ln(z + sqrt(z^2 + 1))
 *
 * PARAMETERS:   arg
 *
 * RETURN:       asinh(arg)
 *      
 ***********************************************************************/
static Complex cplx_asinh(Complex arg) MLIB;
static Complex
cplx_asinh(Complex arg)
{
  Complex a;

  a = cplx_multiply(arg, arg);
  a = cplx_add(cplx_one, a);
  a = cplx_sqrt(a);
  a = cplx_add(a, arg);
  return(cplx_log(a));
}

/***********************************************************************
 *
 * FUNCTION:     cplx_acosh
 * 
 * DESCRIPTION:  complex hyperbolic arccosine
 *               acosh(z) = ln(z + sqrt(z^2 - 1))
 *
 * PARAMETERS:   arg
 *
 * RETURN:       asin(arg)
 *      
 ***********************************************************************/
static Complex cplx_acosh(Complex arg) MLIB;
static Complex
cplx_acosh(Complex arg)
{
  Complex a;

  a = cplx_multiply(arg, arg);
  a = cplx_sub(a, cplx_one);
  a = cplx_sqrt(a);
  a = cplx_add(a, arg);
  return(cplx_log(a));
}

/***********************************************************************
 *
 * FUNCTION:     cplx_atanh
 * 
 * DESCRIPTION:  complex hyperbolic arctangent
 *               atan(z) = 0.5*ln((1+z)/(1-z))
 *
 * PARAMETERS:   arg
 *
 * RETURN:       atan(arg)
 *      
 ***********************************************************************/
static Complex cplx_atanh(Complex arg) MLIB;
static Complex
cplx_atanh(Complex arg)
{
  Complex a, b;

  a = cplx_add(cplx_one, arg);
  b = cplx_sub(cplx_one, arg);
  b = cplx_div(a, b);
  a = cplx_log(b);
  b.real = 0.5*a.real;
  b.imag = 0.5*a.imag;
  return(b);
}

/***********************************************************************
 *
 * FUNCTION:     cplx_oper
 *
 * DESCRIPTION:  Operations on complex numbers
 *
 * PARAMETERS:   On stack - 2 arguments convertible to complex
 *
 * RETURN:       On stack - 1 real number
 *      
 ***********************************************************************/
CError
cplx_oper(Functype *func,CodeStack *stack)
{
	Complex arg1,arg2;
	Complex result;
	CError err;
	
	if ((err=stack_get_val2(stack,&arg1,&arg2,complex)))
	  return err;

	switch (func->num) {
	 case FUNC_PLUS:
		result.real=arg1.real+arg2.real;
		result.imag=arg1.imag+arg2.imag;
		break;
	 case FUNC_MINUS:
		result.real=arg1.real-arg2.real;
		result.imag=arg1.imag-arg2.imag;
		break;
	 case FUNC_DIVIDE: 
		//if (arg2.real==0.0 && arg2.imag==0.0)
		  //return c_divbyzero;
		result = cplx_div(arg1,arg2);
		break;
	 case FUNC_MULTIPLY:
		result = cplx_multiply(arg1,arg2);
		break;
	 case FUNC_POWER:
//		 arg2.imag = math_user_to_rad(arg2.imag);
		 result = cplx_power(arg1,arg2);
		 break;
	 case FUNC_MOD:
		if (arg1.imag!=0.0 || arg2.imag!=0.0)
		  return c_badarg;
		result.real = fmod(arg1.real,arg2.real);
		result.imag = 0.0;		
		break;
	 default:
		return c_badfunc;
	}       

	if (calcPrefs.reducePrecision && finite(result.imag)) {

	   if (fabs(result.real) < PRECISION_TO_ZERO)
		result.real= 0.0;
	   else
		result.real = reduce_precision(result.real);

	   if (fabs(result.imag) < PRECISION_TO_ZERO)
		result.imag= 0.0;
	   else
		result.imag = reduce_precision(result.imag);
	}
	
	err=stack_add_val(stack,&result,complex);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     cplx_math
 * 
 * DESCRIPTION:  General mathematical functions on complex numbers
 *
 * PARAMETERS:   On stack - 1 number, convertible to complex
 *
 * RETURN:       On stack - 1 real or complex result
 *      
 ***********************************************************************/
CError
cplx_math(Functype *func,CodeStack *stack)
{
	Complex carg,result;
	double res;
	CError err;
	
	err=stack_get_val(stack,&carg,complex);
	if (err)
		return err;
	
	switch (func->num) {

	case FUNC_CONJ:
		result.real = carg.real;
		result.imag = -carg.imag;
		return stack_add_val(stack,&result,complex);
	case MATH_ABS:		
		res = hypot(carg.real,carg.imag);
		return stack_add_val(stack,&res,real);		  
	case MATH_ANGLE:
		res = atan2(carg.imag, carg.real);
		res = math_rad_to_user(res);
		return stack_add_val(stack,&res,real);		  
	case MATH_REAL:
		res = carg.real;
		return stack_add_val(stack,&res,real);		  
	case MATH_IMAG:
		res = carg.imag;
		return stack_add_val(stack,&res,real);		  

	case MATH_EXP:
//		carg.imag = math_user_to_rad(carg.imag);
		result = cplx_exp(carg);
		break;
	case MATH_SQRT:
		result = cplx_sqrt(carg);
		break;

	case MATH_LN:
		result = cplx_log(carg);
		result.imag = math_rad_to_user(result.imag);
		break;
	case MATH_LOG:
		result = cplx_log(carg);
		result.imag = math_rad_to_user(result.imag);
		result.real/=log(10);
		result.imag/=log(10);
		break;
	case MATH_LOG2:
		result = cplx_log(carg);
		result.imag = math_rad_to_user(result.imag);
		result.real/=log(2);
		result.imag/=log(2);
		break;

	case MATH_SIN:
		carg.real = math_user_to_rad(carg.real);
		result = cplx_sin(carg);
		break;
	case MATH_COS:
		carg.real = math_user_to_rad(carg.real);
		result = cplx_cos(carg);
		break;
	case MATH_TAN:
		carg.real = math_user_to_rad(carg.real);
		/* We really should have a separate function for this (and
		   the hyperbolic tangent), but we're now weighing speed
		   versus code size, and this is certainly smaller. */
		result = cplx_div(cplx_sin(carg), cplx_cos(carg));
		break;
	case MATH_SINH:
		result = cplx_sinh(carg);
		break;
	case MATH_COSH:
		result = cplx_cosh(carg);
		break;
	case MATH_TANH:
		result = cplx_div(cplx_sinh(carg),cplx_cosh(carg));
		break;

	/* These inverse trigonometric and hyperbolic functions only yield
	   the value at the principal branch. */
	case MATH_ASIN:
		result = cplx_asin(carg);
		result.real = math_rad_to_user(result.real);
		break;
	case MATH_ACOS:
		result = cplx_acos(carg);
		result.real = math_rad_to_user(result.real);
		break;
	case MATH_ATAN:
		result = cplx_atan(carg);
		result.real = math_rad_to_user(result.real);
		break;
	case MATH_ASINH:
		result = cplx_asinh(carg);
		break;
	case MATH_ACOSH:
		result = cplx_acosh(carg);
		break;
	case MATH_ATANH:
		result = cplx_atanh(carg);
		break;
	case MATH_FLOOR:
	case MATH_CEIL:
		res=carg.real;
		stack_add_val(stack,&res,real);	
		if ((err=math_math(func,stack)))
			return err;
		if ((err=stack_get_val(stack,&res,real)))
			return err;		
		result.real=res;
		res=carg.imag;
		stack_add_val(stack,&res,real);	
		if ((err=math_math(func,stack)))
			return err;
		if ((err=stack_get_val(stack,&res,real)))
			return err;		
		result.imag=res;
		break;
	default:
		return c_badarg;
	}       

	if (calcPrefs.reducePrecision && finite(result.imag)) {
	      if (fabs(result.real) < PRECISION_TO_ZERO)
			result.real= 0.0;
		else
			result.real = reduce_precision(result.real);
	      if (fabs(result.imag) < PRECISION_TO_ZERO)
			result.imag= 0.0;
		else
			result.imag = reduce_precision(result.imag);
	}

	err=stack_add_val(stack,&result,complex);
	return err;	   	
}

/***********************************************************************
 *
 * FUNCTION:     cplx_round
 * 
 * DESCRIPTION:  Compute round or trunc on complex numbers
 *
 * PARAMETERS:   On stack 1 or 2 arguments 
 *			(first - complex number ; second - no. of decimal points)
 *
 * RETURN:       On stack 1 complex result
 *      
 ***********************************************************************/
CError
cplx_round(Functype *func,CodeStack *stack)
{
	Complex carg;
	double arg,precision;
	CError err;

	if(func->paramcount == 2)
		if ((err=stack_get_val(stack,&precision,real)))
			return err;
	if ((err=stack_get_val(stack,&carg,complex)))
		return err;
	
	arg=carg.real;
	stack_add_val(stack,&arg,real);	
	if(func->paramcount == 2)
		stack_add_val(stack,&precision,real);	
	if ((err=math_round(func,stack)))
		return err;
	if ((err=stack_get_val(stack,&arg,real)))
		return err;		
	carg.real=arg;

	arg=carg.imag;
	stack_add_val(stack,&arg,real);	
	if(func->paramcount == 2)
		stack_add_val(stack,&precision,real);	
	if ((err=math_round(func,stack)))
		return err;
	if ((err=stack_get_val(stack,&arg,real)))
		return err;		
	carg.imag=arg;

	err=stack_add_val(stack,&carg,complex);
	return err;	   	
}