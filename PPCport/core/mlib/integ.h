/*   
 *   $Id: integ.h,v 1.1 2009/10/17 13:48:34 mapibid Exp $
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

#ifndef _INTEG_H_
#define _INTEG_H_

/* Precision for solving functions */
#define DEFAULT_ERROR (1E-8)
#define DEFAULT_ROMBERG  6
#define MAX_ITER 2000

CError integ_fsolve(Functype *func,CodeStack *stack) NEWFUNC;
CError integ_fsimps(Functype *func,CodeStack *stack) NEWFUNC;
CError integ_fdydx(Functype *func,CodeStack *stack) NEWFUNC;
double integ_simps(double min, double max,CodeStack *fcstack,
		   double error,CodeStack *argarr) NEWFUNC;
double integ_zero(double min, double max, double value, CodeStack *fcstack, 
		  double error,Int16 funcnum,CodeStack *argarr) NEWFUNC;
double integ_derive1(double x,CodeStack *fcstack,double error,
		     CodeStack *argarr) NEWFUNC;
double integ_derive2(double x,CodeStack *fcstack,double error,
		     CodeStack *argarr) NEWFUNC;
double integ_romberg(double min,double max,CodeStack *fcstack,Int16 n,
		     CodeStack *argarr) NEWFUNC;
CError integ_fromberg(Functype *func,CodeStack *stack) NEWFUNC;
double integ_intersect(double min,double max, CodeStack *f1, CodeStack *f2,
		       double error,CodeStack *argarr) NEWFUNC;
CError integ_fintersect(Functype *func,CodeStack *stack) NEWFUNC;

#endif
