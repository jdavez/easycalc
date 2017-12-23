/*
 *   $Id: lstedit.h,v 1.2 2009/11/16 22:15:12 mapibid Exp $
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

#ifndef _LSTEDIT_H_
#define _LSTEDIT_H_

#include "core/mlib/konvert.h"
#define LIST_COUNT   3
#define LIST1_ID 1
#define LIST2_ID 2
#define LIST3_ID 3

typedef struct {
    TCHAR list[LIST_COUNT][MAX_FUNCNAME+1];
} tlistPrefs;


void lstedit_init(Boolean showAns);
void lstedit_load_lists(void);
void lstedit_tbl_load(void);
void lstedit_init_firstitem(int controlId, Complex *cplx);
CError lstedit_append_item(int controlId, Complex *cplx);
CError lstedit_insert_item(int controlId, Complex *cplx, int row);
CError lstedit_get_item(int controlId, Complex *cplx, int row);
CError lstedit_replace_item(int controlId, Complex *cplx, int row);
int lstedit_item_length(int controlId, int row);
int lstedit_list_length(int controlId);
CError lstedit_delete_item(int controlId, int row);
void lstedit_reselect(Int16 controlId);
void lstedit_tbl_deselect(void);
void lstedit_tbl_scroll(Int16 delta);
CError lstedit_save_lists(void);
void lstedit_destroy(void);
//Boolean ListEditHandleEvent(EventPtr event);
//void lstedit_popup_ans(void);


#endif
