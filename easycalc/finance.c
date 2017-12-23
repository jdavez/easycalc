/*
 *   $Id: finance.c,v 1.15 2006/09/16 23:30:58 tvoverbe Exp $
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

#include <PalmOS.h>

#include "defuns.h"
#include "prefs.h"
#include "calc.h"
#include "stack.h"
#include "finance.h"
#include "calcDB.h"
#include "funcs.h"
#include "varmgr.h"
#include "display.h"
#include "fp.h"

#include "calcrsc.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

/* Display mode for financial calculations */
static const TdispPrefs finDPrefs = { 5,true,disp_normal,disp_decimal,false};

const struct {
	UInt16 buttonID;
	UInt16 outputID;
	UInt16 inputID;
	char *var;
	char *description;
	char *begStr;
	char *endStr;
}inpFinForm[]= {
	  {finFVButton,finFVLabel,finFVSet,"fv","Future Value",
	   "fv=fin_fv(I:N:PV:PMT:PYR:1)",
	   "fv=fin_fv(I:N:PV:PMT:PYR:0)"},
	  {finPVButton,finPVLabel,finPVSet,"PV","Present Value",
	   "PV=fin_pv(I:N:PMT:fv:PYR:1)",
	   "PV=fin_pv(I:N:PMT:fv:PYR:0"},
	  {finPMTButton,finPMTLabel,finPMTSet,"PMT","Payment",
	   "PMT=fin_pmt(I:N:PV:fv:PYR:1)",
	   "PMT=fin_pmt(I:N:PV:fv:PYR:0)"},
	  {finNButton,finNLabel,finNSet,"N","Number of payments",
	   "N=fin_n(I:PV:PMT:fv:PYR:1)",
	   "N=fin_n(I:PV:PMT:fv:PYR:0)"},
	  {finIButton,finILabel,finISet,"I","Interest",
	   "I=fzero(-10:1000:\"fv-fin_fv(x:N:PV:PMT:PYR:1)\":1E-6",
	   "I=fzero(-10:1000:\"fv-fin_fv(x:N:PV:PMT:PYR:0\":1E-6"},
	  {finPYRButton,finPYRLabel,finPYRSet,"PYR","Number of payments per year",
	   "PYR=fzero(1E-1:366:\"fv-fin_fv(I:N:PV:PMT:x:1)\":1E-6",
	   "PYR=fzero(1E-1:366:\"fv-fin_fv(I:N:PV:PMT:x:0\":1E-6"},
	  {0,0,NULL,NULL}
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
static void fin_update_fields() IFACE;
static void
fin_update_fields()
{
	CError err;
	char text[MAX_FP_NUMBER+1];
	Trpn item;
	Int16 i;
	FormPtr frm = FrmGetActiveForm();
	
	for (i=0;inpFinForm[i].var!=NULL;i++) {
		item = db_read_variable(inpFinForm[i].var,&err);
		if (err || item.type!=real)
			SysCopyStringResource(text,strUndefined);
		else
			fp_print_double(text,item.u.realval);
		if (!err)
			rpn_delete(item);
		/* Show result on screen */
		FrmHideObject(frm,FrmGetObjectIndex(frm,inpFinForm[i].outputID));
		FrmCopyLabel(frm,inpFinForm[i].outputID,text);
		FrmShowObject(frm,FrmGetObjectIndex(frm,inpFinForm[i].outputID));
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
static void fin_compute(int pos,int begin) IFACE;
static void
fin_compute(int pos,int begin)
{
	CError err;
	CodeStack *stack;
	
	stack = text_to_stack(begin?inpFinForm[pos].begStr:inpFinForm[pos].endStr,
						  &err);
	
	if (err)
		return;
	
	wait_draw();
	err = stack_compute(stack);
	stack_delete(stack);	
	wait_erase();

	if (err)
		alertErrorMessage(err);
	
	FrmUpdateForm(finForm,frmUpdateVars);
//	FrmUpdateForm(finForm,frmRedrawUpdateCode);
	return;
}

/***********************************************************************
 *
 * FUNCTION:     FinHandleEvent
 * 
 * DESCRIPTION:  Event handler for financial calculator
 *               FinCalc always operates in non-complex,
 *               floating mode (see the finDPrefs structure)
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean
FinHandleEvent(EventPtr event)
{
	Boolean handled = false;
	FormPtr frm;
	Int16 controlID;
	Int16 i;
	static TdispPrefs oldprefs;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	 case ctlSelectEvent:
		controlID = event->data.ctlSelect.controlID;
		if (controlID == finDoneButton) {
			calcPrefs.finBegin = CtlGetValue(GetObjectPtr(finBeginButton));
			FrmReturnToForm(0);
			fp_set_prefs(oldprefs);
			handled = true;
			break;
		}
		for (i=0;inpFinForm[i].var!=NULL;i++) {
			if (inpFinForm[i].buttonID==controlID) {
				fin_compute(i,CtlGetValue(GetObjectPtr(finBeginButton)));
				fin_update_fields();
				handled = true;
				break;
			}
			else if (inpFinForm[i].inputID==controlID) {
				if (varmgr_edit(inpFinForm[i].var,
								inpFinForm[i].description,
								variable,false,NULL))
				  fin_update_fields();
				handled = true;
				break;
			}
		}
		break;
	 case frmOpenEvent:
		oldprefs = fp_set_prefs(finDPrefs);
		
		frm = FrmGetActiveForm();
		FrmDrawForm(frm);
		CtlSetValue(GetObjectPtr(calcPrefs.finBegin?finBeginButton:
								 finEndButton),1);
		fin_update_fields();
		handled = true;
		break;
	 case frmCloseEvent:
		/* It seems to me that DefaultBtnId doesn't work
		 * on application exit, this shouldn't be called */
		calcPrefs.finBegin = CtlGetValue(GetObjectPtr(finBeginButton));
		fp_set_prefs(oldprefs);
		/* FrmHandleEvent must handle this event itself */
		handled = false;
		break;
	}
	
	return handled;
}
