/*
 *   $Id: memo.c,v 1.16 2006/09/12 19:40:55 tvoverbe Exp $
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
#include <string.h>

#include "konvert.h"
#include "calcDB.h"
#include "display.h"
#include "memo.h"
#include "stack.h"
#include "calcrsc.h"
#include "calc.h"
#include "prefs.h"
#include "funcs.h"
#include "result.h"
#include "solver.h"

#ifdef HANDERA_SDK
#include "Vga.h"
#endif
#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static DmOpenRef MemopadDB;
static UInt16 memoCategory;
static UInt16 *indexList;

/***********************************************************************
 *
 * FUNCTION:     strslashcat
 * 
 * DESCRIPTION:  Make the string 'quote-safe'
 *
 * PARAMETERS:   str1 - destination string
 *               str2 - source string
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
strslashcat(char *str1,char *str2)
{
	str1+=StrLen(str1);
	while ((*str1++=*str2++))
	  if (*str2=='\"' || *str2=='\\')
		*(str1++)='\\';
}

static char *
memo_dump_item(dbList *list,Int16 i)
{
	char *dump=NULL;
	char *text;
	CError err;
	Trpn item;
	char parameter[MAX_FUNCNAME+1];

	if (list->type[i]==function) {
		err=db_func_description(list->list[i],&text,parameter);
		if (err)
			return NULL;
		dump = MemPtrNew(StrLen(text)+StrLen(list->list[i])+30);
		dump[0] = '\0';
		/* Define the parameter */
		StrCat(dump,"defparamn(\"");
		StrCat(dump,parameter);
		StrCat(dump,"\")\n");
		
		StrCat(dump,list->list[i]);
		StrCat(dump,"()=\"");
		strslashcat(dump,text);
		StrCat(dump,"\"\n");
		MemPtrFree(text);
	} /* Do not export matrices for now */
	else if (list->type[i]==variable) {
		item = db_read_variable(list->list[i],&err);
		if (err)
			return NULL;
		/* Do not work with strings now */
		if (item.type == string) {
			rpn_delete(item);
			return NULL;
		}
		
		text = display_default(item,true);
		dump = MemPtrNew(StrLen(text)+StrLen(list->list[i])+5);
		StrCopy(dump,"");
		StrCat(dump,list->list[i]);
		StrCat(dump,"=");
		StrCat(dump,text);
		StrCat(dump,"\n");
		rpn_delete(item);
		MemPtrFree(text);
	}
	return dump;
}

static void
memo_new_record(char *text)
{
	MemPtr recP;
	MemHandle recH;
	Char null = '\0';
	UInt16 category;
	UInt16 attr;
	UInt16 index = dmMaxRecordIndex;
	char *header = "EasyCalc data structures\n";
	
	recH = DmNewRecord(MemopadDB, &index, StrLen(header)+StrLen(text)+1);
	if(recH != NULL) { 
		recP = MemHandleLock(recH); 
		DmWrite(recP,0,header,StrLen(header));
		DmWrite(recP,StrLen(header),text,StrLen(text));
		DmWrite(recP,StrLen(header) + StrLen(text),&null,1);
		MemHandleUnlock(recH);
		
		DmRecordInfo(MemopadDB, index, &attr, NULL, NULL);
		attr &= ~dmRecAttrCategoryMask;
		
		category = CategoryFind(MemopadDB, "EasyCalc");
		if (category == dmAllCategories)
			attr |= dmUnfiledCategory;
		else
			attr |= category;		
		
		DmSetRecordInfo(MemopadDB, index, &attr, NULL);
		
		DmReleaseRecord(MemopadDB,index,true);		
	}
}

/***********************************************************************
 *
 * FUNCTION:     memo_dump_db
 * 
 * DESCRIPTION:  Create a string dump of an EasyCalc's database
 *               containing definitions of variables and functions
 *
 * PARAMETERS:   type - function,variable or all
 *
 * RETURN:       pointer to a newly allocated memo-dump
 *      
 ***********************************************************************/
static void
memo_dump_db(rpntype type)
{
	char *text;
	dbList *list;
	Int16 i;
	char *dump;
	
	dump = MemPtrNew(MAX_DUMP+20);
	StrCopy(dump,"");

	list = db_get_list(type);
	
	/* Dump the list */
	for (i=0;i<list->size;i++) {
		text = memo_dump_item(list,i);
		if (!text)
			continue;
		if (StrLen(text) > MAX_DUMP) {
			MemPtrFree(text);
			continue;
		}
		if (StrLen(dump)+StrLen(text) > MAX_DUMP) {
			if (type != variable)
				StrCat(dump,"defparamn(\"x\")\n");
			memo_new_record(dump);
			StrCopy(dump,"");
		}
		StrCat(dump,text);
		MemPtrFree(text);
	}
	if (type != variable)
		StrCat(dump,"defparamn(\"x\")\n");
	memo_new_record(dump);

	db_delete_list(list);
	MemPtrFree(dump);
}

/***********************************************************************
 *
 * FUNCTION:     memo_dump
 * 
 * DESCRIPTION:  Dump an EasyCalc db to memopad
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
memo_dump()
{
	UInt16 mode = dmModeReadWrite;
	rpntype type;
	FormPtr frm;
	Int16 button;
	ControlPtr all,vars;

	
	frm = FrmInitForm(memoExportQuestionForm);
#ifdef HANDERA_SDK
	if (handera)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	all = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,memoExportAll));
	vars = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,memoExportVars));

	CtlSetValue(all,1);
	button = FrmDoDialog(frm);
	
	if (CtlGetValue(all))
		type = 0;
	else if (CtlGetValue(vars))
		type = variable;
	else
		type = function;
	FrmDeleteForm(frm);
	if (button == memoExportCancel) {
		return;
	}

	if((MemopadDB = DmOpenDatabaseByTypeCreator('DATA', 'memo', mode)) == NULL)
		return;
	
	memo_dump_db(type);
	
	DmCloseDatabase(MemopadDB);		
}

/***********************************************************************
 *
 * FUNCTION:     memo_draw_item
 * 
 * DESCRIPTION:  Custom draw function for importList, shows an appropriate
 *               title of a memo
 *
 * PARAMETERS:   See palmos referenece
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
memo_draw_item(Int16 itemNum,RectangleType *bounds,
			  Char **itemText)
{
	MemHandle recH;
	char *text;
	Int16 titlelen,i;
	RectangleType rs;
	
	WinGetClip(&rs);
	WinSetClip(bounds);
	
	recH = DmQueryRecord(MemopadDB,indexList[itemNum]);
	text = MemHandleLock(recH);
	
	for (i=0,titlelen=0;text[i]!='\0' && text[i]!='\n';i++,titlelen++)
	  ;
	WinDrawChars(text,titlelen,bounds->topLeft.x,bounds->topLeft.y);
	
	MemHandleUnlock(recH);
	
	WinSetClip(&rs);
}

static Boolean
memo_execute(char *input)
{
	Int16 i,j;
	CodeStack *stack;
	CError err;
	Trpn result;

	i = 0;
	while (input[i]) {
		for (j=i;input[j]!='\n' && input[j];j++)
		  ;
		input[j] = '\0';
		
		if (StrLen(input+i) == 0)
		  break;
		stack = text_to_stack(input+i,&err);
		if (!err) {
			err = stack_compute(stack);
			if (!err) {
				result = stack_pop(stack);
				set_ans_var(result);
				result_set(result);
				rpn_delete(result);
			}
			stack_delete(stack);
		}		
		if (err) 
			return false;
		i=j+1;
	}
	return true;
}

/***********************************************************************
 *
 * FUNCTION:     memo_import_memo
 * 
 * DESCRIPTION:  Executes a selected memo into easycalc
 *
 * PARAMETERS:   itemnum - the item selected by user
 *
 * RETURN:       true - operation ok
 *               false - error during execution
 *      
 ***********************************************************************/
static Boolean
memo_import_memo(UInt16 itemnum)
{
	MemHandle recH;
	char *text;
	char *input;
	Int16 i,memolen;
	Boolean res;
	
	
	recH = DmQueryRecord(MemopadDB,indexList[itemnum]);
	memolen = MemHandleSize(recH);
	input = MemPtrNew(memolen+1);
	text = MemHandleLock(recH);
	memcpy(input,text,memolen);
	input[memolen]='\0';
	MemHandleUnlock(recH);
	
	/* Skip the first line */
	for (i=0;input[i]!='\n' && i<memolen;i++)
	  ;
	i++;

#ifdef SPECFUN_ENABLED
	if (input[i] == ':')
		res = slv_memo_import(input);
	else
#endif
		res = memo_execute(input+i);

	MemPtrFree(input);
	return res;
}

/***********************************************************************
 *
 * FUNCTION:     memo_init_list
 * 
 * DESCRIPTION:  Initialize memoImportList by filling a global
 *               array with indexes of appropriate memos. If
 *               category 'EasyCalc' exists, only memos from this
 *               category are visible
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
memo_init_list(void)
{
	UInt16 mode = dmModeReadOnly;
	ListPtr list;
	UInt16 index,i;
	UInt16 numRecords;
	MemHandle recH;
	
	MemopadDB = DmOpenDatabaseByTypeCreator('DATA', 'memo', mode);
	ErrFatalDisplayIf(MemopadDB==NULL,"Error opening MemoDB");
	
	memoCategory = CategoryFind(MemopadDB, "EasyCalc");
	numRecords = DmNumRecordsInCategory(MemopadDB,memoCategory);
	
	list = GetObjectPtr(memoImportList);	
	
	LstSetListChoices(list,NULL,numRecords);
	LstSetDrawFunction(list,memo_draw_item);

	/* Allocate at least 1 byte to make later the MemPtrFree happy */
	indexList = MemPtrNew(sizeof(*indexList)*numRecords + 1);

	index = 0;
	i=0;
	while ((recH = DmQueryNextInCategory(MemopadDB,&index,memoCategory))) {
		indexList[i] = index;
		i++;
		index++;
	}
}

/***********************************************************************
 *
 * FUNCTION:     memo_destroy_list
 * 
 * DESCRIPTION:  Close memo database & free global list array
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
memo_destroy_list(void)
{
	DmCloseDatabase(MemopadDB);
	MemPtrFree(indexList);
}

/***********************************************************************
 *
 * FUNCTION:     MemoImportHandleEvent
 * 
 * DESCRIPTION:  Event handler for memopad import dialog
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean
MemoImportHandleEvent(EventPtr event)
{
	Boolean handled = false;
	FormPtr frm;
	Int16 controlID;
	Int16 i;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	 case keyDownEvent: {
		 ListPtr list;
		 
		 list = GetObjectPtr(memoImportList);
		 frm = FrmGetActiveForm();
		 switch (event->data.keyDown.chr) {
		  case pageUpChr:
			 FrmHideObject(frm,FrmGetObjectIndex(frm,memoImportList));
			 LstScrollList(list,winUp,LstGetVisibleItems(list)-1);
			 FrmShowObject(frm,FrmGetObjectIndex(frm,memoImportList));
			 handled = true;
			 break;
		  case pageDownChr:
			 FrmHideObject(frm,FrmGetObjectIndex(frm,memoImportList));
			 LstScrollList(list,winDown,LstGetVisibleItems(list)-1);
			 FrmShowObject(frm,FrmGetObjectIndex(frm,memoImportList));
			 handled = true;
			 break;
		 }
		 break;
	 }
	 case ctlSelectEvent:
		controlID = event->data.ctlSelect.controlID;
		if (controlID == memoImportCancel) {
			memo_destroy_list();
			FrmReturnToForm(0);
			handled = true;
			break;
		}
		else if (controlID == memoImportOK) {
			i = LstGetSelection(GetObjectPtr(memoImportList));
			if (i != noListSelection) {
				if (!memo_import_memo(i)) 
				  FrmAlert(altMemoImportError);
				memo_destroy_list();
				FrmReturnToForm(0);
				FrmUpdateForm(calcPrefs.form,frmUpdateResult);
			}
			handled = true;
			break;
		}
		break;
	 case frmOpenEvent:
		frm = FrmGetActiveForm();
		
		memo_init_list();
		
		FrmDrawForm(frm);
		
		handled = true;
		break;		
	 case frmCloseEvent:
		memo_destroy_list();
		/* FrmHandleEvent must handle this event itself */
		handled = false;
		break;
	}
	
	return handled;
}
