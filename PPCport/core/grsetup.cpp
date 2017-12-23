/*
 *   $Id: grsetup.cpp,v 1.2 2011/04/14 19:22:24 mapibid Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000 Ondrej Palkovsky
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
#include <string.h>

//#include "clie.h"
#include "core/mlib/calcDB.h"
#include "core/mlib/konvert.h"
#include "core/mlib/display.h"
#include "core/grsetup.h"
#include "core/grprefs.h"
//#include "graph.h"
#include "core/varmgr.h"
//#include "calcrsc.h"
#include "core/calc.h"
#include "core/mlib/stack.h"
#include "system - UI/EasyCalc.h"

static TCHAR *tblFuncs[] = {_T("Y1"), _T("Y2"), _T("Y3"), _T("Y4"), _T("Y5"), _T("Y6")};
static TCHAR *tblPols[] = {_T("r1"), _T("r2"), _T("r3"), _T("r4"), _T("r5"), _T("r6")};

const TCHAR *grsetup_param_name (void) {
    if (graphPrefs.functype == graph_func)
        return _T("x");
    else if (graphPrefs.functype == graph_polar)
        return _T("t");
    else /* Parametric */
        return _T("t");
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_fn_descr_arr
 * 
 * DESCRIPTION:  Setup the array with names and positions of 
 *               existing and activated user selected functions.
 *
 * PARAMETERS:   descr[MAX_GRFUNCS] - array that will contain 
 *                                    descriptions of funcs
 *               nums[MAX_GRFUNCS] - positions to table of selected functions
 *
 * RETURN:       count of selected functions
 *      
 ***********************************************************************/
int grsetup_fn_descr_arr (TCHAR **descr, int *nums) {
    int i, count;

    for (i=0,count=0 ; i<MAX_GRFUNCS ; i++) {
        // Mapi: only select those which are enabled on display
        if (graphPrefs.grEnable[graphPrefs.functype][i])
            if ((graphPrefs.functype == graph_param)
                && (StrLen(grsetup_get_fname(i*2)))
                && (StrLen(grsetup_get_fname(i*2+1)))) {
                descr[count] = (TCHAR *) grsetup_fn_descr(i*2);
                nums[count] = i;
                count++;
            } else if ((graphPrefs.functype != graph_param)
                       && (StrLen(grsetup_get_fname(i)))) {
                descr[count] = (TCHAR *) grsetup_fn_descr(i);
                nums[count] = i;
                count++;
            }
    }
    return (count);
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_fn_descr
 *
 * DESCRIPTION:  Return name of function, that is selected by the user
 *               on the i'th position in the list
 *
 * PARAMETERS:   i - position of a function in the list
 *
 * RETURN:       Pointer to static function name
 *
 ***********************************************************************/
const TCHAR *grsetup_fn_descr (int i) {
    if (graphPrefs.functype == graph_func)
        return (tblFuncs[i]);
    else if (graphPrefs.functype == graph_polar)
        return (tblPols[i]);
	else
        return (tblFuncs[i/2]);
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_def_name
 *
 * DESCRIPTION:  Return name of function, that should be created specially
 *               for graphing purposes (the 'User' selection)
 *
 * PARAMETERS:   row - the row number in a grsetup table
 *
 * RETURN:       Pointer to static function name
 *
 ***********************************************************************/
const TCHAR *grsetup_def_name (int row) {
    static TCHAR result[MAX_FUNCNAME+1];
    TCHAR tmp[2];

    StrCopy(tmp, _T("1"));
    switch (graphPrefs.functype) {
     case graph_func:
        StrCopy(result, _T("z_grafun"));
        tmp[0] += (TCHAR) row;
        StrCat(result, tmp);
        break;
     case graph_polar:
        StrCopy(result, _T("z_grapol"));
        tmp[0] += (TCHAR) row;
        StrCat(result, tmp);
        break;
     case graph_param:
        StrCopy(result, _T("z_grapar"));
        StrCat(result, (row & 1)? _T("a") : _T("b"));
        tmp[0] += (TCHAR) (row / 2);
        StrCat(result, tmp);
        break;
    }
    return (result);
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_get_fname
 *
 * DESCRIPTION:  Get a name of a function for the given row, that is
 *               already saved in Preferences
 *
 * PARAMETERS:   row - number of row in the displayed table
 *
 * RETURN:       !!! Pointer to a graphPrefs structure, so
 *               functions can modify the name directly
 *
 ***********************************************************************/
TCHAR *grsetup_get_fname (int row) {
    switch (graphPrefs.functype) {
    case graph_func:
        return (graphPrefs.funcFunc[row]);
    case graph_polar:
        return (graphPrefs.funcPol[row]);
    case graph_param:
    default:
        return (graphPrefs.funcPar[row >> 1][row & 1]);
    }
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_cell_contents
 *
 * DESCRIPTION:  Function for drawing the function name or
 *               contents of the user defined function
 *
 * RETURN:       None
 *
 ***********************************************************************/
void grsetup_cell_contents (int row) {
    TCHAR *name;
    const TCHAR *usrname;
    TCHAR *text;
    CError err;

    name = grsetup_get_fname(row); /* name is now a pointer to graphPrefs */
    usrname = grsetup_def_name(row);

    if (StrLen(name)==0)
        GraphConfSetRow(row, _T("")); /* Empty box */
    else if (StrCompare(usrname, name) == 0) {
        /* Special graph function, show contents */
        err = db_func_description(usrname, &text, NULL);
        if (!err) {
            GraphConfSetRow(row, text);
            MemPtrFree(text);
        }
        else
          /* Someone probably deleted it, reset the value */
          StrCopy(name, _T(""));
    } else {
        /* Normal function, show ordinary name */
        TCHAR tmpname[MAX_FUNCNAME+5];
        StrCopy(tmpname, name);
        StrCat(tmpname, _T("()"));
        GraphConfSetRow(row, tmpname);
    }
}
