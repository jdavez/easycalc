/*
 * $Id: varmgr.cpp,v 1.7 2011/02/28 22:07:18 mapibid Exp $
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

#include "stdafx.h"
#include "compat/PalmOS.h"
//#include <LstGlue.h>
//#include <WinGlue.h>

#include "konvert.h"
#include "calcDB.h"
#include "display.h"
#include "varmgr.h"
//#include "calcrsc.h"
#include "stack.h"
//#include "calc.h"
#include "core/core_display.h"
#include "funcs.h"
#include "history.h"
#include "defuns.h"
#include "main.h"
#include "EasyCalc.h"

#ifdef HANDERA_SDK
#include "Vga.h"
#endif
#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

/***********************************************************************
 *
 * FUNCTION:     history_popup
 *
 * DESCRIPTION:  Displays a history list popup.
 *
 * PARAMETERS:   hWnd is a pointer at an OS specific structure describing
 *               the history popup list object.
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void history_popup (Skin *skin, void *hWnd) {
    int i;
    int numitems;
    bool isrpn;
    Trpn item;
    TCHAR *text;

    numitems = history_total();
    for (i=0 ; i<numitems ; i++) {
        isrpn = history_isrpn(i);
        if (isrpn) {
            item = history_get_item(i);
            text = display_default(item, false, NULL);
            rpn_delete(item);
        } else {
            text = history_get_line(i);
        }
        skin->historyAddActionPopup(text, hWnd);
        MemPtrFree(text);
    }
}


/***********************************************************************
 *
 * FUNCTION:     history_action
 *
 * DESCRIPTION:  Execute action from the history popup dialog.
 *
 * PARAMETERS:   hWnd_p     OS specific handle to the dialog window
 *               hWnd_calc  OS specific handle to the calculator window
 *
 * RETURN:       None
 *
 ***********************************************************************/
TCHAR *history_action(Skin *skin, void *hWnd_p, void *hWnd_calc, int selection) {
    TCHAR *result = NULL;

    // Execute the action
    if (history_isrpn(selection)) {
        Trpn item = history_get_item(selection);
        result = display_default(item, true, NULL);
        rpn_delete(item);
        if (_tcslen(result) > 25)
            StrPrintF(result,_T("history(%d)"),selection);
    } else
        result = history_get_line(selection);

    return (result);
}


/***********************************************************************
 *
 * FUNCTION:     varmgr_popup
 *
 * DESCRIPTION:  Popups a menu with list of variables/functions
 *               and their values
 *
 * PARAMETERS:   hWnd is a pointer at an OS specific structure describing
 *               the popup list object.
 *               type is the type of what to display.
 *
 * RETURN:       Name of selected variable/function or NULL
 *
 ***********************************************************************/
static dbList *varlist;
static TCHAR **values;
void varmgr_popup(Skin *skin, void *hWnd, rpntype type) {
    Trpn item;
    TCHAR *text;
    CError err;

    varlist = db_get_list(type);
    if (varlist->size==0) {
        db_delete_list(varlist);
        varlist = NULL;
        return;
    }

    values = (TCHAR **) MemPtrNew(varlist->size * sizeof(*values));
    for (int i=0 ; i<varlist->size ; i++) {
        if (type==variable) {
            item = db_read_variable(varlist->list[i],&err);
            if (!err) {
                text = display_default(item, false, NULL);
                rpn_delete(item);
            }
            else
              text = print_error(err);
        }
        else {
            err = db_func_description(varlist->list[i],&text,NULL);
            if (err)
              text = print_error(err);
        }
        values[i] = (TCHAR *) MemPtrNew((StrLen(varlist->list[i])+StrLen(text)+10)*sizeof(TCHAR));
        StrCopy(values[i], varlist->list[i]);
        if (type==function)
            StrCat(values[i], _T("()"));
        StrCat(values[i], _T(" ->  "));
        StrCat(values[i], text);
        MemPtrFree(text);
    }

    // Display the popup contents to get action
    skin->varMgrPopup(values, varlist->size, hWnd);
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_action
 *
 * DESCRIPTION:  Execute action from the varMgr popup dialog.
 *
 * PARAMETERS:   hWnd_p     OS specific handle to the dialog window
 *               hWnd_calc  OS specific handle to the calculator window
 *
 * RETURN:       None
 *
 ***********************************************************************/
TCHAR *varmgr_action(int selection) {
    TCHAR *text = NULL;

    if (selection >= 0) {
        text = (TCHAR *) MemPtrNew((StrLen(varlist->list[selection])+1)*sizeof(TCHAR));
        StrCopy(text, varlist->list[selection]);
    }
    if (varlist != NULL) {
        for (int i=0 ; i<varlist->size ; i++)
            MemPtrFree(values[i]);
        MemPtrFree(values);
        db_delete_list(varlist);
    }

    return (text);
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_save_variable
 *
 * DESCRIPTION:  Called by an OK button in varEntryForm,
 *               checks if the variable has a correct name and saves
 *               the 'ans' result into it
 *
 * PARAMETERS:   None
 *
 * RETURN:       false if could not save
 *
 ***********************************************************************/
static bool varmgr_save_variable(TCHAR *varname, void *hWnd_p,
                                 bool fromList, bool valueAns) IFACE;
static bool varmgr_save_variable(TCHAR *varname, void *hWnd_p,
                                 bool fromList, bool valueAns) {
    Trpn item;
    CError err;

    if (!varname || !StrLen(varname) || !varfunc_name_ok(varname,variable)) {
        FrmAlert(altBadVariableName, hWnd_p);
        return (false);
    }
    // If not coming from a drop down list, verify if variable
    // already exists, and if yes, ask for confirmation.
    if (!fromList
        && db_record_exists(varname)
        && FrmCustomAlert(altConfirmOverwrite, varname, NULL, NULL, hWnd_p))
        return (false);

    if (valueAns) {
        item = db_read_variable(_T("ans"),&err);
        if (err) {
            FrmAlert(altAnsProblem, hWnd_p);
            return (false);
        }
        db_write_variable(varname,item);
        rpn_delete(item);
    }

    return (true);
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_listVar
 *
 * DESCRIPTION:  displays a small popup menu with names of variables
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
void varmgr_listVar (Skin *skin, void *hWnd) IFACE;
void varmgr_listVar (Skin *skin, void *hWnd) {
    varlist = db_get_list(variable);
    if (varlist->size == 0) {
        db_delete_list(varlist);
        varlist = NULL;
        return;
    }

    // Display the popup contents to get action
    skin->varSavePopup(varlist->list, varlist->size, hWnd);
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_listVar_action
 *
 * DESCRIPTION:  Execute action from the listVar popup dialog.
 *
 * PARAMETERS:   hWnd_p     OS specific handle to the dialog window
 *               hWnd_calc  OS specific handle to the calculator window
 *
 * RETURN:       None
 *
 ***********************************************************************/
bool varmgr_listVar_action(TCHAR *text, void *hWnd_calc, bool saveasvar,
                           bool fromList, bool valueAns) {
    if (varlist) {
        db_delete_list(varlist);
        varlist = NULL;
    }
    if (saveasvar) {
        return (varmgr_save_variable(text, hWnd_calc, fromList, valueAns));
    }
    return (true);
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_popup_builtin
 *
 * DESCRIPTION:  Popup a list of builtin functions
 *
 * PARAMETERS:   None
 *
 * RETURN:       text - name of selected function
 *               NULL - user did not select anything
 *
 ***********************************************************************/
static TCHAR **builtins;
void varmgr_popup_builtin(Skin *skin, void *hWnd) {
    Int16 count;

    // Display the popup contents to get action
    builtins = konvert_get_sorted_fn_list(&count);
    skin->varMgrPopup(builtins, count, hWnd);
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_builtinAction
 *
 * DESCRIPTION:  Execute action from the builtin popup dialog.
 *
 * PARAMETERS:   
 *
 * RETURN:       None
 *
 ***********************************************************************/
TCHAR *varmgr_builtinAction(int selection) {
    TCHAR *text = NULL;

    if (selection >= 0) {
        text = (TCHAR *) MemPtrNew((StrLen(builtins[selection])+1)*sizeof(TCHAR));
        StrCopy(text, builtins[selection]);
    }
    MemPtrFree(builtins);

    return (text);
}


/***********************************************************************
 *
 * FUNCTION:     varmgr_getVarDef (ex varmgr_edit_init)
 *
 * DESCRIPTION:  Initialize a form for Input of a value of a variable
 *
 * PARAMETERS:   frm - form, from which take the values, cannot use
 *                     FrmGetActiveForm, because it is called using
 *                     FrmDoDialog
 *               others - same as for varmgr_edit
 *
 * RETURN:       true  - OK
 *               false - Error occured, i.e. out of memory
 *
 ***********************************************************************/
bool varmgr_getVarDef (TCHAR *varname, TCHAR **varDef) {
	Trpn   item;
	CError err;

    item = db_read_variable(varname, &err);
    if (!err) {
        *varDef = display_default(item, true, NULL);
        if (!(*varDef)) {
            rpn_delete(item);
            return false;
        }
        rpn_delete(item);
        return true;
    }	
    return false;
}

bool varmgr_getFctDef (TCHAR *fctname, TCHAR **fctDef, TCHAR *fctParam) {
	CError err;

    err = db_func_description(fctname, fctDef, fctParam);
    if (!err) {
        return true;
    }
    return false;
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_edit_save
 *
 * DESCRIPTION:  Recomputes and saves value written into input field
 *
 * PARAMETERS:   frm - form, from which take the values, cannot use
 *                     FrmGetAcitveForm, because it is called using
 *                     FrmDoDialog
 *               others - same as for varmgr_edit
 *
 * RETURN:       true on success
 *
 ***********************************************************************/
Boolean varmgr_edit_save(TCHAR *namefield, TCHAR *varfield, TCHAR *paramfield,
                         rpntype type, Boolean editname, void *hWnd_p) IFACE;
Boolean varmgr_edit_save(TCHAR *namefield, TCHAR *varfield, TCHAR *paramfield,
                         rpntype type, Boolean editname, void *hWnd_p) {
    CodeStack *stack;
    CError err;
    Trpn result;

    if (editname) {
        if (!namefield || !varfunc_name_ok(namefield, type)) {
            FrmAlert(altBadVariableName, hWnd_p);
            return false;
        }
        if (db_record_exists(namefield) &&
            FrmCustomAlert(altConfirmOverwrite, namefield, NULL, NULL, hWnd_p))
            return false;
    }

    if (!varfield) {
        FrmAlert(altCompute, hWnd_p);
        return false;
    }

    if (type==function) {
        /* funcs_is_ok fails on default parameter_name */
        if (StrCompare(paramfield,parameter_name)==0
            || varfunc_name_ok(paramfield,variable))
            StrCopy(parameter_name,paramfield);
        else {
            FrmAlert(altBadParameter, hWnd_p);
            return false;
        }
    }
    stack = text_to_stack(varfield,&err);

    if (err) {
        alertErrorMessage(err);
        return false;
    }
    if (type == variable) {
        err = stack_compute(stack);
        if (err) {
            alertErrorMessage(err);
            stack_delete(stack);
            return false;
        }
        result = stack_pop(stack);
        stack_delete(stack);

        if (result.type == variable) {
            err = rpn_eval_variable(&result,result);
            if (err) {
                rpn_delete(result);
                alertErrorMessage(err);
                return false;
            }
        }
        db_write_variable(namefield,result);
        rpn_delete(result);
        return true;
    } else if (type==function) {
        db_write_function(namefield,stack,varfield);
        stack_delete(stack);
        return true;
    }
    return false;
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_get_double
 *
 * DESCRIPTION:  Popup a small form asking user for entering an expression
 *               and check the value (it must be a floating point)
 *
 * PARAMETERS:   title - title the form should have
 *
 * RETURN:       true - user pressed OK and the expression was valid
 *               false - user pressed Cancel
 *               value - the result of expression user entered
 *
 ***********************************************************************/
/*Boolean
varmgr_get_double(double *value,TCHAR *title)
{
    FormPtr frm;
    Int16 button;
    Boolean res;
    TCHAR *text;
    FieldPtr field;
    CodeStack *stack;
    CError err;

    frm = FrmInitForm(dinputForm);
#ifdef HANDERA_SDK
    if (handera)
        VgaFormModify(frm, vgaFormModify160To240);
#endif
    FrmSetTitle(frm,title);
    FrmSetFocus(frm,FrmGetObjectIndex(frm,dinputField));
    field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,dinputField));

    text = display_real(*value);
    FldInsert(field,text,StrLen(text));
    FldSetSelection(field,0,StrLen(text));
    MemPtrFree(text);

    while (1) {
        button = FrmDoDialog(frm);
        if (button == dinputOK) {
            text = FldGetTextPtr(field);
            if (!text)
                FrmAlert(altCompute);
            else {
                stack = text_to_stack(text,&err);
                if (!err) {
                    err = stack_compute(stack);
                    if (!err)
                        err = stack_get_val(stack,(void *)value,real);
                    stack_delete(stack);
                }
                if (err)
                    alertErrorMessage(err);
                else {
                    res = true;
                    break;
                }
            }
        }
        else {
            res = false;
            break;
        }
    }
    FrmDeleteForm(frm);

    return res;
}*/
