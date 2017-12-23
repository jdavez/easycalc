/*
 *   $Id: mtxedit.h,v 1.2 2009/12/15 21:36:54 mapibid Exp $
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

#ifndef _MTXEDIT_H_
#define _MTXEDIT_H_

#define MATRIX_ID 101

#ifndef _MTXEDIT_C_
extern TCHAR mtxedit_matrixName[MAX_FUNCNAME+1];
#endif

void mtxedit_init(Boolean showAns);
void mtxedit_select(void);
void mtxedit_load_matrix(int *nb_rows, int *nb_cols);
void mtxedit_tbl_load(void);
void mtxedit_redim(int rows, int cols);
CError mtxedit_get_item(Complex *cplx, int row, int col);
CError mtxedit_replace_item(Complex *cplx, int row, int col);
int mtxedit_item_length(int row, int col);
void mtxedit_save_matrix(void);
void mtxedit_new(int rows, int cols);
void mtxedit_destroy(void);

#endif
