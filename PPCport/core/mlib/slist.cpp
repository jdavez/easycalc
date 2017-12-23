/*
 *   $Id: slist.cpp,v 1.1 2009/10/17 13:48:34 mapibid Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000,2001,2002 Ondrej Palkovsky
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
#include "stdafx.h"

#include "compat/PalmOS.h"

#include "compat/segment.h"
#include "konvert.h"
#include "stack.h"
#include "mathem.h"
#include "slist.h"
#include "compat/MathLib.h"
#include "complex.h"
#include "display.h"
#include "funcs.h"
#include "matrix.h"
#include "cmatrix.h"

void 
list_delete(List *lst) 
{
	MemPtrFree(lst);
}


/***********************************************************************
 *
 * FUNCTION:     list_new
 * 
 * DESCRIPTION:  Allocate new structure for the list
 *
 * PARAMETERS:   size (number of items)
 *
 * RETURN:       New list
 *      
 ***********************************************************************/
List *
list_new(Int16 size)
{
	List *result;

	result = (List *) MemPtrNew(sizeof(*result) + size*sizeof(result->item[0]));
	if (result == NULL)
	    return NULL;

	result->size = size;

	return result;
}

/***********************************************************************
 *
 * FUNCTION:     list_dup
 * 
 * DESCRIPTION:  Duplicate list (allocate new space, copy..)
 *
 * PARAMETERS:   list
 *
 * RETURN:       duplicated list
 *      
 ***********************************************************************/
List *
list_dup(List *oldlist)
{
	List *result;
	Int16 i;
	
	result = list_new(oldlist->size);
	if (!result)
	    return NULL;
	for (i=0;i<oldlist->size;i++)
		result->item[i] = oldlist->item[i];
	return result;
}


/***********************************************************************
 *
 * FUNCTION:     list_sort
 * 
 * DESCRIPTION:  Sort a list, this functions sorts the numbers according
 *               their absolute value but it respects sign of real part
 *
 * PARAMETERS:   lst - list
 *
 * RETURN:       sorted list
 *      
 ***********************************************************************/
#define CP_ABS(x) (hypot(x.real,x.imag)*sgn(x.real))

static void list_sort(List *lst) NEWFUNC;
static void
list_sort(List *lst)
{
	Int16 gap,i,j;
	Complex temp;

	for (gap = lst->size/2;gap > 0; gap/=2)
		for (i = gap;i < lst->size;i++) {
			for (j=i-gap;j>=0
			  && CP_ABS(lst->item[j]) > CP_ABS(lst->item[j+gap]);
			     j-=gap) {
				temp = lst->item[j];
				lst->item[j] = lst->item[j+gap];
				lst->item[j+gap] = temp;
			}
		}
}

/***********************************************************************
 *
 * FUNCTION:     list_real
 * 
 * DESCRIPTION:  Test if list contains only real numbers
 *
 * PARAMETERS:   list
 *
 * RETURN:       true - only real numbers
 *               false - contains complex numbers
 *      
 ***********************************************************************/
static Boolean list_real(List *lst) NEWFUNC;
static Boolean
list_real(List *lst)
{
	Int16 i;
	
	for (i=0;i < lst->size;i++)
		if (lst->item[i].imag != 0.0)
			return false;

	return true;
}

/***********************************************************************
 *
 * FUNCTION:     list_func2
 * 
 * DESCRIPTION:  Function that modify lists (sorting etc.)
 *
 * PARAMETERS:   list on stack
 *
 * RETURN:       list on stack
 *      
 ***********************************************************************/
CError
list_func2(Functype *func,CodeStack *stack)
{
	List *lst;
	CError err;
	Int16 i;
	Complex tmp;

	err = stack_get_val(stack,&lst,list);
	if (err)
		return err;

	switch (func->num) {
	case LIST_SORTA:
		list_sort(lst);
		break;
	case LIST_SORTD:
		list_sort(lst);
		for (i=0;i < lst->size/2;i++) {
			tmp = lst->item[i];
			lst->item[i] = lst->item[lst->size - i - 1];
			lst->item[lst->size - i - 1] = tmp;
		}
		break;
	case LIST_CUMSUM:
		for (i=1;i<lst->size;i++) {
			lst->item[i].real += lst->item[i-1].real;
			lst->item[i].imag += lst->item[i-1].imag;
		}
		break;
	}
	err = stack_add_val(stack,&lst,list);
	list_delete(lst);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_func
 * 
 * DESCRIPTION:  Functions work on top of complex lists and return
 *               complex numbers
 *
 * PARAMETERS:   list on stack
 *
 * RETURN:       complex on stack
 *      
 ***********************************************************************/
CError
list_func(Functype *func,CodeStack *stack)
{
	List *lst;
	CError err;
	Complex result;
	Int16 i;

	err = stack_get_val(stack,&lst,list);
	if (err)
		return err;

	switch (func->num) {
	case LIST_DIM:
		result.real = lst->size;
		result.imag = 0.0;
		break;
	case LIST_MIN:
		list_sort(lst);
		result = lst->item[0];
		break;
	case LIST_MAX:
		list_sort(lst);
		result = lst->item[lst->size - 1];
		break;
	case LIST_SUM:
		result.real = 0.0;
		result.imag = 0.0;
		for (i=0;i<lst->size;i++)
			result = cplx_add(result,lst->item[i]);
		break;
	case LIST_PROD:
		result.real = lst->item[0].real;
		result.imag = lst->item[0].imag;
		for (i=1;i < lst->size;i++)
			result = cplx_multiply(result,lst->item[i]);
		break;
	case LIST_MEDIAN:
		list_sort(lst);
		if (lst->size % 2)
			result = lst->item[lst->size/2];
		else {
			result = cplx_add(lst->item[lst->size/2-1],
					  lst->item[lst->size/2]);
			result.real /= 2;
			result.imag /= 2;
		}
		break;
	}

	if (result.imag != 0.0)
		err = stack_add_val(stack,&result,complex);
	else
		err = stack_add_val(stack,&result.real,real);

	list_delete(lst);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_sum
 * 
 * DESCRIPTION:  Compute a total sum of a list
 *
 * PARAMETERS:   lst - list
 *
 * RETURN:       sum(lst)
 *      
 ***********************************************************************/
static double list_sum(List *lst) NEWFUNC;
static double
list_sum(List *lst)
{
	Int16 i;
	double result = 0.0;
	
	for (i=0;i<lst->size;i++)
		result += lst->item[i].real;
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     list_mean
 * 
 * DESCRIPTION:  Compute a mean of a list
 *
 * PARAMETERS:   lst - list
 *
 * RETURN:       mean(lst)
 *      
 ***********************************************************************/
static double list_mean(List *lst) NEWFUNC;
static double
list_mean(List *lst)
{
	Int16 i;
	double result;

	result = 0.0;
	for (i=0;i<lst->size;i++)
		result += lst->item[i].real;
	result /= lst->size;
	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     list_moment
 * 
 * DESCRIPTION:  Return nth moment of the list
 *
 * PARAMETERS:   list 
 *               moment
 *
 * RETURN:       result
 *      
 ***********************************************************************/
static double list_moment(List *lst,Int16 moment) NEWFUNC;
static double
list_moment(List *lst,Int16 moment)
{
	Int16 i,j;
	double result,tmp,mean;
	
	mean = list_mean(lst);
	result = 0.0;
	for (i=0;i<lst->size;i++) {
		tmp = (lst->item[i].real - mean);
		for (j=1;j<moment;j++)
			tmp *= (lst->item[i].real - mean);
		result += tmp;
	}
	result /= lst->size;

	return result;
}

/***********************************************************************
 *
 * FUNCTION:     list_variance
 * 
 * DESCRIPTION:  Compute variance of a list
 *
 * PARAMETERS:   List
 *
 * RETURN:       sum((x-mean(x))^2)/(n-1)
 *      
 ***********************************************************************/
static double list_variance(List *lst) NEWFUNC;
static double 
list_variance(List *lst)
{
	double mean;
	double result;
	Int16 i;

	if (lst->size == 1)
		return 0.0;

	mean = list_mean(lst);
	result = 0.0;
	for (i=0;i<lst->size;i++)
		result += (mean - lst->item[i].real)*(mean-lst->item[i].real);
	result /= lst->size - 1;

	return result;
}

static TCHAR * list_linreg(List *lstx,List *lsty) NEWFUNC;
static TCHAR *
list_linreg(List *lstx,List *lsty)
{
	double b1,b0;
	double tmp,tmp2;
	Int16 i;
	double n;
	TCHAR *result,*res1,*res2;

	n = lstx->size; /* = lsty->size */

	for (i=0,tmp=0.0,tmp2=0.0;i<n;i++) {
		tmp += lstx->item[i].real * lsty->item[i].real;
		tmp2 += lstx->item[i].real * lstx->item[i].real;
	}
	tmp *= n;
	tmp -= list_sum(lstx) * list_sum(lsty);
	tmp2 *= n;
	tmp2 -= list_sum(lstx)*list_sum(lstx);
	b1 = tmp / tmp2;
	b0 = list_sum(lsty)/n - b1*list_sum(lstx)/n;
	
	res1 = display_real(b0);
	res2 = display_real(b1);
	result = (TCHAR *) MemPtrNew((StrLen(res1)+StrLen(res2)+10)*sizeof(TCHAR));
	StrCopy(result,res2);
	StrCat(result,_T("x+"));
	StrCat(result,res1);

	MemPtrFree(res1);
	MemPtrFree(res2);
	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     list_mode
 * 
 * DESCRIPTION:  Find a modus (most frequent value) in a list
 *
 * PARAMETERS:   list
 *
 * RETURN:       most frequent value
 *      
 ***********************************************************************/
static double list_mode(List *lst) NEWFUNC;
static double
list_mode(List *lst)
{
	double last, most;
	Int16 mostcount,count,i;

	list_sort(lst);

	last = most = lst->item[0].real;
	mostcount = count = 1;
	
	for (i=1;i<lst->size;i++) {
		if (lst->item[i].real != last) {
			last = lst->item[i].real;
			count = 0;
		}
		count ++;
		if (count > mostcount) {
			mostcount = count;
			most = last;
		}
	}
	return most;
}

CError 
list_regr(Functype *func,CodeStack *stack)
{
	List *lstx,*lsty;
	CError err;
	TCHAR *result;

	err = stack_get_val(stack,&lsty,list);
	if (err)
		return err;
	err = stack_get_val(stack,&lstx,list);
	if (err) {
		list_delete(lsty);
		return err;
	}
	
	if (!list_real(lstx) || !list_real(lsty) ||
	    lstx->size != lsty->size)
		return c_badarg;

	switch (func->num) {
	case LIST_LINREG:
		result = list_linreg(lstx,lsty);
		break;
	default:
		return c_internal;
	}
	
	err = stack_add_val(stack,result,string);
	MemPtrFree(result);
	list_delete(lstx);
	list_delete(lsty);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_stat
 * 
 * DESCRIPTION:  Basic statistical functions that work with
 *               'real' lists and return a real number
 *               Some functions may be modified by a second parameter,
 *               that if non-zero forces the variation to be defined
 *               as 'sum((x-mean(x))^2)/n', 
 *               if zero: 'sum((x-mean(x))^2)/(n-1)'
 *
 * PARAMETERS:   list on stack, optional int
 *
 * RETURN:       real num on stack
 *      
 ***********************************************************************/
CError
list_stat(Functype *func,CodeStack *stack)
{
	List *lst;
	CError err;
	double result;
	UInt32 intarg = 0;
	Int16 i;

	if (func->paramcount > 2) 
		return c_badargcount;
	else if (func->paramcount == 2) {
		err = stack_get_val(stack,&intarg,integer);
		if (err)
			return err;
	}

	err = stack_get_val(stack,&lst,list);
	if (err)
		return err;
	
	if (!list_real(lst)) {
		list_delete(lst);
		return c_badarg;
	}

	switch (func->num) {
	case LIST_SKEWNESS:
		if (list_moment(lst,2) != 0.0) {
			result = list_moment(lst,3);
			result /= pow(sqrt(list_moment(lst,2)),3);
		} else
			result = 0.0;
		break;
	case LIST_KURTOSIS:
		if (list_moment(lst,2) != 0.0) {
			result = list_moment(lst,4);
			result /= pow(list_moment(lst,2),2);
			result -= 3.0;
		} else
			result = 0.0;
		break;
	case LIST_MOMENT:
		if (intarg == 0) {
			list_delete(lst);
			return c_badarg;
		}
		result = list_moment(lst,intarg);
		break;
	case LIST_MEAN:
		result = list_mean(lst);
		break;
	case LIST_GMEAN:
		for (i=0,result=1.0;i<lst->size;i++)
			result *= lst->item[i].real;
		result = pow(result,1.0/((double) lst->size));
		break;
	case LIST_VARIANCE:
		if (intarg)
			result = list_moment(lst,2);
		else
			result = list_variance(lst);
		break;
	case LIST_STDDEV:
		if (intarg)
			result = sqrt(list_moment(lst,2));
		else
			result = sqrt(list_variance(lst));
		break;
	case LIST_MODE:
		result = list_mode(lst);
		break;
	case LIST_VARCOEF:
		result = sqrt(list_variance(lst))/list_mean(lst);
		break;
	}
	err = stack_add_val(stack,&result,real);
	list_delete(lst);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_input
 * 
 * DESCRIPTION:  The list() function, takes all arguments and 
 *               creates a list
 *               If the argument is list, matrix is assumed and
 *               matrix is built
 *
 * PARAMETERS:   1..n parametrs on stack
 *
 * RETURN:       list on stack
 *      
 ***********************************************************************/
CError
list_input(Functype *func,CodeStack *stack)
{
	List *l = NULL;
	Matrix *m = NULL;
	CMatrix *cm = NULL;
	Int16 i,j;
	CError err;
	rpntype itemtype;
	Int16 rows,cols=0;

	/* First get the type of the first parameter */
	err = stack_item_type(stack,&itemtype,0);
	if (err)
		return err;
	
	if (itemtype != list) {
		/* We will build a list now */
		l = list_new(func->paramcount);
		for (i=func->paramcount-1;i>=0;i--) {
			err = stack_get_val(stack,&l->item[i],complex);
			if (err) {
				list_delete(l);
			return err;
			}
		}
		err = stack_add_val(stack,&l,list);
		list_delete(l);
		return err;
	}
	/* We will build matrix, because we got list as argument */
        /* First we build complex matrix and then try to scale it down */
	rows = func->paramcount;
	for (i=func->paramcount-1;i>=0;i--) {
		err = stack_get_val(stack,&l,list);
		if (err) {
			if (cm) cmatrix_delete(cm);
			return err;
		}
		if (!cm) {
			/* The first argument must have correct matrix size */
			cols = l->size;
			cm = cmatrix_new(rows,cols);
		}
		if (l->size > cols) {
			cmatrix_delete(cm);
			list_delete(l);
			return c_baddim;
		}
		for (j=0;j < l->size;j++) 
			MATRIX(cm,i,j) = l->item[j];
		
		list_delete(l);
	}
	/* Try to scale the matrix down to normal one */
	m = cmatrix_to_matrix(cm);
	if (m) {
		err = stack_add_val(stack,&m,matrix);
		matrix_delete(m);
	} else 
		err = stack_add_val(stack,&cm,cmatrix);
	cmatrix_delete(cm);
	return err;
}


/***********************************************************************
 *
 * FUNCTION:     list_rand
 * 
 * DESCRIPTION:  Create an array of random numbers (or randNorm)
 *
 * PARAMETERS:   size of the list
 *
 * RETURN:       list of random numbers
 *      
 ***********************************************************************/
CError
list_rand(Functype *func,CodeStack *stack)
{
	List *result;
	CError err;
	UInt32 intarg;
	UInt32 i;

	err = stack_get_val(stack,&intarg,integer);
	if (err)
		return err;
	
	if (intarg == 0)
		return c_badarg;

	result = list_new(intarg);
	if (!result)
	    return c_memory;

	switch (func->num) {
	case LIST_RAND:
		for (i=0;i < intarg; i++) {
			result->item[i].real = RAND_NUM;
			result->item[i].imag = 0.0;
		}
		break;
	case LIST_RNORM:
	default:
		for (i=0;i < intarg; i++) {
			result->item[i].imag = 0.0;
			result->item[i].real = 2*sqrt(-0.5*log(RAND_NUM))*cos(RAND_NUM*2*M_PIl);
		}
		break;
	}
	err = stack_add_val(stack,&result,list);
	list_delete(result);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_item
 * 
 * DESCRIPTION:  The operation of the list[2] type
 *
 * PARAMETERS:   list, item
 *
 * RETURN:       list[item]
 *      
 ***********************************************************************/
CError
list_item(Functype *func, CodeStack *stack)
{
	CError err;
	List *lst;
	UInt32 ini,end;

	if (func->paramcount == 3){
		err = stack_get_val(stack, &end, integer);
		if (err) 
			return err;
	}else if (func->paramcount != 2)
		return c_badargcount;

	err = stack_get_val(stack, &ini, integer);
	if (err) 
		return err;

	if (func->paramcount == 2)
		end=ini;
	
	err = stack_get_val(stack, &lst, list);
	if (err) 
		return err;
	
	if ( ini==0 || end == 0 || ini > lst->size || end > lst->size){
		list_delete(lst);
		return c_baddim;
	}
	
	if(ini==end){
		if (lst->item[end-1].imag == 0.0)
			err = stack_add_val(stack,&lst->item[end-1].real,real);
		else
			err = stack_add_val(stack,&lst->item[end-1],complex);
	}else{
		Int16 i,dif=end-ini;
		List *lst2=list_new(abs(dif)+1);
		if(!lst2){
			list_delete(lst);
			return c_memory;
		}
		for(i=0;i<lst2->size;i++){
			lst2->item[i]=lst->item[ini-1];
			ini += (UInt32) sgn(dif);
		}
		err = stack_add_val(stack,&lst2,list);
		list_delete(lst2);
	}
	
	list_delete(lst);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_range
 * 
 * DESCRIPTION:  range(num[:start[:step]]) generator
 *
 * PARAMETERS:   num[:start[:step]]
 *
 * RETURN:       list[item]
 *      
 ***********************************************************************/
CError
list_range(Functype *func, CodeStack *stack)
{
	List *result;
	CError err;
	UInt32 listlen;
	UInt32 i;
	double first = 1.0;
	double step = 1.0;

	if (func->paramcount > 3)
		return c_badargcount;
	if (func->paramcount == 3) {
		err = stack_get_val(stack,&step,real);
		if (err)
			return err;
	}
	if (func->paramcount >= 2) {
		err = stack_get_val(stack,&first,real);
		if (err)
			return err;
	}
	
	err = stack_get_val(stack,&listlen,integer);
	if (err)
		return err;
	if (listlen == 0)
		return c_badarg;

	result = list_new(listlen);
	if (!result) return c_memory;

	for (i=0;i < listlen;i++) {
		result->item[i].real = first + i*step;
		result->item[i].imag = 0.0;
	}
	err = stack_add_val(stack,&result,list);
	list_delete(result);

	return err;
}

CError
list_mathem(Functype *func, CodeStack *stack)
{
	double arg1,arg2,res;
	List *result;
	CError err;
	
	if ((err=stack_get_val2(stack,&arg1,&arg2,real)))
		return err;
	
	if (func->num==MATH_PTORX || func->num==MATH_PTORY) 
		arg2 = math_user_to_rad(arg2);
	
	result = list_new(2);
	result->item[0].imag = 0.0;
	result->item[1].imag = 0.0;

	switch (func->num) {
	case LIST_RTOP:
		res = atan2(arg2,arg1);
		res = math_rad_to_user(res);
		result->item[0].real = hypot(arg1,arg2);
		result->item[1].real = res;
		break;
	case LIST_PTOR:
		arg2 = math_user_to_rad(arg2);
		result->item[0].real = cos(arg2)*arg1;
		result->item[1].real = sin(arg2)*arg1;
		break;
	}
	err = stack_add_val(stack,&result,list);
	list_delete(result);
	
	return err;	
}

/**************************************************************************
 *
 * FUNCTION:     list_filter
 * 
 * DESCRIPTION:  One-dimensional digital filter.
 *   Y = FILTER(B,A,X) filters the data in vector X with the filter
 *   described by vectors A and B to create the filtered data Y.
 *   The filter is an implementation of the standard difference equation:
 *      a(1)*y(n) = b(1)*x(n) + b(2)*x(n-1) + ... + b(nb+1)*x(n-nb)
 *                  - a(2)*y(n-1) - ... - a(na+1)*y(n-na)
 *
 * PARAMETERS:  3 lists (B,A,X) on stack
 *
 * RETURN:	filtered list on stack
 *      
 ***************************************************************************/
CError
list_filter(Functype *func,CodeStack *stack)
{
	List *b,*a,*x,*y;
	Int16 i,j,k;
	CError err;

	err = stack_get_val(stack,&x,list);
	if (err)
		return err;

	err = stack_get_val(stack,&a,list);
	if (err) {
		list_delete(x);
		return err;
	}
	err = stack_get_val(stack,&b,list);
	if (err) {
		list_delete(x);
		list_delete(a);
		return err;
	}

	/* the first A coefficient can´t be 0 */
	if (a->item[0].real==0 && a->item[0].imag==0){
		list_delete(b);
		list_delete(a);
		list_delete(x);
		return c_badarg;
	}

	y=list_new(x->size);
	if (!y){
		list_delete(b);
		list_delete(a);
		list_delete(x);
		return c_memory;
	}

	/* complex and real separated to compute faster when reals */
	if (list_real(b) && list_real(a) && list_real(x)){

		for(i=0;i<x->size;i++){
		   y->item[i].real=0.0;
		   if(i<b->size) k=i+1;
		    else k=b->size;
		   for(j=0;j<k;j++)
			y->item[i].real+=(b->item[j].real)*(x->item[i-j].real);
		   if(i>0){
		   	if(i<a->size) k=i+1;
		    	 else k=a->size;
		   	for(j=1;j<k;j++)
			  y->item[i].real-=(a->item[j].real)*(y->item[i-j].real);
		   }
		   y->item[i].real/=a->item[0].real;
		   y->item[i].imag=0.0;
		}
	}else{
		Complex tmp;

		for(i=0;i<x->size;i++){
		   y->item[i].real=0.0;
		   y->item[i].imag=0.0;
		   if(i<b->size) k=i+1;
		    else k=b->size;
		   for(j=0;j<k;j++){
			tmp=cplx_multiply(b->item[j],x->item[i-j]);
		      y->item[i].real+=tmp.real;
		      y->item[i].imag+=tmp.imag;}
		   if(i>0){
		   	if(i<a->size) k=i+1;
		    	 else k=a->size;
		   	for(j=1;j<k;j++){
			  tmp=cplx_multiply(a->item[j],y->item[i-j]);
		        y->item[i].real-=tmp.real;
		        y->item[i].imag-=tmp.imag;}
		   }
		   y->item[i]=cplx_div(y->item[i],a->item[0]);
		}
	}

	list_delete(b);
	list_delete(a);
	list_delete(x);
	err = stack_add_val(stack,&y,list);
	list_delete(y);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_conv
 * 
 * DESCRIPTION:  Convolution and polynomial multiplication.
 *	C = CONV(A, B) convolves vectors A and B.
 *	The resulting vector is length LENGTH(A)+LENGTH(B)-1.
 *    If A and B are vectors of polynomial coefficients, convolving
 *    them is equivalent to multiplying the two polynomials.
 *
 * PARAMETERS:   list, list
 *
 * RETURN:       list
 *      
 ***********************************************************************/
List * list_conv(List *lst1, List *lst2) NEWFUNC;
List *
list_conv(List *lst1, List *lst2)
{
	Int16 m,n,N,i,j,k;
	List *result;

	m=lst1->size;
	n=lst2->size-1;
	N=m+n;
	
	result=list_new(N);
	if (!result) return NULL;

	/* complex and real separated to compute faster when reals */
	if (list_real(lst1) && list_real(lst2)){
		double tmp;

		for(i=0;i<N;i++){
		  if(i>n) j=i-n; else j=0;
		  if(i<m) k=i+1; else k=m;
		  for(tmp=0;j<k;j++)
			tmp+=(lst1->item[j].real)*(lst2->item[i-j].real);
		  result->item[i].imag = 0;
		  result->item[i].real = tmp;
		}
	}
	else{
		Complex tmp;

		for(i=0;i<N;i++){
		  if(i>n) j=i-n; else j=0;
		  if(i<m) k=i+1; else k=m;
		  result->item[i].imag=0;
		  result->item[i].real=0;
		  for(;j<k;j++){
			tmp=cplx_multiply(lst1->item[j],lst2->item[i-j]);
		  	result->item[i].imag+= tmp.imag;
		  	result->item[i].real+= tmp.real;
		  }
		}
	}

	return result;
}

/***********************************************************************
 *
 * FUNCTION:     list_func3
 * 
 * DESCRIPTION:  Function that operate with lists
 *
 * PARAMETERS:   list on stack, list on stack
 *
 * RETURN:       list on stack
 *      
 ***********************************************************************/

CError
list_func3(Functype *func, CodeStack *stack)
{
	List *lst1,*lst2,*result;
	CError err;

	err = stack_get_val(stack,&lst2,list);
	if (err)
		return err;
	err = stack_get_val(stack,&lst1,list);
	if (err) {
		list_delete(lst2);
		return err;
	}

	switch (func->num) {

	case LIST_CONV:
	    result=list_conv(lst1,lst2);
	    if (!result){
		list_delete(lst1);
		list_delete(lst2);
		return c_memory;}
	    break;

	case LIST_KRON:
	    {
		Int16 m,n,i,j,k=0;

		m=lst1->size;
		n=lst2->size;
		result = list_new(m*n);
		if (!result){
			list_delete(lst1);
			list_delete(lst2);
			return c_memory;}

		for(i=0;i<m;i++)
		  for(j=0;j<n;j++)
		  	result->item[k++]=cplx_multiply(lst1->item[i],lst2->item[j]);
	    }
	    break;

	case LIST_SAMPLE:
		if (!list_real(lst2)){
			list_delete(lst1);
			list_delete(lst2);
			return c_badarg;}
	    {
		Int16 m,n,i,k;

		m=lst1->size;
		n=lst2->size;

		result = list_new(n);
		if (!result){
			list_delete(lst1);
			list_delete(lst2);
			return c_memory;}

		for(i=0;i<n;i++){
		  k=(Int16)round(lst2->item[i].real);
		  if(k<1 || k>m){
			list_delete(lst1);
			list_delete(lst2);
			list_delete(result);
			return c_badarg;}
		  result->item[i]=lst1->item[k-1];
		}
	    }
	    break;
	}

	list_delete(lst1);
	list_delete(lst2);
	err = stack_add_val(stack,&result,list);
	list_delete(result);
	return err;
}

/**************************************************************************
 *
 * FUNCTION:	list_concat
 * 
 * DESCRIPTION:	concatenation of two or more lists
 *
 * PARAMETERS:	lists on stack
 *
 * RETURN:		list
 *      
 ***************************************************************************/

CError
list_concat(Functype *func, CodeStack *stack)
{
	List *lst1,*lst2,*result;
	Int16 m,n,i,k;
	CError err = c_noerror;

	if (func->paramcount<2)
		return c_syntax;
		
	for(k=1;k<func->paramcount && !err;k++){

		err = stack_get_val(stack,&lst2,list);
		if (err)
			return err;
		err = stack_get_val(stack,&lst1,list);
		if (err) {
			list_delete(lst2);
			return err;
		}

		m=lst1->size;
		n=lst2->size;
		result = list_new(m+n);
		if (!result){
			list_delete(lst1);
			list_delete(lst2);
			return c_memory;
		}
		for(i=0;i<m;i++)
		  result->item[i] = lst1->item[i];
		for(i=0;i<n;i++)
		  result->item[i+m] = lst2->item[i];

		list_delete(lst1);
		list_delete(lst2);
		err = stack_add_val(stack,&result,list);
		list_delete(result);
	}

	return err;
}

/**************************************************************************
 *
 * FUNCTION:	list_fft
 * 
 * DESCRIPTION:	Fast Fourier Transform
 *	FFT(X,m) is the (N=2^m)-point FFT, padded with zeros if X has less
 *	than N points and truncated if it has more.
 *
 * PARAMETERS:	list x: data
 *			integer m: 2^m=N
 *
 * RETURN:		list
 *      
 ***************************************************************************/
List * list_fft(List *x,Int16 m) NEWFUNC;
List *
list_fft(List *x,Int16 m)
{
	Int16 ii,ij,ip,k,L,Lv2,N;
	Complex T,U,W;
	List *y;

	N = (Int16) pow(2,m++);

	if(x->size>N)
	   x->size=N;

	y=list_new(N);
	if (!y)
	  return NULL;

	for(k=0;k<x->size;k++)
		y->item[k]=x->item[k];
	for(;k<N;k++){
		y->item[k].real=0.0;
		y->item[k].imag=0.0;
	}

	ij=0;
	for(ii=0;ii<N-1;ii++){
	   if(ij<ii){
		T=y->item[ij];
		y->item[ij]=y->item[ii];
		y->item[ii]=T;
	   }
	   for(k=N/2;k<ij+1;k/=2)
		ij=ij-k;
	   ij+=k;
	}

	for(k=1;k<m;k++){
	   L = (Int16) pow(2,k);
	   Lv2=L/2;
	   U.real=1.0;
	   U.imag=0.0;
	   T.real=0.0;
	   T.imag=-M_PIl/Lv2;
	   W=cplx_exp(T);
	   for(ij=0;ij<Lv2;ij++){
		for(ii=ij;ii<N;ii+=L){
		   ip=ii+Lv2;
		   T=cplx_multiply(y->item[ip],U);
		   y->item[ip].real=y->item[ii].real-T.real;
		   y->item[ip].imag=y->item[ii].imag-T.imag;
		   y->item[ii].real+=T.real;
		   y->item[ii].imag+=T.imag;
		}
		U=cplx_multiply(U,W);
	   }
	   U=cplx_multiply(U,W);
	}

	return y;
}

/**************************************************************************
 *
 * FUNCTION:	list_dft
 * 
 * DESCRIPTION:	Discrete Fourier Transform
 *	This procedure is very slow, it can hold your device for a 
 *    loooooonnnnng time with large values of N. Use FFT instead.
 *
 * PARAMETERS:	list, integer
 *
 * RETURN:		list
 *      
 ***************************************************************************/
List * list_dft(List *x,UInt32 n) NEWFUNC;
List *
list_dft(List *x,UInt32 n)
{
	List *y;
	UInt16 i,j,a;
	Complex xj,tmp;

	y=list_new(n);
	if (!y)
	  return NULL;


      /* to compute faster when x is real*/
	if (list_real(x))
	  a=n/2+1;
	else
	  a=n;
	
	for(i=0;i<a;i++){
	  y->item[i].real=0.0;
	  y->item[i].imag=0.0;
	  for(j=0;j<n;j++){
		if(j>=x->size){
		  xj.real=0.0;
		  xj.imag=0.0;
		}else
		  xj=x->item[j];
		tmp.real=0.0;
		tmp.imag=-2*M_PIl*i*j/n;
		tmp=cplx_multiply(xj,cplx_exp(tmp));
		y->item[i].real+=tmp.real;
		y->item[i].imag+=tmp.imag;
	  }
	}

	/* DFT hermitic when x real*/
	if(a!=n)
	  for(i=1;n>a;i++){
		y->item[--n].real=y->item[i].real;
		y->item[n].imag=-(y->item[i].imag);
	  }

	return y;
}

/**************************************************************************
 *
 * FUNCTION:	list_fourier
 * 
 * DESCRIPTION:	Computes fft, ifft, dft,ift and fftshift
 *
 * PARAMETERS:	list,integer
 *
 * RETURN:		list
 *      
 ***************************************************************************/
CError
list_fourier(Functype *func,CodeStack *stack)
{
	List *data,*result;
	UInt32 n=0;
	CError err;

	if (func->paramcount > 2) 
		return c_badargcount;
	else if (func->paramcount == 2) {
		err = stack_get_val(stack,&n,integer);
		if (err)
			return err;
	}
	
	err = stack_get_val(stack,&data,list);
	if (err)
		return err;

	if(!n) n=data->size;

	switch (func->num) {

	case LIST_FFT:
	   {
	    Int16 m;

	    m = (Int16) ceil(log(n)/log(2));

	    result=list_fft(data,m);
	    if (!result){
		list_delete(data);
		return c_memory;}
	    break;
	   }

	case LIST_IFFT:
	   {
	    Int16 i,m;

	    m = (Int16) ceil(log(n)/log(2));

	    n = (UInt32) pow(2,m);
	    for(i=0;i<data->size;i++){
		data->item[i].real/=n;
		data->item[i].imag=-(data->item[i].imag)/n;
	    }
	    result=list_fft(data,m);
	    if (!result){
		list_delete(data);
		return c_memory;}

	    break;
	   }

	case LIST_DFT:
	    result=list_dft(data,n);
	    if (!result){
		list_delete(data);
		return c_memory;}
	    break;

	case LIST_IDFT:
	   {
	    Int16 i;

	    for(i=0;i<data->size;i++){
		data->item[i].real/=n;
		data->item[i].imag=-(data->item[i].imag)/n;
	    }
	    result=list_dft(data,n);
	    if (!result){
		list_delete(data);
		return c_memory;}
	    break;
	   }

	case LIST_DFTSHIFT:
	   {
	    UInt16 i,a;

	    result=list_new(data->size);
	    if (!result){
		list_delete(data);
		return c_memory;}

	    a=n/2+n%2;
	    for(i=0;i<n-a;i++)
		result->item[i]=data->item[i+a];
	    for(a=0;i<n;i++)
		result->item[i]=data->item[a++];
	    break;
	   }
	}

	list_delete(data);
	err = stack_add_val(stack,&result,list);
	list_delete(result);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_factor
 * 
 * DESCRIPTION:  Compute prime factors of a number
 *
 * PARAMETERS:   integer on stack
 *
 * RETURN:       list of factors on stack
 *      
 ***********************************************************************/

CError 
list_factor(Functype *func,CodeStack *stack)
{
	List *result;
	CError err;
	UInt8 fcount;
	UInt32 factors[32];

	UInt32 intarg;
	UInt8 i;


	err = stack_get_val(stack,&intarg,integer);
	if (err)
		return err;
	
	fcount = factorize(intarg, (Int32 *) factors);

	result = list_new(fcount);

	if (!result)
	    return c_memory;	

	for (i=0;i < fcount; i++) {
		result->item[i].real = factors[i];
		result->item[i].imag = 0.0;
	}
	err = stack_add_val(stack,&result,list);
	list_delete(result);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_gcdex
 * 
 * DESCRIPTION:  Compute extended gcd of two numbers
 *
 * PARAMETERS:   two integers on stack
 *
 * RETURN:       list on stack:
 *               [first arg : its cofactor : second arg : its cofactor :
 *                gcd : lcm]
 *      
 ***********************************************************************/

CError 
list_gcdex(Functype *func,CodeStack *stack)
{
	List *result;
	CError err;
	UInt32 arg1, arg2, gcd, lcm;
	Int32 co1, co2;
	UInt8 i;

	err = stack_get_val2(stack,&arg1, &arg2,integer);
	if (err)
		return err;
	
	gcd = gcdex(arg1, arg2, &co1, &co2);
	lcm = gcd == 0 ? 0 : arg1 * arg2 / gcd;

	result = list_new(6);

	if (!result)
	    return c_memory;	

	for (i=0;i < 6; i++) {
		result->item[i].imag = 0.0;
	}

	result->item[0].real = arg1;
	result->item[1].real = co1;
	result->item[2].real = arg2;
	result->item[3].real = co2;
	result->item[4].real = gcd;
	result->item[5].real = lcm;

	err = stack_add_val(stack,&result,list);
	list_delete(result);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_sift
 * 
 * DESCRIPTION:  filter a list based on a list or function
 *
 * PARAMETERS:   two lists or function and list on stack
 *
 * RETURN:       filtered list, or integer 0 if list would be empty,
 *               on stack
 *               (FIXME: easycalc does not like empty lists)
 *      
 ***********************************************************************/

CError
list_sift(Functype *func,CodeStack *stack)
{
	List *lst;
	List *flst = NULL;
	List *result;
	CodeStack *fnc = NULL;
	CError err;
	Int16 i, j;
	Complex c;
	rpntype tp;

	err = stack_item_type(stack, &tp, 1);
	if (err)
		return err;

	if (tp == funcempty || tp == string)
		tp = function;

	if (tp != function && tp != list)
		return c_badarg;

	err = stack_get_val(stack,&lst,list);
	if (err)
		return err;


	if (tp == function) {
		err = stack_get_val(stack,&fnc,function);
	} else {
		err = stack_get_val(stack, &flst, list);
	}
	if (err) {
		list_delete(lst);
		return err;
	}
	if (flst != NULL && flst->size != lst->size) {
		list_delete(lst);
		list_delete(flst);
		return c_baddim;
	}

	j=0;
	for(i=0; i<lst->size; i++) {
		Boolean keep = false;
		if (tp == function) {
			err = func_get_value_cplx(fnc, &(lst->item[i]), 1, &c);
			if(err) {
				stack_delete(fnc);
				list_delete(lst);
				return err;
			}
			keep = (c.real != 0.0 || c.imag != 0.0);
		} else {
			keep = flst->item[i].real != 0.0 || flst->item[i].imag != 0.0;
		}
		if (keep) {
			if (func->num == LIST_FIND) {
				lst->item[j].real = i+1;
				lst->item[j].imag = 0.0;
			} else {
				lst->item[j]=lst->item[i];
			}
			j++;
		}
	}
	
	if (tp == function) {
		stack_delete(fnc);
	} else {
		list_delete(flst);
	}
	if (j == 0) {
		err = stack_add_val(stack, &j, integer);
		list_delete(lst);
	} else {
		result = list_new(j);
		if (!result)
			return c_memory;
		for(i=0; i < j; i++) {
			result->item[i] = lst->item[i];
		}
		list_delete(lst);
		err = stack_add_val(stack,&result,list);
		list_delete(result);
	}
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_map
 * 
 * DESCRIPTION:  map all values of a list with a function
 *
 * PARAMETERS:   one-parameter-function and list on stack
 *
 * RETURN:       mapped list on stack
 *      
 ***********************************************************************/

CError
list_map(Functype *func,CodeStack *stack)
{
	CodeStack *fnc;
	CError err;
	Int16 i;
	Complex c;
	Int16 listcount;

	if (func->paramcount < 2)
		return c_badargcount;
		
	listcount = func->paramcount - 1;
	
	if (listcount == 1) {
		// simple case: only one list
		List *lst;

		err = stack_get_val(stack,&lst,list);
		if (err)
			return err;
		
		err = stack_get_val(stack,&fnc,function);
		if (err) {
			list_delete(lst);
			return err;
		}	
		
		for(i=0; i<lst->size; i++) {
			err = func_get_value_cplx(fnc, &(lst->item[i]), 1, &c);
			if(err) {
				stack_delete(fnc);
				list_delete(lst);
				return err;
			}
			lst->item[i] = c;
		}
		
		stack_delete(fnc);
		err = stack_add_val(stack,&lst,list);
		list_delete(lst);
	} else {
		// general case: more than one list
		List **lsts;
		UInt16 *indices;
		Complex *values;
		UInt16 resultsize = 1;
		List *result;
		Int16 resultindex;
		
		lsts = (List **) MemPtrNew(sizeof(List*) * listcount);
		if (lsts == NULL) 
			return c_memory;
		
		indices = (UInt16 *) MemPtrNew(sizeof(UInt16) * listcount);
		if (indices == NULL) {
			MemPtrFree(lsts);
			return c_memory;
		}

		values = (Complex *) MemPtrNew(sizeof(Complex) * listcount);
		if (values == NULL) {
			MemPtrFree(lsts);
			MemPtrFree(indices);
			return c_memory;
		}

		// make everything NULL to simplify cleanup
		for(i=0; i<listcount; i++) {
			lsts[i] = NULL;
			indices[i] = 0;
		}
		fnc = NULL;
		result = NULL;

		for(i=0; i<listcount; i++) {
			err = stack_get_val(stack,&lsts[i],list);
			if (err) 
				goto map_cleanup;
			resultsize *= lsts[i]->size;
		}
		
		err = stack_get_val(stack,&fnc,function);
		if (err) 
			goto map_cleanup;
		
		result = list_new(resultsize);
		if (result == NULL) {
			err = c_memory;
			goto map_cleanup;
		}

		for(resultindex=0; resultindex < resultsize; resultindex++) {
			for(i=0; i<listcount; i++) {
				values[i] = lsts[i]->item[indices[i]];
			}
			err = func_get_value_cplx(fnc, values, listcount, &c);
			if(err) {
				goto map_cleanup;
			}
			result->item[resultindex] = c;
			for(i=listcount-1; i>=0; i--) {
				indices[i]++;
				if (indices[i] == lsts[i]->size) {
					indices[i] = 0;
				} else {
					break;
				}
			}

		}

		err = stack_add_val(stack,&result,list);
	map_cleanup:
		if (fnc != NULL) stack_delete(fnc);
		if (result != NULL) list_delete(result);
		for(i=0; i<listcount; i++) {
			if (lsts[i] != NULL) {
				list_delete(lsts[i]);
			}
		}
		MemPtrFree(lsts);
		MemPtrFree(indices);
		MemPtrFree(values);
	}
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_zip
 * 
 * DESCRIPTION:  combine values of lisst with a function
 *
 * PARAMETERS:   n-parameter-function and n lists (equal length) on stack
 *
 * RETURN:       zipped list on stack
 *      
 ***********************************************************************/

CError
list_zip(Functype *func,CodeStack *stack)
{
	Int16 listcount;
	CError err;
	Int16 i, j;
	CodeStack *fnc;
	Complex c;
	Complex *values;
	List **lsts;

	if (func->paramcount < 2)
		return c_badargcount;
		
	listcount = func->paramcount - 1;
	
		
	lsts = (List **) MemPtrNew(sizeof(List*) * listcount);
	if (lsts == NULL) 
		return c_memory;
	
	values = (Complex *) MemPtrNew(sizeof(Complex) * listcount);
	if (values == NULL) {
		MemPtrFree(lsts);
		return c_memory;
	}

	// make everything NULL to simplify cleanup
	for(i=0; i<listcount; i++) {
		lsts[i] = NULL;
	}
	fnc = NULL;

	for(i=0; i<listcount; i++) {
		err = stack_get_val(stack,&lsts[i],list);
		if (err) 
			goto zip_cleanup;
		if (lsts[i]->size != lsts[0]->size) {
			err = c_baddim;
			goto zip_cleanup;
		}
	}
	
	err = stack_get_val(stack,&fnc,function);
	if (err) 
		goto zip_cleanup;

	for(i=0; i < lsts[0]->size; i++) {
		for(j=0; j<listcount; j++) {
			values[j] = lsts[j]->item[i];
		}
		err = func_get_value_cplx(fnc, values, listcount, &c);
		if(err) {
			goto zip_cleanup;
		}
		lsts[0]->item[i] = c;
	}

	err = stack_add_val(stack,&lsts[0],list);
	zip_cleanup:
	if (fnc != NULL) stack_delete(fnc);
	for(i=0; i<listcount; i++) {
		if (lsts[i] != NULL) {
			list_delete(lsts[i]);
		}
	}
	MemPtrFree(lsts);
	MemPtrFree(values);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     list_repeat
 * 
 * DESCRIPTION:  Repeat blocks of lists or whole lists
 *
 * PARAMETERS:   list and two integers on stack
 *               (list, count, blocksize)
 *               blocksize == 0 means repeat whole list
 *               blocksize == 1 means repeat each element
 *               and so on (0 <= blocksize <= length of list)
 *
 * RETURN:       repeated list on stack
 *      
 ***********************************************************************/

CError
list_repeat(Functype *func,CodeStack *stack)
{
	List *lst;
	List *result;
	CError err;
	UInt16 i, j, k, c;
	UInt32 repeat, blocksize;

	err = stack_get_val2(stack, &repeat, &blocksize, integer);
	if (err)
		return err;
	if (repeat == 0) 
		return c_badarg;
			

	err = stack_get_val(stack,&lst,list);
	if (err)
		return err;
	if (blocksize == 0)
		blocksize = lst->size;
	
	result = list_new(lst->size * repeat);
	if (!result) {
		list_delete(lst);
		return c_memory;
	}

	c = 0;
	for(i=0; i<lst->size; i+=blocksize) {
		for(k=0; k<repeat; k++) {
			for (j=i; j<i+blocksize && j<lst->size; j++) {
				result->item[c] = lst->item[j];
				c++;
			}
		}
	}
	
	list_delete(lst);
	err = stack_add_val(stack,&result,list);
	list_delete(result);

	return err;
}
