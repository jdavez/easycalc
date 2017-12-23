/*
 *   $Id: grprefs.cpp,v 1.1 2011/02/28 22:07:18 mapibid Exp $
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

#include "core/mlib/display.h"
#include "core/grprefs.h"
#include "core/mlib/defuns.h"
#include "compat/MathLib.h"
#include "core/mlib/stack.h"
#include "core/prefs.h"
#include "core/Main.h"
#include "core/calc.h"
#include "core/mlib/mathem.h"
#include "core/mlib/fp.h"

/* Local structure - values in function mode */
TgrPrefs graphPrefs;
TgrPrefs viewPrefs;

/* Colors of functions and background, axis, grid */
RGBColorType graphRGBColors[] = {
    {0, 255, 0, 0},
    {0, 0, 255, 0},
    {0, 0, 0, 255},
    {0, 128, 128, 0},
    {0, 0, 128, 128},
    {0, 128, 0, 128},
    {0, 0x0b, 0x23, 0x27},       /* axis */
    {0, 128, 128, 128},  /* grid */
    {0, 0xa3, 0xb3, 0x9d} /* background */
};

/* Gray colors: 6 funcs + axis + grid + backgrnd */
IndexedColorType funcolors[9] = {15, 11, 7, 13, 9, 5, 15, 8, 0};

/* Display mode for preferences */
const TdispPrefs grDPrefs = {9, true, disp_normal, disp_decimal, false, false};

/***********************************************************************
 *
 * FUNCTION:     grpref_compl_field
 *
 * DESCRIPTION:  Computes the 'real' value of a text field
 *
 * PARAMETERS:   objectID - id of the field
 *
 * RETURN:       CError - 0 on success
 *               *value - the computed value
 *
 ***********************************************************************/
CError grpref_comp_field(TCHAR *text, double *value) {
    CError err;
    CodeStack *stack;

    stack = text_to_stack(text, &err);
    if (!err) {
        err = stack_compute(stack);
        if (!err)
            err = stack_get_val(stack, (void *) value, real);
        stack_delete(stack);
    }

    return ((CError) (err || !finite(*value)));
}

/***********************************************************************
 *
 * FUNCTION:     grpref_verify_values
 *
 * DESCRIPTION:  Verify values for graph preferences
 *
 * PARAMETERS:   None
 *
 * RETURN:       1 - success
 *               0 - error while computing
 *
 ***********************************************************************/
Int16 grpref_verify_values (void) {
    if ((graphPrefs.xscale < 0.0) || (graphPrefs.yscale < 0.0))
        return (0);

    if ((graphPrefs.xmin >= graphPrefs.xmax) || (graphPrefs.ymin >= graphPrefs.ymax))
        return (0);

    if (graphPrefs.logx && (graphPrefs.xmin <= 0.0))
        return (0);
    if (graphPrefs.logy && (graphPrefs.ymin <= 0.0))
        return (0);

    if (graphPrefs.functype == graph_polar) {
        if ((graphPrefs.fimin > graphPrefs.fimax)
            || ((graphPrefs.fimax-graphPrefs.fimin)/graphPrefs.fistep > 10000.0)
            || (graphPrefs.fistep <= 0.0)) /* check that min<max and step is reasonable */
            return (0);
    } else if (graphPrefs.functype == graph_param) {
        if ((graphPrefs.tmin > graphPrefs.tmax)
            || ((graphPrefs.tmax-graphPrefs.tmin)/graphPrefs.tstep > 10000.0)
            || (graphPrefs.tstep <= 0.0)) /* check that min<max and step is reasonable */
            return (0);
    }

    return (1);
}

/***********************************************************************
 *
 * FUNCTION:     set_axis
 *
 * DESCRIPTION:  
 *
 * PARAMETERS:   
 *
 * RETURN:       
 *
 ***********************************************************************/
CError set_axis (Functype *func, CodeStack *stack) {
    List *lst, *lst2;
    double xscale = graphPrefs.xscale;
    double yscale = graphPrefs.yscale;
    int i;
    CError err;

    if (func->paramcount > 2)
        return (c_badargcount);
    else if (func->paramcount == 2) {
        err = stack_get_val(stack, &lst2, list);
        if (err)
            return (err);
        if (lst2->size != 2){
            MemPtrFree(lst2);
            return (c_baddim);
        }
        if ((lst2->item[0].imag != 0.0) || (lst2->item[1].imag != 0.0)) {
            MemPtrFree(lst2);
            return (c_badarg);
        }
        xscale = lst2->item[0].real;
        yscale = lst2->item[1].real;
        MemPtrFree(lst2);
        if ((xscale <= 0.0) || (yscale <= 0.0))
            return (c_badarg);
    }

    err = stack_get_val(stack, &lst, list);
    if (err)
        return (err);

    if (lst->size != 4) {
        MemPtrFree(lst);
        return (c_baddim);
    }

    for (i=0 ; i<4 ; i++)
      if (lst->item[i].imag != 0.0) {
        MemPtrFree(lst);
        return (c_badarg);
    }

    if((lst->item[0].real >= lst->item[1].real)
        || (lst->item[2].real >= lst->item[3].real)) {
        MemPtrFree(lst);
        return (c_badarg);
    }

    graphPrefs.xmin = lst->item[0].real;
    graphPrefs.xmax = lst->item[1].real;
    graphPrefs.ymin = lst->item[2].real;
    graphPrefs.ymax = lst->item[3].real;
    if (func->paramcount == 2) {
        graphPrefs.xscale = xscale;
        graphPrefs.yscale = yscale;
        graphPrefs.grEnable[graphPrefs.functype][7] = true;
    } else
        graphPrefs.grEnable[graphPrefs.functype][7] = false;


    err = stack_add_val(stack, &lst, list);
    MemPtrFree(lst);

    return (err);
}
