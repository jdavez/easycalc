/*
 *   $Id: guess.c,v 1.8 2007/12/23 13:53:06 cluny Exp $
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

#include <PalmOS.h>
#include <StringMgr.h>

#include "segment.h"
#include "MathLib.h"
#include "stack.h"
#include "funcs.h"
#include "konvert.h"
#include "complex.h"
#include "display.h"
#include "defuns.h"
#include "mathem.h"

#define MAX_DIV_F  1024
#define MAX_FORM_F 128
#define MAX_DIV_CONST 4
#define MAX_DIV_CONST_E 12
#define GUESS_MAX 131072
#define GUESS_FACTORS_MAX 1000000000


#define M_PI     3.14159265358979323846
#define M_E      2.7182818284590452354

static Boolean is_int(double t1) MLIB;
static Boolean
is_int(double t1)
{
	if (IS_ZERO(t1-round(t1)))
	  return true;
	return false;
}

static Boolean guess_pot(double t1,Int32 *cte_lo,Int32 *cte_up) MLIB;
static Boolean
guess_pot(double t1,Int32 *cte_lo,Int32 *cte_up)
{
	Int16 i,j;
	double tmp;

	if (t1<=0) t1=-t1;

	for (i=2;i<MAX_FORM_F;i++) {
	  tmp=t1/i;
	  for (j=2;i<=tmp;j++) {
		tmp/=i;
		if (tmp==1) { 
			if (t1<=0) *cte_lo=-i;
			else *cte_lo=i;
			*cte_up=j;
			return true;
		}
	  }
	}
	return false;
}

static Boolean guess_div(double t1,Int32 *upper,Int32 *lower,Int16 maxlev) MLIB;
static Boolean
guess_div(double t1,Int32 *upper,Int32 *lower,Int16 maxlev)
{            
	Int16 i;
	double tmp;

	for (i=1;i<maxlev;i++) {
		tmp=i*t1;
		if (is_int(tmp)) {
			*upper=round(tmp);
			*lower=i;
			return true;
		}
	}	
	return false;
}

static Boolean guess_e_x(double t1,Int32 *upper,Int32 *lower,Int32 *cte_up,Int32 *cte_lo) MLIB;
static Boolean
guess_e_x(double t1,Int32 *upper,Int32 *lower,Int32 *cte_up,Int32 *cte_lo)
{
	Int16 i;
	double tmp;

	if (t1<=0) t1=-t1;

	/* 1*e^(x/y) */
	tmp=log(t1);
	if (guess_div(tmp,upper,lower,MAX_FORM_F)){
	   *cte_up=1;
	   *cte_lo=1;
	   return true;
	}

	/* x*e^(-y) */
	tmp=t1;
	for (i=1;i<MAX_DIV_CONST_E;i++) {
		tmp*=M_E;
		if (guess_div(tmp,cte_up,cte_lo,MAX_FORM_F)) {
			*upper=-i;
			*lower=1;
			return true;
		}
	}

	/* x*e^(y) */
	tmp=t1;
	for (i=1;i<MAX_DIV_CONST_E;i++) {
		tmp/=M_E;
		if (guess_div(tmp,cte_up,cte_lo,MAX_FORM_F)) {
			*upper=i;
			*lower=1;
			if(*cte_up==0) return false;
			return true;
		}
	}
	return false;
}

static Boolean guess_mult_pi(double t1,Int32 *upper,Int32 *lower,Int16 *power) MLIB;
static Boolean
guess_mult_pi(double t1,Int32 *upper,Int32 *lower,Int16 *power)
{
	Int16 i;
	double tmp;
	
	/* First try dividing it */
	tmp=t1;
	for (i=1;i<MAX_DIV_CONST;i++) {
		tmp/=M_PI;
		if (guess_div(tmp,upper,lower,MAX_FORM_F)) {
			*power=i;
			if(*upper==0) return false;
			return true;
		}
	}

	/* multiplying it */
	tmp=t1;
	for (i=1;i<MAX_DIV_CONST;i++) {
		tmp*=M_PI;
		if (guess_div(tmp,upper,lower,MAX_FORM_F)) {
			*power=-i;
			return true;
		}
	}

	return false;
}

static Boolean guess_log_x(double t1,Int32 *upper,Int32 *lower) MLIB;
static Boolean
guess_log_x(double t1,Int32 *upper,Int32 *lower) 
{
	if(t1>26) return false;
	t1=exp(t1);
	if (guess_div(t1,upper,lower,MAX_FORM_F) && *upper!=0) return true;
	return false;
}

static Boolean guess_sqrt(double t1,Int32 *cte_up,Int32 *cte_lo,Int32 *power) MLIB;
static Boolean
guess_sqrt(double t1,Int32 *cte_up,Int32 *cte_lo,Int32 *power)
{
	Int16 i;
	double tmp;

	if (t1<=0.0) t1=-t1;	

	for (i=2;i<4;i++) {
		tmp=t1/sqrt(i);
		if (guess_div(tmp,cte_up,cte_lo,MAX_FORM_F)) {
			*power=i;
			return true;
		}
	}
	return false;
}

static Boolean guess_pow1(double t1,Int32 *upper,Int32 *lower,Int16 *power) MLIB;
static Boolean
guess_pow1(double t1,Int32 *upper,Int32 *lower,Int16 *power)
{
	Int16 i;
	double tmp;
	
	if (t1<=0.0) t1=-t1;
	
	tmp=t1;
	for (i=2;i<MAX_DIV_CONST_E;i++) {
		tmp*=t1;
		if (guess_div(tmp,upper,lower,MAX_FORM_F)) {
			*power=i;
			if (*upper>GUESS_MAX) return false;
			return true;
		}
	}
	return false;
}

static Boolean guess_real(double t1,char *result) MLIB;
static Boolean
guess_real(double t1,char *result)
{
	Int32 upper,lower,cte_up,cte_lo;
	Int16 power;
	
	if (!finite(t1)){
		if (isnan(t1)){
			StrPrintF(result,"NaN");
			return true;
		}
		if(t1<0.0) StrCopy(result++,"-"); else StrCopy(result++,"+");
		StrPrintF(result,"Inf");
		return true;
	}

	if (IS_ZERO(t1)) {
		StrCopy(result,"0");
		return true;
	} 

	if (is_int(t1))
	if (guess_pot(round(t1),&cte_lo,&cte_up)){
		if(t1<0.0) StrCopy(result++,"-");
		StrPrintF(result,"%ld^%ld",cte_lo,cte_up);
	      return true;
	};

	if (fabs(t1)>GUESS_FACTORS_MAX)
		return false;
	
	if (is_int(t1))  {
		// print factors
		Int32 value = (Int32)round(t1);
		Int32 factors[32];
		UInt8 fcount, i,k=0;
		char* tmp = result;
		
		fcount = factorize(value, factors);
		
		for(i=0; i<fcount; i++) {
			if (i < fcount - 1 && factors[i] == factors[i+1]) 
				k++;
			else {
			  StrPrintF(tmp,"%ld*",factors[i]);
			  while(*tmp) tmp++;
			  if (k) StrPrintF(--tmp,"^%d*",k+1);
			  while(*tmp) tmp++;
			  k=0;
			}
		}
		*--tmp = 0;
		return true; 
	}
	
	if (fabs(t1)>GUESS_MAX)
	  return false;

	if (guess_div(t1,&upper,&lower,MAX_DIV_F)) {
		StrPrintF(result,"%ld/%ld",upper,lower);
		return true;
	}
      if (is_int(1/t1))
	if (guess_pot(round(1/t1),&cte_lo,&cte_up)){
		if(t1<0.0) StrCopy(result++,"-");
		StrPrintF(result,"%ld^-%ld",cte_lo,cte_up);
		return true;
	};

	if (guess_sqrt(t1,&cte_up,&cte_lo,&upper)) {
		if(t1<0.0) StrCopy(result++,"-");
		if (cte_up!=1){
		   StrPrintF(result,"%ld",cte_up);
		   result=result+StrLen(result);	
		}
		if (cte_lo==1)
                   StrPrintF(result,"sqrt(%ld)",upper);
		else
		  StrPrintF(result,"sqrt(%ld)/%ld",upper,cte_lo);
		return true;
	}

	if (guess_mult_pi(t1,&upper,&lower,&power)) {
		if (upper<0){ StrCopy(result++,"-"); upper=-upper;}
		if (power>0) {
			if (upper>1){
			  StrPrintF(result,"%ld",upper);
			  result+=StrLen(result);}
			if (power==1)
			  StrPrintF(result,"pi");
			else
			  StrPrintF(result,"pi^%d",power);
			result+=StrLen(result);
			if (lower>1)
			  StrPrintF(result,"/%ld",lower);		
		}else{
			StrPrintF(result,"%ld/",upper);
			result+=StrLen(result);
			if (lower>1){
			  StrPrintF(result,"%ld",lower);
			  result+=StrLen(result);}
			if (power<-1)
			  StrPrintF(result,"pi^%d",-power);
			else
			  StrCat(result,"pi");
		}
		return true;		
	}          

	if (guess_e_x(t1,&upper,&lower,&cte_up,&cte_lo)) {
		if(t1<0.0) StrCopy(result++,"-");
		if(cte_lo!=1){
		   StrPrintF(result,"%ld/%ld*",cte_up,cte_lo);
		   result=result+StrLen(result);
		} else 
		if(cte_up!=1){
		   StrPrintF(result,"%ld",cte_up);
		   result=result+StrLen(result);
		}

		if (lower==1 && upper>0) 
		  StrPrintF(result,"e^%ld",upper);
		else if (lower==1)
		  StrPrintF(result,"e^%ld",upper);
		else
		  StrPrintF(result,"e^(%ld/%ld)",upper,lower);
		return true;
	}

	if (guess_log_x(t1,&upper,&lower)) {
		if (lower==1)
		  StrPrintF(result,"ln(%ld)",upper);
		else
		  StrPrintF(result,"ln(%ld/%ld)",upper,lower);
		return true;
	}

	if (guess_pow1(t1,&upper,&lower,&power)) {
		if (upper && lower) {
			if(t1<0.0) StrCopy(result++,"-");
			if (power==2 && lower==1)
	  		StrPrintF(result,"sqrt(%ld)",upper);
			else if (power==2)
			  StrPrintF(result,"sqrt(%ld/%ld)",upper,lower);
			else if (lower==1) 
			  StrPrintF(result,"%ld^(1/%d)",upper,power);
			else 
	 		 StrPrintF(result,"(%ld/%ld)^(1/%d)",upper,lower,power);
			return true;
		}
	}

	return false;	
}

static Boolean
guess_complex(Complex arg,char *result) MLIB;
static Boolean
guess_complex(Complex arg,char *result)
{
	Complex_gon arg_gon;
	Int32 upper,lower,rad;
	Int16 i;
	double tmp;
		
	if (!finite(arg.real) || !finite(arg.imag))
	  return false;

	if (fabs(arg.real)>100000 || fabs(arg.imag)>100000)
	  return false;
	
	arg_gon = cplx_to_gon(arg);
	
	if (!guess_real(arg_gon.r,result)) StrCopy(result,display_real(arg_gon.r));
	if (is_int(arg_gon.r) && round(arg_gon.r)==1) StrCopy(result,"e^(");
	  else StrCat(result,"*e^(");
	result=result+StrLen(result);

	tmp = arg_gon.angle/M_PI;	
	if (is_int(tmp)){
	   StrPrintF(result,"%ldpi*i)",(Int32)round(tmp));
	   return true;
	}
	if (guess_div(tmp,&upper,&lower,MAX_FORM_F)){
	  if (upper<0){ StrPrintF(result++,"-"); upper=-upper;}
	  if (upper==1) StrPrintF(result,"pi/%ld*i)",lower);
	  else StrPrintF(result,"%ldpi/%ld*i)",upper,lower);
	  return true;
	}

	tmp=arg_gon.angle-MAX_DIV_CONST*2*M_PI;
	for(i=0;i<2*MAX_DIV_CONST;i++){
	  tmp+=2*M_PI;
	  if (is_int(tmp)){
	  	if (tmp<0){ StrPrintF(result++,"-"); tmp=-tmp;}
	  	if ((Int32)round(tmp)==1) StrPrintF(result,"i)");
		else StrPrintF(result,"%ldi)",(Int32)round(tmp));
		return true;
	  }
	  if (guess_div(tmp,&upper,&lower,MAX_FORM_F)){
		if (upper<0){ StrPrintF(result++,"-"); upper=-upper;}
		if (upper==1) StrPrintF(result,"i/%ld)",lower);
		else StrPrintF(result,"%ldi/%ld)",upper,lower);
		return true;
	  }	
	  if (guess_sqrt(tmp,&upper,&lower,&rad)) {
		if(tmp<0.0) StrPrintF(result++,"-");
		if(upper!=1){
		   StrPrintF(result,"%ld",upper);
		   result=result+StrLen(result);	
		}
		if(lower==1)
               StrPrintF(result,"sqrt(%ld)*i)",rad);
		else
		   StrPrintF(result,"sqrt(%ld)/%ld*i)",rad,lower);
		return true;
	  }
	}

	return false;
}

char * guess(Trpn item) MLIB;
char *
guess(Trpn item)
{
	char *result;
	Boolean guessed;
	
	if (item.type == real) {
		result = MemPtrNew(MAX_FP_NUMBER+1);
		guessed = guess_real(item.u.realval,result);
		if (!guessed) {
			MemPtrFree(result);
			return NULL;
		}
		return result;
	}
	else if (item.type == complex) {
		result = MemPtrNew(MAX_FP_NUMBER*3);
		if (IS_ZERO(item.u.cplxval->imag)){
		   if (guess_real(item.u.cplxval->real,result))
			return result;
		   else {
			MemPtrFree(result);
			return NULL;}
		}
	
		if (IS_ZERO(item.u.cplxval->real))
		  if (guess_real(item.u.cplxval->imag,result)){
		    StrPrintF(result+StrLen(result),"*i");
		    return result;
		};

		guessed = guess_complex(*item.u.cplxval,result);
		if (!guessed) {				
			MemPtrFree(result);
			return NULL;
		}
		return result;
	}
	return NULL;
}
