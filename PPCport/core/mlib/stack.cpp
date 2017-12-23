/*
 *   $Id: stack.cpp,v 1.5 2009/10/17 13:48:34 mapibid Exp $
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
*/


#include "StdAfx.h"

#include "compat/PalmOS.h"
#include <string.h>

#include "konvert.h"
#include "funcs.h"
#include "stack.h"
#include "calcDB.h"
#include "slist.h"
#include "matrix.h"
#include "cmatrix.h"
#include "compat/MathLib.h"
#include "core/prefs.h"

/***********************************************************************
 *
 * FUNCTION:     rpn_pack_record
 * 
 * DESCRIPTION:  Pack a record so that it can be stored in the database
 *
 * PARAMETERS:   item
 *
 * RETURN:       packed item
 *      
 ***********************************************************************/
dbStackItem *
rpn_pack_record(Trpn item)
{
	dbStackItem *packed;	

	packed = (dbStackItem *) MemPtrNew(item.allocsize + sizeof(*packed));
	if (!packed)
		return NULL;

	packed->rpn = item;
	packed->datasize = item.allocsize;
	if (item.allocsize)
		memcpy(packed->data,item.u.data,item.allocsize);

	return packed;
}

Trpn
rpn_unpack_record(dbStackItem *record)
{
	Trpn result;

	result = record->rpn;
	result.allocsize = record->datasize;
	if (record->datasize) {
		result.u.data = (TCHAR *) MemPtrNew(record->datasize);
		memcpy(result.u.data,record->data,record->datasize);
	}
	
	return result;
}

void
rpn_delete(Trpn item)
{
	if (item.allocsize)
		MemPtrFree(item.u.stringval);
}

/***********************************************************************
 *
 * FUNCTION:     rpn_duplicate
 * 
 * DESCRIPTION:  Copy the rpn, duplicating all associated memory
 *
 * PARAMETERS:   dest, source
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
rpn_duplicate(Trpn *dest,Trpn source)
{
	*dest = source;
	if (source.allocsize) {
		dest->u.data = (TCHAR *) MemPtrNew(source.allocsize);
		memcpy(dest->u.data,source.u.data,source.allocsize);
	}
}	

/***********************************************************************
 *
 * FUNCTION:     stack_unpack
 * 
 * DESCRIPTION:  Unpack one-block representation of stack (from DB)
 *               back to usable form
 *
 * PARAMETERS:   packed - packed stack
 *
 * RETURN:       unpacked stack
 *      
 ***********************************************************************/
CodeStack *
stack_unpack(dbPackedStack *packed)
{
	CodeStack *result;
	Trpn item;
	Int16 i,ptr;
	void *tmp;
	dbStackItem *stmp;
	
	result = stack_new(packed->header.count);
	
	ptr=0;
	for (i=0;i<packed->header.count;i++) {
		tmp = packed->data + ptr;
		stmp = (dbStackItem *)tmp;
		item = rpn_unpack_record(stmp);
		stack_push(result,item);
		ptr+=sizeof(dbStackItem)+stmp->datasize;
	}	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     stack_pack
 * 
 * DESCRIPTION:  Create one-block representation of stack, so that
 *               it can be written into DB
 *
 * PARAMETERS:   stack - stack to be packed
 *               *size - resulting pack size
 *
 * RETURN:       Packed stack
 *      
 ***********************************************************************/
dbPackedStack *
stack_pack(CodeStack *stack,Int16 *size)
{
	Int16 asize=0,isize;
	Int16 i,ptr;
	dbPackedStack *result;
	dbStackItem *item;	
	
	for (i=0;i<stack->size;i++) {
		isize = sizeof(*item)+stack->stack[i].allocsize;
		/* Count in the alignment padding */
		isize += isize % 2;
		asize += isize;
	}
	*size = asize+sizeof(*result);
	result = (dbPackedStack *) MemPtrNew(*size);
	result->header.count = stack->size;
	result->header.orig_offset = asize;
	
	ptr = 0;
	for (i=0;i<stack->size;i++) {
		item = rpn_pack_record(stack->stack[i]);
		isize = sizeof(*item) + item->datasize;
		/* PalmOS doesn't like unaligned structures */
		if (isize % 2) {
			item->datasize++;
			memcpy(result->data+ptr,item,isize);
			isize++;
		} else {
			memcpy(result->data+ptr,item,isize);
		}
		ptr+=isize;
		MemPtrFree(item);
	}
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     stack_copy
 * 
 * DESCRIPTION:  Copy one stack over another
 *
 * PARAMETERS:   dest,orig
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
stack_copy(CodeStack *dest,CodeStack *orig)
{
	Int16 i;
	if (dest->allocated < orig->size) 
		ErrFatalDisplayIf(1, _T("Trying to copy larger stack into smaller."));
	for (i=0 ; i<(orig->size) ; i++)
		rpn_duplicate(&dest->stack[i],orig->stack[i]);
	dest->size = orig->size;
}

/***********************************************************************
 *
 * FUNCTION:     stack_get_val
 * 
 * DESCRIPTION:  Get a requested type of value from stack,
 *               conversion is applied when necessary
 *
 * PARAMETERS:   stack - stack from which to read value
 *               arg1 - where will be the value stored
 *               reqtype - requested type (integer,real,list,string,complex)
 *
 * RETURN:       err
 *      
 ***********************************************************************/
CError
stack_get_val(CodeStack *stack,void *arg1,rpntype reqtype)
{
	double realarg = 0.0;
	Complex *cplxarg;
	CError err=c_noerror;
	Trpn tmpitem;
	
	if (stack->size<1)
		return c_noarg;

	tmpitem = stack_pop(stack);
	if (tmpitem.type == variable || tmpitem.type == litem) {
		err = rpn_eval_variable(&tmpitem,tmpitem);
		if (err)
			return err;
	}

#ifdef SPECFUN_ENABLED
	if (reqtype == matrix) {
		/* First try to convert complex matrix to matrix */
		if (tmpitem.type == cmatrix) {
			*((Matrix**)arg1) = cmatrix_to_matrix(tmpitem.u.cmatrixval);
			/* Delete old cmatrix */
			rpn_delete(tmpitem);
			if (*((Matrix **)arg1) == NULL) 
				return c_badarg;
			return c_noerror;
		}
				
		if (tmpitem.type != matrix) {
			rpn_delete(tmpitem);
			return c_badarg;
		}
		*((Matrix**)arg1) = tmpitem.u.matrixval;
		/* Do not rpn_delete, because the matrixval is 
		 * returned */
		return c_noerror;
	}

	if (reqtype == cmatrix) {
		/* Convert matrix to cmatrix if requested */
		if (tmpitem.type == matrix) {
			*((CMatrix**)arg1) = matrix_to_cmatrix(tmpitem.u.matrixval);
			rpn_delete(tmpitem);
			return c_noerror;
		}
		if (tmpitem.type != cmatrix) {
			rpn_delete(tmpitem);
			return c_badarg;
		}
		*((CMatrix**)arg1) = tmpitem.u.cmatrixval;
		/* Do not rpn_delete, because the matrixval is returned */
		return c_noerror;
	}

	if (reqtype == list) {
		if (tmpitem.type != list) {
			rpn_delete(tmpitem);
			return c_badarg;
		}
		/* This is a speed optimization !!!!! */
		/* Do not rpn_delete it, because of the listval */ 
		*((List**)arg1) = tmpitem.u.listval;
		return c_noerror;
	}
#endif
	if (reqtype == function) {
		CodeStack *fcstack;

		if (tmpitem.type==funcempty) 
			fcstack = db_read_function(tmpitem.u.funcname,&err);
		else if (tmpitem.type==string) 
			fcstack = text_to_stack(tmpitem.u.stringval,&err);
		else {
			fcstack = NULL; /* Make gcc happy */
			err = c_badarg;
		}
		rpn_delete(tmpitem);
		if (!err) 
			*((CodeStack **)arg1) = fcstack;
		return err;
	}
  
	if (reqtype==string) {
		TCHAR *tmp;

		if (tmpitem.type!=string) {
			rpn_delete(tmpitem);
			return c_badarg;
		}
		tmp = (TCHAR *) MemPtrNew((StrLen(tmpitem.u.stringval)+1)*sizeof(TCHAR));
		StrCopy(tmp,tmpitem.u.stringval);
		*((TCHAR **)arg1)=tmp;

		rpn_delete(tmpitem);
		return c_noerror;
	}

	/* Now follows numeric types */

	if (tmpitem.type != integer &&
	    tmpitem.type != real &&
	    tmpitem.type != complex) {
		rpn_delete(tmpitem);
		return c_badarg;
	}

	if (reqtype == complex) {
		cplxarg = (Complex *) arg1;
		cplxarg->imag = 0.0;
		if (tmpitem.type == complex) 
			*cplxarg = *(tmpitem.u.cplxval);
		else if (tmpitem.type == real) 
			cplxarg->real = tmpitem.u.realval;
		else if (tmpitem.type == integer)
			cplxarg->real = (Int32)tmpitem.u.intval;
		else
			err = c_badarg;
	} else if (reqtype == real) {
		if (tmpitem.type == real)
			*((double *)arg1) = tmpitem.u.realval;
		else if (tmpitem.type == integer)
			*((double *)arg1) = (Int32)tmpitem.u.intval;
		else if (tmpitem.type == complex) {
			if (IS_ZERO(tmpitem.u.cplxval->imag))
				*((double *)arg1) = tmpitem.u.cplxval->real;
			else
				err = c_badarg;
		} else
			err = c_badarg;
	} else if (reqtype == integer) {
		switch (tmpitem.type) {
		 case integer:
			*((UInt32 *)arg1) = tmpitem.u.intval;
			break;
		 case complex:
			if (IS_ZERO(tmpitem.u.cplxval->imag))
				realarg = tmpitem.u.cplxval->real;
			else {
				err = c_badarg;
				break;
			}
			/* PASS THROUGH, generates warning*/
		 case real:
			if (tmpitem.type == real)
				realarg = tmpitem.u.realval;
			if (-2147483648.0 <= realarg && realarg <= 2147483647.0) {
				*((Int32 *)arg1) = ((Int32) tmpitem.u.realval);
			}
			else
				err = c_badarg;
			break;
		 default:
			err = c_badarg;
		}
	}
	else	  
		err=c_badarg;

	rpn_delete(tmpitem);	
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     stack_get_val2
 * 
 * DESCRIPTION:  Get 2 parameters for stack - designed for 2-parameter
 *               functions with same arg type
 *
 * PARAMETERS:   stack, arg1, arg2, reqtype
 *
 * RETURN:       err
 *      
 ***********************************************************************/
CError
stack_get_val2(CodeStack *stack,
	     void *arg1,void *arg2,rpntype reqtype)
{
	CError err=c_noerror;
	
	if ((err=stack_get_val(stack,arg2,reqtype)))
		return err;       
	err=stack_get_val(stack,arg1,reqtype);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     reduce_precision
 * 
 * DESCRIPTION:  Reduce precision of the floating point number 
 *               representation by rounding the number depending
 *               of the size of the number
 *
 * PARAMETERS:   value
 *
 * RETURN:       ''round''(value)
 *      
 ***********************************************************************/
double
reduce_precision(double value) 
{
	double m, l;
	
	l = round(ROUND_OFFSET - log10(value));
	if (finite(l) && l > 2.0) {
		m = pow(10.0, l);
		value = round(m * value) / m;
	}
	return value;
}

/***********************************************************************
 *
 * FUNCTION:     stack_add_val
 * 
 * DESCRIPTION:  Generic function for adding a result on stack
 *
 * PARAMETERS:   stack - stack where to add value
 *               arg - pointer to value that should be added on stack
 *               reqtype - type of value
 *
 * RETURN:       err
 *      
 ***********************************************************************/
CError
stack_add_val(CodeStack *stack,void *arg,rpntype reqtype)
{
	Trpn value;
	
	value.type=reqtype;
	value.allocsize=0;
	if (reqtype==real)
		value.u.realval=*(double *)arg;
	else if (reqtype == integer)
		value.u.intval=*(UInt32 *)arg;
	else if (reqtype == complex) {
		/* Automatically convert complexes back to real if possible */
		if (((Complex *)arg)->imag == 0.0) {
			value.u.realval = ((Complex *)arg)->real;
			value.type = real;
		} else {
			value.u.cplxval = (Complex *) MemPtrNew(sizeof(Complex));
			value.allocsize = sizeof(Complex);
			*(value.u.cplxval) = *(Complex *)arg;
		}
	} else if (reqtype == string) {
		value.allocsize = (StrLen((TCHAR *)arg)+1)*sizeof(TCHAR);
		value.u.stringval = (TCHAR *) MemPtrNew(value.allocsize);
		StrCopy(value.u.stringval,(TCHAR *)arg);
#ifdef SPECFUN_ENABLED
	} else if (reqtype == list) {
		value.u.listval = list_dup(*((List **)arg));
		if (!value.u.listval)
			return c_memory;
		value.allocsize = sizeof(*value.u.listval) +
			sizeof(value.u.listval->item[0]) * value.u.listval->size;
	} else if (reqtype == matrix) {
		value.u.matrixval = matrix_dup(*((Matrix **)arg));
		if (!value.u.matrixval)
			return c_memory;
		value.allocsize = sizeof(*value.u.matrixval) +
			sizeof(value.u.matrixval->item[0]) * \
			value.u.matrixval->rows * value.u.matrixval->cols;
	} else if (reqtype == cmatrix) {
		value.u.cmatrixval = cmatrix_dup(*((CMatrix **)arg));
		if (!value.u.cmatrixval)
			return c_memory;
		value.allocsize = sizeof(*value.u.cmatrixval) +
			sizeof(value.u.cmatrixval->item[0]) *
			value.u.cmatrixval->rows * value.u.cmatrixval->cols;
#endif
	} else
		return c_badmode;
	
	stack_push(stack,value);
	return c_noerror;
}

CError 
stack_item_type_nr(CodeStack *stack,rpntype *type,Int16 itnum)
{
	Trpn tmp;

	if (stack->size<itnum)
		return c_internal;
	tmp = stack->stack[stack->size - itnum -1];
	
	*type = tmp.type;
	return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     stack_item_type
 * 
 * DESCRIPTION:  Type of item on stack, if it is variable then the
 *               type of variable is returned
 *
 * PARAMETERS:   stack - the stack from which the item is checked
 *               type - pointer to variable where to return the type
 *               itnum - the number of item on stack (0 - first to pop..)
 *
 * RETURN:       error
 *      
 ***********************************************************************/
CError
stack_item_type(CodeStack *stack,rpntype *type,Int16 itnum)
{
	Trpn tmp;
	CError err;
	
	if (stack->size<itnum)
		return c_internal;
	tmp = stack->stack[stack->size - itnum -1];
	
	if (tmp.type!=variable && tmp.type!=litem) {
		*type = tmp.type;
		return c_noerror;
	}
	/* Get the type of the result of the variable */
	err = rpn_eval_variable(&tmp,tmp);
	if (err)
		return err;
	
	*type = tmp.type;
	rpn_delete(tmp);
	
	return c_noerror;
}


#ifdef SPECFUN_ENABLED
/***********************************************************************
 *
 * FUNCTION:     stack_listsimul
 * 
 * DESCRIPTION:  Run a function for every item of a list when a list(s)
 *               was(were) given as a parameter. The lists must be of 
 *               same dimension. 
 *
 * PARAMETERS:   fce - offset of function to defined_funcs[]
 *               Functype - Functype argument to fuction
 *               varstack - variable stack for function
 *
 * RETURN:       err
 *      
 ***********************************************************************/
static CError 
stack_listsimul(Int16 fce,Functype *funcval,CodeStack *varstack) MLIB;
static CError 
stack_listsimul(Int16 fce,Functype *funcval,CodeStack *varstack)
{
	List *lst,*lstres;
	CodeStack *tmpstack,*cmpstack;
	Trpn item;
	CError err=c_noerror;
	Int16 i,j;
	Int16 listsize = 0;
	rpntype type;

	/* Find out dimension of the list */
	tmpstack = stack_new(funcval->paramcount);
	for (i=0;i < funcval->paramcount;i++) {
		item = stack_pop(varstack);
		if (item.type == variable || item.type == litem) {
			err = rpn_eval_variable(&item,item);
			if (err) {
				rpn_delete(item);
				stack_delete(tmpstack);
				return err;
			}
		}
		type = item.type;
		if (type == list) {
			if (listsize == 0)
				listsize = item.u.listval->size;
			else if (listsize != item.u.listval->size) {
				rpn_delete(item);
				stack_delete(tmpstack);
				return c_baddim;
			}
		}
		stack_push(tmpstack,item);
	}

	lstres = list_new(listsize);
	if (!lstres) {
		stack_delete(tmpstack);
		return c_memory;
	}
	cmpstack = stack_new(funcval->paramcount);
	for (i=0;i<listsize;i++) {
		stack_copy(cmpstack,tmpstack);
		for (j=0;j<funcval->paramcount;j++) {
			item = stack_pop(cmpstack);
			if (item.type == list) {
				lst = item.u.listval;
				if (lst->item[i].imag == 0.0)
					err = stack_add_val(varstack,
						      &lst->item[i].real,
						      real);
				else
					err = stack_add_val(varstack,&lst->item[i],
						      complex);
				rpn_delete(item);
				if (err)
					break;
			} else {
				stack_push(varstack,item);
			}
		}
		if (!err)
			err = defined_funcs[fce].function(funcval,varstack);
		if (!err)
			err = stack_get_val(varstack,&lstres->item[i],complex);
		if (err) 
			break;
	}

	if (!err)
		err = stack_add_val(varstack,&lstres,list);
	list_delete(lstres);
	stack_delete(tmpstack);
	stack_delete(cmpstack);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     stack_matrixsimul
 * 
 * DESCRIPTION:  Run a function for every item of a matrix when a matrix
 *               was given as a parameter. The matrices must be of 
 *               same dimension. 
 *
 * PARAMETERS:   fce - offset of function to defined_funcs[]
 *               Functype - Functype argument to fuction
 *               varstack - variable stack for function
 *
 * RETURN:       err
 *      
 ***********************************************************************/
static CError 
stack_matrixsimul(Int16 fce,Functype *funcval,CodeStack *varstack) MLIB;
static CError 
stack_matrixsimul(Int16 fce,Functype *funcval,CodeStack *varstack)
{
	CodeStack *tmpstack,*cmpstack;
	Trpn item;
	CError err=c_noerror;
	Int16 i,j,k;
	Int16 cols=0,rows=0;
	rpntype type;
	CMatrix *cmres;
	Matrix *mres;
	Matrix *m;
	CMatrix *cm;

	/* Find the dimension of the matrix */
	tmpstack = stack_new(funcval->paramcount);
	for (i=0;i < funcval->paramcount;i++) {
		item = stack_pop(varstack);
		if (item.type == variable || item.type == litem) {
			err = rpn_eval_variable(&item,item);
			if (err) {
				rpn_delete(item);
				stack_delete(tmpstack);
				return err;
			}
		}
		type = item.type;
		if (type == matrix || type == cmatrix) {
			if (cols == 0) {
				cols = item.u.matrixval->cols;
				rows = item.u.matrixval->rows;
			} else if (cols != item.u.matrixval->cols ||
				rows != item.u.matrixval->rows) {
				rpn_delete(item);
				stack_delete(tmpstack);
				return c_baddim;
			}
		}
		stack_push(tmpstack,item);
	}

	/* Run the function for every item of matrix */
	cmpstack = stack_new(funcval->paramcount);
	cmres = cmatrix_new(rows,cols);
	for (i=0;i<rows;i++) {
		for (j=0;j<cols;j++) {
			stack_copy(cmpstack,tmpstack);
			for (k=0;k<funcval->paramcount;k++) {
				item = stack_pop(cmpstack);
				if (item.type == matrix) {
					m = item.u.matrixval;
					err = stack_add_val(varstack,
						      &MATRIX(m,i,j),
						      real);
					rpn_delete(item);
				} else if (item.type == cmatrix) {
					cm = item.u.cmatrixval;
					if (MATRIX(cm,i,j).imag == 0.0)
						err = stack_add_val(varstack,
							      &MATRIX(cm,i,j).real,
							      real);
					else
						err = stack_add_val(varstack,
							      &MATRIX(cm,i,j),
							      complex);
					rpn_delete(item);
				} else
					stack_push(varstack,item);
				if (err)
					break;
			}
			if (!err)
				err = defined_funcs[fce].function(funcval,varstack);
			if (!err)
				err = stack_get_val(varstack,&MATRIX(cmres,i,j),
						    complex);
			if (err) 
				break;
		}
		if (err)
			break;
	}
	
	if (!err) {
		mres = cmatrix_to_matrix(cmres);
		if (mres) {
			err = stack_add_val(varstack,&mres,matrix);
			matrix_delete(mres);
		} else
			err = stack_add_val(varstack,&cmres,cmatrix);
	}
	cmatrix_delete(cmres);
	stack_delete(tmpstack);
	stack_delete(cmpstack);

	return err;
}
#endif

void
stack_fix_variables(CodeStack *stack)
{
	Int16 i;

	/* First check that there is no '=' in the stack */
	for (i=0;i<stack->size;i++)
		if (stack->stack[i].type == function &&
		    (stack->stack[i].u.funcval.offs == FUNC_EQUAL || \
		     stack->stack[i].u.funcval.offs == FUNC_DISCARD))
			return;

	for (i=0;i<stack->size;i++) {
		if (stack->stack[i].type == variable) {
			rpn_eval_variable(&stack->stack[i],
					  stack->stack[i]);
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:     stack_compute
 * 
 * DESCRIPTION:  Execute the stack
 *
 * PARAMETERS:   instack - the stack that will be executed
 *
 * RETURN:       error
 *      
 ***********************************************************************/
CError
stack_compute(CodeStack *instack)
{
	CodeStack *varstack;
	Trpn result;
	CError /*Int16*/ retval=c_noerror;
	Trpn item;
#ifdef SPECFUN_ENABLED
	Int16 i;
	Boolean listsimul;
	Int16 matrixcount = 0;
	rpntype type;
	CError err;
#endif

	/* allocate rpnstack to maximum of input elements */
	varstack = stack_new(instack->size);
	while (instack->size) {
		item = stack_pop(instack);
		/* Handle non-operator types */
		if (item.type==integer || item.type==real ||
		    item.type==variable || item.type==funcempty ||
		    item.type==string || item.type == complex ||
		    item.type == list || item.type == matrix ||
		    item.type == litem || item.type == cmatrix) {
			stack_push(varstack,item);
		} else if (item.type==function) {
			Int16 fce=item.u.funcval.offs;
			
			/* Discover if we should run the function for
			 * every item of the list if a list was given
			 * as a parameter */
#ifdef SPECFUN_ENABLED
			listsimul = false;
			if (defined_funcs[fce].listsimul ||
			    defined_funcs[fce].matrixsimul) {
				for (i=0;i < item.u.funcval.paramcount;i++) {
					err=stack_item_type(varstack,&type,i);
					if (!err && type==list) {
						listsimul = true;
						break;
					}
					if (!err && (type == matrix || type == cmatrix))
						matrixcount++;
				}
			}
			if (defined_funcs[fce].listsimul && listsimul) 
				retval = stack_listsimul(fce,&item.u.funcval,
							 varstack);
			/* Run func(M) -> on every item only for 
			 * commands, that contain only 1 matrix in 
			 * parameter list. Functions of more then
			 * 1 matrix parameter generally require special
			 * algorithms.
			 */
			else if (defined_funcs[fce].matrixsimul &&
				 matrixcount == 1) 
				retval = stack_matrixsimul(fce,&item.u.funcval,
							   varstack);
			else
#endif
				retval=defined_funcs[fce].
					function(&item.u.funcval,varstack);
			if (retval)
				break;
		} else if (item.type==functext) {
			if ((retval=text_function(item.u.textfunc.name,
						  item.u.textfunc.paramcount,
						  varstack)))
			  break;
		} else {
			retval=c_internal;
			break;
		}
	}       
	if (varstack->size<1) {
		stack_delete(varstack);
		return retval?retval:c_noresult;
	}
	result = stack_pop(varstack);		
	stack_delete(varstack);	
	stack_push(instack,result);

	return retval;
}

/***********************************************************************
 *
 * FUNCTION:     stack_pop
 * 
 * DESCRIPTION:  Pop an item from stack
 *
 * PARAMETERS:   stack
 *
 * RETURN:       item
 *      
 ***********************************************************************/
Trpn
stack_pop(CodeStack *stack)
{
	ErrFatalDisplayIf(stack->size==0, _T("Null stack size"));
	stack->size--;
	
	return stack->stack[stack->size];
}
  
/***********************************************************************
 *
 * FUNCTION:     stack reverse
 * 
 * DESCRIPTION:  Reverse the order of items on stack (used on the
 *               end of compilation)
 *
 * PARAMETERS:   stack
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
stack_reverse(CodeStack *stack)
{
	Int16 i;
	Trpn tmp;

	for (i=0;i<stack->size/2;i++) {
		tmp = stack->stack[i];
		stack->stack[i] = stack->stack[stack->size-i-1];
		stack->stack[stack->size-i-1] = tmp;
	}
}


/***********************************************************************
 *
 * FUNCTION:     stack_push
 * 
 * DESCRIPTION:  Push a value on stack
 *             ! Doesn't create a new copy, you mustn't free memory
 *               of item that for pushed on stack
 *
 * PARAMETERS:   stack - stack where to push it 
 *               rpn - item
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
stack_push(CodeStack *stack,Trpn rpn)
{
	ErrFatalDisplayIf(stack->size==stack->allocated,
			  _T("RPN Stack overflow"));
	stack->stack[stack->size]=rpn;
	stack->size++;
}

/***********************************************************************
 *
 * FUNCTION:     stack_new
 * 
 * DESCRIPTION:  Creates new stack
 *
 * PARAMETERS:   count - maximum number of items the fit on stack
 *
 * RETURN:       new stack
 *      
 ***********************************************************************/
CodeStack *
stack_new(Int16 count)
{
	CodeStack *result;

	result = (CodeStack *) MemPtrNew(sizeof(*result));
	if (!result)
		return NULL;
	result->stack = (Trpn *) MemPtrNew(sizeof(*result->stack)*count);
	if (!result->stack) {
		MemPtrFree(result);
		return NULL;
	}
	result->size = 0;
	result->allocated = count;

	return result;
}

/***********************************************************************
 *
 * FUNCTION:     stack_delete
 * 
 * DESCRIPTION:  Deletes existing stack and frees all associated memory
 *
 * PARAMETERS:   stack
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
stack_delete(CodeStack *stack)
{
	while (stack->size) {
		stack->size--;
		rpn_delete(stack->stack[stack->size]);
	}
	
	MemPtrFree(stack->stack);
	MemPtrFree(stack);
}
