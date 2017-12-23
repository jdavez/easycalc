/*
 *   $Id: konvert.cpp,v 1.8 2011/02/28 22:07:47 mapibid Exp $
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
#include "StdAfx.h"

#include <stdio.h>
#include "compat/PalmOS.h"
//#include <StringMgr.h>

#include "defuns.h"
#include "compat/MathLib.h"
#include "konvert.h"
#include "mathem.h"
#include "complex.h"
#include "funcs.h"
#include "stack.h"
#include "core/prefs.h"
#include "history.h"
#include "integ.h"
#include "slist.h"
#include "matrix.h"
#include "meqstack.h"
#include "txtask.h"
#include "cmatrix.h"
#include "core/chkstack.h"
#include "fl_num.h"

#ifdef GRAPHS_ENABLED

#include "core/grprefs.h"

#endif

#ifdef SPECFUN_ENABLED

#include "specfun.h"

#endif

void DoubleToAscii(double,TCHAR *);

/* Global parameters */
TCHAR parameter_name[MAX_FUNCNAME+1]=_T("x");


/* Structure, that is used both to call functions from compiled 
 * code (defined_funcs[funcval]) and to decode an expression.
 * The first few elements are directly dependent on the constants used
 * in 'def_ops', as the constant points directly to the position
 * in this table. This dependence is here because of efficiency, it might
 * be removed in the future, because compilation isn't so time-critical
 * task.
 * 
 * Items -
 *   name - name of the function, for operators this is only informational,
 *          it should follow syntax contraints on function/var name
 *   funcnum - a 'helping' constant, allows us to use one computing
 *             function for multiple operators/user functions
 *   function - the function to be called
 *   param_count - number of parameters this function requires,
 *                 for operators only informational
 *                 0 means variable number of arguments
 *   listsimul - if true, then if list(s) is passed as a parameter,
 *               its items are taken one by one and passed to the
 *               function and the result is again saved as a list
 *   matrixsimul - if true, then if matrix is passed as a parameter,
 *                 operation is executed on every item of the matrix.
 *                 if more then 1 matrix is passed as a parameter, 
 *                 the normal function is called with the matrices
 *                 as parameter
 */  

const struct Tdefined_funcs defined_funcs[]={
      {_T(":"),FUNC_EMPTY,empty_arg,1},
      {_T("+"),FUNC_PLUS,math_oper,2,true,true},
      {_T("-"),FUNC_MINUS,math_oper,2,true,true},
      {_T("*"),FUNC_MULTIPLY,math_oper,2,true,true},
      {_T("/"),FUNC_DIVIDE,math_oper,2,true,true},
      {_T("^"),FUNC_POWER,math_oper,2,true},
      {_T("="),FUNC_EQUAL,define_func,2},
      {_T("neg"),FUNC_NEGATE,math_neg,1,true,true},
      {_T(";"),FUNC_DISCARD,discard_arg,1},
      {_T("&"),FUNC_AND,math_bin,2},
      {_T("|"),FUNC_OR,math_bin,2},
      {_T("<"),FUNC_SHL,math_bin,2},
      {_T(">"),FUNC_SHR,math_bin,2},
      {_T("!"),FUNC_X,eval_x,1},
      {_T("%"),FUNC_MOD,math_oper,2,true},
      {_T(" xor "),FUNC_XOR,math_bin,2},
      {_T("=="),FUNC_ISEQUAL,math_cmpeq,2},
      {_T(">"),FUNC_GRTHEN,math_cmp,2},
      {_T("<"),FUNC_LETHEN,math_cmp,2},
      {_T(">="),FUNC_GREQ,math_cmp,2},
      {_T("<="),FUNC_LEEQ,math_cmp,2},
      {_T("\'"),FUNC_CONJ,math_apostrophe,1,true},
      {_T("["),FUNC_ITEM,func_item,0},
/* The 'name' isn't used for previous operators, they are assigned other way */
/* Previous functions are in defined order, be aware of FUNC_ constants */
      {_T("if"),FUNC_IF,func_if,3},
      {_T("sin"),MATH_SIN,math_trigo,1,true,true},
      {_T("cos"),MATH_COS,math_trigo,1,true,true},
      {_T("tan"),MATH_TAN,math_trigo,1,true,true},
      {_T("sinh"),MATH_SINH,math_trigo,1,true,true},
      {_T("cosh"),MATH_COSH,math_trigo,1,true,true},
      {_T("tanh"),MATH_TANH,math_trigo,1,true,true},
      {_T("asin"),MATH_ASIN,math_arctrigo,1,true,true},
      {_T("acos"),MATH_ACOS,math_arctrigo,1,true,true},
      {_T("atan"),MATH_ATAN,math_arctrigo,1,true,true},
      {_T("asinh"),MATH_ASINH,math_arctrigo,1,true,true},
      {_T("acosh"),MATH_ACOSH,math_arctrigo,1,true,true},
      {_T("atanh"),MATH_ATANH,math_arctrigo,1,true,true},
      {_T("exp"),MATH_EXP,math_math,1,true,true},
      {_T("ln"),MATH_LN,math_math,1,true,true},
      {_T("log"),MATH_LOG,math_math,1,true,true},
      {_T("log2"),MATH_LOG2,math_math,1,true,true},
      {_T("sqrt"),MATH_SQRT,math_math,1,true,true},
      {_T("cbrt"),MATH_CBRT,math_math,1,true,true},
      {_T("floor"),MATH_FLOOR,math_math,1,true,true},
      {_T("ceil"),MATH_CEIL,math_math,1,true,true},
      {_T("abs"),MATH_ABS,math_math,1,true,true},
      {_T("fact"),MATH_FACT,math_fact,1,true},
      {_T("gamma"),MATH_GAMMA,math_gamma,1,true,true},
      {_T("todeg"),FUNC_DEGREE,convert_real_to_string,1},      
      {_T("todeg2"),FUNC_DEGREE2,convert_real_to_string,1},
      {_T("int"),0,math_int,1},
      {_T("real"),MATH_REAL,cplx_math,1,true,true},
      {_T("imag"),MATH_IMAG,cplx_math,1,true,true},
      {_T("angle"),MATH_ANGLE,cplx_math,1,true,true},
      {_T("round"),MATH_ROUND,math_round,0,true,true},
      {_T("trunc"),MATH_TRUNC,math_round,0,true,true},
      {_T("npr"),MATH_PERM,math_comb,2},
      {_T("ncr"),MATH_COMB,math_comb,2},
      {_T("defparamn"),0,change_param_name,1},
      {_T("unset"),0,unset_variable,0},
      {_T("hypot"),MATH_HYPOT,math_convcoord,2},
      {_T("rtopr"),MATH_HYPOT,math_convcoord,2},
      {_T("rtopd"),MATH_RTOPD,math_convcoord,2},
      {_T("ptorx"),MATH_PTORX,math_convcoord,2},
      {_T("ptory"),MATH_PTORY,math_convcoord,2},
      {_T("atan2"),MATH_ATAN2,math_convcoord,2},
      {_T("torad"),FUNC_RADIAN,convert_real_to_string,1},
      {_T("tograd"),FUNC_GRAD,convert_real_to_string,1},
      {_T("togonio"),FUNC_GONIO,convert_cplx_to_string,1},
      {_T("ipart"),MATH_TRUNC,math_round,1,true,true},
      {_T("fpart"),MATH_FPART,math_math,1,true,true},
      {_T("sign"),MATH_SIGN,math_math,1,true,true},
      {_T("history"),0,history_command,1},
      {_T("tocis"),FUNC_TOCIS,convert_cplx_to_string,1},
      {_T("conj"),FUNC_CONJ,cplx_math,1,true},
      {_T("randseed"),MATH_SEED,math_seed,1},
      {_T("fin_pv"),MATH_FINPV,math_financial,6},
      {_T("fin_n"),MATH_FINN,math_financial,6},
      {_T("fin_fv"),MATH_FINFV,math_financial,6},
      {_T("fin_pmt"),MATH_FINPMT,math_financial,6},
      {_T("ask"),TXTASK_ASK,txtask_ask,0},
      {_T("gcd"), MATH_GCD, math_gcd, 0, true, true},
      {_T("lcm"), MATH_LCM, math_gcd, 0, true, true},
      {_T("modinv"), MATH_MODINV, math_modinv, 2, true, true},
      {_T("modpow"), MATH_MODPOW, math_modpow, 3, true, true},
      {_T("chinese"), MATH_CHINESE, math_chinese, 0, true, true},
      {_T("phi"), MATH_PHI, math_phi, 1, true, true},
      {_T("isprime"), MATH_ISPRIME, math_primes, 1, true, true},
      {_T("nextprime"), MATH_NEXTPRIME, math_primes, 1, true, true},
      {_T("prevprime"), MATH_PREVPRIME, math_primes, 1, true, true},
#ifdef GRAPHS_ENABLED
// TODO      {_T("axis"),0,set_axis,0},
#endif
#ifdef SPECFUN_ENABLED
      {_T("fzero"),MATH_FZERO,integ_fsolve,0},
      {_T("fsimps"),MATH_FSIMPS,integ_fsimps,0},
      {_T("fromberg"),MATH_FROMBERG,integ_fromberg,0},
      {_T("fd_dx"),MATH_FDYDX1,integ_fdydx,0},
      {_T("fd2_dx"),MATH_FDYDX2,integ_fdydx,0},
      {_T("fmin"),MATH_FMIN,integ_fsolve,0},
      {_T("fmax"),MATH_FMAX,integ_fsolve,0},
      {_T("fvalue"),MATH_FVALUE,integ_fsolve,0},
      {_T("fintersect"),MATH_FINTERSECT,integ_fintersect,0},
      {_T("list"),LIST_INPUT,list_input,0},
      {_T("mean"),LIST_MEAN,list_stat,1},
      {_T("lmin"),LIST_MIN,list_func,1},
      {_T("lmax"),LIST_MAX,list_func,1},
      {_T("sum"),LIST_SUM,list_func,1},
      {_T("median"),LIST_MEDIAN,list_func,1},
      {_T("sorta"),LIST_SORTA,list_func2,1},
      {_T("sortd"),LIST_SORTD,list_func2,1},
      {_T("gmean"),LIST_GMEAN,list_stat,1},
      {_T("prod"),LIST_PROD,list_func,1},
      {_T("variance"),LIST_VARIANCE,list_stat,0},
      {_T("stddev"),LIST_STDDEV,list_stat,0},
      {_T("linreg"),LIST_LINREG,list_regr,2},
      {_T("concat"),0,list_concat,0},
      {_T("conv"),LIST_CONV,list_func3,2},
      {_T("kron"),LIST_KRON,list_func3,2},
      {_T("sample"),LIST_SAMPLE,list_func3,2},
      {_T("filter"),0,list_filter,3},
      {_T("fft"),LIST_FFT,list_fourier,0},
      {_T("ifft"),LIST_IFFT,list_fourier,0},
      {_T("fftshift"),LIST_DFTSHIFT,list_fourier,1},
      {_T("dft"),LIST_DFT,list_fourier,0},
      {_T("idft"),LIST_IDFT,list_fourier,0},
      {_T("factor"),0,list_factor,1},
      {_T("gcdex"),0,list_gcdex,2},
      {_T("sift"),0,list_sift,2},
      {_T("find"),LIST_FIND,list_sift,2},
      {_T("map"),0,list_map,0},
      {_T("zip"),0,list_zip,0},
      {_T("repeat"),0,list_repeat,3},
      {_T("dim"),LIST_DIM,matrix_dim,1},
      {_T("skewness"),LIST_SKEWNESS,list_stat,1},
      {_T("kurtosis"),LIST_KURTOSIS,list_stat,1},
      {_T("varcoef"),LIST_VARCOEF,list_stat,1},
      {_T("cumsum"),LIST_CUMSUM,list_func2,1},
      {_T("lmode"),LIST_MODE,list_stat,1},
      {_T("moment"),LIST_MOMENT,list_stat,2},
      {_T("cov"),FUNC_COV,func_interdef,2},
      {_T("corr"),FUNC_CORR,func_interdef,2},
      {_T("matrix"),MATRIX_INPUT,matrix_input,0},
      {_T("det"),MATRIX_DET,matrix_func,1},
      {_T("rand"),LIST_RAND,list_rand,1},
      {_T("range"),LIST_RANGE,list_range,0},
      {_T("rNorm"),LIST_RNORM,list_rand,1},
      {_T("qBinomial"),FUNC_QBINOM,func_interdef,3},
      {_T("qBeta"),FUNC_QBETA,func_interdef,3},
      {_T("qChiSq"),FUNC_QCHISQ,func_interdef,2},
      {_T("qF"),FUNC_QF,func_interdef,3},
      {_T("qPoisson"),FUNC_QPOISSON,func_interdef,2},
      {_T("qStudentt"),FUNC_QSTUDENTT,func_interdef,2},
      {_T("qWeibull"),FUNC_QWEIBULL,func_interdef,3},
      {_T("qNormal"),FUNC_QNORMAL,func_interdef,1},
      {_T("rref"),MATRIX_ECHELON,matrix_func2,1},
      {_T("identity"),MATRIX_IDENTITY,matrix_func3,1},
      {_T("cmatrix"),CMATRIX_INPUT,cmatrix_input,0},
      {_T("qrq"),MATRIX_QRQ,matrix_func2,1},
      {_T("qrr"),MATRIX_QRR,matrix_func2,1},
      {_T("qrs"),MATRIX_QRS,matrix_func2,1},
      {_T("rtop"),LIST_RTOP,list_mathem,2},
      {_T("ptor"),LIST_PTOR,list_mathem,2},
#endif

#ifdef SPECFUN_ENABLED

      {_T("beta"), MATH_BETA, math_gambeta, 2},
      {_T("igamma"), MATH_IGAMMA, math_gambeta, 2},
      {_T("ibeta"), MATH_IBETA, math_gambeta, 3 },
      {_T("erf"), MATH_ERF, math_gambeta, 1 },
      {_T("erfc"), MATH_ERFC, math_gambeta, 1},

      {_T("besselj"), MATH_BESSELJ, math_bessels, 2},
      {_T("bessely"), MATH_BESSELY, math_bessels, 2},
      {_T("besseli"), MATH_BESSELI, math_bessels, 2},
      {_T("besselk"), MATH_BESSELK, math_bessels, 2},

      {_T("ellc1"), MATH_ELLC1, math_elliptic, 1},
      {_T("ellc2"), MATH_ELLC2, math_elliptic, 1},
      {_T("elli1"), MATH_ELLI1, math_elliptic, 2},
      {_T("elli2"), MATH_ELLI2, math_elliptic, 2},
      {_T("sn"), MATH_SN, math_elliptic, 2},
      {_T("cn"), MATH_CN, math_elliptic, 2},
      {_T("dn"), MATH_DN, math_elliptic, 2},

#if 0
      {_T("airy"), MATH_AIRY, math_bessels, 1},
      {_T("biry"), MATH_BIRY, math_bessels, 1},

      {_T("hypergeomF"), MATH_HYPGG, math_hypergeo, 4},
      {_T("hypergeomM"), MATH_HYPGC, math_hypergeo, 3},
      {_T("legendre"), MATH_ALGNDRF, math_orthpol, 2},
      {_T("sphharm"), MATH_SPHHARM, math_orthpol, 4},
#endif

#endif
      {_T(""),0,NULL}
};

/* Fast lookup table for operators, will be probably changed in the
 * future.
 * 
 * name - sequence of characters that should be recognized as an operator
 * NOTE: longer operaters should come first!
 * operpri - priority of the oprator
 * func - position in defined_funcs array of the corresponding function
 * evalRight - operators like 2^3^4 and 2x^3pi -> 2(x^(3pi))
 */
struct {
    TCHAR *name;
    Int16 operpri;
    Int16 funcoffs;
    Boolean evalRight;
}def_ops[]={
    {_T("<<"),3,FUNC_SHL},
    {_T(">>"),3,FUNC_SHR},
    {_T("=="),2,FUNC_ISEQUAL},
    {_T(">="),2,FUNC_GREQ},
    {_T("<="),2,FUNC_LEEQ},
    {_T("<"),2,FUNC_LETHEN},
    {_T(">"),2,FUNC_GRTHEN},
    {_T("+"),3,FUNC_PLUS},
    {_T("-"),3,FUNC_MINUS},
    {_T("*"),4,FUNC_MULTIPLY},
    {_T("\1"),5,FUNC_MULTIPLY,true}, /* Multiply with special high priority,
                      * used for e.g. 1/3pi, that should be
                      * 1/(3pi) and not (1/3)*pi */
    {_T("/"),4,FUNC_DIVIDE},
    {_T("^"),5,FUNC_POWER,true},
    {_T("="),0,FUNC_EQUAL},
    {_T("&"),3,FUNC_AND},
    {_T("|"),3,FUNC_OR},
    {_T("%"),4,FUNC_MOD},
    {_T(" xor "),3,FUNC_XOR},
    {_T("ˆ"),3,FUNC_XOR},
    {_T("\2"),6,FUNC_CONJ}, /* The apostrophe, it shouldn't be recognized
                 * as a normal operator */
    {_T("\3"),7,FUNC_ITEM}, /* The [ operator (it has parameters) */
    {NULL,0,0}
};

static Tdynmeq * meqstack;

static Int16 konvert_to_meq(TCHAR *buf,Int16 priority,Int16 *param_count,
                TCHAR cbracket, CError *err) PARSER;

static Int16 try_real_num(TCHAR *buf,Int16 priority,CError *err) PARSER;
static Int16
try_real_num(TCHAR *buf,Int16 priority,CError *err)
{
    double t1;
    Int16 res;
    Tmeq item;

    *err = c_noerror;
    if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }

    if (!(res=fl_num(buf,&t1)))
      return 0;
    item.priority=priority;
    item.rpn.type=real;
    item.rpn.u.realval=t1;
    item.rpn.allocsize=0;
    meq_push(meqstack,item);
    return res;
}

static Int16 try_degree(TCHAR *buf,Int16 priority,CError *err) PARSER;
static Int16
try_degree(TCHAR *buf,Int16 priority,CError *err)
{
    Int16 length=0;
    Int16 res,i;
    double result=0,tmp,t1;
    Int16 negate=0;           
    Tmeq item;

    *err = c_noerror;
    if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }

    if (calcPrefs.trigo_mode!=degree || dispPrefs.base!=disp_decimal)
        return 0;
    
    item.rpn.type=real;    
    item.priority=priority;
    if (*buf==_T('-')) {
        buf++;length++;
        negate=1;
    }

    for (i=0;buf[i] >= _T('0') && buf[i] <= _T('9');i++)
        ;
    if (buf[i] != _T('°') && buf[i] != _T('\'')) 
        return 0;

    if (buf[i] == _T('°')) {
        while (*buf>=_T('0') && *buf<=_T('9')) {
            result*=10.0;
            result+=(*buf-_T('0'));
            buf++;length++;
        }
        /* *buf = '°' */
        length++;buf++;
    }

    if (*buf >= _T('0') && *buf <= _T('9')) {

        if (!(res=fl_num(buf,&t1)))
            return 0;
        length+=res;buf+=res;
        
        tmp = 0.0;
        if (*buf==_T('\'')) {
            length++;buf++;
    
            if ((res=fl_num(buf,&tmp)))
                length+=res;
            else
                tmp = 0.0;
        }
        
        result+=t1/60.0;
        result+=tmp/3600.0;
    }
    if (negate)
        result=-result;
    
    item.rpn.u.realval=result;
    item.rpn.allocsize=0;
    meq_push(meqstack,item);
    
    return length;
}

static Int16 try_int_num(TCHAR *buf,Int16 priority,CError *err) PARSER;
static Int16
try_int_num(TCHAR *buf,Int16 priority,CError *err)
{
    Int16 length=0;
    Tmeq item;

    *err = c_noerror;
    if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }

    item.rpn.type=integer;
    item.rpn.allocsize = 0;
    item.rpn.u.intval=0;
    switch (dispPrefs.base) {
     case disp_decimal:
        while (*buf>=_T('0') && *buf<=_T('9')) {
            length++;
            item.rpn.u.intval*=10;
            item.rpn.u.intval+=*buf-_T('0');
            buf++;
        }
        break;
     case disp_octal:
        while (*buf>=_T('0') && *buf<=_T('7')) {
            length++;
            item.rpn.u.intval*=8;
            item.rpn.u.intval+=*buf-_T('0');
            buf++;
        }
        break;
     case disp_binary:
        while (*buf>=_T('0') && *buf<=_T('1')) {
            length++;
            item.rpn.u.intval*=2;
            item.rpn.u.intval+=*buf-_T('0');
            buf++;
        }
        break;
     case disp_hexa:
        while ((*buf>=_T('0') && *buf<=_T('9')) || (*buf>=_T('A') && *buf<=_T('F'))) {
            length++;
            item.rpn.u.intval*=16;
            if (*buf>=_T('0') && *buf<=_T('9'))
                item.rpn.u.intval+=*buf-_T('0');
            else
                item.rpn.u.intval+=*buf-_T('A')+10;
            buf++;
        }
        break;
     default:
        return 0;
    }
    if (length) {
        item.priority=priority;
        meq_push(meqstack,item);
    }
    return length;
}

static Functype
create_funcval(Int16 offset)
{
    Functype result;

    result.offs = offset;
    result.num = defined_funcs[offset].funcnum;
    result.paramcount = defined_funcs[offset].paramcount;

    return result;
}

static Int16 try_op(TCHAR *buf,Int16 priority,CError *err) PARSER;
static Int16 
try_op(TCHAR *buf,Int16 priority,CError *err)
{
    Int16 retval=0;
    Int16 i=0;
    Tmeq item;

    *err = c_noerror;
        if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }
        
    item.priority=priority;    
    for (;def_ops[i].name;i++) {
        if (0==StrNCompare(def_ops[i].name,buf,StrLen(def_ops[i].name))) {
            item.rpn.type=oper;
            item.rpn.u.funcval = create_funcval(def_ops[i].funcoffs);
            item.operpri=def_ops[i].operpri;
            item.evalRight = def_ops[i].evalRight;
            item.rpn.allocsize = 0;
            meq_push(meqstack,item);
            retval=StrLen(def_ops[i].name);
            break;
        }
    }
    return retval;
}

/*****************************************************/
/* The function for the shorthand expressin [a:b:c]  */
/* It simulates the behaviour of list(a:b:c)         */
/*****************************************************/
static Int16
try_shortlist(TCHAR *buf, Int16 priority)
{
    Int16 length=0;
    Int16 tmp,i;
    Int16 prmcnt1 = 1; /* Temp variable for parameter to konvert_to_meq*/
    Tmeq item;
    CError err;

    if (buf[0] != '[')
        return 0;
    length++;buf++;

    for (i=0;defined_funcs[i].name[0];i++) 
        if (StrCompare(defined_funcs[i].name,_T("list"))==0) {
            item.priority=priority;
            item.rpn.type=function;
            item.rpn.allocsize = 0;
            item.rpn.u.funcval = create_funcval(i);
            prmcnt1 = item.rpn.u.funcval.paramcount;
            i = meq_push(meqstack,item);
            if (!(tmp=konvert_to_meq(buf,priority+1,
                         &prmcnt1,_T(']'),&err)))
                return 0;
            meq_update_f_pcount(meqstack,i,prmcnt1);
            return length+tmp;
        }
    return 0;
    
}

static Int16 handleIfArguments(TCHAR *buf, Int16 priority, CError *err) PARSER;
static Int16
handleIfArguments(TCHAR *buf, Int16 priority, CError *err)
{
    TCHAR *ptr;
    Int16 prmcnt1 = 1;
    Int16 level = 0;
    Int16 count = 0;
    Int16 i;
    TCHAR lastchar;
    UInt16 len;
    Tmeq item;
    Char *text;
    static Char sepchar[2] = { _T(':'), _T(')') };

    /* We compile the condition argument as ususal. The true and false
     * arguments are stored as strings. They are compiled and
     * executed later in func_if(). This allows 'lazy evaluation' of
     * the if statement and recursion.
     * Now it is possible to define recursive functions, e.g. the factorial
     * can be defined as: mf()="if(x>1:mf(x-1)*x:1)".
     * Of course we need a check against stack overflow now. See file
     * chkstack.c */

    for (ptr = buf; *ptr && (*ptr != _T(':') || level > 0); ptr++, count++) {
        if (*ptr == _T('(')) level++;
        if (*ptr == _T(')') && level > 0) level--;
    }
    /* Now we have the condition argument, process it as normal */
    lastchar = *ptr;
    *ptr = _T('\0');
    if (!konvert_to_meq(buf, priority, &prmcnt1, _T(')'), err))
        return 0;
    *ptr = lastchar;
    if (*ptr)
        ptr++; count++;

    for (i = 0; i < 2; i++) {
        /* Isolate the true (i=0) or false (i=1) argument */
        for (buf = ptr; *ptr && (*ptr != sepchar[i] || level > 0);
             ptr++, count++) {
            if (*ptr == '(') level++;
            if (*ptr == ')' && level > 0) level--;
        }
        /* Push the true/false argument on the meq stack as a string */
        lastchar = *ptr; *ptr = '\0';
        len = StrLen(buf);
        if (!len) {
            *err = c_syntax;
            return 0;
        }
        text = (TCHAR *) MemPtrNew((len + 1)*sizeof(TCHAR));
        if (!text) {
            *err = c_memory;
            return 0;
        }
        StrCopy(text, buf);
        text[len] = '\0';
        item.priority = priority;
        item.rpn.type = string;
        item.rpn.u.stringval = text;
        item.rpn.allocsize = (len + 1)*sizeof(TCHAR);
        meq_push(meqstack, item);
        *ptr = lastchar;
        if (lastchar)
            ptr++; count++;
    }

    return count;
}

static Int16 
try_func(TCHAR *buf,Int16 priority,CError *err)
{    
    Int16 length=0;
    Int16 tmp,i;
    Int16 prmcnt1 = 1; /* Temp variable for parameter to konvert_to_meq*/
    TCHAR fname[MAX_FUNCNAME+1];
    Tmeq item;

    *err = c_noerror;
        if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }
        
    if (*buf==_T('(')) { /* Only parentheses */
        length++;
        buf++;    
        if (!(tmp=konvert_to_meq(buf,priority,&prmcnt1,')',err)))
            return 0;
        return length+tmp;
    }
    
    /* Check for 1st letter (mustn't be a number
     * so we needn't bother further */
    if (!IS_FUNC_1LETTER(*buf))
      return 0;
    
    while (IS_FUNC_LETTER(*buf) && length<MAX_FUNCNAME) {
        fname[length]=*buf;
        length++;buf++;
    }             
    fname[length]=_T('\0');
    if (*buf!=_T('(')) /* Not a function */
        return 0;
    length++;buf++;
    
    for (i=0;defined_funcs[i].name[0];i++) 
        if (StrCompare(defined_funcs[i].name,fname)==0) {
            item.priority=priority;
            item.rpn.type=function;
            item.rpn.allocsize = 0;
            item.rpn.u.funcval = create_funcval(i);
            prmcnt1 = item.rpn.u.funcval.paramcount;
            i = meq_push(meqstack,item);
            if (StrCompare(_T("if"), fname) == 0) {
                if (!(tmp = handleIfArguments(buf, priority + 1, err)))
                    return 0;
            }
            else if (!(tmp=konvert_to_meq(buf,priority+1,
                         &prmcnt1,_T(')'),err)))
                return 0;
            meq_update_f_pcount(meqstack,i,prmcnt1);
            return length+tmp;
        }
    /* Special - parameter name */
    if (StrCompare(fname,parameter_name) == 0) {
        item.priority = priority;
        item.rpn.type = function;
        item.rpn.allocsize = 0;
        item.rpn.u.funcval = create_funcval(FUNC_X);
        i = meq_push(meqstack,item);
        if (!(tmp=konvert_to_meq(buf,priority+1,
                     &prmcnt1,_T(')'),err)))
            return 0;
        meq_update_f_pcount(meqstack,i,prmcnt1);
        return length+tmp;
    }
    /* Not known function */
    if (*buf==_T(')')) /* Is an 'empty' function */ {
        item.priority=priority;
        item.rpn.type=funcempty;
        item.rpn.allocsize = 0;
        StrCopy(item.rpn.u.funcname,fname);
        meq_push(meqstack,item);
        return length+1;
    }
    /* Is a 'not known'(text) function with arguments */
    item.priority=priority;
    item.rpn.type=functext;
    item.rpn.allocsize = 0;
    /* Allow variable num of parameters for user defined functions */
    prmcnt1 = 0;
    StrCopy(item.rpn.u.textfunc.name,fname);

    i = meq_push(meqstack,item);
    if (!(tmp=konvert_to_meq(buf,priority+1,
                 &prmcnt1,_T(')'),err)))
        return 0;
//    item.rpn.u.textfunc.paramcount = 0;
    meq_update_t_pcount(meqstack,i,prmcnt1);

    return length+tmp;
}

static Int16 try_variable(TCHAR *buf,Int16 priority,CError *err) PARSER;
static Int16
try_variable(TCHAR *buf,Int16 priority,CError *err)
{
    Int16 length=0;
    Tmeq item;

    *err = c_noerror;
        if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }
        
    if (!IS_FUNC_1LETTER(*buf))
        return 0;
    
    item.priority=priority;
    while (IS_FUNC_LETTER(*buf) && length<MAX_FUNCNAME) {
        item.rpn.u.varname[length]=*buf;
        length++;buf++;
    }
    if (*buf==_T('(') || IS_FUNC_LETTER(*buf))
        return 0;
    /*  It is not a function, and it is smaller then MAX_FUNCNAME */
    item.rpn.u.varname[length]='\0';
    if (StrCompare(item.rpn.u.varname,parameter_name)==0) {
        /* Special - parameter of a function */
        item.rpn.type = function;
        item.rpn.u.funcval = create_funcval(FUNC_X);
        item.rpn.u.funcval.paramcount = 0;
        item.rpn.allocsize = 0;
        meq_push(meqstack,item);
        return length;
    }
    item.rpn.type=variable;
    item.rpn.allocsize = 0;
    meq_push(meqstack,item);
    return length;
}

/* Syntax of string: "stringkjdfj", 
 * dquot: "....\"...."
 * slash: "....\\...."
 */
static Int16 try_string(TCHAR *buf,Int16 priority,CError *err) PARSER;
static Int16
try_string(TCHAR *buf,Int16 priority,CError *err)
{
    TCHAR *text;
    Int16 i;
    Int16 slashcount=0;
    Tmeq item;

    *err = c_noerror;
        if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }
        
    if (*buf!=_T('"'))
        return 0;
    buf++;
    text = (TCHAR *) MemPtrNew((MAX_INPUT_LENGTH+1)*sizeof(TCHAR));
      
    
    for(i=0;*buf && *buf!=_T('"');i++,buf++) {
        /* Ignore special meaning of character following slash */
        if (*buf==_T('\\') && *(buf+1)) {
            buf++;
            slashcount++;
        }
        text[i]=*buf;
    }
    if (*buf!=_T('"')) {
        MemPtrFree(text);
        return 0;
    }
    text[i]='\0';

    item.priority=priority;
    item.rpn.type=string;
    item.rpn.u.stringval=text;
    item.rpn.allocsize=(StrLen(text)+1)*sizeof(TCHAR);
    meq_push(meqstack,item);
    return i+2+slashcount;
}

/* Converts a mathematical string to an array of 'tokens', where
 * every token is a number, operator or function. To every operator 
 * is assigned a priority based on the input parameter.
 * Input:
 * buf - string to decode
 * priority - starting priority
 * param_count - exact number of expected parameters delimited by ':'
 * meqcount - counter of used fields on the meq[] 
 * cbracket - closing bracket - to allow both '[' and '('
 * Output:
 * Int16 - 0 - error, otherwise number of characters decoded
 * Output is written to global array 'meq' with a
 * counter 'meqcount'
 * 
 * Basic prioritization:
 *   pr+3 : number, function
 *   pr+2 : '-' on the nonstandard place, connected to following number
 *   pr+1   : operators, they have added oper-priority, that is 
 *          later (meq_prioritize) used to reprioritize the neighbour items
 *          
 *   pr   : ':' -        
 *        : ';' - terminates completely whole expression,
 *                it's also given a special type 'discard',
 *                that is later recognized at the level of konvert_to_rpn
 * 
 * Algorithm:
 * Every mathematical expression consists of:
 * TOK1 - number|function
 * OPER - operator
 *  - TOK1[OPER TOK2[OPER TOK3...]]
 * with some exceptions, of course...
 */
#define EAT_SPACE     for (;*buf == _T(' ');buf++,length++);
static Int16
konvert_to_meq(TCHAR *buf,Int16 priority,Int16 *param_count, TCHAR cbracket,
               CError *err)
{    
    Int16 tmp;
    Int16 length=0;
    Int16 param_encountered = 1;
    Int16 (*try_num)(TCHAR *,Int16,CError *);
    Tmeq item;

    *err = c_noerror;
    if (chkStack()) {
        *err = c_stacklow;
        return 0;
    }

    if (dispPrefs.forceInteger)
        try_num=try_int_num;
    else
        try_num=try_real_num;
    
    /* We should allow an empty beginning */
    if (*buf==_T(';') && priority==0) {
        buf++;length++;
    }      

    EAT_SPACE;
    
    /* We can begin with '-',number or function */
    if (*buf==_T('-')) {
        buf++;
        length+=1;

        item.priority = priority+2;
        item.rpn.type = function;
        item.rpn.u.funcval = create_funcval(FUNC_NEGATE);
        item.rpn.allocsize = 0;
        meq_push(meqstack,item);
    }
    
    while (1) {
        EAT_SPACE;
        /* try_degree must be BEFORE try_num, because otherwise
         * the degree form wouldn't be recognized */
        if ((tmp=try_degree(buf,priority+3,err)) ||
            (!*err && (tmp=try_num(buf,priority+3,err))) ||
            (!*err && (tmp=try_func(buf,priority+3,err))) ||
            (!*err && (tmp=try_variable(buf,priority+3,err))) ||
            (!*err && (tmp=try_string(buf,priority+3,err))) ||
            (!*err && (tmp=try_shortlist(buf,priority+3)))) {
            buf+=tmp;
            length+=tmp;
        }
        else {
              return 0; /* Syntax error  or stack low */
        }

        /* Time for postfix operator */
        if (*buf == _T('\'')) {
            try_op(_T("\2"),priority+1,err);
            buf+=1;
            length+=1;
        } else if (*buf == _T('[')) {
            Int16 ptrcount;
            Int16 pos;

            try_op(_T("\3"),priority+1,err);
            /* Remember the position of the command, so that
             * we can update parameter count when it is known
             */
            pos = meq_count(meqstack) - 1;
            ptrcount = meqstack->array[pos].rpn.u.funcval.paramcount;

            buf+=1;
            length+=1;
            if (!(tmp = konvert_to_meq(buf,priority+3,&ptrcount,
                            _T(']'),err))) 
                return 0;
            /* Update the parameter count of the calling function */
            meq_update_f_pcount(meqstack,pos,ptrcount+1);

            buf+=tmp;
            length+=tmp;
        }

        EAT_SPACE;
        /* End of expression?, priority=0 means we are at starting level */
        if (!*buf || (*buf==cbracket && priority>0) 
            || (*buf==_T(';') && priority>0)) {
            if (*buf==cbracket)
                length++;
            if (*param_count == 0) 
                *param_count = param_encountered;
            else if (param_encountered != *param_count)
                return 0; /* Bad parameter count */
            return length;
        }
        
        EAT_SPACE;
        
        if ((tmp=try_op(buf,priority+1,err))) {
            buf+=tmp;
            length+=tmp;
        }
        /* Next parameter */
        else if (*buf==_T(':')) {
            param_encountered++;
            buf++;
            length++;

            item.priority = priority;
            item.rpn.type = function;
            item.rpn.u.funcval = create_funcval(FUNC_EMPTY);
            item.rpn.allocsize = 0;
            meq_push(meqstack,item);
        }
        /* This is for syntax like '3x'=>'3*x' and 3(3+4)*/
        else if (IS_FUNC_LETTER(*buf) || *buf==_T('(')) {
            try_op(_T("\1"),priority+1,err); /* Special high-priority '*' */
        }        
        else if (*buf==_T(';') && priority==0) {
            buf++;
            length++;
            item.priority = 0;
            item.rpn.type = discard;
            item.rpn.u.funcval = create_funcval(FUNC_DISCARD);
            item.rpn.allocsize = 0;
            meq_push(meqstack,item);
        }
        else 
          return 0; /* Syntax error */

        EAT_SPACE;
        
        /* Allow syntax like 3^-4 */
        if (*buf==_T('-')) {
            buf++;
            length+=1;
            
            item.priority = priority+2;
            item.rpn.type = function;
            item.rpn.u.funcval = create_funcval(FUNC_NEGATE);
            item.rpn.allocsize = 0;
            meq_push(meqstack,item);
        }
    }
}

/* Finds operators and raises priority of things with higher priority
 * backwards and forwards from the operator 
 * 
 * Input:
 * meq - global array that is filled with decoded expression from 
 *       konvert_to_meq
 * meqcount
 * Output:
 * meq
 */
static void meq_prioritize(void) PARSER;
static void
meq_prioritize(void)
{
    Int16 i,j;
    Tmeq *meq;
    Int16 meqcount;

    meq = meqstack->array;
    meqcount = meq_count(meqstack);

    /* Prioritize from operators everything, that has
     * pri>pri_of_the_operator */
    for (i=0;i<meqcount;i++) {
        if (meq[i].rpn.type==oper && meq[i].operpri>0) {
            /* Prioritize backwards */
            for (j=i-1;j>=0 && meq[j].priority>meq[i].priority;j--)
                meq[j].priority+=meq[i].operpri;
            /* Prioritize forwards */
            for (j=i+1;j<meqcount && meq[j].priority>meq[i].priority;j++)
                meq[j].priority+=meq[i].operpri;
        }
    }
    
    /* Prioritize the particular operators*/
    for (i=0;i<meqcount;i++) {
        if (meq[i].rpn.type==oper)
            meq[i].priority+=meq[i].operpri;
    }
    /* Prioritize forward +1 for evalRight operators (handle 2^3^4)
     * and 2x^3pi -> 2(x^(3pi))
     * This shouldn't break all other priorities, as it only
     * raises priority of things, that are already higher then
     * the priority of the POWER operator */
    for (i=0;i<meqcount;i++) {
        if (meq[i].rpn.type==oper && meq[i].evalRight)
            for (j=i+1;j<meqcount && meq[j].priority>=meq[i].priority;j++)
                meq[j].priority++;
    }
    /* Convert operators to ordinary function */
    for (i=0;i<meqcount;i++) {
        if (meq[i].rpn.type==oper)
            meq[i].rpn.type=function;
    }
}


/* Converts a prioritized array to Reverse Polish Notation (at least I hope
 * it is) stack. 
 * 
 * Meq is now prioritized after konvert_to_meq and meq_prioritize,
 * so we just 'sort' it following way:
 * for item in meq[]:
 *   if operstack empty - put on operstack
 *   else flush everything with higher priority from operstack to rpnstack
 *   put item on operstack
 * flush operstack to rpn
 * 
 * Input:
 * meq, meqcount
 * Output:
 * rpn, rpncount - global array, resulting rpn
 */
static CodeStack *konvert_to_rpn(void) PARSER;
static CodeStack *
konvert_to_rpn(void)
{
    Tdynmeq * operstack = meq_new();
    CodeStack *result = stack_new(meq_count(meqstack));
    Tmeq item;

    while (meq_count(meqstack)) {
        item = meq_fetch(meqstack);
        if (item.rpn.type==discard) {
            /* The ';' terminates the everything processed till now,
             * so we flush the operstack and append a ';' function */
            while (meq_count(operstack))
                stack_push(result,meq_pop(operstack).rpn);
            item.rpn.type=function;
            stack_push(result,item.rpn);
        } 
        else {
            if (meq_count(operstack) == 0)
                meq_push(operstack,item);
            else {
                while (meq_count(operstack) > 0 && 
                       meq_last(operstack).priority >= item.priority) {
                    /* Handle case -2^3 -> -(2^3), another exception :-( */
                    if (meq_last(operstack).priority-item.priority==1 &&
                        meq_last(operstack).rpn.u.funcval.offs==FUNC_NEGATE &&
                        item.rpn.u.funcval.offs==FUNC_POWER)
                        break;
                    stack_push(result,meq_pop(operstack).rpn);
                }
                meq_push(operstack,item);
            }
        }
    }
    /* Flush the rest on the rpn stack */
    while (meq_count(operstack)) 
        stack_push(result,meq_pop(operstack).rpn);
    meq_free(operstack);

    return result;
}

/* Convert a string to CodeStack, for conversion is used a global
 * array, the result is then copied to a newly allocated space 
 */
CodeStack *
text_to_stack(TCHAR *buf,CError *err)
{
    CodeStack *result;
    Int16 prmcount;

    meqstack = meq_new();
    
    prmcount = 1;
    if (!konvert_to_meq(buf,0,&prmcount,_T(')'),err)) {
        /* Free the alocated variables */
        meq_free(meqstack);
        if (*err == c_noerror)
            *err = c_syntax;
        return NULL;
    }
    meq_prioritize();
    result = konvert_to_rpn();
    stack_reverse(result);
    
    meq_free(meqstack);
    *err = c_noerror;
    return result;
}

Err
mlib_init_mathlib(void)
{
    Err error = 0;

//    error = SysLibFind(MathLibName, &MathLibRef);
//    if (error)
//        error = SysLibLoad(LibType, MathLibCreator, &MathLibRef);
//    if (!error)
//        error = MathLibOpen(MathLibRef, MathLibVersion);
//    if (error)
//        FrmAlert(altMathlib);
    return error;
}

void mlib_close_mathlib(void)
{
//    Err error;
//    UInt16 usecount;
//
//    error = MathLibClose(MathLibRef,&usecount);
//    ErrFatalDisplayIf(error, _T("Can't close MathLib"));
//    if (usecount == 0)
//      SysLibRemove(MathLibRef);
}

/***********************************************************************
 *
 * FUNCTION:     konvert_get_sorted_fn_list
 *
 * DESCRIPTION:  Return a sorted list of builtin functions
 *
 * PARAMETERS:   *count - return number of items
 *
 * RETURN:       TCHAR ** - dynamical array of pointers to static names
 *      
 ***********************************************************************/
TCHAR **
konvert_get_sorted_fn_list(Int16 *count)
{
    Int16 gap,i,j;
    TCHAR *temp;
    TCHAR **names;
    
    
    /* count the functions */
    for (i=0,*count=0;defined_funcs[i].name[0];i++)
      if (IS_FUNC_1LETTER(defined_funcs[i].name[0]))
        (*count)++;
    
    names = (TCHAR **) MemPtrNew(*count * sizeof(*names));
    
    for (i=0,j=0;defined_funcs[i].name[0];i++)
      if (IS_FUNC_1LETTER(defined_funcs[i].name[0]))
        names[j++]=(TCHAR *)defined_funcs[i].name;
    
    /* Shell sort the array */
    for (gap = *count/2;gap > 0; gap/=2)
      for (i = gap;i < *count;i++)
        for (j=i-gap;j>=0 && StrCompare(names[j],names[j+gap])>0; j-=gap) {
            temp = names[j];
            names[j] = names[j+gap];
            names[j+gap] = temp;
        }
    
    return names;
}
