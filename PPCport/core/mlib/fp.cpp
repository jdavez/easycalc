/*
 *   $Id: fp.cpp,v 1.7 2011/02/28 22:07:47 mapibid Exp $
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

#include "stdafx.h"

#ifdef UNIX
#define StrChr(x,y) strchr(x,y)
#define StrCopy(x,y)    strcpy(x,y)
#define StrLen(x)   strlen(x)

#include <stdlib.h>
#include <string.h>
#else
#include "compat/PalmOS.h"
//#include <StringMgr.h>
#include "compat/MathLib.h"
#endif
#include "defuns.h"
#include "display.h"
#include "fp.h"

static double SCI_LIMIT_MAX;   /* Above this number show it as xEy */
static double SCI_LIMIT_MIN;   /* Under this number show as xE-y */
//static double DISP_MIN_LIMIT = 1E-15; /* Show numbers as sin(pi) as 1E-16,
//                     it's 0 */
//static double DISP_MAX_LIMIT = 1E300; /* Don't show bigger numbers, it
//                     * makes problems */
static double BASE = 10.0;
TCHAR flPointChar = _T('.');
TCHAR flSpaceChar = _T(' ');

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
        flSpaceChar=_T(',');
        flPointChar=_T('.');
        break;
     case nfApostrophePeriod:
        flSpaceChar=_T('\'');
        flPointChar=_T('.');
        break;
     case nfPeriodComma:
        flSpaceChar=_T('.');
        flPointChar=_T(',');
        break;
     case nfSpaceComma:
        flSpaceChar=_T(' ');
        flPointChar=_T(',');
        break;
     case nfApostropheComma:
     default:
        flSpaceChar=_T('\'');
        flPointChar=_T(',');
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
//      SCI_LIMIT_MAX=1E10;
        SCI_LIMIT_MAX=pow(BASE,10);
//      SCI_LIMIT_MIN=1E-5;
        SCI_LIMIT_MIN=pow(BASE,-5);
        break;
     case disp_sci:
     case disp_eng:
        SCI_LIMIT_MAX=1.00;
        SCI_LIMIT_MIN=1.01;
        break;
    }
//  if (dispPrefs.reducePrecision) {
//      DISP_MIN_LIMIT = 1E-15;
//      REDUCE_PREC = 1E-6;
//  } else {
//      DISP_MIN_LIMIT = 1E-100;
//      REDUCE_PREC = 1E-90;
//  }
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
    fp_reread_prefs();
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
static void cvt_fltoa(TCHAR *strP,double value) PARSER;
static void
cvt_fltoa(TCHAR *strP,double value)
{
//    TCHAR *cP;
    TCHAR tmpstr[MAX_FP_NUMBER+3];
    Int16 i,ichar;

    if (value==0.0) {
        StrCopy(strP,_T("0"));
        return;
    }
    /* We convert the number backwards */
// Mapi: speeding things up.
//    for (i=MAX_FP_NUMBER-2;i>=0;i--) {
    for (i=MAX_FP_NUMBER-2 ; (i>=0)&&(value!=0.0) ; i--) {
        ichar = (Int16)fmod(value,BASE) + _T('0');
        tmpstr[i] = ichar>_T('9')?ichar-_T('9')-1+_T('A'):ichar;
        value /= BASE;
        value=trunc(value);
    }

    tmpstr[MAX_FP_NUMBER-1] = _T('\0');

//    for( cP = tmpstr; *cP && (*cP == _T('0')); )
//      ++cP;
//
//    StrCopy(strP,cP);
    StrCopy(strP,tmpstr+i+1);
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
fp_print(TCHAR *strP,double value,Int16 myfrac) PARSER;
static void
fp_print(TCHAR *strP,double value,Int16 myfrac)
{
    Int16 i;
    double ipart;
    double fpart;
    double limit;
    TCHAR str[MAX_FP_NUMBER*2];

    if (isinf(value)) {
        if (isinf(value)>0)
          StrCopy(strP,_T("+Inf"));
        else
          StrCopy(strP,_T("-Inf"));
        return;
    }
    if (!finite(value)) {
        StrCopy(strP,_T("NaN"));
        return;
    }

    if( value < 0.0 ) {
        *strP++ = _T('-');
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
        fpart = 0.0;    /* overflowed */
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
                *strP++ = _T('0');
            }
            *strP++ = _T('\0');
        }
        if (dispPrefs.stripZeros) {
            /* remove trailing 0's */
            for (i = StrLen(str)-1;i>=0 && str[i]==_T('0');i--) {
                str[i]=_T('\0');
                myfrac--;
            }
        }
        if (StrLen(str)>0) {
            *strP++ = flPointChar;
            for(i=StrLen(str); i<myfrac;i++ )
                *strP++ = _T('0');  /* need some padding at the beginning */
            StrCopy(strP,str);  /* and now the value */
        }
    }
}

static const TCHAR expNames[] = {_T('f'),_T('p'),_T('n'),_T('u'),_T('m'),0,_T('k'),_T('M'),_T('G'),_T('T')};

static void fp_print_exp(TCHAR *str,Int16 expon) PARSER;
static void
fp_print_exp(TCHAR *str,Int16 expon)
{
    Int16 i;

    if (dispPrefs.base == disp_decimal) {
        if (dispPrefs.mode == disp_eng && dispPrefs.cvtUnits) {
            i = expon/3 + 5;
            if (i<0 || i>9)
              StrPrintF(str,_T("E%d"),expon);
            else
              StrPrintF(str,_T("%c"),expNames[i]);
        }
        else
          StrPrintF(str,_T("E%d"),expon);
    }
    else
        StrPrintF(str, _T("*%d^%d"),(Int16)dispPrefs.base, expon);
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
void fp_print_double (TCHAR *strP,double value) PARSER;
void fp_print_double (TCHAR *strP, double value) {
    Int16 ex;
    TCHAR tmpstr[MAX_FP_NUMBER*2];


    if (!finite(value)) {
        /* Handle the NaN, +Inf & -Inf cases */
        fp_print(strP,value,dispPrefs.decPoints);
    }
//  else if (fabs(value)>DISP_MAX_LIMIT) {
//      StrCopy(strP,_T("It's too big"));
//  }
//  else if (fabs(value)<DISP_MIN_LIMIT) {
//      /* Don't show too small numbers */
//      fp_print(strP,0,dispPrefs.decPoints);
//      if (dispPrefs.mode!=disp_normal)
//        fp_print_exp(strP+StrLen(strP),0);
//  }
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
        // Mapi: this loops at infinity when value = 0.0 ! Corrected.
        //for (ex=0;fabs(value)<0.99999999 || (ex % 3);ex++,value*=BASE)
        ex = 0;
        if (!FlpIsZero(value))
            while ((fabs(value ) < 0.999999999999999) || (ex % 3)) {
                ex++;
                value *= BASE;
            }

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
            StrCopy(strP,_T("Cannot display number."));
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
fp_print_g_double(Char *s, double value, Int16 prec) {
    FlpCompDouble fcd;
    double round_factor;
    int e, e1, i;
    double *pd, *pd1;
    TCHAR sign = _T('\0'), *start = s;
    int dec = 0;
    int size = sizeof(double);
    bool is_negative = false;

    /* Check for reasonable value of prec. IEEE double. IEEE double
     * precision has about 16 significant decimal digits */
    if (prec < 0 || prec > 16)
        prec = 7;

    /* Initialize round_factor */
    round_factor = 0.5;
    for (i=0 ; i<prec ; i++)
        round_factor /= 10.0;

    /* Check for NaN, +Inf, -Inf, 0 */
    fcd.d = value;
    if ((fcd.ul[__HIX] & 0x7ff00000) == 0x7ff00000) {
        if ((fcd.fdb.manH == 0) && (fcd.fdb.manL == 0)) {
            if (fcd.fdb.sign)
                StrCopy(s, _T("-Inf"));
            else
                StrCopy(s, _T("Inf"));
        }
        else {
            StrCopy(s, _T("NaN"));
        }
        return;
    }
    if (FlpIsZero(fcd)) {
        StrCopy(s, _T("0"));
        return;
    }

    /* Make positive and store sign */
    if (FlpGetSign(fcd)) {
        *s++ = _T('-');
        is_negative = true;
        FlpSetPositive(fcd);
        value = fcd.d;
        prec--; // Keep one char for '-', so this decreases precision
    }

    /* Compute round_factor */
    if ((unsigned) fcd.fdb.exp < 0x3ff) {
        /* round_factor for value < 1.0 */
        for (e=1, e1=256, pd=pow1, pd1=pow2 ;
             e1 ;
             e1>>=1, ++pd, ++pd1) {
            if (*pd1 > fcd.d) {
                e += e1;
                fcd.d *= *pd;
                round_factor *= *pd1;
            }
        }
    }
    else {
        /* round_factor for value >= 1.0 */
        for (e=0, e1=256, pd=pow1, pd1=pow2 ;
             e1 ;
             e1>>=1, ++pd, ++pd1) {
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
        for (e=1, e1=256, pd=pow1, pd1=pow2 ;
             e1 ;
             e1>>=1, ++pd, ++pd1) {
            if (*pd1 > fcd.d) {
                e += e1;
                fcd.d = fcd.d * *pd;
            }
        }
        fcd.d = fcd.d * 10.0;
        /* Do not print exponent for the -0.xxxx case */
        if (e <= 1) {
            *s++ = _T('0');
            *s++ = _T('.');
            dec = -1;
        }
        else
            sign = _T('-');
    }
    else {
        /* Build positive exponent */
        for (e=0, e1=256, pd=pow1, pd1=pow2 ;
             e1 ;
             e1>>=1, ++pd, ++pd1) {
            if (*pd <= fcd.d) {
                e += e1;
                fcd.d = fcd.d * *pd1;
            }
        }
        if (e < prec)
            dec = e;
        else
            sign = _T('+');
    }

    /* Extract decimal digits of mantissa */
    for (i=0 ; i< prec ; ++i, --dec) {
        Int32 d = (Int32) (fcd.d);
        *s++ = d + _T('0');
        if (!dec)
            *s++ = _T('.');
        fcd.d = fcd.d - (double)d;
        fcd.d = fcd.d * 10.0;
    }

    /* Remove trailing zeros and decimal point */
    while (s[-1] == _T('0'))
        s--;
    if (s[-1] == _T('.')) {
        s--;
        dec = 0;
    } else {
        dec = 1;
    }

    // New function added to fit within text room: adjust precision
    // if there is an exponent, size ...
    /* Build exponent */
    TCHAR expstr[16];
    int explen = 0;
    if (sign!= _T('\0')) {
        expstr[0] = _T('E');
        expstr[1] = sign;
        StrIToA(expstr+2, e);
        explen = _tcslen(expstr);
    } else
        expstr[0] = _T('\0');
    /* Make room for the exponent */
    int len = s - start; // Note: this is returning the count of TCHARS, whatever size it is !
    int minlen = 1;
    if (is_negative) { // Keep '-' + 1 digit at least
        minlen = 2;
        prec++; // Restore back original prec value, now we include '-' in the length
    }
    while ((len > minlen) && (len+explen > prec)) {
        if ((--s)[-1] == _T('.')) { // Removing '.'
            s--;
            len--;
            dec = 0;
        }
        len--;
    }
    /* Add exponent */
    _tcscpy(s, expstr);
}
