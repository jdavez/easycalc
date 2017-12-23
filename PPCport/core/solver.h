/*
 *   $Id: solver.h,v 1.1 2011/02/28 22:07:18 mapibid Exp $
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

#ifndef _SOLVER_H_
#define _SOLVER_H_

#define SOLVER_ID 201

#define SLV_OFF       1
#define SLV_SOLVE     2
#define SLV_CALCULATE 3
#define SLV_ON        4

#ifndef _SOLVER_C_
extern DmOpenRef slv_gDB;

extern Int16 slv_selectedWorksheet;
extern double worksheet_min;
extern double worksheet_max;
extern double worksheet_prec;
TCHAR worksheet_title[];
extern TCHAR *worksheet_note;
#endif

Int16 slv_db_open(void);
Int16 slv_db_close(void);
void slv_init(void);
void slv_close(void);
Int16 slv_save_worksheet(void);
void slv_select_worksheet(void);
Int16 slv_new_worksheet(TCHAR *name);
void slv_update_worksheet(void);
void slv_destroy_varlist(void);
Int16 slv_init_varlist(void);
CError slv_solve(Int16 selection);
void slv_calculate(void);
void slv_create_initial_note(void);
TCHAR *slv_getVar(int i);
void slv_update_help(int i);
Boolean slv_comp_field(TCHAR *text, double *value);

Boolean slv_memo_import(TCHAR *text);
Boolean slv_export_memo(FILE *f, TCHAR *separator);

#endif
