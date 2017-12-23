/*
 * $Id: finance.h,v 1.1 2011/02/28 22:07:18 mapibid Exp $
 *
 * Scientific Calculator for Palms.
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

#ifndef _FINANCE_H_
#define _FINANCE_H_

#define FIN_FV   0
#define FIN_PV   1
#define FIN_PMT  2
#define FIN_N    3
#define FIN_I    4
#define FIN_PYR  5
#define FIN_TPMT 6
#define FIN_COST 7

typedef struct {
    UInt16 ID;
    TCHAR *var;
    TCHAR *description;
    TCHAR *begStr;
    TCHAR *endStr;
} t_finDef;

#ifndef _FINANCE_C_
extern TdispPrefs finDPrefs;
extern t_finDef inpFinForm[];
extern t_finDef outFinForm[2];
#endif

void fin_update_fields(void);
void fin_compute(int pos, int begin);

#endif
