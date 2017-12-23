/*
 *   $Id: history.h,v 1.3 2009/10/17 13:48:34 mapibid Exp $
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

#ifndef _HISTORY_H_
#define _HISTORY_H_

#define HISTORY_RECORDS 30

typedef struct {
	Boolean isrpn;
	Int16 xxxpad;
	union {
		dbStackItem item;
		TCHAR text[1];
	}u;
}tHistory;

CError history_command(Functype *func,CodeStack *stack) MLIB;
Int16 history_close(void) MLIB;
Int16 history_open(void) MLIB;
void history_shrink(Int16 count) MLIB;
void history_add_line(TCHAR *line) MLIB;
void history_add_item(Trpn item) MLIB;
Boolean history_isrpn(UInt16 num) MLIB;
UInt16 history_total(void) MLIB;
TCHAR * history_get_line(UInt16 num) MLIB;
Trpn  history_get_item(UInt16 num) MLIB;

#endif
