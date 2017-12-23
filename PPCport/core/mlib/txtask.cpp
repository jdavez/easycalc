/*
 *   $Id: txtask.cpp,v 1.2 2009/12/15 21:37:44 mapibid Exp $
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
#include "compat/segment.h"
//#include "calcrsc.h"
#include "konvert.h"
#include "core/calc.h"
#include "funcs.h"
#include "display.h"
#include "stack.h"
#include "txtask.h"
#include "system - UI/EasyCalc.h"

CError txtask_ask (Functype *func, CodeStack *stack) {
    t_asktxt_param param;
    TCHAR *title, *text = NULL;
    CError err;
    Trpn tmpitem;
    CodeStack *tmpstack;
    Boolean res = 0;

    if (func->paramcount == 1) {
        /* Do not show dialog on graphs */
        if (DisableInput)
            return (c_interrupted);
        err = stack_get_val(stack, &title, string);
        if (err)
            return (err);
    } else if (func->paramcount == 2) {
        tmpitem = stack_pop(stack);
        err = stack_get_val(stack, &title, string);
        if (err) {
            rpn_delete(tmpitem);
            return (err);
        }
        /* Return default value if graphing */
        if (DisableInput) {
            MemPtrFree(title);
            stack_push(stack, tmpitem);
            return (c_noerror);
        }
        /* If it is variable, try to eval it, if it doesn't exist,
         * ignore it */
        if (tmpitem.type==variable)
            err = rpn_eval_variable(&tmpitem, tmpitem);
        if (!err)
            text = display_default(tmpitem, true, NULL);
        rpn_delete(tmpitem);
    } else
        return (c_badargcount);

    param.asktxt = title;
    param.defaultvalue = text;
    res = popupAskTxt(&param);
    if (text)
        MemPtrFree(text);

    if (res) {
        text = param.answertxt;
        if (!text)
            FrmAlertMain(altCompute);
        else {
            tmpstack = text_to_stack(text,&err);
            if (!err) {
                err = stack_compute(tmpstack);
                if (!err)
                    tmpitem = stack_pop(tmpstack);
                stack_delete(tmpstack);
            }
            if (err)
                alertErrorMessage(err);
        }
        MemPtrFree(text);
    }
    MemPtrFree(title);

    if (res) {
        stack_push(stack, tmpitem);
        return (c_noerror);
    }
    return (c_interrupted);
}
