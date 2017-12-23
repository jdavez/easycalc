/*
 *   $Id: cmatrix.h,v 1.10 2006/09/22 15:06:32 cluny Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000,2001,2002 Ondrej Palkovsky
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

#ifndef _CMATRIX_H_
#define _CMATRIX_H_

#define CMATRIX_INPUT   2101
#define CMATRIX_DET     2102
#define CMATRIX_ECHELON 2103
#define CMATRIX_IDENTITY 2104
#define CMATRIX_QRQ 2105
#define CMATRIX_QRR 2106
#define CMATRIX_QRS 2107

CError cmatrix_input(Functype *func,CodeStack *stack) MATFUNC;
CMatrix * cmatrix_new(Int16 rows,Int16 cols) MATFUNC;
CMatrix * cmatrix_dup(CMatrix *m) MATFUNC;
void cmatrix_delete(CMatrix *m) MATFUNC;
Matrix * cmatrix_to_matrix(CMatrix *cm) MATFUNC;
CError cmatrix_oper(Functype *func,CodeStack *stack) MATFUNC;
CMatrix * matrix_to_cmatrix(Matrix *m) MATFUNC;
CError cmatrix_oper_cplx(Functype *func,CodeStack *stack) MATFUNC;
CError cmatrix_func2(Functype *func,CodeStack *stack) MATFUNC;
CError cmatrix_func(Functype *func,CodeStack *stack) MATFUNC;
CError cmatrix_dim(Functype *func, CodeStack *stack) MATFUNC;
CError cmatrix_item(Functype *func, CodeStack *stack) MATFUNC;

#endif
