/*
 *   $Id: display.c,v 1.13 2007/12/19 15:47:26 cluny Exp $
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

#include <PalmOS.h>

#include "konvert.h"
#include "calc.h"
#include "mathem.h"
#include "display.h"
#include "defuns.h"
#include "funcs.h"
#include "prefs.h"
#include "fp.h"
#include "stack.h"
#include "MathLib.h"

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
char *
display_complex(Complex number)
{
	char *result;
	
	result = MemPtrNew(dispPrefs.decPoints*2+45);
	result[0] = '\0';
	
	if (number.real != 0.0 || number.imag==0.0) {
		fp_print_double(result,number.real);
		if (number.imag > 0.0 && finite(number.imag))
		  StrCat(result,"+");
	}
	if (number.imag != 0.0 && !isnan(number.imag)) {
		fp_print_double(result+StrLen(result),number.imag);
		StrCat(result,"i");
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
char *
display_real(double number)
{
	char *result;
		 
	result = MemPtrNew(dispPrefs.decPoints+64); /* The ipart shouldn't be more then 10 */
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

	result = MemPtrNew(33);	/* 32 bits + '\0' */
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
			number = -number;
			*result = '-';
			result1 = result + 1;
		}
		base=10;
		break;
	}

	for (i=0; number; i++, number /= base)
		if ((number % base) < 10)
			result1[i]='0'+(number % base);
		else
			result1[i]='A'+(number % base) - 10;

	result1[i]='\0';		
	/* Reverse the string */
	for (j = 0, i--; j < i; j++, i--) {
		tmp = result1[i];
		result1[i] = result1[j];
		result1[j] = tmp;
	}

	if (StrLen(result) == 0)
		StrCopy(result,"0");

	return result;
}

char *
display_list(List *list)
{
	char *result,*tmp;
	Int16 i;

	result = MemPtrNew((UInt32)(dispPrefs.decPoints*2+10)*list->size + 10);
	if (!result) {
		alertErrorMessage(c_memory);
		return NULL;
	}
	StrCopy(result,"[");
	for (i=0;i<list->size;i++) {
		tmp = display_complex(list->item[i]);
		StrCat(result,tmp);
		StrCat(result,":");
		MemPtrFree(tmp);
	}
	/* Cut off the last ':' */
	result[StrLen(result)-1] = '\0';
	StrCat(result,"]");

	return result;
}

char *
display_matrix(Matrix *m)
{
	Int16 i,j;
	char *result,*tmp;

	result = MemPtrNew((dispPrefs.decPoints+15)*m->rows*m->cols + 10);
	StrPrintF(result,"[");
	for (i=0;i<m->rows;i++) {
		if (i) StrCat(result,":");
		StrCat(result,"[");
		for (j=0;j<m->cols;j++) {
			if (j) StrCat(result,":");
			tmp = display_real(MATRIX(m,i,j));
			StrCat(result,tmp);
			MemPtrFree(tmp);
		}
		StrCat(result,"]");
	}
	StrCat(result,"]");
	return result;
}

char *
display_cmatrix(CMatrix *m)
{
	Int16 i,j;
	char *result,*tmp;

	result = MemPtrNew((dispPrefs.decPoints+15)*m->rows*m->cols*2 + 10);
	StrPrintF(result,"[");
	for (i=0;i<m->rows;i++) {
		if (i) StrCat(result,":");
		StrCat(result,"[");
		for (j=0;j<m->cols;j++) {
			if (j) StrCat(result,":");
			tmp = display_complex(MATRIX(m,i,j));
			StrCat(result,tmp);
			MemPtrFree(tmp);
		}
		StrCat(result,"]");
	}

	StrCat(result,"]");
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
char *
display_default(Trpn rpn, Boolean complete)
{
	char *result;
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
		result = MemPtrNew(StrLen(rpn.u.stringval)+1);
		StrCopy(result,rpn.u.stringval);
		break;
	case list:
		if (complete)
			result = display_list(rpn.u.listval);
		else {
			result = MemPtrNew(20);
			StrPrintF(result,"list(..%d..)",rpn.u.listval->size);
		}
		break;
	case matrix:
		if (complete)
			result = display_matrix(rpn.u.matrixval);
		else {
			result = MemPtrNew(20);
			StrPrintF(result,"matrix(%d,%d)",
				  rpn.u.matrixval->rows,rpn.u.matrixval->cols);
		}
		break;
	case cmatrix:
		if (complete)
			result = display_cmatrix(rpn.u.cmatrixval);
		else {
			result = MemPtrNew(20);
			StrPrintF(result,"cmatrix(%d,%d)",
				  rpn.u.cmatrixval->rows,
				  rpn.u.cmatrixval->cols);
		}
		break;
	case funcempty:
		result = MemPtrNew(StrLen(rpn.u.funcname)+5);
		StrCopy(result,rpn.u.funcname);
		StrCat(result,"()");
		break;
	default:
		result = MemPtrNew(16);
		StrCopy(result,"Unknown type.");
		break;
	}	
	if (deleterpn)
		rpn_delete(rpn);
	return result;
}
