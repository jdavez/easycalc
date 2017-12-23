/*
 *   $Id: solver.cpp,v 1.1 2011/02/28 22:07:18 mapibid Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 2002 Ondrej Palkovsky
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
#include "compat/MathLib.h"
#include <string.h>
#include "konvert.h"
#include "calcDB.h"
#include "defuns.h"
#include "display.h"
#include "stack.h"
#include "calc.h"
//#include "calcrsc.h"
#include "varmgr.h"
#define _SOLVER_C_
#include "solver.h"
#include "funcs.h"
#include "integ.h"
#include "mathem.h"
#include "prefs.h"
#include "compat/dbutil.h"
#include "system - UI/EasyCalc.h"

#ifdef HANDERA_SDK
#include "Vga.h"
#endif
#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

#define DEFAULT_MIN -1E5
#define DEFAULT_MAX 1E5

static struct {
    TCHAR **list;
    int     size;
} varitems;
//static char HORIZ_ELLIPSIS;
Int16 slv_selectedWorksheet = -1;

double worksheet_min;
double worksheet_max;
double worksheet_prec;
TCHAR worksheet_title[MAX_WORKSHEET_TITLE+1];
TCHAR *worksheet_note = NULL;

DmOpenRef slv_gDB;
TCHAR oldparam[MAX_FUNCNAME+1];

struct dbworksheet {
    double min, max, prec;
    TCHAR title[MAX_WORKSHEET_TITLE+1];
    TCHAR equation[1];
};

#define VARNAME_WIDTH  42

/***********************************************************************
 *
 * FUNCTION:     slv_add_variable
 *
 * DESCRIPTION:  Adds variable name to the 'varitems' variable list
 *               (list of variables of the equation)
 *
 * PARAMETERS:   varname
 *
 * RETURN:
 *
 ***********************************************************************/
static void slv_add_variable (TCHAR *varname) IFACE;
static void slv_add_variable (TCHAR *varname) {
    Int16 i, j;
    int   comp = -1;

    if (is_constant(varname))
        return;
    for (i=0 ; (i < varitems.size)
               &&((comp = StrCompare(varname, varitems.list[i])) > 0)
             ; i++)
        ;
    if (comp == 0)
        return;

    /* Move all items one position forward */
    for (j=varitems.size ; j>i ; j--)
        varitems.list[j] = varitems.list[j-1];
    varitems.list[i] = (TCHAR *) MemPtrNew((StrLen(varname)+1)*sizeof(TCHAR));
    StrCopy(varitems.list[i], varname);
    varitems.size++;
}

/***********************************************************************
 *
 * FUNCTION:     slv_update_worksheet
 *
 * DESCRIPTION:  This is called when there is a change of active worksheet
 *               - sets all internal variables to worksheet values
 *               - shows/hide display controls, sets texts etc.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void slv_update_worksheet (void) IFACE;
void slv_update_worksheet (void) {
    struct dbworksheet *record;
    MemHandle recordHandle;
    TCHAR *noteptr;

    if (slv_selectedWorksheet == -1) {
        SlvEnableAll(FALSE);
        StrCopy(worksheet_title, _T(""));
        LstEditSetLabel(SOLVER_ID, worksheet_title);
        return;
    }

    recordHandle = DmQueryRecord(slv_gDB, slv_selectedWorksheet);
    record = (struct dbworksheet *) MemHandleLock(recordHandle);
    StrCopy(worksheet_title, record->title);
    worksheet_min  = record->min;
    worksheet_max  = record->max;
    worksheet_prec = record->prec;

    SlvEqFldSet(record->equation);

    noteptr = record->equation + StrLen(record->equation) + 1;
    StrCopy(worksheet_note, noteptr);

    MemHandleUnlock(recordHandle);

    LstEditSetLabel(SOLVER_ID, worksheet_title);

    SlvEnableAll(TRUE);
}

/***********************************************************************
 *
 * FUNCTION:     slv_init_varlist
 *
 * DESCRIPTION:  Initialize the on-screen list with names of variables
 *               values get computed on-the-fly while drawing list
 *
 * PARAMETERS:   None
 *
 * RETURN:       (Mapi) Number of vars or -1 if invalid
 *
 ***********************************************************************/
Int16 slv_init_varlist (void) IFACE;
Int16 slv_init_varlist (void) {
    CodeStack *stack = NULL;
    TCHAR *equation;
    CError err;
    Int16 varcount, i;

    SlvVarSetRow (-1, NULL, NULL); // Clear list of variable
    if (slv_selectedWorksheet == -1) {
        SlvSetExeButton(SLV_OFF);
        return (-1);
    }

    equation = SlvEqFldGet();
    if (!equation) {
        MemPtrFree(equation);
        SlvSetExeButton(SLV_OFF);
        return (0);
    }

    StrCopy(oldparam, parameter_name);
    StrCopy(parameter_name, _T("**"));
    stack = text_to_stack(equation, &err);
    StrCopy(parameter_name, oldparam);
    MemPtrFree(equation);
    if (err) {
        SlvSetExeButton(SLV_OFF);
        return (0);
    }

    /* Now get the number of variables from stack */
    varcount = 0;
    for (i=0 ; i<stack->size ; i++) {
        if ((stack->stack[i].type == variable) ||
            (stack->stack[i].type == litem))
            varcount++;
    }
    varitems.list = (TCHAR **) MemPtrNew(varcount * sizeof(*varitems.list));
    varitems.size = 0;

    for (i=0 ; i<stack->size ; i++) {
        if (stack->stack[i].type == variable)
            slv_add_variable(stack->stack[i].u.varname);
        else if (stack->stack[i].type == litem)
            slv_add_variable(stack->stack[i].u.litemval.name);
    }

    Trpn   item;
    TCHAR *text;
    for (i=0 ; i<varitems.size ; i++) {
        item = db_read_variable(varitems.list[i], &err);
        if (!err) {
            text = display_default(item, false, NULL);
            rpn_delete(item);
        } else {
            text = NULL;
        }
        SlvVarSetRow(i, varitems.list[i], text);
        if (text)
            MemPtrFree(text);
    }

    /* Now distinguish if it is equation or not */
    if ((stack->stack[0].type == function) &&
        (stack->stack[0].u.funcval.offs == FUNC_EQUAL)) {
        SlvSetExeButton(SLV_SOLVE);
    } else {
        SlvSetExeButton(SLV_CALCULATE);
    }

    stack_delete(stack);
    return (varitems.size);
}

/***********************************************************************
 *
 * FUNCTION:     slv_destroy_varlist
 *
 * DESCRIPTION:  Free dynamically allocated structures for
 *               the onscreen field
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
void slv_destroy_varlist (void) IFACE;
void slv_destroy_varlist (void) {
    Int16 i;

    if (!varitems.list)
        return;

    for (i=0;i<varitems.size;i++)
        MemPtrFree(varitems.list[i]);
    MemPtrFree(varitems.list);
    varitems.list = NULL;
    varitems.size = 0;
}


/***********************************************************************
 *
 * FUNCTION:     slv_solve_real
 *
 * DESCRIPTION:  Solve the equation using fzero() for variable varname
 *               and store the result in the variable
 *
 * PARAMETERS:   equation,varname
 *
 * RETURN:       err
 *
 ***********************************************************************/
static CError slv_solve_real (TCHAR *equation, TCHAR *varname) IFACE;
static CError slv_solve_real (TCHAR *equation, TCHAR *varname) {
    CodeStack *stack;
    CError err;
    double result;

    StrCopy(oldparam,parameter_name);
    StrCopy(parameter_name,varname);
    stack = text_to_stack(equation,&err);
    StrCopy(parameter_name,oldparam);
    if (err)
        return err;

    /* Now we expect, that FUNC_EQUAL is the first item */
    /* And we should definitely check funcval.num istead, but
     * this is not unique yet */
    if (stack->stack[0].type != function ||
        stack->stack[0].u.funcval.offs != FUNC_EQUAL) {
        stack_delete(stack);
        return c_syntax;
    }
    /* Now change '=' to '-' */
    stack->stack[0].u.funcval.offs = FUNC_MINUS;
    stack->stack[0].u.funcval.num = FUNC_MINUS;
    /* And now let's solve it */
    result = integ_zero(worksheet_min, worksheet_max,0.0,
                stack,worksheet_prec,MATH_FZERO,NULL);
    if (!finite(result)) {
        stack_delete(stack);
        return c_compimp;
    }
    /* Round the result somewhat */
//  result /= 1E-5;
//  result = round(result) * 1E-5;
    /* And now save the result */
    stack_delete(stack);
    db_save_real(varname,result);

    return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     slv_solve
 *
 * DESCRIPTION:  This is called when button 'solve' is pressed
 *               and it solves equation for the selected variable
 *
 * PARAMETERS:   selection - selected variable
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError slv_solve (Int16 selection) IFACE;
CError slv_solve (Int16 selection) {
    CError err;
    TCHAR *eq;

    wait_draw();
    err = slv_solve_real((eq = SlvEqFldGet()), varitems.list[selection]);
    MemPtrFree(eq);
    wait_erase();
    if (err)
        alertErrorMessage(err);
    return (err);
}

/***********************************************************************
 *
 * FUNCTION:     slv_calculate
 *
 * DESCRIPTION:  Calculate result of the expression
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void slv_calculate (void) IFACE;
void slv_calculate (void) {
    CodeStack *stack;
    TCHAR *equation;
    CError err;
    Trpn result;
    TCHAR *text;

    equation = SlvEqFldGet();
    if (!equation)
        return;

    StrCopy(oldparam, parameter_name);
    StrCopy(parameter_name, _T("**"));
    stack = text_to_stack(equation, &err);
    StrCopy(parameter_name, oldparam);
    if (!err) {
        if ((stack->stack[0].type == function)
            && (stack->stack[0].u.funcval.offs == FUNC_EQUAL))
            return;

        err = stack_compute(stack);
        if (!err) {
            result = stack_pop(stack);
            text = display_default(result, false, NULL);
            FrmCustomAlert(altSolverResult, text, NULL, NULL, NULL);
            db_write_variable(_T("solvres"), result);
            MemPtrFree(text);
            rpn_delete(result);
        }
        stack_delete(stack);
    }
    MemPtrFree(equation);
    if (err)
        alertErrorMessage(err);
}

/***********************************************************************
 *
 * FUNCTION:     slv_getVar
 *
 * DESCRIPTION:  Mapi (new): get var name from index in list of variables
 *
 * PARAMETERS:   i - selected variable
 *
 * RETURN:       var name
 *
 ***********************************************************************/
TCHAR *slv_getVar (int i) IFACE;
TCHAR *slv_getVar (int i) {
    return (varitems.list[i]);
}


/***********************************************************************
 *
 * FUNCTION:     slv_compare_record
 *
 * DESCRIPTION:  Compare 2 records in solver database - used for
 *               binary search in solver DB
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 ***********************************************************************/
static Int16 slv_compare_record(struct dbworksheet *rec1,
                struct dbworksheet *rec2,
                Int16 unesedInt16,SortRecordInfoPtr unused1,
                SortRecordInfoPtr unused2,
                MemHandle appInfoH) MLIB;
static Int16 slv_compare_record(struct dbworksheet *rec1,struct dbworksheet *rec2,
                 Int16 unesedInt16,SortRecordInfoPtr unused1,
                 SortRecordInfoPtr unused2,
                 MemHandle appInfoH) {
    return (StrCompare(rec1->title, rec2->title));
}

/***********************************************************************
 *
 * FUNCTION:     slv_db_open
 *
 * DESCRIPTION:  Open the solver database, create new one if needed
 *
 * PARAMETERS:
 *
 * RETURN:       0 - success, err - otherwise
 *
 ***********************************************************************/
Int16 slv_db_open (void) IFACE;
Int16 slv_db_open (void) {
    return (open_db(SOLVERDBNAME, SOLVER_DB_VERSION, LIB_ID, SOLVERDBTYPE,
                    &slv_gDB));
}

Int16 slv_db_close (void) IFACE;
Int16 slv_db_close (void) {
    return DmCloseDatabase(slv_gDB);
}


/***********************************************************************
 *
 * FUNCTION:     slv_update_help
 *
 * DESCRIPTION:  Called when user selects line in the variable list,
 *               show help text if available in the 'Note'
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void slv_update_help (int i) IFACE;
void slv_update_help (int i) {
    TCHAR *hlpstring;
    TCHAR *tmp, *colon;
    TCHAR *varname;
    Int16 size;

    if (slv_selectedWorksheet == -1)
        return;

    if (i == noListSelection) {
        SlvVarHelp(_T(""));
        return;
    }
    varname = varitems.list[i];

    for (tmp=worksheet_note ; tmp ; tmp=StrChr(tmp, _T('\n'))) {
        if (*tmp == _T('\n'))
            tmp++;
        colon = StrChr(tmp, _T(':'));
        if (!colon)
            continue;
        size = (colon - tmp);
        if (size != StrLen(varname))
            continue;
        if (memcmp(tmp, varname, size*sizeof(TCHAR)) == 0) {
            do {
                colon++;
            } while (*colon == _T(' '));
#ifdef _WINDOWS
            tmp = StrChr(colon, _T('\r'));
            if (!tmp) // If no '\r', search for '\n'. New line in Windows is "\r\n"
                tmp = StrChr(colon, _T('\n'));
#else
            tmp = StrChr(colon, _T('\n'));
#endif
            if (tmp)
                size = (tmp - colon);
            else
                size = StrLen(colon);
            if (size > MAX_WORKSHEET_HELP)
                size = MAX_WORKSHEET_HELP;
            hlpstring = (TCHAR *) MemPtrNew((size+1)*sizeof(TCHAR));
            memcpy(hlpstring, colon, size*sizeof(TCHAR));
            hlpstring[size] = _T('\0');
            SlvVarHelp(hlpstring);
            MemPtrFree(hlpstring);
            return;
        }
    }
    SlvVarHelp(_T(""));
}


/***********************************************************************
 *
 * FUNCTION:     slv_init
 *
 * DESCRIPTION:  Mapi(new): initialize solver
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 ***********************************************************************/
void slv_init (void) IFACE;
void slv_init (void) {
    worksheet_note = (TCHAR *) MemPtrNew(MAX_WORKSHEET_NOTE*sizeof(TCHAR));
    slv_selectedWorksheet = calcPrefs.solverWorksheet;
    slv_db_open();
    slv_select_worksheet();
    slv_update_worksheet();
    slv_init_varlist();
    slv_update_help(-1); // No variable selected yet
    SlvSetExeButton(SLV_OFF);
}

void slv_close (void) IFACE;
void slv_close (void) {
    calcPrefs.solverWorksheet = slv_selectedWorksheet;
    slv_save_worksheet();
    slv_destroy_varlist();
    slv_db_close();
    slv_selectedWorksheet = -1; // Needed when remaining messages in queue are coming after close
    MemPtrFree(worksheet_note);
}

/***********************************************************************
 *
 * FUNCTION:     slv_worksheet_name
 *
 * DESCRIPTION:  Get name of worksheet on the current database position
 *
 * PARAMETERS:   pos - position in the database
 *
 * RETURN:       MemPtrNew()'d name of the database
 *
 ***********************************************************************/
static TCHAR *slv_worksheet_name(Int16 pos) IFACE;
static TCHAR *slv_worksheet_name(Int16 pos) {
    MemHandle recordHandle;
    struct dbworksheet *record;
    TCHAR *result;

    recordHandle = DmQueryRecord(slv_gDB, pos);
    record = (struct dbworksheet *) MemHandleLock(recordHandle);
    result = (TCHAR *) MemPtrNew((StrLen(record->title)+1)*sizeof(TCHAR));
    StrCopy(result, record->title);
    MemHandleUnlock(recordHandle);

    return (result);
}

/***********************************************************************
 *
 * FUNCTION:     slv_save_worksheet_vars
 *
 * DESCRIPTION:  Create new worksheet and save it to database. If the
 *               itemnum is specified, replace the worksheet number 'itemnum'
 *               by the worksheet being saved
 *
 * PARAMETERS:
 *
 * RETURN:       New record (worksheet) position in datbase
 *
 ***********************************************************************/
static Int16 slv_save_worksheet_vars (Int16 itemnum, TCHAR *title, TCHAR *equation,
                     double min, double max, double prec,
                     TCHAR *note) IFACE;
static Int16 slv_save_worksheet_vars (Int16 itemnum, TCHAR *title, TCHAR *equation,
                     double min, double max, double prec,
                     TCHAR *note) {
    struct dbworksheet *newrecord, *newrecordptr;
    MemHandle myRecordMemHandle;
    UInt16 recordNumber, size;

    if (equation)
        size = sizeof(*newrecord) + (StrLen(equation)+1)*sizeof(TCHAR)
               + (StrLen(note)+1)*sizeof(TCHAR);
    else
        size = sizeof(*newrecord) + 1*sizeof(TCHAR)
               + (StrLen(note)+1)*sizeof(TCHAR);
    newrecord = (struct dbworksheet *) MemPtrNew(size);
    StrCopy(newrecord->title, title);
    if (equation)
        StrCopy(newrecord->equation, equation);
    else
        newrecord->equation[0] = _T('\0');
    StrCopy(newrecord->equation+StrLen(newrecord->equation)+1, note);
    newrecord->min = min;
    newrecord->max = max;
    newrecord->prec = prec;

    if (itemnum != -1) {
        /* Delete existing record */
        DmRemoveRecord(slv_gDB, itemnum);
    } else { // No item num specified, verify on name
        recordNumber = DmFindSortPosition(slv_gDB, newrecord, 0,
                                          (DmComparF *) slv_compare_record,
                                          0);
        if (recordNumber > 0) {  /* We might modify the record */
            struct dbworksheet *record;
            MemHandle recordMemHandle;
            Int16 foundIt;

            recordMemHandle = DmQueryRecord(slv_gDB, recordNumber-1);
            record = (struct dbworksheet *) MemHandleLock(recordMemHandle);
            foundIt = (StrCompare(newrecord->title, record->title) == 0);
            MemHandleUnlock(recordMemHandle);
            if (foundIt) {
                /* Delete existing record */
                DmRemoveRecord(slv_gDB, recordNumber-1);
            }
        }
    }
    /* Create a new one */
    recordNumber = DmFindSortPosition(slv_gDB, newrecord, 0,
                                      (DmComparF *) slv_compare_record,
                                      0);
    myRecordMemHandle = DmNewRecord(slv_gDB, &recordNumber, size);
    newrecordptr = (struct dbworksheet *) MemHandleLock(myRecordMemHandle);
    DmWrite(newrecordptr, 0, newrecord, size);
    MemHandleUnlock(myRecordMemHandle);
    DmReleaseRecord(slv_gDB, recordNumber, true); /* Now the recordNumber contains real index */
    MemPtrFree(newrecord);
    return (recordNumber);
}

/***********************************************************************
 *
 * FUNCTION:     slv_save_worksheet
 *
 * DESCRIPTION:  Saves current selected worksheet to database
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       New worksheet position in database
 *
 ***********************************************************************/
Int16 slv_save_worksheet (void) IFACE;
Int16 slv_save_worksheet (void) {
    TCHAR *eq;

    if (slv_selectedWorksheet == -1)
        return (-1);

    eq = SlvEqFldGet();
    Int16 rc = slv_save_worksheet_vars(slv_selectedWorksheet, worksheet_title,
                                       eq, worksheet_min, worksheet_max,
                                       worksheet_prec, worksheet_note);
    MemPtrFree(eq);
    return (rc);
}

/***********************************************************************
 *
 * FUNCTION:     slv_new_worksheet
 *
 * DESCRIPTION:  Create a new blank worksheet with given name. If such
 *               worksheet already exist, do not overwrite it and instead
 *               switch to the old one
 *
 * PARAMETERS:   name of the new worksheet
 *
 * RETURN:       Worksheet position in database
 *
 ***********************************************************************/
Int16 slv_new_worksheet (TCHAR *name) IFACE;
Int16 slv_new_worksheet (TCHAR *name) {
    UInt16 recordNumber;
    MemHandle myRecordMemHandle;
    struct dbworksheet *newrecord, *newrecordptr;
    UInt16 size;

    size = sizeof(*newrecord) + 2*sizeof(TCHAR);
    newrecord = (struct dbworksheet *) MemPtrNew(size);
    StrCopy(newrecord->title, name);
    newrecord->equation[0] = _T('\0');
    newrecord->equation[1] = _T('\0');
    newrecord->min  = DEFAULT_MIN;
    newrecord->max  = DEFAULT_MAX;
    newrecord->prec = DEFAULT_ERROR;

    recordNumber = DmFindSortPosition(slv_gDB, newrecord, 0,
                                      (DmComparF *) slv_compare_record,
                                      0);
    if (recordNumber > 0) {  /* We might modify the record */
        struct dbworksheet *record;
        MemHandle recordMemHandle;
        Int16 foundIt;

        recordMemHandle = DmQueryRecord(slv_gDB, recordNumber-1);
        record = (struct dbworksheet *) MemHandleLock(recordMemHandle);
        foundIt = (StrCompare(newrecord->title, record->title) == 0);
        MemHandleUnlock(recordMemHandle);
        if (foundIt) {
            /* Return position */
            MemPtrFree(newrecord);
            return (recordNumber-1);
        }
    }
    myRecordMemHandle = DmNewRecord(slv_gDB, &recordNumber, size);
    newrecordptr = (struct dbworksheet *) MemHandleLock(myRecordMemHandle);
    DmWrite(newrecordptr, 0, newrecord, size);
    MemHandleUnlock(myRecordMemHandle);
    DmReleaseRecord(slv_gDB, recordNumber, true); /* Now the recordNumber contains real index */
    MemPtrFree(newrecord);
    return (recordNumber);
}

/***********************************************************************
 *
 * FUNCTION:     slv_comp_text
 *
 * DESCRIPTION:  Compute text and expect it executes into double
 *
 * PARAMETERS:   text <- input, value <- output
 *
 * RETURN:       error
 *
 ***********************************************************************/
static CError slv_comp_text (TCHAR *text, double *value) IFACE;
static CError slv_comp_text (TCHAR *text, double *value) {
    CError err;
    CodeStack *stack;

    stack = text_to_stack(text, &err);
    if (!err) {
        err = stack_compute(stack);
        if (!err)
            err = stack_get_val(stack, (void *)value, real);
        stack_delete(stack);
    }
    return err;
}


/***********************************************************************
 *
 * FUNCTION:     slv_comp_field
 *
 * DESCRIPTION:  Computes the 'real' value of a text field
 *
 * PARAMETERS:   objectID - id of the field
 *
 * RETURN:       CError - 0 on success
 *               *value - the computed value
 *
 ***********************************************************************/
Boolean slv_comp_field (TCHAR *text, double *value) IFACE;
Boolean slv_comp_field (TCHAR *text, double *value) {
    CError err;

    err = slv_comp_text(text, value);
    if (err)
        FrmAlert(altGraphBadVal, NULL);

    return ((err != 0) || !finite(*value));
}


/***********************************************************************
 *
 * FUNCTION:     slv_set_field
 *
 * DESCRIPTION:  Set the display field to the number value
 *
 * PARAMETERS:   objectID - Id of the control
 *               newval - value
 *               frm - form pointer
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
//static void slv_set_field(UInt16 objectID,double newval,FormPtr frm) IFACE;
//static void
//slv_set_field(UInt16 objectID,double newval,FormPtr frm)
//{
//    FieldPtr pole;
//    char *text;
//
//    pole = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,objectID));
//
//    text = display_real(newval);
//
//    FldDelete(pole,0,FldGetTextLength(pole));
//    FldInsert(pole,text,StrLen(text));
//    MemPtrFree(text);
//}


/***********************************************************************
 *
 * FUNCTION:     slv_select_worksheet
 *
 * DESCRIPTION:  Handle the worksheet selection popup menu
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       New selected worksheet number
 *
 ***********************************************************************/
void slv_select_worksheet (void) IFACE;
void slv_select_worksheet (void) {
    TCHAR **namelist;
    Int16 size, i;

    size = DmNumRecords(slv_gDB);
    namelist = (TCHAR **) MemPtrNew(sizeof(*namelist)*size);
    for (i=0 ; i<size ; i++)
        namelist[i] = slv_worksheet_name(i);

    /* The list is sorted alphabetically */
    LstEditSetListChoices(SOLVER_ID, namelist, size);
    if (slv_selectedWorksheet >= 0)
        LstEditSetLabel(SOLVER_ID, namelist[slv_selectedWorksheet]);
    else
        LstEditSetLabel(SOLVER_ID, _T(""));

    /* Free allocated space */
    for (i=0 ; i<size ; i++)
        MemPtrFree(namelist[i]);
    MemPtrFree(namelist);
}

/***********************************************************************
 *
 * FUNCTION:     slv_create_initial_note
 *
 * DESCRIPTION:  Create Note to the worksheet that contains a template
 *               with variable names and ':' so that the user can see,
 *               how to comment variables
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void slv_create_initial_note (void) {
    Int16 i;

    if (!varitems.size)
        return;

    StrCopy(worksheet_note, libLang->translate(_T("$$VARIABLE DESCR")));

    for (i=0 ; i<varitems.size ; i++) {
        StrCat(worksheet_note, varitems.list[i]);
#ifdef _WINDOWS
        StrCat(worksheet_note, _T(": \r\n"));
#else
        StrCat(worksheet_note, _T(": \n"));
#endif
    }
}

/***********************************************************************
 *
 * FUNCTION:     slv_write_txt
 *
 * DESCRIPTION:  Append text to the database at the current offset,
 *               increase offset
 *
 * PARAMETERS:   record - database record where to write
 *               text - the text
 *               offset - offset from where we should start writing
 *
 * RETURN:
 *
 ***********************************************************************/
static void slv_write_txt (MemPtr record, TCHAR *text,UInt16 *offset) IFACE;
static void slv_write_txt (MemPtr record, TCHAR *text,UInt16 *offset) {
    DmWrite(record, *offset, text, StrLen(text));
    *offset += StrLen(text);
}

/***********************************************************************
 *
 * FUNCTION:     slv_append_real
 *
 * DESCRIPTION:  Append real number in text representation to the string
 *
 * PARAMETERS:   str - where to print, num - what to print
 *
 * RETURN:
 *
 ***********************************************************************/
static void slv_append_real (TCHAR *str,double num) IFACE;
static void slv_append_real (TCHAR *str,double num) {
    TCHAR *text;

    text = display_real(num);
    StrCat(str, text);
    MemPtrFree(text);
}

/***********************************************************************
 *
 * FUNCTION:     slv_memo_import
 *
 * DESCRIPTION:  Import worksheet from the text identified by
 *               parameter. Note: this is not called from 'Solver',
 *               that's why we have to open/close the Solver database.
 *
 * PARAMETERS:   text
 *
 * RETURN:
 *
 ***********************************************************************/
Boolean slv_memo_import (TCHAR *text) {
    TCHAR title[MAX_WORKSHEET_TITLE+1];
    TCHAR *tmp, *lineend, *tmp1;
    double min, max, prec;
    CError err;
    Int16 res;
    int size;

    /* Get the title */
    tmp = StrChr(text, _T('\n'));
    if (!tmp)
        return (false);
    tmp1 = tmp+1;
    size = (tmp-text);
    if (*(tmp-1) == _T('\r'))
         size--;
    if (size > MAX_WORKSHEET_TITLE)
        return (false);
    memcpy(title, text, size*sizeof(TCHAR));
    title[size] = _T('\0');
    text = tmp1;

    /* Now we should get the numbers */
    lineend = StrChr(text, _T('\n'));
    if (!lineend)
        return (false);
    tmp1 = lineend+1;
    if (*(lineend-1) == _T('\r'))
         lineend--;
    /* Skip the colon */
    text++;
    /* Skip the spaces */
    while (*text == _T(' '))
        text++;
    /* Get min value */
    tmp = StrChr(text, _T(' '));
    if (tmp && (tmp < lineend)) {
        *tmp = _T('\0');
        err = slv_comp_text(text, &min);
        if (err)
            return (false);
        text = tmp+1;
        /* Get max value */
        while (*text == _T(' '))
            text++;
        tmp = StrChr(text, _T(' '));
        if (!tmp || (lineend < tmp))
            return (false);
        *tmp = _T('\0');
        err = slv_comp_text(text, &max);
        if (err)
            return (false);
        text = tmp+1;
        /* Get prec value */
        while (*text == _T(' '))
            text++;
        tmp = StrChr(text, _T(' '));
        if (!tmp || (lineend < tmp))
            tmp = lineend;
        *tmp = _T('\0');
        err = slv_comp_text(text, &prec);
        if (err)
            return (false);
        if (min >= max)
            return (false);
    } else {
        min  = DEFAULT_MIN;
        max  = DEFAULT_MAX;
        prec = DEFAULT_ERROR;
    }
    text = tmp1;
    tmp = StrChr(text, _T('\n'));
    if (!tmp)
        tmp = _T("");
    else {
        tmp[0] = _T('\0');
        tmp++;
    }
    /* Now we have equation in 'text' and not in 'tmp'(or \0 if no note) */
    slv_db_open();
    res = slv_save_worksheet_vars(-1, title, text, min, max, prec, tmp);
    slv_db_close();
    if (res == -1)
        return (false);
    return (true);
}

/***********************************************************************
 *
 * FUNCTION:     slv_export_memo
 *
 * DESCRIPTION:  Export worksheet to MemoPad
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - OK, false - otherwise
 *
 ***********************************************************************/
Boolean slv_export_memo (FILE *f, TCHAR *separator) IFACE;
Boolean slv_export_memo (FILE *f, TCHAR *separator) {
//#ifdef _WINDOWS
//    TCHAR *newline = _T("\r\n");
//#else
    TCHAR *newline = _T("\n");
//#endif
    int                 size;
    TCHAR               configuration[100];
    struct dbworksheet *record;
    MemHandle           recordHandle;
    TCHAR              *noteptr;


    slv_db_open();

    int numWorksheets = DmNumRecords(slv_gDB);
    for (int i=0 ; i<numWorksheets ; i++) {
        recordHandle = DmQueryRecord(slv_gDB, i);
        record = (struct dbworksheet *) MemHandleLock(recordHandle);

        StrCopy(configuration, _T(":"));
        slv_append_real(configuration, record->min);
        StrCat(configuration, _T(" "));
        slv_append_real(configuration, record->max);
        StrCat(configuration, _T(" "));
        slv_append_real(configuration, record->prec);

        size = StrLen(record->equation) + 1;
        noteptr = record->equation + size;

        _fputts(record->title, f);
        _fputts(newline, f);
        _fputts(configuration, f);
        _fputts(newline, f);
        _fputts(record->equation, f);
        _fputts(newline, f);
        _fputts(noteptr, f);
        size = StrLen(noteptr);
        if (size && (noteptr[size-1] != _T('\n'))) // Add a '\n' if needed
            _fputts(newline, f);

        MemHandleUnlock(recordHandle);

        _fputts(separator, f);
        _fputts(newline, f);
    }

    slv_db_close();

    return (true);
}
