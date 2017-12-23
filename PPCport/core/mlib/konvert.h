/*   
 *   $Id: konvert.h,v 1.3 2009/10/17 13:48:34 mapibid Exp $
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


#ifndef _konvert_h_
#define _konvert_h_

#include "compat/segment.h"

#define MAX_FUNCNAME 10
#define MAX_VARNAME 10

/* First function/variable character */
#define IS_FUNC_1LETTER(x) (((x)>=_T('a') && (x)<=_T('z')) || (x)==_T('_') || (x)==(TCHAR)181 || \
                           ((x)>=_T('G') && (x)<=_T('Z')))
/* Other characters, inside varname can be numbers */
#define IS_FUNC_LETTER(x)  (IS_FUNC_1LETTER(x) || ((x)>=_T('0') && (x)<=_T('9')) || \
								((x)>=_T('A') && (x)<=_T('Z')))

typedef enum {
	degree,
	radian,
	grad
}Ttrigo_mode;

typedef enum {
	notype=0,
	integer,
	real,
	function,
	functext,
	funcempty,
	variable,
	string,	
/* These 2 next shouldn't be in rpn, it's temporary */	
	oper,
	discard,
	complex,
	list,
	matrix,
	litem,
	cmatrix
}rpntype;

typedef enum {
	c_noerror=0,
	c_syntax,
	c_badfunc,
	c_noarg,	
	c_badarg,
	c_internal,
	c_badmode,
	c_noresult,
	c_divbyzero,
	c_notvar,
	c_badresult,
	c_deeprec,
	c_compimp,
	c_range,
	c_badargcount,
	c_baddim,
	c_singular,
	c_interrupted,
	c_memory,
	c_stacklow
}CError;

typedef struct {
	double real;
	double imag;
}Complex;

typedef struct {
	double r;
	double angle;
}Complex_gon;

typedef struct {
	Int16 offs;
	Int16 num;
	Int16 paramcount;
}Functype;

typedef struct {
	Int16 paramcount;
	TCHAR name[MAX_FUNCNAME+1];
}TextFunctype;

typedef struct {
	Int16 row;
	Int16 col;
	TCHAR name[MAX_FUNCNAME+1];
}LitemType;

typedef struct {
	UInt16 size;
	Complex item[1];
}List;

#define MATRIX(m,r,c)   m->item[m->cols * (r) + (c)]
typedef struct {
	Int16 rows,cols;
	double item[1];
}Matrix;

typedef struct {
	Int16 rows,cols;
	Complex item[1];
}CMatrix;
	
/* item on the RPN stack */
typedef struct {
	rpntype type;
	UInt16 allocsize; /* If we should free the item or not */
	union {
		UInt32 intval;
		double realval;
		Functype funcval;  /* Internal function call */
		TextFunctype textfunc; /* User defined function call */
		LitemType litemval;
		TCHAR funcname[MAX_FUNCNAME+1]; /* Empty function */
		TCHAR varname[MAX_FUNCNAME+1];
		TCHAR *stringval;
		Complex *cplxval;
		TCHAR *data;
		List *listval;
		Matrix *matrixval;
		CMatrix *cmatrixval;
	}u;
}Trpn;

typedef struct {
	Boolean evalRight;
	Int16 priority;
	Int16 operpri;
	Trpn rpn;
}Tmeq;

/* stack */
typedef struct {
	UInt16 size;
	UInt16 allocated;
	Trpn *stack;
}CodeStack;

struct Tdefined_funcs {
	const TCHAR name[MAX_FUNCNAME+1];
	Int16 funcnum;
	CError (*function)(Functype *func,CodeStack *stack);
	Int16 paramcount;
	Boolean listsimul;
	Boolean matrixsimul;
};

CodeStack *text_to_stack(TCHAR *buf, CError *error);
Err mlib_init_mathlib(void) MLIB;
void mlib_close_mathlib(void) MLIB;
TCHAR ** konvert_get_sorted_fn_list(Int16 *count);

extern const struct Tdefined_funcs defined_funcs[];
extern Ttrigo_mode trigo_mode;
extern TCHAR parameter_name[MAX_FUNCNAME+1];

#endif