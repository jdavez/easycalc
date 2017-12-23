/*
 *   $Id: display.cpp,v 1.7 2009/12/15 21:37:44 mapibid Exp $
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

#include "stdafx.h"

#include "compat/PalmOS.h"
#include "konvert.h"
#include "core/core_display.h"
#include "Skin.h"
#include "Easycalc.h"
#include "mathem.h"
#define _DISPLAY_C_
#include "display.h"
#include "defuns.h"
#include "funcs.h"
#include "core/prefs.h"
#include "fp.h"
#include "stack.h"
#include "compat/MathLib.h"

TdispPrefs dispPrefs;

/***********************************************************************
 *
 * FUNCTION:     display_complex
 *
 * DESCRIPTION:  Return a textualy formated complex number
 *
 * NOTE;         Returns a dynamically allocated structure,
 *               it must be freed after use
 *
 * PARAMETERS:   number
 *
 * RETURN:       textualy formated number
 *
 ***********************************************************************/
TCHAR *
display_complex(Complex number)
{
    TCHAR *result;

    result = (TCHAR *) MemPtrNew((dispPrefs.decPoints*2+45)*sizeof(TCHAR));
    result[0] = _T('\0');

    if (number.real != 0.0 || number.imag==0.0) {
        fp_print_double(result,number.real);
        if (number.imag > 0.0 && finite(number.imag))
          StrCat(result,_T("+"));
    }
    if (number.imag != 0.0 && !isnan(number.imag)) {
        fp_print_double(result+StrLen(result),number.imag);
        StrCat(result,_T("i"));
    }

    return result;
}

/***********************************************************************
 *
 * FUNCTION:     double_real
 *
 * DESCRIPTION:  Return a nicely formatted real number
 *
 * NOTE:         Result is a dynamically allocated text!
 *
 * PARAMETERS:   number
 *
 * RETURN:       formatted text
 *
 ***********************************************************************/
TCHAR *
display_real(double number)
{
    TCHAR *result;

    result = (TCHAR *) MemPtrNew((dispPrefs.decPoints+64)*sizeof(TCHAR)); /* The ipart shouldn't be more than 10 */
    fp_print_double(result,number);

    return result;
}

Char *
display_integer(UInt32 number, Tbase mode)
{
    Int16 base;
    Int16 i, j;
    Int32 signednum;
    Char *result, *result1, tmp;

    result = (TCHAR *) MemPtrNew(33*sizeof(TCHAR)); /* 32 bits + '\0' */
    result1 = result; /* result1 points to digits after possible '-' sign */

    switch (mode) {
     case disp_binary:
        base=2;
        break;
     case disp_octal:
        base=8;
        break;
     case disp_hexa:
        base=16;
        break;
     case disp_decimal:
     default:
        /* for decimal display signed number */
        if ((signednum = number) < 0) {
            number = -signednum;
            *result = _T('-');
            result1 = result + 1;
        }
        base=10;
        break;
    }

    for (i=0; number; i++, number /= base)
        if ((number % base) < 10)
            result1[i]=_T('0')+(number % base);
        else
            result1[i]=_T('A')+(number % base) - 10;

    result1[i]=_T('\0');
    /* Reverse the string */
    for (j = 0, i--; j < i; j++, i--) {
        tmp = result1[i];
        result1[i] = result1[j];
        result1[j] = tmp;
    }

    if (StrLen(result) == 0)
        StrCopy(result,_T("0"));

    return result;
}

TCHAR *
display_list(List *list)
{
    TCHAR *result,*tmp;
    Int16 i;

    result = (TCHAR *) MemPtrNew(((UInt32)(dispPrefs.decPoints*2+10)*list->size + 10)*sizeof(TCHAR));
    if (!result) {
        alertErrorMessage(c_memory);
        return NULL;
    }
    StrCopy(result,_T("["));
    for (i=0;i<list->size;i++) {
        tmp = display_complex(list->item[i]);
        StrCat(result,tmp);
        StrCat(result,_T(":"));
        MemPtrFree(tmp);
    }
    /* Cut off the last ':' */
    result[StrLen(result)-1] = _T('\0');
    StrCat(result,_T("]"));

    return result;
}

TCHAR *
display_matrix(Matrix *m)
{
    Int16 i,j;
    TCHAR *result,*tmp;
    int index;

    result = (TCHAR *) MemPtrNew(((dispPrefs.decPoints+15)*m->rows*m->cols + 10)*sizeof(TCHAR));
    index = StrPrintF(result,_T("["));
    for (i=0;i<m->rows;i++) {
        if (i)
            StrCat(result+index,_T(":"));
        StrCat(result+index,_T("["));
        for (j=0;j<m->cols;j++) {
            if (j) StrCat(result+index,_T(":"));
            tmp = display_real(MATRIX(m,i,j));
            StrCat(result+index,tmp);
            MemPtrFree(tmp);
            index += StrLen(result+index);
        }
        StrCat(result+index,_T("]"));
    }
    StrCat(result+index,_T("]"));
    return result;
}

TCHAR *
display_cmatrix(CMatrix *m)
{
    Int16 i,j;
    TCHAR *result,*tmp;

    result = (TCHAR *) MemPtrNew(((dispPrefs.decPoints+15)*m->rows*m->cols*2 + 10)*sizeof(TCHAR));
    StrPrintF(result,_T("["));
    for (i=0;i<m->rows;i++) {
        if (i) StrCat(result,_T(":"));
        StrCat(result,_T("["));
        for (j=0;j<m->cols;j++) {
            if (j) StrCat(result,_T(":"));
            tmp = display_complex(MATRIX(m,i,j));
            StrCat(result,tmp);
            MemPtrFree(tmp);
        }
        StrCat(result,_T("]"));
    }

    StrCat(result,_T("]"));
    return result;
}

/***********************************************************************
 *
 * FUNCTION:     display_default
 *
 * DESCRIPTION:  Default displaying routine, automatically detects
 *               type of input and formats it accordingly
 *
 * NOTE:         Result is a dynamically allocated, muste be freed after use.
 *
 * PARAMETERS:   rpn
 *               complete - if true, the result can be chewed in be
 *                          the compiler and produces itself
 *
 * RETURN:       formatted rpn suitable for output
 *
 ***********************************************************************/
TCHAR *
display_default(Trpn rpn, Boolean complete, rpntype *resultType)
{
    TCHAR *result;
    CError err;
    Boolean deleterpn = false;

    if (rpn.type==variable || rpn.type == litem) {
            if (rpn.type == litem)
            deleterpn = true;
        err=rpn_eval_variable(&rpn,rpn);
        if (err) {
            result = print_error(err);
            return result;
        }
    }

    if (resultType)
        *resultType = rpn.type;
    switch (rpn.type) {
    case integer:
        result = display_integer(rpn.u.intval,dispPrefs.base);
        break;
    case complex:
        result = display_complex(*rpn.u.cplxval);
        break;
    case real:
        result = display_real(rpn.u.realval);
        break;
    case string:
        result = (TCHAR *) MemPtrNew((StrLen(rpn.u.stringval)+1)*sizeof(TCHAR));
        StrCopy(result,rpn.u.stringval);
        break;
    case list:
        if (complete)
            result = display_list(rpn.u.listval);
        else {
            result = (TCHAR *) MemPtrNew(20*sizeof(TCHAR));
            StrPrintF(result,_T("list(..%d..)"),rpn.u.listval->size);
        }
        break;
    case matrix:
        if (complete)
            result = display_matrix(rpn.u.matrixval);
        else {
            result = (TCHAR *) MemPtrNew(20*sizeof(TCHAR));
            StrPrintF(result,_T("matrix(%d,%d)"),
                  rpn.u.matrixval->rows,rpn.u.matrixval->cols);
        }
        break;
    case cmatrix:
        if (complete)
            result = display_cmatrix(rpn.u.cmatrixval);
        else {
            result = (TCHAR *) MemPtrNew(20*sizeof(TCHAR));
            StrPrintF(result,_T("cmatrix(%d,%d)"),
                  rpn.u.cmatrixval->rows,
                  rpn.u.cmatrixval->cols);
        }
        break;
    case funcempty:
        result = (TCHAR *) MemPtrNew((StrLen(rpn.u.funcname)+5)*sizeof(TCHAR));
        StrCopy(result,rpn.u.funcname);
        StrCat(result,_T("()"));
        break;
    default:
        result = (TCHAR *) MemPtrNew(16*sizeof(TCHAR));
        StrCopy(result,_T("Unknown type."));
        break;
    }
    if (deleterpn)
        rpn_delete(rpn);
    return result;
}
