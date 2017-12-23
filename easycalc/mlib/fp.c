/*
 *   $Id: fp.c,v 1.17 2007/12/15 13:25:56 cluny Exp $
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


#ifdef UNIX
#define StrChr(x,y)	strchr(x,y)
#define StrCopy(x,y)	strcpy(x,y)
#define StrLen(x)	strlen(x)

#include <stdlib.h>
#include <string.h>
#else
#include <PalmOS.h>
#include <StringMgr.h>
#include <MathLib.h>
#endif
#include "defuns.h"
#include "display.h"
#include "fp.h"

static double SCI_LIMIT_MAX;   /* Above this number show it as xEy */
static double SCI_LIMIT_MIN;   /* Under this number show as xE-y */
//static double DISP_MIN_LIMIT = 1E-15; /* Show numbers as sin(pi) as 1E-16,
//				       it's 0 */
//static double DISP_MAX_LIMIT = 1E300; /* Don't show bigger numbers, it
//				       * makes problems */
static double BASE = 10.0;
char flPointChar = '.';
char flSpaceChar = ' ';
 
/* If the number is smaller, reduce the number of 
 * decimal points, so we do not get something like
 * 9.999999987E-10
 */
//static double REDUCE_PREC = 1E-6;
#define REDUCE_TO 6

void fp_setup_flpoint(void) PARSER;
void fp_setup_flpoint(void)
{
	/* Set the floating 'point' to '.' or ',' */	
	switch (PrefGetPreference(prefNumberFormat)) {
	case nfCommaPeriod:
		flSpaceChar=',';
		flPointChar='.';
		break;
	 case nfApostrophePeriod:
		flSpaceChar='\'';
		flPointChar='.';
		break;
	 case nfPeriodComma:
		flSpaceChar='.';
		flPointChar=',';
		break;
	 case nfSpaceComma:
		flSpaceChar=' ';
		flPointChar=',';
		break;
	 case nfApostropheComma:
	 default:
		flSpaceChar='\'';
		flPointChar=',';
		break;
	}
}

/***********************************************************************
 *
 * FUNCTION:     fp_reread_prefs
 * 
 * DESCRIPTION:  Setups some maximum/miniums according to displayPrefs
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void fp_reread_prefs(void) PARSER;
static void fp_reread_prefs(void)
{
	BASE = dispPrefs.base;
	switch (dispPrefs.mode) {
	 case disp_normal:
//		SCI_LIMIT_MAX=1E10;
		SCI_LIMIT_MAX=pow(BASE,10);
//		SCI_LIMIT_MIN=1E-5;
		SCI_LIMIT_MIN=pow(BASE,-5);
		break;
	 case disp_sci:
	 case disp_eng:		
		SCI_LIMIT_MAX=1.00;
		SCI_LIMIT_MIN=1.01;
		break;	
	}
//	if (dispPrefs.reducePrecision) {
//		DISP_MIN_LIMIT = 1E-15;
//		REDUCE_PREC = 1E-6;
//	} else {
//		DISP_MIN_LIMIT = 1E-100;
//		REDUCE_PREC = 1E-90;
//	}
}

TdispPrefs fp_set_prefs(TdispPrefs prefs) PARSER;
TdispPrefs
fp_set_prefs(TdispPrefs prefs)
{
	TdispPrefs oldprefs = dispPrefs;

	dispPrefs = prefs;
	fp_reread_prefs();

	return oldprefs;
}

void fp_set_base(Tbase base) PARSER; 
void
fp_set_base(Tbase base)
{
	dispPrefs.base = base;
	fp_reread_prefs();
}

Tdisp_mode fp_set_dispmode(Tdisp_mode newmode) PARSER;
Tdisp_mode 
fp_set_dispmode(Tdisp_mode newmode)
{
	Tdisp_mode oldmode = dispPrefs.mode;
	dispPrefs.mode = newmode;
	return oldmode;
}
	
	

/***********************************************************************
 *
 * FUNCTION:     cvt_fltoa
 * 
 * DESCRIPTION:  Convert a truncated float to ascii
 *
 * PARAMETERS:   value
 *
 * RETURN:       strP - converted value
 *      
 ***********************************************************************/
static void cvt_fltoa(char *strP,double value) PARSER;
static void
cvt_fltoa(char *strP,double value)
{
	char *cP;
	char tmpstr[MAX_FP_NUMBER+3];
	Int16 i,ichar;	

	if (value==0.0) {
		StrCopy(strP,"0");
		return;
	}
	/* We convert the number backwards */
	for (i=MAX_FP_NUMBER-2;i>=0;i--) {
		ichar = (Int16)fmod(value,BASE) + '0';
		tmpstr[i] = ichar>'9'?ichar-'9'-1+'A':ichar;
		value /= BASE;
		value=trunc(value);
	}
	
	tmpstr[MAX_FP_NUMBER-1] = '\0';
	
	for( cP = tmpstr; *cP && (*cP == '0'); )
	  ++cP;
	
	StrCopy(strP,cP);
}

/***********************************************************************
 *
 * FUNCTION:     fp_print
 * 
 * DESCRIPTION:  Prints a double to string
 *
 * PARAMETERS:   value 
 *
 * RETURN:       strP - converted value
 *      
 ***********************************************************************/
static void
fp_print(char *strP,double value,Int16 myfrac) PARSER;
static void
fp_print(char *strP,double value,Int16 myfrac)
{
	Int16 i;
	double ipart;
	double fpart;
	double limit;
	char str[MAX_FP_NUMBER*2];
	
	if (isinf(value)) {
		if (isinf(value)>0) 
		  StrCopy(strP,"+Inf");
		else
		  StrCopy(strP,"-Inf");
		return;
	}
	if (!finite(value)) {
		StrCopy(strP,"NaN");
		return;
	}

	if( value < 0.0 ) {
		*strP++ = '-';
		value = -value;
	}

	ipart = trunc(value);
	value -= ipart;
	/* recover frac part */
	for (limit = 1.0,i=myfrac; i> 0;i--) {
		value *= BASE;
		limit *= BASE;
	}
	
	value += 0.5;
	if( value >= limit ) {
		fpart = 0.0;	/* overflowed */
		ipart++;
	}
	else
		fpart = value;

	cvt_fltoa(strP,ipart);

	if (myfrac || !dispPrefs.stripZeros) {
		cvt_fltoa(str,fpart);
		strP += StrLen(strP);

		if (!dispPrefs.stripZeros && fpart < 1.0) {
			*strP++ = flPointChar;
			for (i = 0; i < myfrac; i++) {
				*strP++ = '0';
			}
			*strP++ = '\0';
		}
		if (dispPrefs.stripZeros) {
			/* remove trailing 0's */
			for (i = StrLen(str)-1;i>=0 && str[i]=='0';i--) {
				str[i]='\0';
				myfrac--;
			}	      
		}      
		if (StrLen(str)>0) {
			*strP++ = flPointChar;
			for(i=StrLen(str); i<myfrac;i++ )
				*strP++ = '0';	/* need some padding at the beginning */
			StrCopy(strP,str);	/* and now the value */
		}
	}
}

static const char expNames[] = {'f','p','n','u','m',0,'k','M','G','T'};

static void fp_print_exp(char *str,Int16 expon) PARSER;
static void
fp_print_exp(char *str,Int16 expon)
{
	Int16 i;
	
	if (dispPrefs.base == disp_decimal) {
		if (dispPrefs.mode == disp_eng && dispPrefs.cvtUnits) {
			i = expon/3 + 5;
			if (i<0 || i>9)
			  StrPrintF(str,"E%d",expon);
			else 
			  StrPrintF(str,"%c",expNames[i]);
		}
		else
		  StrPrintF(str,"E%d",expon);	
	}
	else 
		StrPrintF(str, "*%d^%d",(Int16)dispPrefs.base, expon);
}

/***********************************************************************
 *
 * FUNCTION:     fp_print_double
 * 
 * DESCRIPTION:  Function used for general number formatting according
 *               all rules from dispPrefs, adds the 'E+-' things
 *               for too big numbers, scientific & engineer purposes
 *
 * PARAMETERS:   value
 *
 * RETURN:       strP - converted value
 *      
 ***********************************************************************/
void fp_print_double(char *strP,double value) PARSER;
void 
fp_print_double(char *strP, double value)
{
	Int16 ex;
	char tmpstr[MAX_FP_NUMBER*2];
	     
	
	if (!finite(value)) {
		/* Handle the NaN, +Inf & -Inf cases */
		fp_print(strP,value,dispPrefs.decPoints);
	}
//	else if (fabs(value)>DISP_MAX_LIMIT) {
//		StrCopy(strP,"It's too big");
//	}
//	else if (fabs(value)<DISP_MIN_LIMIT) { 
//		/* Don't show too small numbers */
//		fp_print(strP,0,dispPrefs.decPoints);
//		if (dispPrefs.mode!=disp_normal)
//		  fp_print_exp(strP+StrLen(strP),0);
//	}
	else if (fabs(value)>=1 && dispPrefs.mode==disp_eng) {
		/* Engineer mode selected */
		Int16 mydec = dispPrefs.decPoints - 1;

		for (ex=0;fabs(value)>=pow(BASE,3) || (ex % 3);ex++,value/=BASE)
		  ;
		if (fabs(value) >= 100.0)
			mydec -= 2;
		else if (fabs(value) >= 10.0)
			mydec -= 1;
		if (mydec < 0)
			mydec = 0;
		fp_print(strP,value,mydec);
		fp_print_exp(tmpstr,ex);
		StrCat(strP,tmpstr);
	}
	else if (fabs(value)<1.0 && dispPrefs.mode==disp_eng) {
		Int16 mydec = dispPrefs.decPoints - 1;
		/* Engineer mode selected */
		for (ex=0;fabs(value)<0.99999999 || (ex % 3);ex++,value*=BASE)
		  ;

		if (fabs(value) >= 100.0)
			mydec -= 2;
		else if (fabs(value) >= 10.0)
			mydec -= 1;
		if (mydec < 0)
			mydec = 0;
		fp_print(strP,value,mydec);
		fp_print_exp(tmpstr,-ex);
		StrCat(strP,tmpstr);
	}
	else if (dispPrefs.base == disp_binary || dispPrefs.base == disp_octal
		 || dispPrefs.base == disp_hexa) {
		if (fabs(value) > 1E14)
			StrCopy(strP,"Cannot display number.");
		else
			fp_print(strP,value,dispPrefs.decPoints);
	}
	else if (fabs(value)>SCI_LIMIT_MAX) {
		/* Normal/Scientific mode */
		for (ex=0;fabs(value)>=BASE;value/=BASE,ex++)
		  ;
		fp_print(strP,value,dispPrefs.decPoints);
		fp_print_exp(tmpstr,ex);
		StrCat(strP,tmpstr);
	}
	else if(fabs(value)<SCI_LIMIT_MIN && fabs(value)>0) {
		/* Normal/Scientific mode */
		for (ex=0;fabs(value)<0.99999999;value*=BASE,ex--)
		  ;
		fp_print(strP,value,dispPrefs.decPoints);
		fp_print_exp(tmpstr,ex);
		StrCat(strP,tmpstr);
	}
	else {
		/* No special formatting required */
		fp_print(strP,value,dispPrefs.decPoints);
	}       
}

/***********************************************************************
 *
 * FUNCTION:     fp_print_g_double
 *
 * DESCRIPTION:  Formats double numbers for output on the graph screen
 *               using a Fortran like %g format.
 *               The code is derived from the printDouble function in lispme:
 *               http://www.lispme.de/lispme/gcc/tech.html#fpio
 *
 * PARAMETERS:   s - output string
 *               value - value to be formatted
 *
 * RETURN:       nothing
 *
 ***********************************************************************/

/* Conversion constants */
static double pow1[] = {
        1e256, 1e128, 1e064,
        1e032, 1e016, 1e008,
        1e004, 1e002, 1e001
};

static double pow2[] = {
        1e-256, 1e-128, 1e-064,
        1e-032, 1e-016, 1e-008,
        1e-004, 1e-002, 1e-001
};

void fp_print_g_double(Char *s, double value, Int16 prec) PARSER;
void
fp_print_g_double(Char *s, double value, Int16 prec)
{
	FlpCompDouble fcd;
	double round_factor;
	short e, e1, i;
	double *pd, *pd1;
	char sign = '\0';
	short dec = 0;

	/* Check for reasonable value of prec. IEEE double. IEEE double
	 * precision has about 16 significant decimal digits */
	if (prec < 0 || prec > 16) prec = 7;

	/* Initialize round_factor */
	round_factor = 0.5;
	for (i = 0; i < prec; i++) round_factor /= 10.0;

	/* Check for NaN, +Inf, -Inf, 0 */
	fcd.d = value;
	if ((fcd.ul[0] & 0x7ff00000) == 0x7ff00000) {
		if (fcd.fdb.manH == 0 && fcd.fdb.manL == 0) {
			if (fcd.fdb.sign)
				StrCopy(s, "-Inf");
			else
				StrCopy(s, "Inf");
		}
		else {
			StrCopy(s, "NaN");
		}
		return;
	}
	if (FlpIsZero(fcd)) {
		StrCopy(s, "0");
		return;
	}

	/* Make positive and store sign */
	if (FlpGetSign(fcd)) {
		*s++ = '-';
		FlpSetPositive(fcd);
		value = fcd.d;
	}

	/* Compute round_factor */
	if ((unsigned) fcd.fdb.exp < 0x3ff) {
		/* round_factor for value < 1.0 */
		for (e = 1, e1 = 256, pd = pow1, pd1 = pow2; e1;
		     e1 >>= 1, ++pd, ++pd1) {
			if (*pd1 > fcd.d) {
				e += e1;
				fcd.d *= *pd;
				round_factor *= *pd1;
			}
		}
	}
	else {
		/* round_factor for value >= 1.0 */
		for (e = 0, e1 = 256, pd = pow1, pd1 = pow2; e1;
		     e1 >>= 1, ++pd, ++pd1) {
			if (*pd <= fcd.d) {
					e += e1;
					fcd.d *= *pd1;
					round_factor *= *pd;
			}
		}
		round_factor *= 10.0;
	}
	fcd.d = value + round_factor;

	if ((unsigned) fcd.fdb.exp < 0x3ff) {
		/* Build negative exponent */
		for (e = 1, e1 = 256, pd = pow1, pd1 = pow2; e1;
		     e1 >>= 1, ++pd, ++pd1) {
			if (*pd1 > fcd.d) {
				e += e1;
				fcd.d = fcd.d * *pd;
			}
		}
		fcd.d = fcd.d * 10.0;
		/* Do not print exponent for the -0.xxxx case */
		if (e <= 1) {
			*s++ = '0';
			*s++ = '.';
			dec = -1;
		}
		else
			sign = '-';
	}
	else {
		/* Build positive exponent */
		for (e = 0, e1 = 256, pd = pow1, pd1 = pow2; e1;
		     e1 >>= 1, ++pd, ++pd1) {
			if (*pd <= fcd.d) {
				e += e1;
				fcd.d = fcd.d * *pd1;
			}
		}
		if (e < prec)
			dec = e;
		else
			sign = '+';
	}

	/* Extract decimal digits of mantissa */
	for (i = 0; i < prec; ++i, --dec) {
		Int32 d = fcd.d;
		*s++ = d + '0';
		if (!dec)
			*s++ = '.';
		fcd.d = fcd.d - (double)d;
		fcd.d = fcd.d * 10.0;
	}

	/* Remove trailing zeros and decimal point */
	while (s[-1] == '0')
		*--s = '\0';
	if (s[-1] == '.')
		*--s = '\0';

	/* Append exponent */
	if (sign) {
		*s++ = 'E';
		*s++ = sign;
		StrIToA(s, e);
	}
	else
		*s = '\0';
}
