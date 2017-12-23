/*   
 *   $Id: fl_num.cpp,v 1.1 2009/10/17 13:48:34 mapibid Exp $
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
*/
#include "StdAfx.h"

#include "compat/PalmOS.h"

#include "compat/segment.h"
#include "konvert.h"
#include "display.h"
#include "fp.h"
#include "core/prefs.h"

#ifdef PALMOS
#define isdigit(x) ((x>=_T('0')) && (x<=_T('9')))
#endif

/* Is a number, equal to /[0-9]+/ */
static Int16 is_ccbz(TCHAR *input,double *cislo) MLIB;
static Int16
is_ccbz(TCHAR *input,double *cislo)
{
	Int16 i,j;
	Int16 spaces = 0;

	/* Count to 'i' the length of a number */
	for (i=0;;i++) {
		if (input[i]>=_T('0') && input[i]<=_T('9'))
			j = input[i]-_T('0');
		else if (input[i]>=_T('A') && input[i]<=_T('F'))
			j = input[i] - _T('A') + 10;
		else if (calcPrefs.acceptOSPref && input[i] == flSpaceChar) {
			spaces++;
			continue;
		} else
			break;
		if (j>dispPrefs.base-1)
			break;
	}		

	if (cislo) {
		*cislo=0.0;
		for (j=0;j<i;j++) {
			if (input[j] == flSpaceChar)
				continue;
			*cislo*=dispPrefs.base;
			*cislo+=input[j]<=_T('9')?input[j]-_T('0'):input[j]-_T('A')+10;
		}
		j= (Int16) (*cislo);
	}

	if (spaces == i)
		return 0;
	return i;
}

static Int16 has_expon(TCHAR *input,double *cislo) MLIB;
static Int16
has_expon(TCHAR *input,double *cislo)
{
	TCHAR *tmp=input;
	Int16 delka=0;
	Int16 i;
	
	if (*tmp==_T('E')) {
		Int16 negate=0;
		double expon;
		
		tmp++;delka++;
		if (*tmp==_T('-')) {
			negate=1;
			tmp++;delka++;
		}
		else if (*tmp==_T('+')) {
			tmp++;delka++;
		}
	        if (!(i=is_ccbz(tmp,&expon)))
		  return 0;
		delka+=i;
/* Prevent excessive waiting */
		if (expon>350.0)
		  return 0;
		
		*cislo=1.0;
		if (!negate)
		  for (i=0;i<expon;i++)
		    *cislo*=10.0;
		else
		  for (i=0;i<expon;i++)
		    *cislo/=10.0;
	}
	return delka;
}

/* If floating number, return number of characters describing
 * the number */
Int16 fl_num(TCHAR *input,double *cislo) MLIB;
Int16
fl_num(TCHAR *input,double *cislo)
{	
	TCHAR *tmp=input;
	double cele;
	double expon;
	Int16 i=0,j;
	double dfl;
	Int16 delka=0;
		
	if ((i=is_ccbz(tmp,&cele))) {
		tmp+=i;
		delka+=i;
	}
	else
	  cele=0;       
	
	if (calcPrefs.acceptOSPref && *tmp != flPointChar && delka==0)
		return 0;
	else if (!calcPrefs.acceptOSPref && *tmp != _T('.') && *tmp!=_T(',') && delka==0)
		return 0;
	else if (((calcPrefs.acceptOSPref && *tmp != flPointChar)
		  || (!calcPrefs.acceptOSPref && *tmp!=_T('.') && *tmp!=_T(',')))
		 && delka>0) {
		*cislo=cele;
		if ((i=has_expon(tmp,&expon))) {
			*cislo*=expon;
			delka+=i;tmp+=i;	
		}

		/* If a space is a thousend delimiter, check that
		 * there is not another number following a few spaces.
		 * this would interpret '3 4' as '34' or '3*4' depending
		 * on preferences if I didn't check here
		 */
		if (flSpaceChar == _T(' ') && *tmp == _T(' ')) {
			for (;*tmp == ' ';tmp++)
				;
			if (*tmp >= _T('0') && *tmp <= _T('9'))
				/* space and number found */
				return 0;
		}
		return delka;
	}
	tmp++;delka++;
	
	/* If we have floating point part */
	if ((i=is_ccbz(tmp,NULL))) {
		delka+=i;
		
		dfl=1.0/(double)dispPrefs.base;
		for (j=0;j<i;j++) {		
			if (calcPrefs.acceptOSPref && tmp[j] == flSpaceChar)
				continue;
			cele+=((double)(tmp[j]<=_T('9')?tmp[j]-_T('0'):tmp[j]-_T('A')+10))*dfl;
			dfl/=dispPrefs.base;
		}
		tmp+=i;
	}
	if (!delka)
		return 0;
	*cislo=cele;

	if ((i=has_expon(tmp,&expon))) {
		*cislo*=expon;
		delka+=i;tmp+=i;	
	}

	/* If a space is a thousend delimiter, check that
	 * there is not another number following a few spaces.
	 * this would interpret '3 4' as '34' or '3*4' depending
	 * on preferences if I didn't check here
	 */
	if (flSpaceChar == _T(' ') && *tmp == _T(' ')) {
		for (;*tmp == _T(' ');tmp++)
			;
		if (*tmp >= _T('0') && *tmp <= _T('9'))
			/* space and number found */
			return 0;
	}
	return delka;
}
