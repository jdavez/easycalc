/*   
 * $Id: varmgr.c,v 1.23 2006/09/16 23:30:58 tvoverbe Exp $
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

#include <PalmOS.h>
#include <LstGlue.h>
#include <WinGlue.h>

#include "konvert.h"
#include "calcDB.h"
#include "display.h"
#include "varmgr.h"
#include "calcrsc.h"
#include "stack.h"
#include "calc.h"
#include "funcs.h"
#include "history.h"
#include "defuns.h"
#include "main.h"

#ifdef HANDERA_SDK
#include "Vga.h"
#endif
#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

/***********************************************************************
 *
 * FUNCTION:     hist_draw_item
 *
 * DESCRIPTION:  Callback function for drawing history list
 *
 * PARAMETERS:   See PalmOS Reference
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void hist_draw_item(Int16 itemNum,RectangleType *bounds,
						   Char **itemText) IFACE;
static void hist_draw_item(Int16 itemNum,RectangleType *bounds,
						   Char **itemText)
{
	Trpn item;
	char *text;
	Boolean isrpn;
	UInt16 width,xoffset;

	isrpn = history_isrpn(itemNum);

	if (isrpn) {
		item = history_get_item(itemNum);
		text = display_default(item,false);
		rpn_delete(item);
		
		width = FntCharsWidth(text,StrLen(text));
		if (width > bounds->extent.x)
			xoffset = 0;
		else
			xoffset = bounds->extent.x - width;
	} else {
		text = history_get_line(itemNum);
		xoffset = 0;
	}
	
	WinGlueDrawTruncChars(text,StrLen(text),bounds->topLeft.x + xoffset,
			  bounds->topLeft.y,bounds->extent.x - xoffset);
	MemPtrFree(text);
}

/***********************************************************************
 *
 * FUNCTION:     history_popup
 * 
 * DESCRIPTION:  Displays a history list and returns a selected item
 *
 * PARAMETERS:   None
 *
 * RETURN:       history(n), where n is the item number
 *      
 ***********************************************************************/
char *
history_popup(void)
{
	ListPtr list;
	Int16 i;
	char *result;
	UInt16 numitems;
	Trpn item;
	
	numitems = history_total();
	if (numitems==0) 
	  return NULL;

	list = GetObjectPtr(histList);
	LstMakeItemVisible(list,0);
	LstSetListChoices(list,NULL,numitems);
	LstSetDrawFunction(list,hist_draw_item);
	LstSetHeight(list,
				 numitems>11?11:numitems);
	LstSetSelection(list,0);
	i = LstPopupList(list);
	
	if (i==noListSelection)
	  result = NULL;
	else {
		if (history_isrpn(i)) {
			item = history_get_item(i);
			result = display_default(item,true);
			rpn_delete(item);
			if (StrLen(result) > 25) 
				StrPrintF(result,"history(%d)",i);
		} else 
			result = history_get_line(i);
//		result = MemPtrNew(15); /* history(xxxx) */
//		StrPrintF(result,"history(%d)",i);
	}
	return result;
}


/***********************************************************************
 *
 * FUNCTION:     varmgr_popup
 * 
 * DESCRIPTION:  Popups a menu with list of variables/functions
 *               and their values
 * 
 * PARAMETERS:   Type of what to display
 *
 * RETURN:       Name of selected variable/function or NULL
 *      
 ***********************************************************************/
char *
varmgr_popup(rpntype type)
{
	dbList *varlist;
	char **values;
	Trpn item;
	char *text;
	CError err;
	Int16 i;
	ListPtr list;
	
	varlist = db_get_list(type);
	if (varlist->size==0) {
		db_delete_list(varlist);
		return NULL;
	}
	values = MemPtrNew(varlist->size * sizeof(*values));
	for (i=0;i<varlist->size;i++) {
		if (type==variable) {
			item = db_read_variable(varlist->list[i],&err);
			if (!err) {
				text = display_default(item,false);
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
		values[i] = MemPtrNew(StrLen(varlist->list[i])+StrLen(text)+10);
		StrCopy(values[i],varlist->list[i]);
		if (type==function)
		  StrCat(values[i],"()");
		StrCat(values[i]," ->  ");
		StrCat(values[i],text);
		MemPtrFree(text);
	}
	
	list = GetObjectPtr(varList);
	LstSetListChoices(list,values,varlist->size);
	LstSetHeight(list,
				 varlist->size>MAX_LIST_LENGTH?MAX_LIST_LENGTH:varlist->size);
	/* We have it sorted alphabetically */
	LstGlueSetIncrementalSearch(list, true);
	i = LstPopupList(list);
	if (i==noListSelection)
	  text = NULL;
	else {
		text = MemPtrNew(StrLen(varlist->list[i])+1);
		StrCopy(text,varlist->list[i]);
	}
	  	
	for (i=0;i<varlist->size;i++)
	  MemPtrFree(values[i]);
	MemPtrFree(values);
	db_delete_list(varlist);
	
	return text;
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
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void varmgr_save_variable(void) IFACE;
static void
varmgr_save_variable()
{
	char *varname;
	Trpn item;
	CError err;
	
	varname = FldGetTextPtr(GetObjectPtr(varEntryField));
	if (!varname || !StrLen(varname)) {
		FrmAlert(altBadVariableName);
		return;
	}
	if (!varfunc_name_ok(varname,variable))
	  return;
	
	item = db_read_variable("ans",&err);
	if (err) {
		FrmAlert(altAnsProblem);
		return;
	}
	db_write_variable(varname,item);
	rpn_delete(item);
	
	FrmReturnToForm(0);
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_get_var
 * 
 * DESCRIPTION:  displays a small popup menu with names of variables
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static char * varmgr_get_var(void) IFACE;
static char *
varmgr_get_var(void)
{
	dbList *varlist;
	ListPtr list;
	Int16 i;
	char *result;
	
	varlist = db_get_list(variable);
	if (varlist->size==0) {
		db_delete_list(varlist);
		return NULL;
	}
	list = GetObjectPtr(varEntryList);
	LstSetListChoices(list,varlist->list,varlist->size);
	LstSetHeight(list,
				 varlist->size>5?5:varlist->size);
	LstGlueSetIncrementalSearch(list, true);
	i = LstPopupList(list);
	if (i==noListSelection)
	  result = NULL;
	else {
		result = MemPtrNew(StrLen(varlist->list[i])+1);
		StrCopy(result,varlist->list[i]);
	}
	db_delete_list(varlist);
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     VarmgrEntryHandleEvent
 * 
 * DESCRIPTION:  Event handler for varEntryForm, used to enter
 *               a name of variable to save a 'ans'
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean
VarmgrEntryHandleEvent(EventPtr event)
{
	Boolean handled = false;
	FormPtr frm;
	Int16 controlID;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	 case keyDownEvent:
		if (event->data.keyDown.chr=='\n') {
			varmgr_save_variable();
			handled=true;
		}
		break;
	 case ctlSelectEvent:
		controlID = event->data.ctlSelect.controlID;
		if (controlID == varEntryCancel) {
			FrmReturnToForm(0);
			handled = true;
			break;
		}
		if (controlID == varEntryOK) {
			varmgr_save_variable();
			handled = true;
			break;
		}
		if (controlID == varEntryListBut) {
			char *varname;
			FieldPtr field;
			
			handled = true;
			
			varname = varmgr_get_var();
			if (!varname)
			  break;
			field = GetObjectPtr(varEntryField);
			FldDelete(field,0,FldGetTextLength(field));
			FldInsert(field,varname,StrLen(varname));
			MemPtrFree(varname); 
			varmgr_save_variable();
			break;
		}
		break;
	 case frmOpenEvent:
		frm = FrmGetActiveForm();
		FrmDrawForm(frm);
		
		FrmSetFocus(frm,FrmGetObjectIndex(frm,varEntryField));
		
		handled = true;
		break;		
	 case frmCloseEvent:
		/* FrmHandleEvent must handle this event itself */
		handled = false;
		break;
	}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:     varmgr_edit_init
 * 
 * DESCRIPTION:  Initialize a form for Input of a value of a variable
 *
 * PARAMETERS:   frm - form, from which take the values, cannot use
 *                     FrmGetAcitveForm, because it is called using
 *                     FrmDoDialog
 *               others - same as for varmgr_edit
 *
 * RETURN:       true  - OK
 *               false - Error occured, i.e. out of memory
 *      
 ***********************************************************************/
static Boolean
varmgr_edit_init(FormPtr frm,char *varname,char *title,rpntype type,
				 Boolean editname,const char *parameter) IFACE;
static Boolean
varmgr_edit_init(FormPtr frm,char *varname,char *title,rpntype type,
				 Boolean editname,const char *parameter)
{
	char *text;
	char *fmtvalue;
	Trpn item;
	CError err;
	MemHandle namehandle;
	FieldPtr namefield,varfield;
	FieldAttrType fieldattr;

	FrmSetTitle(frm,title);
	
	varfield = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,varEditField));
	namefield = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,varEditNameField));
	
	namehandle = MemHandleNew(MAX_FUNCNAME+1);
	text = MemHandleLock(namehandle);
	StrCopy(text,varname);
	MemHandleUnlock(namehandle);
	
	FldGetAttributes(namefield,&fieldattr);
	if (editname) {
		fieldattr.editable = true;
		fieldattr.underlined = grayUnderline;
	}
	else {
		fieldattr.editable = false;
		fieldattr.underlined = noUnderline;
	}
	FldSetAttributes(namefield,&fieldattr);
	FldSetTextHandle(namefield,namehandle);
		
	if (type == variable) {
		item = db_read_variable(varname,&err);
		if (!err) {
			fmtvalue = display_default(item,true);
			if (!fmtvalue) {
				rpn_delete(item);
				return false;
			}
			if (StrLen(fmtvalue) > 100)
				FldSetMaxChars(varfield,StrLen(fmtvalue)+20);
			FldInsert(varfield,fmtvalue,StrLen(fmtvalue));
			FldSetSelection(varfield,0,StrLen(fmtvalue));
			MemPtrFree(fmtvalue);
			rpn_delete(item);
		}	
	} 
	else if (type == function) {
		FieldPtr paramfield;
		
		err = db_func_description(varname,&fmtvalue,parameter_name);
		if (!err) {
			FldInsert(varfield,fmtvalue,StrLen(fmtvalue));
			FldSetSelection(varfield,0,StrLen(fmtvalue));
			MemPtrFree(fmtvalue);
		}
		/* Show the parameter field */
		FrmShowObject(frm,FrmGetObjectIndex(frm,varEditParamLabel));
		FrmShowObject(frm,FrmGetObjectIndex(frm,varEditParam));
		paramfield = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,varEditParam));
		FldInsert(paramfield,parameter_name,StrLen(parameter_name));
	}
	if (editname)
	  FrmSetFocus(frm,FrmGetObjectIndex(frm,varEditNameField));
	else
	  FrmSetFocus(frm,FrmGetObjectIndex(frm,varEditField));

	return true;
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
static Boolean
varmgr_edit_save(FormPtr frm,const char *varname,char *title,rpntype type,
				 Boolean editname) IFACE;
static Boolean
varmgr_edit_save(FormPtr frm,const char *varname,char *title,rpntype type,
				 Boolean editname)
{
	CodeStack *stack;
	FieldPtr namefield,varfield,paramfield;
	CError err;
	Trpn result;
	char *text;
	char newvarname[MAX_FUNCNAME+1];

	namefield = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,varEditNameField));
	varfield = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,varEditField));
	paramfield = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,varEditParam));
	
	if (editname) {
		text = FldGetTextPtr(namefield);
		if (!text || !varfunc_name_ok(text,type)) {
			FrmAlert(altBadVariableName);
			return false;
		}
		if (db_record_exists(text) &&			
			FrmCustomAlert(altConfirmOverwrite,text,NULL,NULL)) 
		  return false;
		StrCopy(newvarname,text);
	}
	else
	  StrCopy(newvarname,varname);
	
	text = FldGetTextPtr(varfield);
	if (!text) {
		FrmAlert(altCompute);
		return false;
	}
	
	if (type==function) {
		char *param;
		
		param = FldGetTextPtr(paramfield);
		/* funcs_is_ok fails on default parameter_name */
		if (StrCompare(param,parameter_name)==0
			|| varfunc_name_ok(param,variable)) 
			StrCopy(parameter_name,param);
		else {
			FrmAlert(altBadParameter);
			return false;
		}
	}
	stack = text_to_stack(text,&err);
	
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
		db_write_variable(newvarname,result);
		rpn_delete(result);
		return true;
	}
	else if (type==function) {
		db_write_function(newvarname,stack,text);
		stack_delete(stack);
		return true;
	}
	return false;
}

static Boolean
varmgr_edit_handler(EventPtr event)
{
	Boolean handled = false;
	EventType newevent;
	UInt16 controlID;
	FieldPtr field;
	char *text;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	case ctlSelectEvent:
		controlID=event->data.ctlSelect.controlID;
		switch (controlID) {
		case varListButton:
			text = varmgr_popup(variable);
			if (text) {
				field = GetObjectPtr(varEditField);
				FldInsert(field, text, StrLen(text));
				MemPtrFree(text);
			}
			handled = true;
			break;
		case varFuncListButton:
		case varBuiltinListButton:
			if (controlID == varFuncListButton)
				text = varmgr_popup(function);
			else
				text = varmgr_popup_builtin();
			if (text) {
				main_insert(varEditField,text,false,
					    true,false,NULL);
				MemPtrFree(text);
			}
			handled = true;
			break;
		}
		break;
	case keyDownEvent:
		if (event->data.keyDown.chr=='\n') {
			/* Simulate a press of a 'OK' button */
			newevent.eType = ctlSelectEvent;
			newevent.penDown =false;
			/* others are not important */
			newevent.data.ctlSelect.controlID = varEditOK;
			newevent.data.ctlSelect.pControl = GetObjectPtr(varEditOK);
			newevent.data.ctlSelect.on = false;
			EvtAddEventToQueue(&newevent);
			handled=true;
		}
		break;
	}
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_edit
 *
 * DESCRIPTION:  Popup a variable edit dialog
 *
 * PARAMETERS:   varname - name of the variable
 *               title - title of the new form
 *               type - function or variable
 *               callback - function, that should be called
 *                          upon succesful save (usually update)
 *               editname - user can edit the variable name and can
 *                          and can create a new variable/function
 *
 * RETURN:       true - variable was changed
 *               false - user pressed cancel
 *      
 ***********************************************************************/
Boolean
varmgr_edit(const char *varname,char *title,rpntype type,Boolean editname,
			const char *parameter)
{
	FormPtr frm;
	Int16 button;
	char myvarname[MAX_FUNCNAME+1];
	char oldparam[MAX_FUNCNAME+1];
	
	/* Assume, that varname can change because the event
	 * handler of another form can be called when 
	 * FrmDoDialog ends - it always appears on debug ROM
	 */
	StrCopy(myvarname,varname); 

	StrCopy(oldparam,parameter_name);
	if (parameter)
	  StrCopy(parameter_name,parameter);
	/* parameter_name CAN be modified by varmgr_edit_init
	 * depending on the existing function */
	frm = FrmInitForm(varEditForm);
#ifdef HANDERA_SDK
	if (handera)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	if (varmgr_edit_init(frm,myvarname,title,type,editname,parameter)) {
		FrmSetEventHandler(frm,varmgr_edit_handler);
		do {
			button = FrmDoDialog(frm);
			if (button==varEditCancel)
				break;
			if (varmgr_edit_save(frm,myvarname,title,type,editname))
				break;
		}while(1);
	}
	else
		button = varEditCancel;
	StrCopy(parameter_name,oldparam);
	FrmDeleteForm(frm);

	if (button == varEditOK)
		return true;
	else
		return false;
}

Boolean
varmgr_get_complex(Complex *value,char *title)
{
	FormPtr frm;
	Int16 button;
	Boolean res;
	char *text;
	FieldPtr field;
	CodeStack *stack;
	CError err;
	Complex result;
	
	frm = FrmInitForm(dinputForm);
#ifdef HANDERA_SDK
	if (handera)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	FrmSetTitle(frm,title);
	FrmSetFocus(frm,FrmGetObjectIndex(frm,dinputField));
	field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,dinputField));

	text = display_complex(*value);
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
						err = stack_get_val(stack,&result,
								    complex);
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

	if (res)
		*value = result;
	
	return res;
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_get_double
 *
 * DESCRIPTION:  Popup a small form asking user for entering an expression
 *               and check the value (it must be a floting point)
 *
 * PARAMETERS:   title - title the form should have
 *
 * RETURN:       true - user pressed OK and the expression was valid
 *               false - user pressed Cancel
 *               value - the result of expression user entered
 *      
 ***********************************************************************/
Boolean
varmgr_get_double(double *value,char *title)
{
	FormPtr frm;
	Int16 button;
	Boolean res;
	char *text;
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
}

/***********************************************************************
 *
 * FUNCTION:     varmgr_get_varstring
 *
 * DESCRIPTION:  Popup a small form asking user for entering a variable
 *               name and check it before returning
 *
 * PARAMETERS:   title - title the form should have
 *
 * RETURN:       true - user pressed OK and varname was valid
 *               false - user pressed Cancel
 *               varname - the result of expression user entered
 *      
 ***********************************************************************/
Boolean
varmgr_get_varstring(char *varname,char *title)
{
	FormPtr frm;
	Int16 button;
	Boolean res;
	char *text;
	FieldPtr field;
	
	frm = FrmInitForm(dinputForm);
#ifdef HANDERA_SDK
	if (handera)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	FrmSetTitle(frm,title);
	FrmSetFocus(frm,FrmGetObjectIndex(frm,dinputField));
	field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,dinputField));
	while (1) {
		button = FrmDoDialog(frm);
		if (button == dinputOK) {
			text = FldGetTextPtr(field);
			if (!text || !varfunc_name_ok(text,variable))
				FrmAlert(altBadVariableName);
			else {
				StrCopy(varname,text);
				res = true;
				break;
			}
		}
		else {
			res = false;
			break;
		}
	}
	FrmDeleteForm(frm);
	
	return res;
}

/***********************************************************************
 *
 * FUNCTION:     varmger_popup_builtin
 *
 * DESCRIPTION:  Popup a list of builtin functions
 *
 * PARAMETERS:   None
 *
 * RETURN:       text - name of selected function
 *               NULL - user did not select anything
 *      
 ***********************************************************************/
char *
varmgr_popup_builtin(void)
{
	ListPtr list;
	char **builtins;
	Int16 count;
	char *text;
	Int16 i;
	
	builtins = konvert_get_sorted_fn_list(&count);
	
	list = GetObjectPtr(varBuiltinList);
	LstSetListChoices(list,builtins,count);
	/* We have it sorted alphabetically */
	LstGlueSetIncrementalSearch(list, true);
	i = LstPopupList(list);
	if (i==noListSelection)
	  text = NULL;
	else {
		text = MemPtrNew(StrLen(builtins[i])+1);
		StrCopy(text,builtins[i]);
	}
	  	
	MemPtrFree(builtins);
	
	return text;
}
