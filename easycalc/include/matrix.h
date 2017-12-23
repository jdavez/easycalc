/*
 *   $Id: matrix.h,v 1.10 2006/09/12 19:40:55 tvoverbe Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000,2001 Ondrej Palkovsky
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

#ifndef _MATRIX_H_
#define _MATRIX_H_

#define MATRIX_MAX  50

#define MATRIX_INPUT   2001
#define MATRIX_DET     2002
#define MATRIX_ECHELON 2003
#define MATRIX_IDENTITY 2004
#define MATRIX_QRQ 2005
#define MATRIX_QRR 2006
#define MATRIX_QRS 2007

CError matrix_input(Functype *func,CodeStack *stack) MATFUNC;
void matrix_delete(Matrix *m) MATFUNC;
Matrix * matrix_new(Int16 rows,Int16 cols) MATFUNC;
Matrix * matrix_dup(Matrix *m) MATFUNC;
CError matrix_oper(Functype *func,CodeStack *stack) MATFUNC;
CError matrix_oper_real(Functype *func,CodeStack *stack) MATFUNC;
CError matrix_func(Functype *func,CodeStack *stack) MATFUNC;
CError matrix_func2(Functype *func,CodeStack *stack) MATFUNC;
CError matrix_item(Functype *func, CodeStack *stack) MATFUNC;
CError  matrix_dim(Functype *func, CodeStack *stack) MATFUNC;
CError matrix_func3(Functype *func,CodeStack *stack) MATFUNC;

#endif
