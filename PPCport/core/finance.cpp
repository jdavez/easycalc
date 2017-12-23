/*
 *   $Id: finance.cpp,v 1.1 2011/02/28 22:07:18 mapibid Exp $
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
 *
*/

#include "StdAfx.h"
#include "compat/PalmOS.h"

#include "defuns.h"
#include "prefs.h"
#include "calc.h"
#include "stack.h"
#define _FINANCE_C_
#include "finance.h"
#include "calcDB.h"
#include "funcs.h"
#include "varmgr.h"
#include "display.h"
#include "fp.h"

#include "system - UI/EasyCalc.h"

//#include "calcrsc.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

/* Display mode for financial calculations */
TdispPrefs finDPrefs = {5, true, disp_normal, disp_decimal, false};

t_finDef inpFinForm[]= {
      {FIN_PV, _T("PV"), _T("$$PRESENT VALUE"),
       _T("PV=fin_pv(I:N:PMT:fv:PYR:1)"),
       _T("PV=fin_pv(I:N:PMT:fv:PYR:0")},
      {FIN_FV, _T("fv"), _T("$$FUTURE VALUE"),
       _T("fv=fin_fv(I:N:PV:PMT:PYR:1)"),
       _T("fv=fin_fv(I:N:PV:PMT:PYR:0)")},
      {FIN_N, _T("N"), _T("$$NB PAYMENTS"),
       _T("N=fin_n(I:PV:PMT:fv:PYR:1)"),
       _T("N=fin_n(I:PV:PMT:fv:PYR:0)")},
      {FIN_I, _T("I"), _T("$$INTEREST"),
       // Mapi: reduce investigation range to [0,1000], and be more precise
       //_T("I=fzero(-10:1000:\"fv-fin_fv(x:N:PV:PMT:PYR:1)\":1E-6"),
       //_T("I=fzero(-10:1000:\"fv-fin_fv(x:N:PV:PMT:PYR:0\":1E-6")},
       _T("I=fzero(0:1000:\"fv-fin_fv(x:N:PV:PMT:PYR:1)\":1E-8"),
       _T("I=fzero(0:1000:\"fv-fin_fv(x:N:PV:PMT:PYR:0\":1E-8")},
      {FIN_PYR, _T("PYR"), _T("$$NB PMT PER YEAR"),
       // Mapi: do not change PYR when I == 0
       //_T("PYR=fzero(1E-1:366:\"fv-fin_fv(I:N:PV:PMT:x:1)\":1E-6"),
       //_T("PYR=fzero(1E-1:366:\"fv-fin_fv(I:N:PV:PMT:x:0\":1E-6")},
       _T("PYR=if(I>0:fzero(1E-1:366:\"fv-fin_fv(I:N:PV:PMT:x:1)\":1E-6):PYR)"),
       _T("PYR=if(I>0:fzero(1E-1:366:\"fv-fin_fv(I:N:PV:PMT:x:0)\":1E-6):PYR)")},
      {FIN_PMT, _T("PMT"), _T("$$PAYMENT"),
       _T("PMT=fin_pmt(I:N:PV:fv:PYR:1)"),
       _T("PMT=fin_pmt(I:N:PV:fv:PYR:0)")},
      {-1, NULL, NULL, NULL, NULL}
};

t_finDef outFinForm[2] = {
      {FIN_TPMT, _T("TPMT"), _T("$$TOTAL PAYMENT"),
       _T("TPMT=N*PMT"),
       _T("TPMT=N*PMT")},
      {FIN_TPMT, _T("TCOST"), _T("$$TOTAL COST"),
       _T("TCOST=N*PMT+PV-fv"),
       _T("TCOST=N*PMT+PV-fv")}
};

/***********************************************************************
 *
 * FUNCTION:     fin_update_fields
 *
 * DESCRIPTION:  Updates the onscreen representation of
 *               financial variables
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
void fin_update_fields (void) IFACE;
void fin_update_fields (void) {
    CError err;
    TCHAR  text[MAX_FP_NUMBER+1], *value;
    Trpn   item;
    Int16  i;

    FinSetRow(-1, NULL, NULL);
    for (i=0 ; inpFinForm[i].var!=NULL ; i++) {
        item = db_read_variable(inpFinForm[i].var, &err);
        if (err || (item.type != real))
            value = NULL;
        else {
            fp_print_double(text, item.u.realval);
            value = text;
        }
        if (!err)
            rpn_delete(item);
        /* Show result on screen */
        FinSetRow(i, inpFinForm[i].var, value);
    }

    for (i=0 ; i<2 ; i++) {
        item = db_read_variable(outFinForm[i].var, &err);
        if (err || (item.type != real))
            value = NULL;
        else {
            fp_print_double(text, item.u.realval);
            value = text;
        }
        if (!err)
            rpn_delete(item);
        /* Show result on screen */
        FinShowValue(i, value);
    }
}

/***********************************************************************
 *
 * FUNCTION:     fin_compute
 *
 * DESCRIPTION:  Computes a new value for a financial variable
 *
 * PARAMETERS:   pos - index into inpFinForm for a selected variable
 *               begin - the status of the Begin/End switch
 *
 * RETURN:
 *
 ***********************************************************************/
void fin_compute (int pos, int begin) IFACE;
void fin_compute (int pos, int begin) {
    CError err;
    CodeStack *stack;

    // Allow using 'if' in the formula, by copying to writable memory.
    // Indeed, the processing of 'if' is writing '\0' at some places
    // in the string.
    TCHAR *inp = _mtcsdup(begin ? inpFinForm[pos].begStr : inpFinForm[pos].endStr);
    stack = text_to_stack(inp, &err);
    if (err)
        return;
    wait_draw();
    err = stack_compute(stack);
    stack_delete(stack);
    mfree(inp);
    wait_erase();
    if (err)
        alertErrorMessage(err);

    stack = text_to_stack((begin ? outFinForm[0].begStr : outFinForm[0].endStr),
                          &err);
    if (err)
        return;
    err = stack_compute(stack);
    stack_delete(stack);
    if (err)
        alertErrorMessage(err);

    stack = text_to_stack((begin ? outFinForm[1].begStr : outFinForm[1].endStr),
                          &err);
    if (err)
        return;
    err = stack_compute(stack);
    stack_delete(stack);
    if (err)
        alertErrorMessage(err);

    return;
}
