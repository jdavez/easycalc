/*
 *   $Id: mathem.c,v 1.38 2007/12/19 15:39:21 cluny Exp $
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
 *
 *  2000-10-11 - Rafael R. Sevilla <dido@pacific.net.ph>
 *               Added code to compute the Euler gamma function and modified
 *               the factorial code to use it for non-integral arguments.
 *  2000-10-13 - Rafael R. Sevilla <dido@pacific.net.ph>
 *               Added hooks for complex hyperbolic functions, and complex
 *               inverse circular and hyperbolic functions.
 *  2001-12-08 - John Hodapp <bigshot@email.msn.com>
 *               - Added function to convert rad/deg/grd for non-period trig
 *               functions. Add call to math_trigo.
 *               (an interesting side note is that most other calculators make the
 *                explicit assumtion of Radians for hyperbolic functions even if in
 *                Degree mode.  This calculator inputs/outputs based on mode setting).
 *               - Commented out set to 0 for reduce precision in  math_arctrig
 *               as was causing problem with atanh (see comments in that routine).
 *               "Per bug fix request w/r cosh and sinh".  
 *               - Fixes/precision increase for Gamma coeficients.
 *               "Per bug fix request for integers > 3".
 */

#include <PalmOS.h>

#include "MathLib.h"
#include "konvert.h"
#include "funcs.h"
#include "mathem.h"
#include "stack.h"
#include "defuns.h"
#include "prefs.h"
#include "complex.h"
#include "matrix.h"
#include "cmatrix.h"
#include "display.h"

#define MAX_FACT 33
#define MAX_INT_FACT 100

/***********************************************************************
 *
 * FUNCTION:     math_user_to_rad_mod
 * 
 * DESCRIPTION:  Convert user selected degree system to radians
 *
 * PARAMETERS:   Degrees/Grad/Radians angle
 *
 * RETURN:       Radians angle
 *
 * 8Dec2001 used by periodic trig functions     
 ***********************************************************************/
double
math_user_to_rad_mod(double angle)
{
	if (calcPrefs.trigo_mode==degree) 
		return fmod(angle,360.0) * (M_PIl/180);
	else if (calcPrefs.trigo_mode==grad) 
		return fmod(angle,400.0) * (M_PIl/200);
	else
		return fmod(angle,(2*M_PIl));
}

/***********************************************************************
 *
 * FUNCTION:     math_user_to_rad
 * 
 * DESCRIPTION:  Convert user selected degree system to radians
 *
 * PARAMETERS:   Degrees/Grad/Radians angle
 *
 * RETURN:       Radians angle
 *
 * 8Dec2001 used by non-periodic trig functions     
 ***********************************************************************/
double
math_user_to_rad(double angle)
{
	if (calcPrefs.trigo_mode==degree) 
		return (angle * M_PIl / 180.0);
	else if (calcPrefs.trigo_mode==grad) 
		return (angle * M_PIl / 200.0);
	else
		return angle;
}

/***********************************************************************
 *
 * FUNCTION:     math_rad_to_user
 * 
 * DESCRIPTION:  Convert radians to user selected degree system
 *
 * PARAMETERS:   Radians angle
 *
 * RETURN:       Degrees/Grad/Radians angle
 *      
 ***********************************************************************/
double 
math_rad_to_user(double angle)
{
	if (calcPrefs.trigo_mode==degree) 
		return (angle * 180.0 / M_PIl);
	else if (calcPrefs.trigo_mode==grad) 
		return (angle * 200.0 / M_PIl);
	else
		return angle;
}

/***********************************************************************
 *
 * FUNCTION:     math_cmpeq
 * 
 * DESCRIPTION:  Compare function 'equal'
 *
 * PARAMETERS:   On stack - 2 arguments, convertible to complex
 *               complex nums are comparable
 *               There should be a type detection to allow
 *               comparing other types then numbers
 *
 * RETURN:       On stack - 1 Integer result, 0 - false
 *      
 ***********************************************************************/
CError
math_cmpeq(Functype *func,CodeStack *stack)
{
	Complex arg1,arg2;
	UInt32 result;
	CError err;

	if ((err=stack_get_val2(stack,&arg1,&arg2,complex)))
		return err;
	
	result = IS_ZERO(arg1.real-arg2.real) && IS_ZERO(arg1.imag-arg2.imag);
	return stack_add_val(stack,&result,integer);
}

/***********************************************************************
 *
 * FUNCTION:     math_cmp
 * 
 * DESCRIPTION:  Compare functions
 *
 * PARAMETERS:   On stack - 2 arguments, convertible to double
 *               Integer gets converted without problem
 *
 * RETURN:       On stack - 1 result
 *      
 ***********************************************************************/
CError
math_cmp(Functype *func,CodeStack *stack)
{
	double arg1,arg2;
	UInt32 result;
	CError err;

	if ((err=stack_get_val2(stack,&arg1,&arg2,real)))
		return err;	

	switch (func->num) {
	case FUNC_GRTHEN:
		result = arg1 > arg2;
		break;
	case FUNC_LETHEN:
		result = arg1 < arg2;
		break;
	case FUNC_GREQ:
		result = arg1 >= arg2;
		break;
	case FUNC_LEEQ:
		result = arg1 <= arg2;
		break;
	default:
		return c_internal;
	}
	return stack_add_val(stack,&result,integer);
}

/***********************************************************************
 *
 * FUNCTION:     math_bin
 * 
 * DESCRIPTION:  Binary aritmetics
 *
 * PARAMETERS:   On stack - 2 arguments, convertible to integer
 *
 * RETURN:       On stack - 1 result
 *      
 ***********************************************************************/
CError 
math_bin(Functype *func,CodeStack *stack)
{
	UInt32 arg1,arg2;
	UInt32 result;
	CError err;
	
	if ((err=stack_get_val2(stack,&arg1,&arg2,integer)))
	  return err;	
	switch (func->num) {
	 case FUNC_AND:
		result=arg1 & arg2;
		break;
	 case FUNC_OR:
		result=arg1 | arg2;
		break;
	 case FUNC_XOR: 
		result=arg1 ^ arg2;
		break;
	 case FUNC_SHL:
		result=arg1 << arg2;
		break;
	 case FUNC_SHR:
		result=arg1 >> arg2;
		break; 
	 default:
		return c_internal;
	}
	return stack_add_val(stack,&result,integer);
}

/***********************************************************************
 *
 * FUNCTION:     int_oper
 * 
 * DESCRIPTION:  Integer operations
 *
 * PARAMETERS:   On stack - 2 arguments, convertible to integer
 *
 * RETURN:       On stack - 1 type integer
 *      
 ***********************************************************************/
static CError 
int_oper(Functype *func,CodeStack *stack) MLIB;
static CError 
int_oper(Functype *func,CodeStack *stack)
{
	Int32 arg1, arg2;
	Int32 tmp;
	Int32 result;
	CError err;
	
	if ((err = stack_get_val2(stack, &arg1, &arg2, integer)))
		return err;
	switch (func->num) {
	 case FUNC_PLUS:
		result = arg1 + arg2;
		break;
	 case FUNC_MINUS:
		result = arg1 - arg2;
		break;
	 case FUNC_DIVIDE:
		if (!arg2)
			return c_divbyzero;
		result = arg1 / arg2;
		break;
	 case FUNC_MOD:
		if (!arg2)
			return c_divbyzero;
		result = arg1 % arg2;
		break;
	 case FUNC_MULTIPLY:
		result = arg1 * arg2;
		break;
	 case FUNC_POWER:
		if (arg2 == 0) {
			result = 1;
			break;
		}
		if (arg2 < 0)
			return c_badarg;
		tmp = arg1;
		while (--arg2)
			tmp *= arg1;
		result = tmp;
		break;
	 default:
		return c_badfunc;
	}       
	return stack_add_val(stack, &result, integer);
}

/***********************************************************************
 *
 * FUNCTION:     real_oper
 * 
 * DESCRIPTION:  Operations on real numbers
 *
 * PARAMETERS:   On stack - 2 numbers convertible to real
 *
 * RETURN:       On stack - 1 real number
 *      
 ***********************************************************************/
static CError
real_oper(Functype *func,CodeStack *stack) MLIB;
static CError
real_oper(Functype *func,CodeStack *stack)
{
	double arg1,arg2;
	double result;
	CError err;

	if ((err=stack_get_val2(stack,&arg1,&arg2,real)))
		return err;
	switch (func->num) {
	 case FUNC_PLUS:
		result=arg1+arg2;
		break;
	 case FUNC_MINUS:
		result=arg1-arg2;
		break;
	 case FUNC_DIVIDE:
		if (arg2==0)
			//return c_divbyzero;
			result=!isnan(arg1)*sgn(arg1)/arg2;
		else
			result=arg1/arg2;
		break;
	 case FUNC_MOD:
		if (arg2==0)
			return c_divbyzero;
		result =fmod(arg1,arg2);
		break;
	 case FUNC_MULTIPLY:
		result=arg1*arg2;
		break;
	 case FUNC_POWER:
	   /* Raise a negative number to a nonintegral power to fall to
	      complex math. */
	   if (arg1 < 0.0 && (!IS_ZERO(arg2-round(arg2)))) {
	   	err = stack_add_val(stack, &arg1, real);
		if (err)
			return err;
	   	err = stack_add_val(stack, &arg2, real);
	   	return(err ? err : cplx_oper(func, stack));
	   }

	   result=pow(arg1,arg2);
	   break;
	 default:
		return c_badfunc;
	}

	if (calcPrefs.reducePrecision) 
		result = reduce_precision(result);

	return stack_add_val(stack,&result,real);
}

/***********************************************************************
 *
 * FUNCTION:     math_oper
 * 
 * DESCRIPTION:  Generic function, that selects using some criteria
 *               which function (cplx_oper,real_oper,int_oper) 
 *               should perform the requested operation
 * 
 * NOTE:         Doesn't modify the stack
 * 
 * PARAMETERS:   Detects type of 2 arguments
 *
 * RETURN:       Adds 1 object on stack
 *      
 ***********************************************************************/
CError 
math_oper(Functype *func,CodeStack *stack)
{
	CError err;
	rpntype type1, type2;
	
	err = stack_item_type(stack,&type1,1);
	if (err)
	  return err;
	err = stack_item_type(stack,&type2,0);
	if (err)
	  return err;

#ifdef SPECFUN_ENABLED
	if ((type1 == matrix && type2 == cmatrix) ||
		 (type1 == cmatrix && type2 == matrix) ||
		 (type1 == cmatrix && type2 == cmatrix))
		return cmatrix_oper(func,stack);
	else if ((type1 == cmatrix && type2 != cmatrix) ||
		 (type1 != cmatrix && type2 == cmatrix) ||
		 (type1 == matrix && type2 == complex) ||
		 (type1 == complex && type2 == matrix))
		return cmatrix_oper_cplx(func,stack);
	else if ((type1 == matrix && type2 != matrix) 
		 || (type1 != matrix && type2 == matrix)) 
		return matrix_oper_real(func,stack);
	else if (type1 == matrix && type2 == matrix)
		return matrix_oper(func,stack);
#endif

	/* if both arguments are integer, the user probably wanted 
	 * integer operations */
	if (dispPrefs.forceInteger || (type1==integer && type2==integer))
	  return int_oper(func,stack);
	else if (type1==complex || type2==complex)
	  return cplx_oper(func,stack);
	else 
	  return real_oper(func,stack);
}

/* Negate function, uses appropriate functions for types
 * Input - 1 real/integer, Output - 1 real/integer */
/***********************************************************************
 *
 * FUNCTION:     math_neg
 * 
 * DESCRIPTION:  Negates a number, preserving a type of argument
 *
 * PARAMETERS:   On stack - 1 number
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError 
math_neg(Functype *func,CodeStack *stack)
{
	CError err=0;
	rpntype type;
	
	err=stack_item_type(stack,&type,0);
	if (err)
		return err;
	
	if (func->num!=FUNC_NEGATE)
		return c_internal;
	if (type==real) {
		double darg;
		
		err=stack_get_val(stack,&darg,real);
		if (err)
			return err;
		darg = -darg;
		return stack_add_val(stack,&darg,real);
	}
	else if (type==integer) {
		UInt32 iarg;
		
		err=stack_get_val(stack,&iarg,integer);
		if (err) 
			return err;
		iarg = -iarg;
		return stack_add_val(stack,&iarg,integer);		
	}
	else if (type==complex) {
		Complex carg;
		
		err = stack_get_val(stack,&carg,complex);
		if (err)
			return err;
		carg.real = -carg.real;
		carg.imag = -carg.imag;
		return stack_add_val(stack,&carg,complex);
	}
	else
		return c_badarg;
}

/* Simulation of logb(), as the MathLib's one returns bad result */
static double logbin(double arg) MLIB;
static double
logbin(double arg) {
	return log(arg)/log(2);
}

static double fpart(double arg) MLIB;
static double
fpart(double arg) {
	return arg-trunc(arg);
}
	  

double 
sgn(double x)
{
   if (x == 0.0)
       return 0;
   else if (x < 0.0)
       return -1.0;
   else 
       return 1.0;
}

/* Read the const names in mathem.h, they poInt16 in this array */
const struct {
	double (*func)(double);
}math_func[]={
	{sin},{cos},{tan},{sinh},{cosh},{tanh},
	{asin},{acos},{atan},{asinh},{acosh},{atanh},
	{exp},{log},{log10},{sqrt},{cbrt},{fabs},{logbin},
	{fpart},{floor},{sgn},{ceil}
};

/***********************************************************************
 *
 * FUNCTION:     math_arctrigo
 * 
 * DESCRIPTION:  Perform computation of functions, that do 
 *               degree/radian/grad conversion on output
 *               asin,acos,atan,asinh,acosh,atanh
 *
 * PARAMETERS:   On stack - 1 number
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError
math_arctrigo(Functype *func,CodeStack *stack)
{
	double arg,res;
	CError err;
	rpntype type;

	err = stack_item_type(stack,&type,0);
	if (err)
	  return err;

	/* complex is defined for these operations */
	if ((func->num==MATH_ASIN || func->num==MATH_ACOS ||
	     func->num==MATH_ATAN ||
	     func->num==MATH_ASINH || func->num==MATH_ACOSH ||
	     func->num==MATH_ATANH) &&
	    type==complex)
	  return  cplx_math(func,stack);

	err=stack_get_val(stack,&arg,real);
	if (err)
	  return err;

	/* Fall to complex results for asin and acos if argument is
	   abs(arg) is greater than 1. */
	if (fabs(arg) > 1.0 && 
	    (func->num == MATH_ASIN || func->num == MATH_ACOS)) {
		err = stack_add_val(stack, &arg, real);
		return(err ? err : cplx_math(func, stack));
	}
	
	res=math_func[func->num].func(arg);
	if (func->num == MATH_ASIN || func->num == MATH_ACOS ||
	    func->num == MATH_ATAN)
		res = math_rad_to_user(res);

	/* 8Dec2001 COMMENTED OUT below as caused problem with atanh (>=) +/- 1 with reduce precision on
	   should be either + or - inf if 1, NAN if outside this range */
	/* Avoid the sin(pi) = 1E-15 problem */
	/*if (calcPrefs.reducePrecision && fabs(res) < PRECISION_TO_ZERO)
	  res = 0.0;
	*/

	return stack_add_val(stack,&res,real);
}

#define GAMMPARM 5
#define SQRT2PI 2.5066282746310005024

/* Euler Gamma function.  Uses Lanczos' approximation as described
   in Numerical Recipes.  Applies the gamma function reflection formula for
   arguments less than 1. */
/* Updated gammacoeffs 8Dec2001 - improved accuracy for N>3 */
double euler_gamma(double z)
{
  double x, tmp, ser, gam;
  int j;
  static double gammacoeffs[] = { 76.18009172947146, -86.50532032941677, 24.01409824083091,
				  -1.231739572450155, 0.1208650973866179e-2, -0.5395239384953e-5 };

  x = (z < 1) ? (1-z) : z;
  x--;
  tmp = x + GAMMPARM + 0.5;
  tmp = pow(tmp, x+0.5)*exp(-tmp);
  ser = 1.000000000190015;
  for (j=0; j<6; j++) {
    x++;
    ser += gammacoeffs[j]/x;
  }
  gam = tmp*ser*SQRT2PI;
  /* apply the reflection formula if needed */
  if (z < 1)
    gam = M_PIl/(gam*sin(M_PIl*(1-z)));
  return(gam);
}

/***********************************************************************
 *
 * FUNCTION:     math_gamma
 * 
 * DESCRIPTION:  Euler gamma function
 *
 * PARAMETERS:   On stack - 1 number
 *
 * RETURN:       On stack - 1 number
 * 
 ***********************************************************************/
CError math_gamma(Functype *func, CodeStack *stack)
{
	double arg;
	double res;	
	CError err;

	err=stack_get_val(stack,&arg,real);
	if (err)
	  return err;
 	/* return error if arg > MAX_FACT or if the argument is a negative
	   integer. */
	if (arg>MAX_FACT)
	  return c_range;
	if (IS_ZERO(arg-round(arg)) && arg <= 0)
	  return c_badarg;	/* negative int or too big */
	res = euler_gamma(arg);
	return stack_add_val(stack,&res,real);
}

/***********************************************************************
 *
 * FUNCTION:     math_fact
 * 
 * DESCRIPTION:  Factorial
 *               This now uses the Euler gamma function if non integral
 *               arguments are passed.  Error only if the argument is a
 *               negative number.
 *
 * PARAMETERS:   On stack - 1 number
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError math_fact(Functype *func,CodeStack *stack)
{	
	double arg;
	double res=1.0;	
	CError err;
		
	err=stack_get_val(stack,&arg,real);
	if (err) 
	  return err;

	/* Return error if integer > MAX_INT_FACT or non-integer > MAX_FACT */
	if (arg>MAX_FACT && (arg>MAX_INT_FACT || !IS_ZERO(arg-round(arg))))
	  return c_range;
	
	/* Return error if negative integer */
	if (IS_ZERO(arg-round(arg)) && arg < 0)
	  return c_badarg;    

	if (!IS_ZERO(arg-round(arg))) {
	        res=euler_gamma(arg+1.0);
		return stack_add_val(stack,&res,real);
	}
	
	/* go back to basics and use the usual computations for integral
	   arguments.  */
	while (arg>0.0) {
		res*=arg;
		arg = arg - 1.0;
	}
	
	return stack_add_val(stack,&res,real);
}


/***********************************************************************
 *
 * FUNCTION:     math_trigo
 * 
 * DESCRIPTION:  Perform computation of functions, that do 
 *               degree/radian/grad conversion on input
 *               sin,cos,tan,sinh,cosh,tanh
 *
 * PARAMETERS:   On stack - 1 number
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError 
math_trigo(Functype *func,CodeStack *stack)
{
	double arg,res;
	CError err;
	rpntype type;
	
	err = stack_item_type(stack,&type,0);
	if (err)
	  return err;

	/* complex is defined for these operations */
	if ((func->num==MATH_SIN || func->num==MATH_COS ||
	     func->num==MATH_TAN ||
	     func->num==MATH_SINH || func->num==MATH_COSH ||
	     func->num==MATH_TANH) &&
	    type==complex)
	  return  cplx_math(func,stack);

	err=stack_get_val(stack,&arg,real);
	if (err)
	  return err;

	if (func->num==MATH_SIN || func->num==MATH_COS ||
	    func->num==MATH_TAN) 	
		arg = math_user_to_rad_mod(arg);
//	else if (func->num==MATH_SINH || func->num==MATH_COSH ||
//	     func->num==MATH_TANH) 	
//	  arg = math_user_to_rad(arg);

	res=math_func[func->num].func(arg);

	/* Avoid the sin(pi) = 1E-15 problem */
	if (calcPrefs.reducePrecision && finite(res) && fabs(res) < PRECISION_TO_ZERO)
		res = 0.0;

	return stack_add_val(stack,&res,real);
}

/***********************************************************************
 *
 * FUNCTION:     math_math
 * 
 * DESCRIPTION:  Perform computation of functions, that do not
 *               need any input/output conversion
 *              
 * PARAMETERS:   On stack - 1 number
 *
 * RETURN:       On stack - 1 number
 *      
 ***********************************************************************/
CError
math_math(Functype *func,CodeStack *stack)
{
	double arg,res;
	CError err;
	rpntype type;
	
	err = stack_item_type(stack,&type,0);
	if (err)
	  return err;
	
	/* complex is defined for these operations */
	if (/*(func->num==MATH_ABS || func->num==MATH_EXP ||
		func->num==MATH_SQRT || func->num==MATH_LN || func->num==MATH_LOG || 
		func->num==MATH_LOG2|| func->num==MATH_FLOOR || 
		func->num==MATH_CEIL) &&*/
		type==complex)
	  return  cplx_math(func,stack);

	err=stack_get_val(stack,&arg,real);
	if (err)
		return err;

	if (arg < 0.0 && (func->num==MATH_SQRT || func->num == MATH_LN ||
			  func->num==MATH_LOG || func->num == MATH_LOG2)) {
		err = stack_add_val(stack, &arg, real);
		return(err ? err : cplx_math(func, stack));
	}
	res=math_func[func->num].func(arg);
	return stack_add_val(stack,&res,real);
}

/* (int) functions - returns value with rpntype=integer*/
CError 
math_int(Functype *func,CodeStack *stack)
{
	UInt32 arg;
	CError err;
	
	err=stack_get_val(stack,&arg,integer);
	if (err)
	  return err;
	return stack_add_val(stack,&arg,integer);
}

CError
math_convcoord(Functype *func,CodeStack *stack)
{
	double arg1,arg2;
	double res;
	CError err;
	
	if ((err=stack_get_val2(stack,&arg1,&arg2,real)))
		return err;
	
	if (func->num==MATH_PTORX || func->num==MATH_PTORY) 
		arg2 = math_user_to_rad(arg2);
	
	switch (func->num) {
	case MATH_HYPOT:
		res = hypot(arg1,arg2);
		break;
	case MATH_RTOPD:
		res = atan2(arg2,arg1);
		res = math_rad_to_user(res);
		break;
	case MATH_ATAN2:
		res = atan2(arg1,arg2);
		res = math_rad_to_user(res);
		break;
	case MATH_PTORX:
		res = cos(arg2)*arg1;
		break;
	case MATH_PTORY:
		res = sin(arg2)*arg1;
		break;
	}
	return stack_add_val(stack,&res,real);
}

/***********************************************************************
 *
 * FUNCTION:     math_comb
 * 
 * DESCRIPTION:  Compute ncr(combination) and npr(permutation)
 *
 * PARAMETERS:   On stack 2 arguments
 *
 * RETURN:       On stack 1 argument
 *      
 ***********************************************************************/
CError
math_comb(Functype *func,CodeStack *stack)
{
	UInt32 arg1,arg2;
	UInt32 i;
	double res=1.0;
	CError err;
	
	if ((err=stack_get_val2(stack,&arg1,&arg2,integer)))
	  return err;
	
	if (arg2>arg1)
	  return c_badarg;
	if (arg1-arg2>MAX_FACT && arg2>MAX_FACT)
	  return c_range;
	
	for (i=arg1;i>arg1-arg2;i--)
	  res*=i;
	
	if (func->num==MATH_COMB) {
		for (i=1;i<=arg2;i++)
		  res/=(double)i;
	}
	
	return stack_add_val(stack,&res,real);
}

/***********************************************************************
 *
 * FUNCTION:     math_round
 * 
 * DESCRIPTION:  Compute round or trunc
 *
 * PARAMETERS:   On stack 1 or 2 arguments (second - no. of decimal points)
 *
 * RETURN:       On stack 1 argument
 *      
 ***********************************************************************/
CError
math_round(Functype *func,CodeStack *stack)
{
	double precision=0.0;
	double arg;
	CError err;
	rpntype type;

	if (func->paramcount == 2) {
		if ((err=stack_get_val(stack,&precision,real)))
			return err;
		if (!IS_ZERO(round(precision)-precision))
			return c_badarg;
		precision = round(precision);
	} else if (func->paramcount > 2) 
		return c_badargcount;

	err = stack_item_type(stack,&type,0);
	if (err)
		return err;
	
	if (type==complex){
		if(func->paramcount == 2)
			stack_add_val(stack,&precision,real);
		return  cplx_round(func,stack);
	}
	
	if ((err=stack_get_val(stack,&arg,real))) 
		return err;

	if (precision)
		arg = arg*pow(10.0,precision);
	if (func->num == MATH_ROUND)
		arg = round(arg);
	else if (func->num == MATH_TRUNC){
		if(!IS_ZERO(arg-round(arg)) || IS_ZERO(arg))
			arg=trunc(arg);}
	else
		return c_internal;
	if (precision)
		arg = arg/pow(10.0,precision);

	return stack_add_val(stack,&arg,real);
}

/***********************************************************************
 *
 * FUNCTION:     math_apostrophe
 * 
 * DESCRIPTION:  Handle the apostrophe operator, it works
 *               either as conjugate on complex nums or transpose
 *               for matrices
 *
 * PARAMETERS:   On stack 1 argument
 *
 * RETURN:       On stack 1 argument
 *      
 ***********************************************************************/
CError
math_apostrophe(Functype *func,CodeStack *stack)
{
	rpntype type;
	CError err;

	err = stack_item_type(stack,&type,0);
	if (err)
		return err;

#ifdef SPECFUN_ENABLED
	if (type == matrix) 
		/* Apostrophe is transpose */
		return matrix_func2(func,stack);
	else if (type == cmatrix)
		return cmatrix_func2(func,stack);
#endif
	
	/* Apostrophe is conjugate */
	return cplx_math(func,stack);
}

CError
math_seed(Functype *func,CodeStack *stack)
{
	CError err;
	UInt32 intarg;


	err = stack_get_val(stack,&intarg,integer);
	if (err)
		return err;
	if (intarg == 0)
		return c_badarg;

	SysRandom(intarg);

	intarg = 0;
	return stack_add_val(stack,&intarg,integer);
}

#define FIN_FV_BEGIN(I,N,PV,PMT,PYR)  (-(PV*pow(1+(I/PYR/100),N)+PMT*(1+(I/PYR/100))*(pow(1+(I/PYR/100),N)-1)/(I/PYR/100)))
#define FIN_FV_END(I,N,PV,PMT,PYR) (-(PV*pow(1+(I/PYR/100),N)+PMT*(pow(1+(I/PYR/100),N)-1)/(I/PYR/100)))
#define FIN_FV_ZERO(I,N,PV,PMT,PYR) (-(PV+PMT*N))

#define FIN_PV_BEGIN(I,N,PMT,FV,PYR) ((-FV-PMT*(1+(I/PYR/100))*(pow(1+(I/PYR/100),N)-1)/(I/PYR/100))/pow(1+(I/PYR/100),N))
#define FIN_PV_END(I,N,PMT,FV,PYR) ((-FV-PMT*(pow(1+(I/PYR/100),N)-1)/(I/PYR/100))/pow(1+(I/PYR/100),N))
#define FIN_PV_ZERO(I,N,PMT,FV,PYR) (-PMT*N-FV)

#define FIN_PMT_BEGIN(I,N,PV,FV,PYR) ((-FV-PV*pow(1+(I/PYR/100),N))*((I/PYR/100)/(pow(1+(I/PYR/100),N)-1)/(1+(I/PYR/100))))
#define FIN_PMT_END(I,N,PV,FV,PYR) ((-FV-PV*pow(1+(I/PYR/100),N))*(I/PYR/100)/(pow(1+(I/PYR/100),N)-1))
#define FIN_PMT_ZERO(I,N,PV,FV,PYR) (-(FV+PV)/N)

#define FIN_N_BEGIN(I,PV,PMT,FV,PYR) (log10((-FV*(I/PYR/100)+PMT*(1+(I/PYR/100)))/(PV*(I/PYR/100)+PMT*(1+(I/PYR/100))))/log10(1+(I/PYR/100)))
#define FIN_N_END(I,PV,PMT,FV,PYR) (log10((-FV*(I/PYR/100)+PMT)/(PV*(I/PYR/100)+PMT))/log10(1+(I/PYR/100)))
#define FIN_N_ZERO(I,PV,PMT,FV,PYR) (-(FV+PV)/PMT)


CError
math_financial(Functype *func,CodeStack *stack)
{
	CError err;
	UInt32 begin;
	double arg[5];
	double result;
	Int16 i;

	err = stack_get_val(stack,&begin,integer);
	if (err)
		return err;

	for (i=4;i>=0;i--) {
		err = stack_get_val(stack,&arg[i],real);
		if (err)
			return err;
		if (!finite(arg[i]))
			return c_badarg;
	}
	switch (func->num) {
	case MATH_FINPMT:
		if (arg[0] == 0.0)
			result = FIN_PMT_ZERO(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else if (begin)
			result = FIN_PMT_BEGIN(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else
			result = FIN_PMT_END(arg[0],arg[1],arg[2],arg[3],arg[4]);
		break;
	case MATH_FINPV:
		if (arg[0] == 0.0)
			result = FIN_PV_ZERO(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else if (begin)
			result = FIN_PV_BEGIN(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else
			result = FIN_PV_END(arg[0],arg[1],arg[2],arg[3],arg[4]);
		break;
	case MATH_FINN:
		if (arg[0] == 0.0)
			result = FIN_N_ZERO(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else if (begin)
			result = FIN_N_BEGIN(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else
			result = FIN_N_END(arg[0],arg[1],arg[2],arg[3],arg[4]);
		break;
	case MATH_FINFV:
		if (arg[0] == 0.0)
			result = FIN_FV_ZERO(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else if (begin)
			result = FIN_FV_BEGIN(arg[0],arg[1],arg[2],arg[3],arg[4]);
		else
			result = FIN_FV_END(arg[0],arg[1],arg[2],arg[3],arg[4]);
		break;
	}
	return stack_add_val(stack,&result,real);
}

/***********************************************************************
 *
 * FUNCTION:     factorize
 * 
 * DESCRIPTION:  Calculate prime factors of an integer
 *
 * PARAMETERS:   value: Value to factorize
 *               factors: Array to store factors into
 *                        This array must be large enough to hold all
 *                        factors. 32 is sufficient for every Int32 
 *                        (think of -2^31).
 *
 * RETURN:       number of factors stored into array
 *      
 ***********************************************************************/

UInt8 factorize(Int32 value, Int32 factors[]) {
	UInt8 fcount = 0;
	Int32 factor = 2;

	if (value == 0) {
		factors[0] = 0;
		return 1;
	}
	if (value < 0) {
		factors[fcount++] = -1;
		value = -value;
	}
	if (value == 1) {
		factors[fcount++] = 1;
	}
	while(factor <= 65536 && factor*factor <= value) {
		while (value % factor == 0) {
			factors[fcount++] = factor;
			value /= factor;
		}
		factor++;
	}
	if (value != 1) {
		factors[fcount++] = value;
	}
	return fcount;
}

/***********************************************************************
 *
 * FUNCTION:     gcd
 * 
 * DESCRIPTION:  Calculate greatest common divisor (Euclidian algorithm)
 *
 * PARAMETERS:   a, b: Values to calculate gcd of
 *
 * RETURN:       gcd
 *      
 ***********************************************************************/

UInt32 gcd(UInt32 a, UInt32 b) {
	UInt32 tmp;

	if (a < b) {
		tmp=a; a=b; b=tmp;
	}
	while (b != 0) {
		tmp = a%b;
		a = b;
		b = tmp;
	}
	return a;
}

/***********************************************************************
 *
 * FUNCTION:     gcdex
 * 
 * DESCRIPTION:  Calculate greatest common divisor and cofactors
 *               (extended Euclidian algorithm)
 *
 *               Implementation based on
 *               <URL:http://en.wikipedia.org/w/index.php
 *                    ?title=Extended_Euclidean_algorithm
 *                    &oldid=52728059#Iterative_method>
 *
 * PARAMETERS:   a, b: Values to calculate gcd of
 *               coA, coB: Where to store cofactors
 *
 * RETURN:       gcd
 *      
 ***********************************************************************/

UInt32 gcdex(UInt32 a, UInt32 b, Int32* coA, Int32* coB) {
	UInt32 x=0, lastx = 1, y = 1, lasty = 0;
	UInt32 tmp, q;

	if (a < b) {
		tmp=a; a=b; b=tmp;
		x = lasty = 1;
		y = lastx = 0;
	}

	while(b != 0) {
		tmp = b;
		q = a/b;
		b = a % b;
		a = tmp;

  		tmp = x;
		x = lastx - q * x;
		lastx = tmp;	
		tmp = y;
		y = lasty - q * y;
		lasty = tmp;
    	}

	*coA = lastx;
	*coB = lasty;
	return a;
}

/***********************************************************************
 *
 * FUNCTION:     math_gcd
 * 
 * DESCRIPTION:  Compute gcd or lcm
 *
 * PARAMETERS:   On stack any number of integers
 *
 * RETURN:       On stack 1 argument
 *      
 ***********************************************************************/

CError
math_gcd(Functype *func,CodeStack *stack)
{
	UInt32 arg1,arg2, tmp;
	UInt16 i;
	CError err;
	
	if ((err=stack_get_val(stack,&arg1,integer)))
		return err;

	for(i=1; i < func->paramcount; i++) {
		if ((err=stack_get_val(stack,&arg2,integer)))
			return err;
		tmp = gcd(arg1, arg2);
		if (func->num == MATH_LCM) {
			if (tmp != 0) { // lcm(0,0) == gcd(0,0) == 0
				tmp = arg1 *arg2 / tmp;
			}
		}
		arg1 = tmp;
	}

	return stack_add_val(stack,&arg1,integer);
}

/***********************************************************************
 *
 * FUNCTION:     math_modinv
 * 
 * DESCRIPTION:  Compute inverse modulo m
 *
 * PARAMETERS:   On stack 2 integers:
 *                  a: value to invert
 *                  m: modulus
 *
 * RETURN:       On stack 1 argument
 *                  x: inverse, i. e.
 *                       0 <= x < m
 *                       (x * a) % m == 1
 *      
 ***********************************************************************/

CError math_modinv(Functype *func,CodeStack *stack) {
	UInt32 arg, mod, gcd;
	Int32 result, coB;
	CError err;

	err = stack_get_val2(stack,&arg, &mod,integer);
	if (err)
		return err;
	gcd = gcdex(arg, mod, &result, &coB);
	if (gcd != 1)
		return c_badarg;
	result %= (Int32) mod;
	if (result<0) result += mod;
	arg = result;
	return stack_add_val(stack, &arg, integer);
}

/***********************************************************************
 *
 * FUNCTION:     math_modpow
 * 
 * DESCRIPTION:  Compute powers modulo m
 *
 * PARAMETERS:   On stack 3 integers:
 *                  b: base
 *                  e: exponent
 *                  m: modulus
 *
 * RETURN:       On stack 1 argument: (b ^ e) % m
 *      
 ***********************************************************************/

CError 
math_modpow(Functype *func,CodeStack *stack)
{
	UInt32 base, exponent, modulus;
	UInt32 result = 1;
	CError err;

	if ((err=stack_get_val(stack,&modulus,integer)))
	  return err;	
	if ((err=stack_get_val2(stack,&base,&exponent,integer)))
	  return err;


	while (exponent != 0) {
		if (exponent % 2 == 1) {
			result = (result * base) % modulus;
			exponent--;
		} else {
			base = (base * base) % modulus;
			base %= modulus;
			exponent >>= 1;
		}
	}

	return stack_add_val(stack, &result, integer);
}

/***********************************************************************
 *
 * FUNCTION:     math_chinese
 * 
 * DESCRIPTION:  Compute chinese remainder theorem
 *
 * PARAMETERS:   On stack even number of integers.
 *               Each pair (a, m) means (x % m) == a.
 *               All m must be coprime.
 *
 * RETURN:       On stack 1 argument, which is the solution x of all
 *               equations
 *      
 ***********************************************************************/

CError math_chinese(Functype *func,CodeStack *stack) {
	CError err;
	UInt32 arg1, mod1, arg2, mod2, gcd;
	Int32 co1, co2, tmp;
	UInt16 i;

	if (func->paramcount % 2 != 0) {
		return c_badargcount;
	}

	err = stack_get_val2(stack, &arg1, &mod1, integer);
	if (err)
		return err;

	for(i=2; i < func->paramcount; i += 2) {
		err = stack_get_val2(stack, &arg2, &mod2, integer);
		if (err)
			return err;
		gcd = gcdex(mod1, mod2, &co1, &co2);
		if (gcd != 1)
			return c_badarg;
		tmp = co1 * mod1 * arg2 + co2 * mod2 * arg1;
		mod1 *= mod2;
		tmp %= ((Int32)mod1);
		if (tmp < 0) tmp += mod1;
		arg1 = tmp;
	}
	return stack_add_val(stack, &arg1, integer);
}

/***********************************************************************
 *
 * FUNCTION:     math_phi
 * 
 * DESCRIPTION:  Compute totient
 *
 * PARAMETERS:   On stack 1 integer
 *
 * RETURN:       On stack 1 integer
 *      
 ***********************************************************************/

CError
math_phi(Functype *func,CodeStack *stack)
{
	UInt32 value;
	UInt32 result = 1;
	UInt32 factors[32];
	UInt8 fcount, i;
	CError err;

	if ((err=stack_get_val(stack,&value,integer)))
		return err;	

	if (value == 0) {
		result = 0;
	} else {
		fcount = factorize(value, factors);
		for(i=0; i<fcount; i++) {
			if (i>0 && factors[i] == factors[i-1]) {
				// double factor
				result *= factors[i];
			} else {
				// first occurrence of factor
				result *= (factors[i] -1);
			}
		}
	}
		
	return stack_add_val(stack, &result, integer);
}

/***********************************************************************
 *
 * FUNCTION:     math_primes
 * 
 * DESCRIPTION:  Check primality or find next/previous prime
 *
 * PARAMETERS:   On stack 1 integer
 *
 * RETURN:       On stack 1 integer
 *      
 ***********************************************************************/

CError math_primes(Functype *func,CodeStack *stack) {
	CError err;
	UInt32 val;
	Boolean isprime;
	UInt32 i;

	err = stack_get_val(stack, &val, integer);
	if (err)
		return err;

	while(true) {
		isprime = (val > 1);
		for(i=2; i < 65536 && i*i <= val; i++) {
			if (val % i == 0) {
				isprime = false;
				break;
			}
		}
		if (func->num == MATH_ISPRIME) {
			val = isprime ? 1 : 0;
			break;
		} else if (isprime) {
			break;
		} else if (func->num == MATH_NEXTPRIME) {
			val++;
		} else {
			val--;
		}
	}

	return stack_add_val(stack, &val, integer);
}
