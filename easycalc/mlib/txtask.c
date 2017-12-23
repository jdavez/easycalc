/*
 *   $Id: txtask.c,v 1.6 2006/09/16 23:30:58 tvoverbe Exp $
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

#include <PalmOS.h>
#include "segment.h"
#include "calcrsc.h"
#include "konvert.h"
#include "calc.h"
#include "funcs.h"
#include "display.h"
#include "stack.h"
#include "txtask.h"

CError
txtask_ask(Functype *func, CodeStack *stack)
{
	FormPtr frm;
	Int16 button;
	char *title,*text = NULL;
	FieldPtr field;
	CError err;
	Trpn tmpitem;
	CodeStack *tmpstack;
	Boolean res = 0;
	
	if (func->paramcount == 1) {
		/* Do not show dialog on graphs */
		if (DisableInput)
			return c_interrupted;
		err = stack_get_val(stack,&title,string);
		if (err)
			return err;
	} else if (func->paramcount == 2) {
		tmpitem = stack_pop(stack);
		err = stack_get_val(stack,&title,string);
		if (err) {
			rpn_delete(tmpitem);
			return err;
		}
		/* Return default value if graphing */
		if (DisableInput) {
			MemPtrFree(title);
			stack_push(stack,tmpitem);
			return c_noerror;
		}
		/* If it is variable, try to eval it, if it doesn't exist,
		 * ignore it */
		if (tmpitem.type==variable) 
			err = rpn_eval_variable(&tmpitem,tmpitem);
		if (!err)
			text = display_default(tmpitem,true);
		rpn_delete(tmpitem);
	} else
	    return c_badargcount;

	frm = FrmInitForm(dinputForm);
	FrmSetTitle(frm,title);
	FrmSetFocus(frm,FrmGetObjectIndex(frm,dinputField));
	field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,dinputField));

	if (text) {
		FldInsert(field,text,StrLen(text));
		FldSetSelection(field,0,StrLen(text));
		MemPtrFree(text);
	}

	while (1) {
		button = FrmDoDialog(frm);
		if (button == dinputOK) {
			text = FldGetTextPtr(field);
			if (!text)
				FrmAlert(altCompute);
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
	MemPtrFree(title);

	if (res) {
		stack_push(stack,tmpitem);
		return c_noerror;
	}
	return c_interrupted;
}
