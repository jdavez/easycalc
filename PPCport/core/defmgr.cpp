/*
 *   $Id: defmgr.cpp,v 1.4 2009/12/24 16:35:58 mapibid Exp $
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

#include "stdafx.h"

#include "compat/PalmOS.h"
//#include <WinGlue.h>
#include "konvert.h"
#include "calcDB.h"
#include "defmgr.h"
#include "defuns.h"
#include "display.h"
#include "stack.h"
//#include "calc.h"
//#include "calcrsc.h"
#include "varmgr.h"

#include "core/core_display.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static dbList *varItems=NULL;
static TCHAR HORIZ_ELLIPSIS;

#define VARNAME_WIDTH  42

/***********************************************************************
 *
 * FUNCTION:     def_draw_item
 * 
 * DESCRIPTION:  Custom drawing function for list, it draws the line
 *               as a table of 2 item, separated by space,
 *               computes items on-the-fly, because it is faster
 *               than allocating the whole thing in advance 
 *               and consumes less memory
 *
 * PARAMETERS:   read PalmOS Reference
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void def_draw_item(int itemNum, TCHAR **itemText, Skin *skin, void *hWnd) IFACE;
static void def_draw_item(int itemNum, TCHAR **itemText, Skin *skin, void *hWnd) {
	TCHAR *text, *deftext;	
	CError err;
	Trpn item;
	
	if (varItems->type[itemNum] == function) {
		text = (TCHAR *) MemPtrNew((MAX_FUNCNAME+10)*sizeof(TCHAR));
		StrCopy(text, itemText[itemNum]);
		StrCat(text, _T("()"));
		
		err = db_func_description(varItems->list[itemNum], &deftext, NULL);
		if (err) 
		    deftext = print_error(err);

        skin->varDefList(text, deftext, itemNum, hWnd);
		MemPtrFree(text);
        if (!err)
            MemPtrFree(deftext); // Do not forget to free memory !!
	} else {
		text = itemText[itemNum];
		
		item = db_read_variable(varItems->list[itemNum], &err);
		if (!err) {
			deftext = display_default(item, false, NULL);
			rpn_delete(item);
		}
		else
            deftext = print_error(err);
        skin->varDefList(text, deftext, itemNum, hWnd);
        if (!err)
            MemPtrFree(deftext); // Do not forget to free memory !!
	}
}

/***********************************************************************
 *
 * FUNCTION:     def_init_varlist
 * 
 * DESCRIPTION:  Initialize the on-screen list with names of variables
 *               values get computed on-the-fly while drawing list
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void def_init_varlist(Skin *skin, void *hWnd) IFACE;
void def_init_varlist(Skin *skin, void *hWnd) {
    skin->varDefReset(hWnd);
	varItems = db_get_list(notype);
	if (!varItems->size) {
		db_delete_list(varItems);
		varItems = NULL;
		return;
	}
    for (int i=0 ; i<varItems->size ; i++) {
        def_draw_item(i, varItems->list, skin, hWnd);
    }
}

/***********************************************************************
 *
 * FUNCTION:     def_destroy_varlist
 * 
 * DESCRIPTION:  Free dynamically allocated structures for
 *               the onscreen field
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void def_destroy_varlist (void) IFACE;
void def_destroy_varlist (void) {
	if (!varItems)
	    return;
	
	db_delete_list(varItems);
	varItems = NULL;
}

rpntype def_type (int i) IFACE;
rpntype def_type (int i) {
    return (varItems->type[i]);
}

TCHAR *def_name (int i) {
    return(varItems->list[i]);
}

void def_delete (int i) {
    db_delete_record(varItems->list[i]);
}