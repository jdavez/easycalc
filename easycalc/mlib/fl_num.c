/*   
 *   $Id: fl_num.c,v 1.5 2006/09/12 19:40:56 tvoverbe Exp $
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

#include <PalmOS.h>

#include "segment.h"
#include "konvert.h"
#include "display.h"
#include "fp.h"
#include "prefs.h"

#define isdigit(x) (x>='0' && x<='9')

/* Is a number, equal to /[0-9]+/ */
static Int16 is_ccbz(char *input,double *cislo) MLIB;
static Int16
is_ccbz(char *input,double *cislo)
{
	Int16 i,j;
	Int16 spaces = 0;

	/* Count to 'i' the length of a number */
	for (i=0;;i++) {
		if (input[i]>='0' && input[i]<='9')
			j = input[i]-'0';
		else if (input[i]>='A' && input[i]<='F')
			j = input[i] - 'A' + 10;
		else if (calcPrefs.acceptPalmPref && input[i] == flSpaceChar) {
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
			*cislo+=input[j]<='9'?input[j]-'0':input[j]-'A'+10;
		}
		j=*cislo;
	}

	if (spaces == i)
		return 0;
	return i;
}

static Int16 has_expon(char *input,double *cislo) MLIB;
static Int16
has_expon(char *input,double *cislo)
{
	char *tmp=input;
	Int16 delka=0;
	Int16 i;
	
	if (*tmp=='E') {
		Int16 negate=0;
		double expon;
		
		tmp++;delka++;
		if (*tmp=='-') {
			negate=1;
			tmp++;delka++;
		}
		else if (*tmp=='+') {
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

/* Is floating number, return number of characters describing
 * the number */
Int16 fl_num(char *input,double *cislo) MLIB;
Int16
fl_num(char *input,double *cislo)
{	
	char *tmp=input;
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
	
	if (calcPrefs.acceptPalmPref && *tmp != flPointChar && delka==0)
		return 0;
	else if (!calcPrefs.acceptPalmPref && *tmp != '.' && *tmp!=',' && delka==0)
		return 0;
	else if (((calcPrefs.acceptPalmPref && *tmp != flPointChar)
		  || (!calcPrefs.acceptPalmPref && *tmp!='.' && *tmp!=','))
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
		if (flSpaceChar == ' ' && *tmp == ' ') {
			for (;*tmp == ' ';tmp++)
				;
			if (*tmp >= '0' && *tmp <= '9')
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
			if (calcPrefs.acceptPalmPref && tmp[j] == flSpaceChar)
				continue;
			cele+=((double)(tmp[j]<='9'?tmp[j]-'0':tmp[j]-'A'+10))*dfl;
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
	if (flSpaceChar == ' ' && *tmp == ' ') {
		for (;*tmp == ' ';tmp++)
			;
		if (*tmp >= '0' && *tmp <= '9')
			/* space and number found */
			return 0;
	}
	return delka;
}
