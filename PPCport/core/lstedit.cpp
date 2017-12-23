/*
 *   $Id: lstedit.cpp,v 1.3 2009/12/15 21:36:54 mapibid Exp $
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

#include "stdafx.h"

#include "compat/PalmOS.h"
//#include <LstGlue.h>
//#include <WinGlue.h>

//#include "calcrsc.h"
#include "lstedit.h"
#include "konvert.h"
#include "calc.h"
#include "slist.h"
#include "stack.h"
#include "calcDB.h"
#include "display.h"
#include "varmgr.h"
#include "defuns.h"
#include "system - UI/StateManager.h"
#include "system - UI/EasyCalc.h"
#include "compat/Lang.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static Int16 firstLine;
static Int16 maxLines = 1;
static struct {
    Int16 col;
    Int16 row;
    Boolean offscreen;
}tblSelection = {-1,-1,true};

static tlistPrefs *listPrefs;

static List *loadedList[LIST_COUNT] = {NULL,NULL,NULL};

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_contents
 *
 * DESCRIPTION:  Draws a normal cell of table
 *
 * PARAMETERS:   See PalmOS reference
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
//static void
//lstedit_tbl_contents(void *table,Int16 row,Int16 column,
//             RectangleType *bounds) PARSER;
//static void
//lstedit_tbl_contents(void *table,Int16 row,Int16 column,
//             RectangleType *bounds)
//{
//    Int16 real;
//    TCHAR *text;
//    FontID oldfont;
//
//    real = firstLine + row;
//    if (loadedList[column-1] == NULL
//        || loadedList[column-1]->size <= real)
//        return;
//
//    oldfont = FntSetFont(stdFont);
//    text = display_complex(loadedList[column-1]->item[real]);
//    WinGlueDrawTruncChars(text,StrLen(text),
//              bounds->topLeft.x+2,bounds->topLeft.y,
//              bounds->extent.x-2);
//    MemPtrFree(text);
//    FntSetFont(oldfont);
//}

/***********************************************************************
 *
 * FUNCTION:     lstedit_delete_lists
 *
 * DESCRIPTION:  Free memory ocupied by loaded lists
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
static void lstedit_delete_lists (void) PARSER;
static void lstedit_delete_lists(void) {
    Int16 i;

    for (i=0 ; i<LIST_COUNT ; i++)
        if (loadedList[i] != NULL) {
            list_delete(loadedList[i]);
            loadedList[i] = NULL;
        }
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_save_lists
 *
 * DESCRIPTION:  Saves loaded lists back to database. Should be called
 *               after every modification
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError lstedit_save_lists (void) PARSER;
CError lstedit_save_lists (void) {
    Int16 i;
    Trpn item;
    CodeStack *stack;
    CError err = c_noerror;

    stack = stack_new(1);
    for (i=0 ; i<LIST_COUNT ; i++)
        if (StrLen(listPrefs->list[i])) {
            err = stack_add_val(stack, &loadedList[i], list);
            if (!err) {
                item = stack_pop(stack);
                err = db_write_variable(listPrefs->list[i], item);
                rpn_delete(item);
            }
        }
    stack_delete(stack);
    return err;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_upd_maxlines
 *
 * DESCRIPTION:  Update the maxLines variable, containing the length
 *               of longest loaded list
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
static void lstedit_upd_maxlines (void) PARSER;
static void lstedit_upd_maxlines (void) {
    Int16 i;

    maxLines = 0;
    for (i=0 ; i<LIST_COUNT ; i++)
        if ((loadedList[i] != NULL)
            && (loadedList[i]->size > maxLines))
            maxLines = loadedList[i]->size;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_load_lists
 *
 * DESCRIPTION:  Reads list name from Preferences and loads them
 *               into global loadedLists variable
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void lstedit_load_lists (void) PARSER;
void lstedit_load_lists (void) {
    Int16 i;
    Trpn item;
    CError err;

    lstedit_delete_lists();
    for (i=0 ; i<LIST_COUNT ; i++)
        if (StrLen(listPrefs->list[i])) {
            item = db_read_variable(listPrefs->list[i], &err);
            if (!err && item.type == list) {
                loadedList[i] = list_dup(item.u.listval);
            } else
                StrCopy(listPrefs->list[i],_T(""));
            if (!err)
                rpn_delete(item);
        }
    LstEditSetLabel(1, listPrefs->list[0]);
    LstEditSetLabel(2, listPrefs->list[1]);
    LstEditSetLabel(3, listPrefs->list[2]);

    lstedit_upd_maxlines();
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_load
 *
 * DESCRIPTION:  Load index values in the first column of ListEditor,
 *               starting from 1, and load list contents on screen.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void lstedit_tbl_load (void) PARSER;
void lstedit_tbl_load (void) {
    TCHAR *text1, *text2, *text3;
    Int16 i;

    // First, clear contents
    LstEditSetRow(-1, 0, NULL, NULL, NULL);

    // Then fill table
    for (i=0 ; i<maxLines ; i++) {
        if ((loadedList[0] == NULL) || (loadedList[0]->size <= i))
            text1 = NULL;
        else
            text1 = display_complex(loadedList[0]->item[i]);
        if ((loadedList[1] == NULL) || (loadedList[1]->size <= i))
            text2 = NULL;
        else
            text2 = display_complex(loadedList[1]->item[i]);
        if ((loadedList[2] == NULL) || (loadedList[2]->size <= i))
            text3 = NULL;
        else
            text3 = display_complex(loadedList[2]->item[i]);

        LstEditSetRow(i, i+1, text1, text2, text3);

        if (text1)
            MemPtrFree(text1);
        if (text2)
            MemPtrFree(text2);
        if (text3)
            MemPtrFree(text3);
    }
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_init
 *
 * DESCRIPTION:  Constructor for List Editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void lstedit_init (Boolean showAns) PARSER;
void lstedit_init (Boolean showAns) {
    listPrefs = &(stateMgr->state.listPrefs);
    firstLine = 0;
    tblSelection.col = tblSelection.row = -1;

    if (showAns) {
        StrCopy(listPrefs->list[0], _T("ans"));
    }
    // Initialize column header drop downs
    lstedit_reselect(LIST1_ID);
    lstedit_reselect(LIST2_ID);
    lstedit_reselect(LIST3_ID);
    // Load lists and set correct values in headers
    lstedit_load_lists();
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_scroll
 *
 * DESCRIPTION:  Scroll the table specified number of rows
 *
 * PARAMETERS:   tbl - pointer to table
 *               delta - how many lines (+down, -up) to scroll
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void lstedit_tbl_scroll (Int16 delta) PARSER;
void lstedit_tbl_scroll (Int16 delta) {
    firstLine +=delta;
    if (firstLine < 0)
        firstLine = 0;
    lstedit_tbl_load();
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_destroy
 *
 * DESCRIPTION:  Destructor for List Editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void lstedit_destroy (void) PARSER;
void lstedit_destroy (void) {
    lstedit_delete_lists();
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_deselect
 *
 * DESCRIPTION:  Deselect a variable, clear label that shows contents
 *               of variable
 *
 * PARAMETERS:   tbl - pointer to the table
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void lstedit_tbl_deselect (void) PARSER;
void lstedit_tbl_deselect (void) {
    if (tblSelection.row != -1) {
        LstEditDeselect();
        tblSelection.row = tblSelection.col = -1;
    }
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_init_firstitem
 *
 * DESCRIPTION:  (Mapi) Initialize a loaded list with an item. Was part of
 *               lstedit_reselect() before, is now a call back from UI.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void lstedit_init_firstitem (int controlId, Complex *cplx) PARSER;
void lstedit_init_firstitem (int controlId, Complex *cplx) {
    int i = controlId - LIST1_ID;
    if (loadedList[i])
        MemPtrFree(loadedList[i]);
    loadedList[i] = list_new(1);
    loadedList[i]->item[0] = *cplx;

    tblSelection.offscreen = true;
    tblSelection.row = 0;
    tblSelection.col = i + 1;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_append_item
 *
 * DESCRIPTION:  (Mapi) Add an item to a list. New.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError lstedit_append_item (int controlId, Complex *cplx) PARSER;
CError lstedit_append_item (int controlId, Complex *cplx) {
    int row = loadedList[controlId - LIST1_ID]->size; // Insert at end.
    return (lstedit_insert_item(controlId, cplx, row));
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_insert_item
 *
 * DESCRIPTION:  (Mapi) Add an item to a list at given place.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError lstedit_insert_item (int controlId, Complex *cplx, int row) PARSER;
CError lstedit_insert_item (int controlId, Complex *cplx, int row) {
    int l = controlId - LIST1_ID;
    List *lst, *oldlst;
    CError err = c_noerror;
    int i;

    lst = list_new(loadedList[l]->size+1);
    if (!lst) {
        return (c_memory);
    }

    if (row > loadedList[l]->size)
        row = loadedList[l]->size; // Security ..
    for (i=0 ; i<row ; i++)
        lst->item[i] = loadedList[l]->item[i];
    lst->item[row] = *cplx;
    for (i=row ; i<loadedList[l]->size ; i++)
        lst->item[i+1] = loadedList[l]->item[i];

    oldlst = loadedList[l];
    loadedList[l] = lst;
    err = lstedit_save_lists();
    if (err) {
        loadedList[l] = oldlst; /* restore original one */
        list_delete(lst);
        return (err);
    } else
        list_delete(oldlst);

    lstedit_upd_maxlines();
    tblSelection.offscreen = true;

    return (err);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_get_item
 *
 * DESCRIPTION:  (Mapi) Get item value from a list.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError lstedit_get_item (int controlId, Complex *cplx, int row) PARSER;
CError lstedit_get_item (int controlId, Complex *cplx, int row) {
    int l = controlId - LIST1_ID;
    CError err = c_noerror;

    if (row > loadedList[l]->size)
        return (c_range); // Security ..
    *cplx = loadedList[l]->item[row];

    return (err);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_replace_item
 *
 * DESCRIPTION:  (Mapi) Replace an item value in a list.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError lstedit_replace_item (int controlId, Complex *cplx, int row) PARSER;
CError lstedit_replace_item (int controlId, Complex *cplx, int row) {
    int l = controlId - LIST1_ID;
    CError err = c_noerror;

    if (row > loadedList[l]->size)
        return (c_range); // Security ..
    loadedList[l]->item[row] = *cplx;

    err = lstedit_save_lists();
    tblSelection.offscreen = true;

    return (err);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_item_length
 *
 * DESCRIPTION:  (Mapi) Retrieve display length of an item.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
int lstedit_item_length (int controlId, int row) PARSER;
int lstedit_item_length (int controlId, int row) {
    TCHAR *text = display_complex(loadedList[controlId-LIST1_ID]->item[row]);
    int len = _tcslen(text);
    MemPtrFree(text);
    return (len);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_list_length
 *
 * DESCRIPTION:  (Mapi) Retrieve length of a list.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
int lstedit_list_length (int controlId) PARSER;
int lstedit_list_length (int controlId) {
    return (loadedList[controlId-LIST1_ID]->size);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_delete_item
 *
 * DESCRIPTION:  (Mapi) Delete an item from a list at given place.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError lstedit_delete_item (int controlId, int row) PARSER;
CError lstedit_delete_item (int controlId, int row) {
    int l = controlId - LIST1_ID;

    if (loadedList[l]->size == 1)
        return (c_range);
    for (int i=row+1 ; i<loadedList[l]->size ; i++)
        loadedList[l]->item[i-1] = loadedList[l]->item[i];
    --loadedList[l]->size;

    lstedit_save_lists();  /* should succeed since list gets smaller */
    lstedit_upd_maxlines();
    tblSelection.offscreen = true;

    return (c_noerror);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_reselect
 *
 * DESCRIPTION:  Shows a popup dialog with existing list and 'New' and
 *               'Delete'. Creates new list, delete new list or just
 *               selects a new list
 *
 * PARAMETERS:   controlId - popuptrigger that was pressed
 *
 ***********************************************************************/
void lstedit_reselect (Int16 controlId) PARSER;
void lstedit_reselect (Int16 controlId) {
    dbList *varlist;
    TCHAR **values;
    Int16 i;

    varlist = db_get_list(list);
    values = (TCHAR **) MemPtrNew(sizeof(*values) * (varlist->size));

    for (i=0 ; i<varlist->size ; i++)
        values[i] = varlist->list[i];
    LstEditSetListChoices(controlId, values, varlist->size);
    LstEditSetLabel(controlId, listPrefs->list[controlId-LIST1_ID]);

    /* Set incremental search */
    // No equivalent in Windows (in fact, this is done natively, but I don't know
    // how many chars .. (up to 5 in the Glue API).
//    LstGlueSetIncrementalSearch(slst, true);

    MemPtrFree(values);
    db_delete_list(varlist);
}
