/*
 *   $Id: defmgr.c,v 1.15 2006/09/12 19:40:55 tvoverbe Exp $
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
*/

#include <PalmOS.h>
#include <WinGlue.h>
#include "konvert.h"
#include "calcDB.h"
#include "defmgr.h"
#include "defuns.h"
#include "display.h"
#include "stack.h"
#include "calc.h"
#include "calcrsc.h"
#include "varmgr.h"
#include "calc.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static dbList *varItems=NULL;
static char HORIZ_ELLIPSIS;
static Int16 selectedLine = 0;

#define VARNAME_WIDTH  42

/***********************************************************************
 *
 * FUNCTION:     def_draw_item
 * 
 * DESCRIPTION:  Custom drawing function for list, it draws the line
 *               as a table of 2 item, separated by space,
 *               computes items on-the-fly, because it is faster
 *               then allocating the whole thing in advance 
 *               and consumes less memory
 *
 * PARAMETERS:   read PalmOS Reference
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void def_draw_item(Int16 itemNum,RectangleType *bounds,
						  Char **itemText) IFACE;
static void
def_draw_item(Int16 itemNum,RectangleType *bounds,
			  Char **itemText)
{
	Int16 maxwidth,len;
	char *text;	
	CError err;
	Trpn item;
	RectangleType rp,nrp;
	
	WinGetClip(&rp);
	nrp = *bounds;
	nrp.extent.x = VARNAME_WIDTH;
	WinSetClip(&nrp);
	if (varItems->type[itemNum]==function) {
		text = MemPtrNew(MAX_FUNCNAME+10);
		StrCopy(text,itemText[itemNum]);
		StrCat(text,"()");
		WinGlueDrawTruncChars(text,StrLen(text),
				  bounds->topLeft.x,bounds->topLeft.y,
				  VARNAME_WIDTH);
		MemPtrFree(text);
		
		err = db_func_description(varItems->list[itemNum],&text,NULL);
		if (err) 
		  text = print_error(err);
	}
	else {
		WinDrawChars(itemText[itemNum],StrLen(itemText[itemNum]),
					 bounds->topLeft.x,bounds->topLeft.y);
		
		item = db_read_variable(varItems->list[itemNum],&err);
		if (!err) {
			text = display_default(item,false);
			rpn_delete(item);
		}
		else
		  text = print_error(err);
	}
	WinSetClip(&rp);
	
	maxwidth = bounds->extent.x - VARNAME_WIDTH-1;

	len = StrLen(text);
	/* Umm.. FntDrawTruncChars is a PalmOS3.1 feature :-( */
	if (FntCharsWidth(text,len)>maxwidth) {
		while (FntCharsWidth(text,len)>maxwidth)
		  len--;
		text[len-1]=HORIZ_ELLIPSIS;
	}	
	WinDrawChars(text,len,bounds->topLeft.x+VARNAME_WIDTH+1,
				 bounds->topLeft.y);
	MemPtrFree(text);
}

/***********************************************************************
 *
 * FUNCTION:     def_init_varlist
 * 
 * DESCRIPTION:  Initialize the on-screen list with names of variables
 *               values get computed on-the-fly while drawing list
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
def_init_varlist(void) IFACE;
static void
def_init_varlist(void)
{
	ListPtr list;
	
	varItems = db_get_list(notype);
	if (!varItems->size) {
		db_delete_list(varItems);
		varItems = NULL;
		LstSetListChoices(GetObjectPtr(defList),NULL,0);
		return;
	}
	list = GetObjectPtr(defList);
	LstSetListChoices(list,varItems->list,varItems->size);
	LstSetDrawFunction(list,def_draw_item);
}

/***********************************************************************
 *
 * FUNCTION:     def_destroy_varlist
 * 
 * DESCRIPTION:  Free dynamically allocated structures for
 *               the onscreen field
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
def_destroy_varlist(void) IFACE;
static void
def_destroy_varlist(void)
{
	if (!varItems)
	  return;
	
	db_delete_list(varItems);
	varItems = NULL;
}

static Boolean
def_modify(Int16 i) IFACE;
static Boolean
def_modify(Int16 i)
{
	char tmptxt[MAX_RSCLEN];

	if (varItems->type[i] == function) {
		SysCopyStringResource(tmptxt,varFuncModTitle);
		return varmgr_edit(varItems->list[i],tmptxt,
				   function,false,NULL);
	}
	else {
		SysCopyStringResource(tmptxt,varVarModTitle);
		return varmgr_edit(varItems->list[i],tmptxt,
				     variable,false,NULL);
	}
}

/***********************************************************************
 *
 * FUNCTION:     DefmgrHandleEvent
 * 
 * DESCRIPTION:  Event handler for Definition manager main window
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean 
DefmgrHandleEvent(EventPtr event)
{
	Boolean handled = true;
	FormPtr frm;
	Int16 controlID;
	Int16 i;
	Boolean update;
	char tmptxt[MAX_RSCLEN];

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	 case keyDownEvent: {
		 ListPtr list;
		 
		 frm = FrmGetActiveForm();
		 list = GetObjectPtr(defList);
		 switch (event->data.keyDown.chr) {
		  case pageUpChr: 
			 FrmHideObject(frm,FrmGetObjectIndex(frm,defList));
			 LstScrollList(list,winUp,LstGetVisibleItems(list)-1);
			 FrmShowObject(frm,FrmGetObjectIndex(frm,defList));
			 break;
		  case pageDownChr:
			 FrmHideObject(frm,FrmGetObjectIndex(frm,defList));
			 LstScrollList(list,winDown,LstGetVisibleItems(list)-1);
			 FrmShowObject(frm,FrmGetObjectIndex(frm,defList));
			 break;
		 default:
			 handled = false;
			 break;
		 }
		 break;
	 }
	case lstSelectEvent:
		if (selectedLine == event->data.lstSelect.selection) {
			if (def_modify(selectedLine))
				FrmUpdateForm(defForm,frmUpdateVars);
		} else
			selectedLine = event->data.lstSelect.selection;
		break;
	 case ctlSelectEvent:
		controlID = event->data.ctlSelect.controlID;
		switch (controlID) {
		 case defDone:
			def_destroy_varlist();
			FrmReturnToForm(0);
			break;
		 case defModify:
			i = LstGetSelection(GetObjectPtr(defList));
			if (i != noListSelection) 
				if (def_modify(i))
					FrmUpdateForm(defForm,frmUpdateVars);
			break;
		 case defNew:
			i = LstPopupList(GetObjectPtr(defPopupList));
			update = false;
			if (i==0) { /* User has choosen 'function' */
				SysCopyStringResource(tmptxt,varFuncNewTitle);
				update = varmgr_edit("",tmptxt,function,true,NULL);
			}
			else if (i==1) { /* User has choosen 'variable' */
				SysCopyStringResource(tmptxt,varVarNewTitle);
				update = varmgr_edit("",tmptxt,variable,true,NULL);
			}
			
			if (update)
			  FrmUpdateForm(defForm,frmUpdateVars);
			  
			break;
		 case defDelete:
			i = LstGetSelection(GetObjectPtr(defList));
			
			if (i == noListSelection)
			  break;
			if (varItems->type[i] == variable)
			  SysCopyStringResource(tmptxt,varStrVariable);
			else
			  SysCopyStringResource(tmptxt,varStrFunction);
			if (!FrmCustomAlert(altConfirmDelete,tmptxt,
					    varItems->list[i],NULL)) {
				db_delete_record(varItems->list[i]);
				FrmUpdateForm(defForm,frmUpdateVars);
			}
			break;
		default:
			handled = false;
			break;
		}
		break;
	 case frmOpenEvent:
		ChrHorizEllipsis(&HORIZ_ELLIPSIS);
		frm = FrmGetActiveForm();
		
		def_init_varlist();
		selectedLine = 0;
		
		FrmDrawForm(frm);
		
		break;		
	 case frmCloseEvent:
		def_destroy_varlist();
		/* FrmHandleEvent must handle this event itself */
		handled = false;
		break;
	 case frmUpdateEvent:
		if (event->data.frmUpdate.updateCode == frmUpdateVars) {
			def_destroy_varlist();
			def_init_varlist();
			LstDrawList(GetObjectPtr(defList));
		} else
			handled = false;
		break;
	default:
		handled = false;
		break;
	}
	
	return handled;
}
