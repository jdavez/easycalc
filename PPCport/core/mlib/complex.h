/*   
 *   $Id: complex.h,v 1.1 2009/10/17 13:48:34 mapibid Exp $
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

#ifndef _COMPLEX_H_
#define _COMPLEX_H_

#define cplx_abs(x) (hypot((x).real,(x).imag))

Complex_gon cplx_to_gon(Complex x) MLIB;
CError cplx_oper(Functype *func,CodeStack *stack) MLIB;
CError cplx_math(Functype *func,CodeStack *stack) MLIB;
CError cplx_round(Functype *func,CodeStack *stack) MLIB;
Complex cplx_add(Complex arg1,Complex arg2) MLIB;
Complex cplx_multiply(Complex arg1,Complex arg2) MLIB;
Complex cplx_sub(Complex arg1,Complex arg2) MLIB;
Complex cplx_div(Complex arg1,Complex arg2) MLIB;
Complex cplx_sqrt(Complex arg) MLIB;
Complex cplx_exp(Complex carg) MLIB;

#endif
