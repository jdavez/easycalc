/*
 *   $Id: funcs.c,v 1.53 2007/09/07 18:15:18 tvoverbe Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999 Ondrej Palkovsky
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
 *               Added Euler-Mascheroni constant (0.5772...) and use of 'j'
 *               as the imaginary unit (I've got an electronic engineer's
 *               training...sorry).
 */

#include <PalmOS.h>
#include <StringMgr.h>

#include "defuns.h"
#include "konvert.h"
#include "funcs.h"
#include "calcDB.h"
#include "MathLib.h"
#include "stack.h"
#include "calc.h"
#include "mathem.h"
#include "complex.h"
#include "history.h"
#include "prefs.h"
#include "fp.h"
#include "slist.h"
#include "matrix.h"
#include "cmatrix.h"

/* If we are just drawing the graph, set this variable. */
/* It will allow us to suppress user input in user definable functions */
Boolean DisableInput = false;

struct {
	Trpn *argument;
	Int16 argcount;
}func_arguments[MAX_RECURS*2];
Int16 func_argcount;

const char *funcInterDef[] = {
	/* covariance */
	"sum((x(1)-mean(x(1)))*(x(2)-mean(x(2))))/dim(x(1))",
	/* corr. coefficient */
	"cov(x(1):x(2))/(stddev(x(1):1)*stddev(x(2):1))",
	/* qbinomial */
	"ibeta(x(1):x(2)-x(1)+1:x(3))",
	/* qbeta */
	"1-ibeta(x(2):x(3):x(1))",
	/* qchisq */
	"1-igamma(x(2)/2:x(1)/2)",
	/* qF */
	"ibeta(x(3)/2:x(2)/2:x(3)/(x(3)+x(2)*x(1)))",
	/* qPoisson */
	"igamma(x(1):x(2))",
	/* qStudentt */
	"0.5(1+sign(x(1))*(ibeta(x(2)/2:0.5:x(2)/(x(2)+x(1)^2))-1)",
	/* qWeibull */
	"exp(-(x(1)/x(2))^x(3))",
	/* qNormal */
	"erfc(x/sqrt(2))/2"
};

static CError x_func(Trpn *rpn,Int16 offs) PARSER;
static CError
x_func(Trpn *rpn,Int16 offs)
{
	Int16 roffset;

	if (!func_argcount)
		return c_badarg;

	roffset = func_arguments[func_argcount-1].argcount - offs - 1;
	if (roffset < 0 || roffset >= func_arguments[func_argcount-1].argcount)
		return c_noarg;

	rpn_duplicate(rpn,
		      func_arguments[func_argcount-1].argument[roffset]);
	return c_noerror;
}

CError  eval_x(Functype *func,CodeStack *stack) PARSER;
CError 
eval_x(Functype *func,CodeStack *stack)
{
	CError err;
	Trpn tmp;
	UInt32 t1;
	Int16 offs;
	
	if (func->paramcount == 1) {
		err = stack_get_val(stack,&t1,integer);
		if (err)
			return err;
		offs = t1 - 1;
	} else /* 0 */
		offs = 0;

	err = x_func(&tmp,offs);
	if (err)
		return err;
	stack_push(stack,tmp);
	return c_noerror;
}

static CError dnum_func(Trpn *rpn,double val) PARSER;
static CError
dnum_func(Trpn *rpn,double val)
{
	rpn->type=real;
	rpn->u.realval=val;
	rpn->allocsize = 0;
	return c_noerror;	
}

static CError i_func(Trpn *rpn,double val) PARSER;
static CError
i_func(Trpn *rpn,double val)
{
	rpn->type=complex;
	rpn->u.cplxval = MemPtrNew(sizeof(Complex));
	rpn->allocsize = sizeof(Complex);
	rpn->u.cplxval->real = 0.0;
	rpn->u.cplxval->imag = 1.0;
	return c_noerror;
}

static CError rand_func(Trpn *rpn,double val) PARSER;
static CError
rand_func(Trpn *rpn,double val)
{
	rpn->type = real;
	rpn->u.realval = RAND_NUM;
	rpn->allocsize = 0;
	return c_noerror;
}

static CError time_func(Trpn *rpn, double val) PARSER;
static CError 
time_func(Trpn *rpn, double val)
{
	rpn->type = integer;
	rpn->u.intval = TimGetSeconds();
	rpn->allocsize = 0;
	return c_noerror;
}

static CError rnorm_func(Trpn *rpn,double val) PARSER;
static CError
rnorm_func(Trpn *rpn,double val)
{
	rpn->type = real;
	rpn->u.realval = 2*sqrt(-0.5*log(RAND_NUM))*cos(RAND_NUM*2*M_PIl);
	rpn->allocsize = 0;
	return c_noerror;
}

const struct {
	char *name;
	CError (*func)(Trpn *rpn,double val);
	double param;
}ConstTable[]={
	  {"pi",dnum_func,3.1415926535897932384},
	  {"e",dnum_func,2.7182818284590452354},	  
	  {"euler", dnum_func, 0.5772156649015328606},
	  {"i",i_func},
	  {"j",i_func},
	  {"T",dnum_func,1E12},
	  {"G",dnum_func,1E9},
	  {"M",dnum_func,1E6},
	  {"k",dnum_func,1E3},
	  {"m",dnum_func,1E-3},
	  {"u",dnum_func,1E-6},
	  {"n",dnum_func,1E-9},
	  {"p",dnum_func,1E-12},
	  {"f",dnum_func,1E-15},
	  {"rand",rand_func},
	  {"rNorm",rnorm_func},
	  {"now",time_func},
	  {NULL,NULL}
};

/* Take variable *source and put it's content to *dest */
/* Source & dest CAN be the same */
CError 
rpn_eval_variable(Trpn *dest,Trpn source)
{
	Int16 i;
	Trpn tmp;
	CError err;
	
	if (source.type == variable) {
            /* Compare it with constants */
		for (i=0;ConstTable[i].name;i++) 
			if (StrCompare(ConstTable[i].name,source.u.varname)==0)
				return ConstTable[i].func(dest,ConstTable[i].param);
		
		tmp = db_read_variable(source.u.varname,&err);
		if (err)
			return err;
	
		*dest=tmp;
		return c_noerror;
	} else if (source.type == litem) {
		CodeStack *tstack;
		Functype *func=MemPtrNew(sizeof(*func));
		UInt32 row=source.u.litemval.row;
		UInt32 col=source.u.litemval.col;
		
		tmp = db_read_variable(source.u.litemval.name,&err);
		if (err)
			return err;
		
		func->paramcount=3;
		tstack = stack_new(3);
		
		if(tmp.type==list)
			err = stack_add_val(tstack,&tmp.u.listval,list);
		else if(tmp.type==matrix)
			err = stack_add_val(tstack,&tmp.u.matrixval,matrix);
		else if(tmp.type==cmatrix)
			err = stack_add_val(tstack,&tmp.u.cmatrixval,cmatrix);
		else
			err = c_badarg;
		
		if(!err)
			err = stack_add_val(tstack,&row,integer);
			err = stack_add_val(tstack,&col,integer);
#ifdef SPECFUN_ENABLED
		if(!err){	
			if(tmp.type==list)
				err = list_item(func,tstack);
			else if(tmp.type==matrix)
				err = matrix_item(func,tstack);
			else
				err = cmatrix_item(func,tstack);
		}
#endif
		if(!err)
			*dest = stack_pop(tstack);

		MemPtrFree(func);
		stack_delete(tstack);
		rpn_delete(tmp);
		return err;
	}
	return c_internal;
}

CError
set_ans_var(Trpn ans)
{
	CError err;
	Boolean freeitem = false;

	if (ans.type==variable || ans.type==litem) {
		err=rpn_eval_variable(&ans,ans);
		if (err)
			return err;
		freeitem = true;
	}
	err = db_write_variable("ans",ans);
	if (!err)
		history_add_item(ans);
	if (freeitem)
		rpn_delete(ans);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     change_param_name
 * 
 * DESCRIPTION:  Allows the user to change the name of the 'x' parameter
 *               to something else. I'm not sure if this function
 *               should be here..
 * 
 * NOTE:         Doesn't do any error checking, probably shouldn't be used
 *
 * PARAMETERS:   On stack - string, new name
 *
 * RETURN:       On stack - string, new name
 *      
 ***********************************************************************/
CError 
change_param_name(Functype *func,CodeStack *stack)
{
	CError err;
	char *name;
	char *oldname;
	Trpn tmp;
	
	err = stack_get_val(stack,&name,string);
	if (err)
	  return err;
	
	if (StrLen(name)>MAX_FUNCNAME)
	  return c_badarg;
	oldname = MemPtrNew(StrLen(parameter_name)+1);
	StrCopy(oldname,parameter_name);
	StrCopy(parameter_name,name);
	/* rpn_get_val returns dynamically created variable */
	MemPtrFree(name);
		
    tmp.type=string;
	tmp.u.stringval=oldname;
	tmp.allocsize=StrLen(oldname)+1;
	stack_push(stack,tmp);
	
	return c_noerror;
}


/***********************************************************************
 *
 * FUNCTION:     unset_variable
 * 
 * DESCRIPTION:  Remove variables/functions
 *
 * PARAMETERS:   1..n variable/funcempty (on stack)
 *
 * RETURN:       number of vars/funcs removed (on stack)
 *      
 ***********************************************************************/
CError
unset_variable(Functype *func,CodeStack *stack)
{
	Trpn tmp;
	Int32 i,n=0;

	for(i=0;i<func->paramcount;i++){	
		tmp = stack_pop(stack);
		if (tmp.type==variable)
	  	  n+=db_delete_record(tmp.u.varname);
		else if (tmp.type==funcempty)
	  	  n+=db_delete_record(tmp.u.funcname);
		else {
			rpn_delete(tmp);
			return c_badarg;
		}
	}
	rpn_delete(tmp);

	return stack_add_val(stack,&n,integer);
}

Boolean
is_constant(char *name)
{
	Int16 i;

	for (i=0;ConstTable[i].name;i++)
		if (StrCompare(name,ConstTable[i].name)==0) 
			return true;
	return false;
}

/***********************************************************************
 *
 * FUNCTION:     define_func
 * 
 * DESCRIPTION:  Handler for 'a=something' and 'f()="something"'
 *               compiles the value of something and saves the result
 *               in a database
 *
 * PARAMETERS:   On stack - variable/funcempty, value of the var/func
 *
 * RETURN:       funcempty or variable
 *      
 ***********************************************************************/
CError 
define_func(Functype *func,CodeStack *stack)
{
	CError err = c_noerror;
	Trpn item1,item2;

	item1 = stack_pop(stack);
	item2 = stack_pop(stack);

	/* Evaluate variable, if it is var... */
	if (item1.type==variable || item1.type==litem) {
		err=rpn_eval_variable(&item1,item1);
		if (err) {
			rpn_delete(item1);
			rpn_delete(item2);
			return err;
		}
	}

	/* Definition of function */
	if (item2.type == funcempty && item1.type == string) {
		CodeStack *newstack;

		newstack=text_to_stack(item1.u.stringval,&err);
		if (err) {
			rpn_delete(item1);
			rpn_delete(item2);
			return err;
		}
		db_write_function(item2.u.funcname,newstack,item1.u.stringval);

		stack_delete(newstack);

		rpn_delete(item1);
		item1.type=funcempty;
		StrCopy(item1.u.funcname,item2.u.funcname);
		item1.allocsize=0;
	} else if (item2.type == funcempty && item1.type == funcempty) {
                /* Handle f()=g() type of assignment */
		CodeStack *newstack;
		char *origtext = NULL;
		char oldparam[MAX_FUNCNAME+1];
		
		/* Save default parameter name */
		StrCopy(oldparam,parameter_name);
		
		err = db_func_description(item1.u.funcname,&origtext,
					  parameter_name);
		if (!err) 
			newstack = db_read_function(item1.u.funcname,&err);
		
		if (err) {
			if (origtext)
				MemPtrFree(origtext);
			StrCopy(parameter_name,oldparam);
			rpn_delete(item1);
			rpn_delete(item2);
			return err;
		}
		
		db_write_function(item2.u.funcname,newstack,origtext);

		stack_delete(newstack);
		MemPtrFree(origtext);
		StrCopy(parameter_name,oldparam);
		rpn_delete(item1);
		/* Return the resulting funcname */
		item1.type=funcempty;
		StrCopy(item1.u.funcname,item2.u.funcname);
		item1.allocsize=0;
	} else if (item2.type == litem) {
		/* In place modification of list/matrix (a[1]=5) */
		Trpn tmp;
		Int16 row = item2.u.litemval.row;
		Int16 col = item2.u.litemval.col;
		CodeStack *tstack;
		
		tstack = stack_new(1);
		stack_push(tstack,item1);
		
		tmp = db_read_variable(item2.u.litemval.name,&err);
		if (!err) {
			if (tmp.type == list) {
				List *lst = tmp.u.listval;
				if ( row==0 || col == 0 || row > lst->size || col > lst->size)
					err = c_baddim;
				else if(row==col){
					err = stack_get_val(tstack,&lst->item[col-1],complex);
				}
#ifdef SPECFUN_ENABLED
				else{
					List *lst2;
					err = stack_get_val(tstack,&lst2,list);
					if(!err){
						Int16 i,dif=col-row;
						if(lst2->size != abs(dif)+1)
							err = c_baddim;
						else for(i=0;i<lst2->size;i++){
							lst->item[row-1]=lst2->item[i];
							row+=sgn(dif);
						}
						list_delete(lst2);
					}
				}
#endif
				if (!err)
					db_write_variable(item2.u.litemval.name,tmp);
			} else if (tmp.type == matrix) {
				Matrix *m = tmp.u.matrixval;
				if ( row > m->rows || col > m->cols || (col == 0 && row==0))
					err = c_baddim;
				else if(col && row)
					err = stack_get_val(tstack,&MATRIX(m,row-1,col-1),real);
#ifdef SPECFUN_ENABLED
				else{
					List *lst;
					err = stack_get_val(tstack,&lst,list);
					if(!err){
						Int16 i;
						if(row==0){	
							if(lst->size != m->rows)
								err = c_baddim;
							for(i=0;(!err && i<m->rows);i++){
								if(!IS_ZERO(lst->item[i].imag))
									err = c_badarg;
								MATRIX(m,i,col-1)=lst->item[i].real;
							}
						}else{
							if(lst->size != m->cols)
								err = c_baddim;
							for(i=0;(!err && i<m->cols);i++){
								if(!IS_ZERO(lst->item[i].imag))
									err = c_badarg;
								MATRIX(m,row-1,i)=lst->item[i].real;
							}
						}
						list_delete(lst);
					}
				}
#endif
				if (!err)
					db_write_variable(item2.u.litemval.name,tmp);
			} else if (tmp.type == cmatrix) {
				CMatrix *m = tmp.u.cmatrixval;
				if ( row > m->rows || col > m->cols || (col == 0 && row==0))
					err = c_baddim;
				else if(col && row)
					err = stack_get_val(tstack,&MATRIX(m,row-1,col-1),complex);
#ifdef SPECFUN_ENABLED
				else{
					List *lst;
					err = stack_get_val(tstack,&lst,list);
					if(!err){
						Int16 i;
						if(row==0){	
							if(lst->size != m->rows)
								err = c_baddim;
							else for(i=0;i<m->rows;i++)
								MATRIX(m,i,col-1)=lst->item[i];
						}else{
							if(lst->size != m->cols)
								err = c_baddim;
							else for(i=0;i<m->cols;i++)
								MATRIX(m,row-1,i)=lst->item[i];
						}
						list_delete(lst);
					}
				}
#endif
				if (!err)
					db_write_variable(item2.u.litemval.name,tmp);
			} else 
				err = c_badarg;
			if(!err)
				/* Return all the list/matrix, not only the  element(s)  */
				stack_push(stack,tmp);
			else
				rpn_delete(tmp);
		}
		stack_delete(tstack);
		rpn_delete(item2);
		return err;
	} else if (item2.type == variable) {
		/* Definition of a variable */
		/* Check, if there isn't constant with same name */       
		if (is_constant(item2.u.varname)) {
			rpn_delete(item1);
			rpn_delete(item2);
			return c_notvar;
		}
		db_write_variable(item2.u.varname,item1);
	} else 
		err = c_badarg;

	if (!err) {
		stack_push(stack,item1);
		rpn_delete(item2);
		return c_noerror;
	} else {
		rpn_delete(item1);
		rpn_delete(item2);
		return err;
	}
}

/***********************************************************************
 *
 * FUNCTION:     exec_function
 * 
 * DESCRIPTION:  Executes provided function
 *
 * PARAMETERS:   funcstack - function to be executed
 *               varstack - where to get parameters and where to store result
 *               paramcount - nomber of parameters
 *
 * RETURN:       err
 *      
 ***********************************************************************/
CError
exec_function(CodeStack *funcstack,CodeStack *varstack, Int16 paramcount)
{
	Trpn *argarr;
	CError err;
	Int16 i,j;
	
	argarr = MemPtrNew(sizeof(*argarr) * paramcount);
	for (i=0;i<paramcount;i++) {
		argarr[i] = stack_pop(varstack);
		if (argarr[i].type==variable || argarr[i].type==litem) {
			err=rpn_eval_variable(&argarr[i],argarr[i]);
			if (err) {
				for (j=0;j<i;j++)
					rpn_delete(argarr[j]);
				MemPtrFree(argarr);
				return err;
			}
		}
	}
	/* Set 'argarr' as function argument */
	set_func_argument(argarr,paramcount);

	err = stack_compute(funcstack);
	/* Set function argument back to original */
	decr_func_argument();

	if (!err) 
		stack_push(varstack,stack_pop(funcstack));

	for (i=0;i<paramcount;i++)
		rpn_delete(argarr[i]);
	MemPtrFree(argarr);
	
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     text_function
 * 
 * DESCRIPTION:  Calls a user defined function (f())
 *
 * PARAMETERS:   name - name of the text function
 *               On stack - parameter for a function
 *
 * RETURN:       On stack - result
 *      
 ***********************************************************************/
CError
text_function(char *name,Int16 paramcount,CodeStack *stack)
{
	CError err;
	CodeStack *newstack;

/* The recursion should be less then MAX_RECURS */
	if (func_argcount>MAX_RECURS)
		return c_deeprec;

	newstack = db_read_function(name,&err);
	if (err) 
		return err;
	
	err = exec_function(newstack,stack,paramcount);

	stack_delete(newstack);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     func_interdef
 * 
 * DESCRIPTION:  'User' functions defined be me (I was lazy to code them
 *               in C)
 *
 * PARAMETERS:   Some number from stack
 *
 * RETURN:       result on stack
 *      
 ***********************************************************************/
CError
func_interdef(Functype *func,CodeStack *stack)
{
	CError err;
	CodeStack *myfunc;

	myfunc = text_to_stack((char *)funcInterDef[func->num],&err);
	if (err)
		return err;
	err = exec_function(myfunc,stack,func->paramcount);
	stack_delete(myfunc);
	
	return err;
}

CError
func_if(Functype *func,CodeStack *stack)
{
	CError err;
	Trpn arg, argtrue, argfalse;
	UInt32 value;

	argfalse = stack_pop(stack);
	argtrue = stack_pop(stack);
	err = stack_get_val(stack,&value,integer);
	if (err) {
		rpn_delete(argtrue);
		rpn_delete(argfalse);
		return err;
	}
	if (value) {
		rpn_delete(argfalse);
		arg = argtrue;
	} else {
		rpn_delete(argtrue);
		arg = argfalse;
	}
	if (arg.type == string) {
		CodeStack *tmp = text_to_stack(arg.u.stringval,&err);
		rpn_delete(arg);
		if (!err)
			err = stack_compute(tmp);
		if (!err)
			arg=stack_pop(tmp);
		if (tmp)
			stack_delete(tmp);
		if (err)
			return err;
	}
	stack_push(stack,arg);
	return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     empty_arg
 * 
 * DESCRIPTION:  Empty operator, that doesn't modify the stack
 *               Used mainly for the parameter delimiter (':')
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
CError
empty_arg(Functype *func,CodeStack *stack)
{
	return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     discard_arg
 * 
 * DESCRIPTION:  Discards the argument, used for a ';' operator
 *
 * PARAMETERS:   On stack - 1 parameter
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
CError 
discard_arg(Functype *func,CodeStack *stack)
{
	Trpn tmp;
	
	if (stack->size<1)
	  return c_noerror;

	tmp = stack_pop(stack);
	rpn_delete(tmp);

	return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     set_func_argument
 * 
 * DESCRIPTION:  Push a parameter to a parameter stack
 * 
 * NOTE:         The parameter doesn't get copied, it can thus be
 *               later modified and the changes will take effect
 *               on every new stack_compute without the need
 *               of decr_func_argument. The argument gets copied
 *               upon calling the 'x' variable name 
 *
 * PARAMETERS:   arg - argument array to be a new 'x', 'x(1)'...
 *               argcount - size of an argument array
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
set_func_argument(Trpn *arg,UInt16 argcount)
{
	func_arguments[func_argcount].argument=arg;
	func_arguments[func_argcount].argcount = argcount;
	func_argcount++;
}

/***********************************************************************
 *
 * FUNCTION:     decr_func_argument
 * 
 * DESCRIPTION:  Remove a function parameter from a parameter stack
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
decr_func_argument(void)
{
	func_argcount--;
}

CError
convert_cplx_to_string(Functype *func,CodeStack *stack)
{
	Complex arg;
	Complex_gon tmp;
	CError err;
	char *res;
	
	err=stack_get_val(stack,&arg,complex);
	if (err)
		return err;

	tmp = cplx_to_gon(arg);
	if (func->num == FUNC_GONIO) {
		res = MemPtrNew(MAX_FP_NUMBER*2+10);
	
		fp_print_double(res,tmp.r);
		StrCat(res,"e^(");
		fp_print_double(res+StrLen(res),tmp.angle);
		StrCat(res,"i)");
	}
	else if (func->num == FUNC_TOCIS) {
		tmp.angle = math_rad_to_user(tmp.angle);
	    
		res = MemPtrNew(MAX_FP_NUMBER*2+20);
		fp_print_double(res,tmp.r);
		StrCat(res,"(cos(");
		fp_print_double(res+StrLen(res),tmp.angle);
		StrCat(res,")+i*sin(");
		fp_print_double(res+StrLen(res),tmp.angle);
		StrCat(res,"))");
	}
	else
		return c_internal;
	err = stack_add_val(stack,res,string);
	MemPtrFree(res);
	
	return err;
}

CError 
convert_real_to_string(Functype *func,CodeStack *stack)
{
	double arg;
	char *res,*tmp;
	CError err;

	err=stack_get_val(stack,&arg,real);
	if (err)
		return err;
	
	if (!finite(arg)) 
		return c_badarg;
	
	res = MemPtrNew(MAX_FP_NUMBER+20);
	tmp = res;
	if (func->num==FUNC_RADIAN) {
		fp_print_double(res,arg/M_PIl);
		StrCat(res,"pi");
	}	
	else if (func->num==FUNC_DEGREE || func->num == FUNC_DEGREE2) {
		if (arg<0) {
			*tmp++='-';
			arg=-arg;
		}
		/* The numbers must be added 1E-12 to avoid
		   cases like 1+5/60 -> 1o04'
		*/
		fp_print_double(tmp,trunc(arg+1E-12));
		tmp+=StrLen(tmp);
		*tmp++='°';
		if (arg-trunc(arg)<1E-15)
			StrCopy(tmp,"0\'");
		else {
			if (func->num == FUNC_DEGREE) {
				fp_print_double(tmp,trunc(60*(arg-trunc(arg+1E-12))+1E-12));
				tmp+=StrLen(tmp);
				*tmp++='\'';
				fp_print_double(tmp,60*(arg*60-trunc(arg*60+1E-12)));
			} else { /* FUNC_DEGREE2 */
				fp_print_double(tmp,60*(arg-trunc(arg+1E-12))+1E-12);
				StrCat(tmp,"'");
			}
		}
	}

	err = stack_add_val(stack,res,string);
	MemPtrFree(res);
	
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     varfunc_name_ok
 * 
 * DESCRIPTION:  Checks if the name of variable/function is correct
 *               in the system, and if it doesn't clash with some
 *               function name/constant name/parameter name
 *
 * PARAMETERS:   name - name to be checked
 *               type - function or variable
 * 
 * RETURN:       true - name ok
 *               false - name incorrect
 *      
 ***********************************************************************/
Boolean
varfunc_name_ok(char *name,rpntype type)
{
	Int16 i;
	
	if (StrLen(name)==0 || StrLen(name)>MAX_FUNCNAME)
	  return false;
	
	if (!IS_FUNC_1LETTER(name[0]))
	  return false;
	
	for (i=0;name[i];i++)
	  if (!IS_FUNC_LETTER(name[i]))
		return false;
	if (StrCompare(name,parameter_name)==0)
	  return false;
	if (type==variable && is_constant(name))
		return false;
	else if (type==function) {
		for (i=0;defined_funcs[i].name[0];i++)
		  if (StrCompare(name,defined_funcs[i].name)==0)
			return false;
	}
	return true;
}

/***********************************************************************
 *
 * FUNCTION:     func_get_value
 * 
 * DESCRIPTION:  Return a real value of a given function with
 *               a given parameter
 *
 * PARAMETERS:   origstack - the compiled function
 *               param - argument of function
 *               result - result of operation NaN if failed
 *               addparams - pointer to array of additional parameters
 *                    - if it is NULL, nothing is passed as additional
 *                    - if it is defined, it must be array holding
 *                      additional parameters + 1, where the last
 *                      additional parameter left undefined to have 
 *                      place for the special addon
 *               numparams - the number of additional arguments
 *               THE ADDITIONAL PARAMETERS MUST CONTAIN RESOLVED VARIABLES
 *
 * RETURN:       err
 *
 * NOTE:         This function automatically disables input methods of
 *               functions, because it is used by GUI tools that generally
 *               do not expect you to enter otehr values
 *      
 ***********************************************************************/
CError
func_get_value(CodeStack *origstack,double param, double *result,
	       CodeStack *addparams)
{
	Trpn argument;
	CodeStack *stack;
	CError err;
	Boolean oldinput;

	oldinput = DisableInput;
	DisableInput = true;
	
	stack = stack_new(origstack->size);
	stack_copy(stack,origstack);

	argument.type = real;
	argument.allocsize = 0;
	argument.u.realval = param;
	if (addparams == NULL) {
		set_func_argument(&argument,1);
	} else {
		stack_push(addparams,argument);
		set_func_argument(addparams->stack,addparams->size);
	}
	err = stack_compute(stack);
	decr_func_argument();
	
	if (addparams != NULL) {
		/* Return the addparams stack to the original state */
		stack_pop(addparams);
	}

	if (!err) 
		err = stack_get_val(stack,result,real);
	if (err)
		*result = NaN;
	stack_delete(stack);

	DisableInput = oldinput;

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     func_get_value_cplx
 * 
 * DESCRIPTION:  Return a complex value of a given function with
 *               given parameters
 *
 * PARAMETERS:   origstack - the compiled function
 *               params - arguments of function
 *               paramcount - number of arguments
 *               result - result of operation, (NaN + NaN i) if failed
 *
 * RETURN:       err
 *      
 ***********************************************************************/

CError
func_get_value_cplx(CodeStack *origstack,Complex* params, UInt16 paramcount, 
		    Complex *result)
{
	CodeStack *stack;
	CError err = c_noerror;
	CodeStack *paramstack;
	UInt16 i;

	stack = stack_new(origstack->size);
	if (!stack)
		return c_memory;
	stack_copy(stack,origstack);

	paramstack = stack_new(paramcount);
	if (!paramstack) {
		stack_delete(stack);
		return c_memory;
	}
	
	for(i=0; i < paramcount && err == c_noerror; i++) {
		err = stack_add_val(paramstack, &params[i], complex);
	}

	if (!err) {
		set_func_argument(paramstack->stack,paramstack->size);
		err = stack_compute(stack);
		decr_func_argument();
	}

	if (!err) 
		err = stack_get_val(stack,result,complex);
	if (err) {
		result->real = NaN;
		result->imag = NaN;
	}
	stack_delete(stack);
	stack_delete(paramstack);

	return err;
}

#ifdef SPECFUN_ENABLED
CError
func_item(Functype *func, CodeStack *stack)
{
	rpntype type1,type2;
	CError err;

	err = stack_item_type(stack,&type2,(func->paramcount-1));
	if (err)
		return err;
	
	if (func->paramcount==2 && type2==list)
		err = stack_item_type_nr(stack,&type1,1);
	else if (func->paramcount==3)
		err = stack_item_type_nr(stack,&type1,2);
	else
		err = c_badargcount;
	if (err)
		return err;
	
	if (type1 == variable) {
		UInt32 arg1=0,arg2=0;
		Trpn item,ritem;
		
		err = stack_get_val(stack,&arg2,integer);
		if (err)
			return err;
		if (func->paramcount == 3) {
			err = stack_get_val(stack,&arg1,integer);
			if (err)
				return err;
		}else
			arg1=arg2;
		
		item = stack_pop(stack);
		ritem.allocsize = 0;
		ritem.type = litem;
		StrCopy(ritem.u.litemval.name,item.u.varname);
		ritem.u.litemval.row = arg1;
		ritem.u.litemval.col = arg2;
		stack_push(stack,ritem);
		rpn_delete(item);
	} else switch (type2){
		case list:
			return list_item(func,stack);
		case matrix:
			return matrix_item(func,stack);
		case cmatrix:
			return cmatrix_item(func,stack);
		default:
			return c_badarg;
	}
	return c_noerror;
}

#else /* Not specfun enabled */
CError
func_item(Functype *func, CodeStack *stack)
{
	return c_badarg;
}
#endif

/* Linker hates when it finds an empty segment */
void
dummy_dummy(void) SPECFUN;
void
dummy_dummy(void)
{
}

void
dummy_dummy2(void) GRAPH;
void
dummy_dummy2(void)
{
}

void
dummy_dummy3(void) NEWFUNC;
void
dummy_dummy3(void)
{
}

void
dummy_dummy4(void) MATFUNC;
void
dummy_dummy4(void)
{
}
