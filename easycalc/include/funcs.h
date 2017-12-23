/*   
 *   $Id: funcs.h,v 1.20 2006/09/12 19:40:55 tvoverbe Exp $
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

#ifndef _FUNCS_H_
#define _FUNCS_H_

#define FUNC_IF          0

/* Constants as indices to funcInterDef table */
#define FUNC_COV         0
#define FUNC_CORR        1
#define FUNC_QBINOM      2
#define FUNC_QBETA       3
#define FUNC_QCHISQ      4
#define FUNC_QF          5
#define FUNC_QPOISSON    6
#define FUNC_QSTUDENTT   7
#define FUNC_QWEIBULL    8
#define FUNC_QNORMAL     9

/* Maximal depth in user functions */
#define MAX_RECURS 10
/* When the 'real' argument is zero */
#define IS_ZERO(x)      (fabs(x)<1E-6)

#define FUNC_EMPTY     0
#define FUNC_PLUS      1
#define FUNC_MINUS     2
#define FUNC_MULTIPLY  3
#define FUNC_DIVIDE    4
#define FUNC_POWER     5
#define FUNC_EQUAL     6
#define FUNC_NEGATE    7
#define FUNC_DISCARD   8
#define FUNC_AND       9
#define FUNC_OR        10
#define FUNC_SHL       11
#define FUNC_SHR       12
#define FUNC_X         13
#define FUNC_MOD       14
#define FUNC_XOR       15
#define FUNC_ISEQUAL   16
#define FUNC_GRTHEN    17
#define FUNC_LETHEN    18
#define FUNC_GREQ      19
#define FUNC_LEEQ      20
#define FUNC_CONJ      21
#define FUNC_ITEM      22

#define FUNC_DEGREE     1
#define FUNC_RADIAN     2
#define FUNC_GONIO      3
#define FUNC_TOCIS      4
#define FUNC_DEGREE2    5

#define RAND_NUM    ((double) SysRandom(0) / (double)sysRandomMax)

CError convert_real_to_string(Functype *func,CodeStack *stack) PARSER;
CError convert_cplx_to_string(Functype *func,CodeStack *stack) PARSER;
CError define_func(Functype *func,CodeStack *stack) PARSER;
CError change_param_name(Functype *func,CodeStack *stack) PARSER;
CError unset_variable(Functype *func,CodeStack *stack) PARSER;
CError discard_arg(Functype *func,CodeStack *stack) PARSER;
CError empty_arg(Functype *func,CodeStack *stack) PARSER;
CError rpn_eval_variable(Trpn *dest,Trpn source) PARSER;
CError set_ans_var(Trpn ans) PARSER;
CError eval_x(Functype *func,CodeStack *stack) PARSER;
CError text_function(char *name,Int16 paramcount,CodeStack *stack) PARSER;
void set_func_argument(Trpn *arg,UInt16 argcount) PARSER;
void decr_func_argument(void) PARSER;
Boolean varfunc_name_ok(char *name,rpntype type) PARSER;
CError func_get_value(CodeStack *origstack,double param, double *result,
		      CodeStack *addparms) PARSER;
CError func_get_value_cplx(CodeStack *origstack,Complex* params, UInt16 paramcount, Complex *result) PARSER;
CError exec_function(CodeStack *funcstack,CodeStack *varstack, Int16 paramcount) PARSER;
CError func_interdef(Functype *func,CodeStack *stack) PARSER;
CError func_if(Functype *func,CodeStack *stack) PARSER;
CError func_item(Functype *func, CodeStack *stack) PARSER;
Boolean is_constant(char *name) PARSER;

extern Boolean DisableInput;

#endif
