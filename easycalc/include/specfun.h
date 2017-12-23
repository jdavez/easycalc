/*   
 *   $Id: specfun.h,v 1.3 2006/09/12 19:40:55 tvoverbe Exp $
 * 
 *   Scientific Calculator for Palms.
 *   This file Copyright (C) 2000 Rafael Sevilla
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
 *  You can contact me at 'dido@pacific.net.ph'.
 *
 */

#ifndef _SPECFUN_H
#define _SPECFUN_H

#include "segment.h"

CError math_gambeta(Functype *func, CodeStack *stack) SPECFUN;
CError math_bessels(Functype *func, CodeStack *stack) SPECFUN;
CError math_elliptic(Functype *func, CodeStack *stack) SPECFUN;
CError math_hypergeo(Functype *func, CodeStack *stack) SPECFUN;
CError math_orthpol(Functype *func, CodeStack *stack) SPECFUN;
double polevl(double x, double *coef, int N ) SPECFUN;

#define EPSILON 1e-10

#endif

