/*
 *   $Id: integ.cpp,v 1.2 2009/12/15 21:37:44 mapibid Exp $
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
 */
#include "StdAfx.h"

#include "compat/PalmOS.h"

#include "compat/MathLib.h"
#include "konvert.h"
#include "funcs.h"
#include "stack.h"
#include "mathem.h"
#include "integ.h"
#include "defuns.h"
#include "matrix.h"

static double ncr(Int32 a, Int32 b) NEWFUNC;
static double
ncr(Int32 a, Int32 b)
{
	Int32 i;
	double result = 1;

	for (i=a;i > a-b;i--)
		result *= i;
	for (i=2;i <= b; i++)
		result /= i;

	return result;
}

static double fact(Int32 i) NEWFUNC;
static double 
fact(Int32 i)
{
	double result=1.0;
    
	do 
		result*=i;
	while(--i);
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     integ_d1_item
 * 
 * DESCRIPTION:  Coefficient for 1st derivation
 *
 * PARAMETERS:   n - member for which we calculate the coef
 *
 * RETURN:       1+2^2+3^2+.../(fact(2n-1))
 *      
 ***********************************************************************/
static double
integ_d1_item(Int16 n) NEWFUNC;
static double
integ_d1_item(Int16 n)
{
	Int16 i;
	double result=1.0;

	for (i=2;i<n;i++)
		result*= i*i;
	return result/fact(2*n-1);
}

/***********************************************************************
 *
 * FUNCTION:     integ_d2_item
 * 
 * DESCRIPTION:  Coefficient for 2nd derivation
 *
 * PARAMETERS:   n - member for which we calculate the coef
 *
 * RETURN:       1+2^2+3^2+.../(2fact(2n))
 *      
 ***********************************************************************/
static double
integ_d2_item(Int16 n) NEWFUNC;
static double
integ_d2_item(Int16 n)
{
	Int16 i;
	double result=1.0;
	
	for (i=2;i<n;i++)
		result*=i*i;
	return result/(fact(2*n)*2.0);
}

static CError
integ_get_add_params(CodeStack *stack,CodeStack **arg,Int16 argnum) NEWFUNC;
static CError
integ_get_add_params(CodeStack *stack,CodeStack **arg,Int16 argnum)
{
	Int16 i;
	CodeStack *argarr;
	CError err;
	Trpn item;

	if (!argnum) 
		return c_noerror;

	argarr = stack_new(argnum+1);
	for (i = 0;i < argnum;i++) {
		item = stack_pop(stack);
		if (item.type==variable || item.type==litem) {
			err=rpn_eval_variable(&item,item);
			if (err) {
				stack_delete(argarr);
				return err;
			}
		}
		stack_push(argarr,item);
	}
	*arg = argarr;
	return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     integ_differ_point
 * 
 * DESCRIPTION:  Find n'th differention starting at 'x' using step 'step'
 *               This algorithm is not recursive, it uses values from 
 *               the 'Pascal triangle' 
 *               (e.g. 2nd diff = f(x)-2f(x+step)+f(x+2step)).
 *               It tries to eliminate floating point error by computing
 *               the result symmetrically from both sides
 *
 * PARAMETERS:   x - starting point for diffs
 *               n - which diff to compute
 *               step - the distance between 2 neighbor points
 *               fcstack - function
 *
 * RETURN:       result
 *      
 ***********************************************************************/
static double
integ_differ_point(double x,Int16 n,double step,CodeStack *fcstack,
		   CodeStack *argarr) NEWFUNC;
static double
integ_differ_point(double x,Int16 n,double step,CodeStack *fcstack,
		   CodeStack *argarr)
{
	double result = 0.0;
	double tmp;
	Int16 i;
    
	for (i=0;i <= n/2;i++) {
		func_get_value(fcstack,x+step*i,&tmp,argarr);
		tmp *= ncr(n,i);
		if ((n-i) % 2)
			result -= tmp;
		else
			result +=tmp;

		if ((n-i) <= i)
			break;
		
		func_get_value(fcstack,x + step*(n-i),&tmp,argarr);
		tmp *= ncr(n,(n-i));
		if (i % 2)
			result -= tmp;
		else
			result +=tmp; 
	}
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     integ_derive2
 * 
 * DESCRIPTION:  Find a second derivation of a function, uses Stirling
 *               interpolation formula
 *
 * PARAMETERS:   x - the point where the derivation should be found
 *               fcstack - function
 *               error - the maximum allowed error (this doesn't fully
 *                       apply here, because the function may not converge
 *                       correctly)
 *
 * RETURN:       result or NaN
 *      
 ***********************************************************************/
double
integ_derive2(double x,CodeStack *fcstack,double error,
	      CodeStack *argarr) NEWFUNC;
double
integ_derive2(double x,CodeStack *fcstack,double error,CodeStack *argarr)
{
	double step;
	double result,tmp;
	Int16 i;

	step = fabs(x) * 1E-7;
	if (step < 1E-5)
		step = 1E-5;

	result = integ_differ_point(x-step,2,step,fcstack,argarr);

	for (i=2;i<7;i++) {
		tmp = integ_d2_item(i) 
			* integ_differ_point(x-i*step,2*i,step,fcstack,argarr);
		if (i % 2)
			result += tmp;
		else
			result -= tmp;

		if (fabs(tmp) < error*step)
			break;
	}

	return result/(step*step);
}

/***********************************************************************
 *
 * FUNCTION:     integ_derive1
 * 
 * DESCRIPTION:  Find a first derivation of a function, uses Stirling
 *               interpolation formula
 *
 * PARAMETERS:   x - the point where the derivation should be found
 *               fcstack - function
 *               error - the maximum allowed error (this doesn't fully
 *                       apply here, because the function may not converge
 *                       correctly)
 *
 * RETURN:       result or NaN
 *      
 ***********************************************************************/
double 
integ_derive1(double x,CodeStack *fcstack,double error,CodeStack *argarr) NEWFUNC;
double
integ_derive1(double x,CodeStack *fcstack,double error,CodeStack *argarr)
{
	double step;
	double result,tmp;
	Int16 i;

	step = fabs(x) * 1E-7;
	if (step < 1E-5)
		step = 1E-5;

	result = (integ_differ_point(x-step,1,step,fcstack,argarr) + 
		  integ_differ_point(x,1,step,fcstack,argarr))/2;
	for (i=2;i<7;i++) {
		tmp = integ_d1_item(i)/2 * 
			(integ_differ_point(x-i*step,2*i-1,step,fcstack,argarr) +
			 integ_differ_point(x-(i-1)*step,2*i-1,step,fcstack,argarr));
		if (i % 2)
			result += tmp;
		else
			result -= tmp;
		if (fabs(tmp) < error*step)
			break;
	}
	return result/step;
}

/***********************************************************************
 *
 * FUNCTION:     integ_romberg
 * 
 * DESCRIPTION:  Romberg integration, faster and more precise then Simps
 *
 * PARAMETERS:   min,max - bounds of the interval
 *               stack - function
 *               n - level of polynom that will be used for approx.
 *
 * RETURN:       integral(min,max,f())
 *      
 ***********************************************************************/
#define MATRIX1(m,r,c)    MATRIX(m,r-1,c-1)

double
integ_romberg(double min,double max,CodeStack *fcstack,Int16 n,
	      CodeStack *argarr) NEWFUNC;
double
integ_romberg(double min,double max,CodeStack *fcstack,Int16 n,
	      CodeStack *argarr)
{
	Matrix *m;
	Int16 i,j,k;
	double h,sum,res1,res2;

	/* Replace the variables by numbers to speed execution */
	stack_fix_variables(fcstack);

	m = matrix_new(2,n);
	h = max - min;

	func_get_value(fcstack,min,&res1,argarr);
	func_get_value(fcstack,max,&res2,argarr);
	MATRIX1(m,1,1) = h*(res1 + res2)/2.0;

	for (i=2;i<=n;i++) {
		sum = 0.0;
		for (k=1;k <= (1 << (i-2));k++) {
			func_get_value(fcstack,
				       min+((double)k-0.5)*h,
				       &res1,argarr);
			sum += res1;
		}
		MATRIX1(m,2,1) = 0.5*(MATRIX1(m,1,1) + h*sum);
		
		for (j=2;j<=i;j++) {
			sum = pow(4.0,(double)(j-1));
			MATRIX1(m,2,j) = (sum*MATRIX1(m,2,j-1)-MATRIX1(m,1,j-1))/(sum-1.0);
		}
		h /= 2.0;
		for (j=1;j<=i;j++)
			MATRIX1(m,1,j) = MATRIX1(m,2,j);
	}
	res1 = MATRIX1(m,2,n);
	matrix_delete(m);
	return res1;
}


/***********************************************************************
 *
 * FUNCTION:     integ_simps
 * 
 * DESCRIPTION:  Numerical integration of a function using simpson method
 *
 * PARAMETERS:   min,max - bounds of an interval
 *               fcstack - function
 *               error - maximum allowed error
 *
 * RETURN:       result or NaN
 *      
 ***********************************************************************/
double
integ_simps(double min, double max,CodeStack *fcstack,
	    double error,CodeStack *argarr) NEWFUNC;
double
integ_simps(double min, double max,CodeStack *fcstack,
	    double error,CodeStack *argarr)
{
	double step,count,sum,res;
	UInt32 i;

	/* Replace the variables by numbers to speed execution */
	stack_fix_variables(fcstack);

	step = sqrt(sqrt(error));
	count = round((max - min) / step);
	step = (max - min) / count;
	
	sum = 0.0;
	func_get_value(fcstack,min,&res,argarr);
	if (!finite(res))
		return NaN;
	sum += res;

	func_get_value(fcstack,max,&res,argarr);
	if (!finite(res))
		return NaN;
	sum += res;
	
	for (i=1;i<count;i++) {
		func_get_value(fcstack,min + i*step,&res,argarr);
		if (!finite(res))
			return NaN;
		if (i % 2)
			sum += 4*res;
		else
			sum += 2*res;
	}
	sum *= step/3;
	return sum;
}


/***********************************************************************
 *
 * FUNCTION:     integ_intersect
 * 
 * DESCRIPTION:  Finds intersection of 2 functions (effectively finding
 *               a root of a function f=f1()-f2()
 *
 * PARAMETERS:   min,max - bounds of the interval
 *               f1,f2 - functions that should intersect on the interval
 *               error - allowed error
 *
 * RETURN:       result or NaN
 *      
 ***********************************************************************/
double
integ_intersect(double min,double max, CodeStack *f1, CodeStack *f2,
		double error,CodeStack *argarr) NEWFUNC;
double
integ_intersect(double min,double max, CodeStack *f1, CodeStack *f2,
		double error,CodeStack *argarr)
{
	double tmp,resmin,resmax;
	double res1,res2,res;
	Int32 count = 0;

	/* Replace the variables by numbers to speed execution */
	stack_fix_variables(f1);
	stack_fix_variables(f2);


	func_get_value(f1, min, &res1,argarr);
	func_get_value(f2, min, &res2,argarr);
	if (!finite(res1) || !finite(res2))
		return NaN;
	resmin = res1 - res2;

	func_get_value(f1, max, &res1,argarr);
	func_get_value(f2, max, &res2,argarr);
	if (!finite(res1) || !finite(res2))
		return NaN;
	resmax = res1 - res2;

	if (!finite(resmin) || !finite(resmax)) 
		return NaN;

	if (resmin == 0.0)
		return min;
	if (resmax == 0.0)
		return max;
	if (resmin * resmax > 0.0)
		return NaN;

	res = resmin;
	tmp = (max+min)/2.0;

	while (fabs(max-min) > error && count < MAX_ITER) { 
		count++;
		
		func_get_value(f1,tmp,&res1,argarr);
		func_get_value(f2,tmp,&res2,argarr);

		if (!finite(res1) || !finite(res2)) 
			return NaN;
		res = res1 - res2;
		
		if (res*resmin<0.0) {
			resmax = res;
			max = tmp;
		}
		else if (res*resmax<0.0){
			resmin = res;
			min = tmp;
		}
		else 
			break;

		tmp = (max+min)/2.0;
	}
	if (count == MAX_ITER || fabs(res)>1.0)
		return NaN;

	return tmp;
}

#define INTERVAL_PARTS  128
static Err 
integ_guess_interval(double *min, double *max, double value,
		     CodeStack *fcstack,CodeStack *argarr) NEWFUNC;
static Err
integ_guess_interval(double *min, double *max, double value,
		     CodeStack *fcstack,CodeStack *argarr)
{
	Int16 i;
	CError err1,err2;
	double firstmin,firstmax;
	double resmin,resmax;

	for (i=0;i<INTERVAL_PARTS;i++) {
		firstmin = *min + i*fabs(*max-*min)/INTERVAL_PARTS;;
		firstmax = *min + (i+1)*fabs(*max-*min)/INTERVAL_PARTS;
		err1 = func_get_value(fcstack, firstmin, &resmin,argarr);
		err2 = func_get_value(fcstack, firstmax, &resmax,argarr);
		if (!err1 && !err2 && finite(resmin) && finite(resmax) \
		    && (resmin-value)*(resmax-value) < 0.0) {
			*min = firstmin;
			*max = firstmax;
			return c_noerror;
		}
	}
	return c_compimp;
}


/***********************************************************************
 *
 * FUNCTION:     integ_zero
 * 
 * DESCRIPTION:  Find a zero/value/minimum/maximum of a function
 *
 * PARAMETERS:   min,max - bounds of interval
 *               value - if searching for zero, it should be 0,
 *                       else the value we are searching for
 *               fcstack - function on which it operates
 *               error   - the maximum allowed error
 *               funcnum - MATH_FZERO,MATH_VALUE - find value of func
 *                         MATH_FMIN,FMAX - find min/max by finding
 *                         zero of first derivation
 *               argarr,argnum - additional arguments to user function
 *                             - you can pass NULL,0 to pass no other args
 *
 * RETURN:       result or NaN
 *      
 ***********************************************************************/
double
integ_zero(double min, double max, double value, CodeStack *fcstack, 
	   double error,Int16 funcnum,CodeStack *argarr) NEWFUNC;
double
integ_zero(double min, double max, double value, CodeStack *fcstack, 
	   double error,Int16 funcnum,CodeStack *argarr)
{
	double resmin,resmax,tmp;
	double firstmin,firstmax;
	double xmin,xmax;
	double res;
	Int32 count = 0;

	/* Replace the variables by numbers to speed execution */
	stack_fix_variables(fcstack);

	if (funcnum == MATH_FMIN || funcnum == MATH_FMAX) {
		resmin = integ_derive1(min,fcstack,0.1,argarr);
		resmax = integ_derive1(max,fcstack,0.1,argarr);
		func_get_value(fcstack, min, &firstmin,argarr);
		func_get_value(fcstack, max, &firstmax,argarr);
	} else {
		func_get_value(fcstack, min, &resmin,argarr);
		func_get_value(fcstack, max, &resmax,argarr);
	}

	if (!finite(resmin) || !finite(resmax)) {
		if (funcnum == MATH_FZERO || funcnum == MATH_FVALUE) {
			if (integ_guess_interval(&min,&max,value,fcstack,argarr))
				return NaN;
			func_get_value(fcstack, min, &resmin,argarr);
			func_get_value(fcstack, max, &resmax,argarr);
		} else
			return NaN;
	}

	resmin -= value;
	resmax -= value;

	xmin = min;
	xmax = max;

	if (resmin*resmax>0.0) {
		/* The Min and Max functions should return an
		   absolute min/max even if local min doesn't exist */
		if (funcnum == MATH_FMIN || funcnum == MATH_FMAX) {
			tmp = (min + max) / 2;
			/* Test if in the middle isn't a reverse 
			   derive */
			res = integ_derive1(tmp,fcstack,0.1,argarr);
			if (!finite(res) || resmin*res>0.0) {
				/* Find absolute min/max */
				/* firstmin/max - value of a function */
				/* resmin/max - derivation of a function */
				/* res - value of a function in the middle */
				func_get_value(fcstack, tmp, &res,argarr);
				if (!finite(resmin) || !finite(resmax) ||
				    !finite(res))
					return NaN;
				if (funcnum == MATH_FMIN) {
					if (resmin > 0.0 && resmax > 0.0 &&
					    firstmin <= firstmax && 
					    firstmin <= res)
						return min;
					if (resmin < 0.0 && resmax < 0.0 &&
					    firstmax <= firstmin && 
					    firstmax <= res)
						return max;
				} else { /* MATH_FMAX */
					if (resmin > 0.0 && resmax > 0.0 &&
					    firstmax >= firstmin &&
					    firstmax >= res)
						return max;
					if (resmin < 0.0 && resmax < 0.0 &&
					    firstmin >= firstmax &&
					    firstmin >= res)
						return min;
				}
				return NaN;
			}
			if ((funcnum == MATH_FMIN && res > 0.0) ||
			    (funcnum == MATH_FMAX && res < 0.0)) {
				max = tmp;
				resmax = res;
			} else {
				min = tmp;
				resmin = res;
			}
		} else { /* funcnum = ZERO | VALUE */
			/* partition the interval to X parts and
			 * try to find if the root is not in one of them
			 */
			if (integ_guess_interval(&min,&max,value,fcstack,argarr)) 
				return NaN;
			func_get_value(fcstack, min, &resmin,argarr);
			func_get_value(fcstack, max, &resmax,argarr);
			resmin -= value;
			resmax -= value;
		}
	}

	/* Return absolute min/max if we do not have reasonable 
	 * values for local */
	if (funcnum == MATH_FMIN && resmin >= 0.0) {
		func_get_value(fcstack, min, &resmin,argarr);
		func_get_value(fcstack, max, &resmax,argarr);
		if (!finite(resmin) || !finite(resmax))
			return NaN;
		return resmin < resmin ? min : max;
	} else if (funcnum == MATH_FMAX && resmin <= 0.0) {
		func_get_value(fcstack, min, &resmin,argarr);
		func_get_value(fcstack, max, &resmax,argarr);
		if (!finite(resmin) || !finite(resmax))
			return NaN;
		return resmin > resmax ? min : max;
	}

	if (resmin==0.0)
		max=tmp=min;
	else if (resmax==0.0)
		min=tmp=max;
	else
		tmp = (max+min)/2.0;

	while (fabs(max-min) > error && count < MAX_ITER) { 
		count++;
		if (funcnum == MATH_FMIN || funcnum == MATH_FMAX)
			res = integ_derive1(tmp,fcstack,0.1,argarr);
		else 
			func_get_value(fcstack,tmp,&res,argarr);

		if (!finite(res)) 
			return NaN;

		res -= value;

		if (res*resmin<0.0) {
			resmax = res;
			max = tmp;
		}
		else if (res*resmax<0.0){
			resmin = res;
			min = tmp;
		}
		else 
			break;

		tmp = (max+min) / 2.0;
	}
	/* It's possible we didn't compute a 'zero' but something else,
	 * if the result isn't near 0 return error */

	if (count == MAX_ITER || fabs(res)>1.0)
		return NaN;

	/* Check for absolute min/max if it is not a better result 
	 * then the local one */
	func_get_value(fcstack, tmp, &res,argarr);
	if (funcnum == MATH_FMIN && firstmin < res)
		return xmin;
	if (funcnum == MATH_FMIN && firstmax < res)
		return xmax;
	if (funcnum == MATH_FMAX && firstmin > res)
		return xmin;
	if (funcnum == MATH_FMAX && firstmax > res)
		return xmax;
	
	return tmp;
}

/***********************************************************************
 *
 * FUNCTION:     integ_fdydx
 * 
 * DESCRIPTION:  Do a numerical derivation using Stirling formula
 *
 * PARAMETERS:   Syntax: f(point:f())
 *
 * RETURN:       Derivation
 *      
 ***********************************************************************/
CError
integ_fdydx(Functype *func,CodeStack *stack)
{
	CError err;
	double point;
	double error = DEFAULT_ERROR;
	double result;
	CodeStack *fcstack;
	CodeStack *argarr = NULL;

	if (func->paramcount >= 3) {
		/* Fill the structure with additional parameters */
		err = integ_get_add_params(stack,&argarr,func->paramcount - 3);
		if (err)
			return err;
		if ((err = stack_get_val(stack,&error,real)))
			goto error;
		if (error < 0.0) {
			err = c_badarg;
			goto error;
		}
	} else if (func->paramcount != 2)
		return c_badargcount;
	
	if ((err=stack_get_val(stack,&fcstack,function)))
		goto error;
	if ((err=stack_get_val(stack,&point,real))) {
		stack_delete(fcstack);
		goto error;
	}

	switch (func->num) {
	case MATH_FDYDX1:
		result = integ_derive1(point,fcstack,error,argarr);
		break;
	case MATH_FDYDX2:
	default:
		result = integ_derive2(point,fcstack,error,argarr);
		break;
	}

	stack_delete(fcstack);
	err = stack_add_val(stack,&result,real);
error:
	if (argarr)
		stack_delete(argarr);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     integ_fromberg
 * 
 * DESCRIPTION:  Wrapper for the romberg integration, fromberg() function
 *
 * PARAMETERS:   fromberg(min:max:f()[:n])
 *
 * RETURN:       Integration of the f() function
 *      
 ***********************************************************************/
CError
integ_fromberg(Functype *func,CodeStack *stack)
{
	CodeStack *fcstack;
	CError err;
	double result,min,max;
	UInt32 n = DEFAULT_ROMBERG;
	CodeStack *argarr = NULL;
	
	if (func->paramcount >= 4) {
		/* Fill the structure with additional parameters */
		err = integ_get_add_params(stack,&argarr,func->paramcount - 4);
		if (err)
			return err;

		if ((err = stack_get_val(stack,&n,integer)))
			goto error;
		if (n == 0 || n > 50) {
			err = c_badarg;
			goto error;
		}
	} else if (func->paramcount != 3)
		return c_badargcount;
	
	if ((err=stack_get_val(stack,&fcstack,function)))
		goto error;
	if ((err=stack_get_val2(stack,&min,&max,real))) {
		stack_delete(fcstack);
		goto error;
	}
	if (max < min) {
		stack_delete(fcstack);
		err = c_compimp;
		goto error;
	}

	result = integ_romberg(min,max,fcstack,n,argarr);
	stack_delete(fcstack);
	if (!finite(result)) {
		err = c_compimp;
		goto error;
	}
	
	err = stack_add_val(stack,&result,real);

error:       
	if (argarr)
		stack_delete(argarr);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     integ_fsimps
 * 
 * DESCRIPTION:  Do a numerical integral using the Simpson formula
 *
 * PARAMETERS:   Syntax: f(min:max:function[:error])
 * NOTE:         Treats undefined points as zeros
 *
 * RETURN:       Finite integral 
 *      
 ***********************************************************************/
CError
integ_fsimps(Functype *func,CodeStack *stack)
{
	CodeStack *fcstack;
	CError err;
	double result,min,max;
	double error = DEFAULT_ERROR;
	CodeStack *argarr = NULL;
	
	if (func->paramcount >= 4) {
		/* Fill the structure with additional parameters */
		err = integ_get_add_params(stack,&argarr,func->paramcount - 4);
		if (err)
			return err;
		
		if ((err = stack_get_val(stack,&error,real)))
			goto error;
		if (error < 0.0) {
			err = c_badarg;
			goto error;
		}
	} else if (func->paramcount != 3)
		return c_badargcount;
	
	if ((err=stack_get_val(stack,&fcstack,function)))
		goto error;
	if ((err=stack_get_val2(stack,&min,&max,real))) {
		stack_delete(fcstack);
		goto error;
	}
	if (max < min) {
		stack_delete(fcstack);
		err = c_compimp;
		goto error;
	}

	result = integ_simps(min,max,fcstack,error,argarr);

	stack_delete(fcstack);
	err = stack_add_val(stack,&result,real);
error:
	if (argarr)
		stack_delete(argarr);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     integ_fsolve
 * 
 * DESCRIPTION:  Find a 'zero' of a function either by
 *               dividing the interval 
 *
 * PARAMETERS:   syntax is f(min:max:function[:error])
 *
 * RETURN:       real number on stack
 *      
 ***********************************************************************/
CError
integ_fsolve(Functype *func,CodeStack *stack)
{
	double min,max;
	double result;
	double error = DEFAULT_ERROR;
	double value = 0.0;
	CodeStack *fcstack;
	CError err;
	CodeStack *argarr = NULL;
	Int16 argnum;

	if ((func->num == MATH_FVALUE && func->paramcount >= 5)
	    || (func->num != MATH_FVALUE && func->paramcount >=4)) {
		if (func->num == MATH_FVALUE)
			argnum = func->paramcount - 5;
		else
			argnum = func->paramcount - 4;
		/* Fill the structure with additional parameters */
		err = integ_get_add_params(stack,&argarr,argnum);
		if (err)
			return err;

		if ((err = stack_get_val(stack,&error,real)))
			goto error;
		if (error < 0.0) {
			err = c_badarg;
			goto error;
		}
	} else if ((func->num == MATH_FVALUE && func->paramcount != 4)
		   || (func->num != MATH_FVALUE && func->paramcount != 3)) 
		return c_badargcount;
	
	if ((err=stack_get_val(stack,&fcstack,function)))
		goto error;

	if (func->num == MATH_FVALUE)
		if ((err=stack_get_val(stack,&value,real))) {
			stack_delete(fcstack);
			goto error;
		}

	if ((err=stack_get_val2(stack,&min,&max,real))) {
		stack_delete(fcstack);
		goto error;
	}
 
	if (max<min) {
		stack_delete(fcstack);
		goto error;
	}

	result = integ_zero(min,max,value,fcstack,error,func->num,argarr);

	stack_delete(fcstack);
	/* It's possible we didn't compute a 'zero' but something else,
	 * if the result isn't near 0 return error */

	if (!finite(result)) {
		err = c_compimp;
		goto error;
	}

	err = stack_add_val(stack,&result,real);
error:
	if (argarr) 
		stack_delete(argarr);
	return err;
}	

/***********************************************************************
 *
 * FUNCTION:     integ_fintersect
 * 
 * DESCRIPTION:  Find intersection of 2 functions
 *
 * PARAMETERS:   syntax is f(min:max:f1():f2()[:error])
 *
 * RETURN:       real number on stack
 *      
 ***********************************************************************/
CError
integ_fintersect(Functype *func,CodeStack *stack)
{
	double min, max;
	double error = DEFAULT_ERROR;
	double result;
	CodeStack *f1,*f2;
	CError err;
	CodeStack *argarr = NULL;

	if (func->paramcount >= 5) {
		/* Fill the structure with additional parameters */
		err = integ_get_add_params(stack,&argarr,func->paramcount - 5);
		if (err)
			return err;
		if ((err = stack_get_val(stack,&error,real)))
			goto error;
		if (error < 0.0) {
			err = c_badarg;
			goto error;
		}
	} else if (func->paramcount != 4) 
		return c_badargcount;
	
	err=stack_get_val(stack,&f2,function);
	if (err)
		goto error;
	err=stack_get_val(stack,&f1,function);
	if (err) {
		stack_delete(f2);
		goto error;
	}
	if ((err=stack_get_val2(stack,&min,&max,real))) {
		stack_delete(f1);
		stack_delete(f2);
		goto error;
	}

	result = integ_intersect(min,max,f1,f2,error,argarr);
	stack_delete(f1);
	stack_delete(f2);
	
	if (!finite(result)) {
		err = c_compimp;
		goto error;
	}
	err = stack_add_val(stack,&result,real);
error:
	if (argarr)
		stack_delete(argarr);
	return err;
}

