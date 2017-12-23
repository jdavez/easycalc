/*
 *   $Id: lstedit.c,v 1.17 2006/10/09 19:09:14 tvoverbe Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000,2001 Ondrej Palkovsky
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

#include "calcrsc.h"
#include "lstedit.h"
#include "konvert.h"
#include "calc.h"
#include "slist.h"
#include "stack.h"
#include "calcDB.h"
#include "display.h"
#include "varmgr.h"
#include "defuns.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

#define LIST_COUNT   3

static Int16 firstLine;
static Int16 maxLines = 1;
static struct {
	Int16 col;
	Int16 row;
	Boolean offscreen;
}tblSelection = {-1,-1,true};

static struct {
	char list[LIST_COUNT][MAX_FUNCNAME+1];
}listPrefs;

static List *loadedList[LIST_COUNT] = {NULL,NULL,NULL};

static Boolean showAns = false;

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_contents_num
 * 
 * DESCRIPTION:  Display the line number (first column) in bold font
 *
 * PARAMETERS:   See PalmOS Reference
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_tbl_contents_num(void *table,Int16 row,Int16 column,
			 RectangleType *bounds) PARSER;
static void
lstedit_tbl_contents_num(void *table,Int16 row,Int16 column,
			 RectangleType *bounds)
{
	FontID oldfont;
	char text[5];

	oldfont = FntSetFont(boldFont);
	StrPrintF(text,"%d",TblGetItemInt(table,row,column));
	WinDrawChars(text,StrLen(text),bounds->topLeft.x+3,
		     bounds->topLeft.y);
	FntSetFont(oldfont);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_contents
 * 
 * DESCRIPTION:  Draws a normal cell of table
 *
 * PARAMETERS:   See PalmOS reference
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_tbl_contents(void *table,Int16 row,Int16 column, 
		     RectangleType *bounds) PARSER;
static void
lstedit_tbl_contents(void *table,Int16 row,Int16 column, 
		     RectangleType *bounds)
{
	Int16 real;
	char *text;
	FontID oldfont;

	real = firstLine + row;
	if (loadedList[column-1] == NULL
	    || loadedList[column-1]->size <= real)
		return;

	oldfont = FntSetFont(stdFont);
	text = display_complex(loadedList[column-1]->item[real]);
	WinGlueDrawTruncChars(text,StrLen(text),
			  bounds->topLeft.x+2,bounds->topLeft.y,
			  bounds->extent.x-2);
	MemPtrFree(text);
	FntSetFont(oldfont);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_delete_lists
 * 
 * DESCRIPTION:  Free memory ocupied by loaded lists
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_delete_lists(void) PARSER;
static void
lstedit_delete_lists(void)
{
	Int16 i;

	for (i=0;i<LIST_COUNT;i++) 
		if (loadedList[i] != NULL) {
			list_delete(loadedList[i]);
			loadedList[i] = NULL;
		}
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_save_lists
 * 
 * DESCRIPTION:  Saves loaded lists back to database. Should be called
 *               after every modification
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static CError
lstedit_save_lists(void) PARSER;
static CError
lstedit_save_lists(void)
{
	Int16 i;
	Trpn item;
	CodeStack *stack;
	CError err = c_noerror;

	stack = stack_new(1);
	for (i=0;i<LIST_COUNT;i++) 
		if (StrLen(listPrefs.list[i])) {
			err = stack_add_val(stack,&loadedList[i],list);
			if (!err) {
				item = stack_pop(stack);
				err = db_write_variable(listPrefs.list[i],item);
				rpn_delete(item);
			}
		}
	stack_delete(stack);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_upd_maxlines
 * 
 * DESCRIPTION:  Updated the maxLines variable, containing the length
 *               of longest loaded list
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_upd_maxlines(void) PARSER;
static void
lstedit_upd_maxlines(void)
{
	Int16 i;

	maxLines = 0;
	for (i=0;i<LIST_COUNT;i++)
		if (loadedList[i] != NULL 
		    && loadedList[i]->size > maxLines)
			maxLines = loadedList[i]->size;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_load_lists
 * 
 * DESCRIPTION:  Reads list name from Preferences and loads them
 *               into global loadedLists variable
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_load_lists(void) PARSER;
static void
lstedit_load_lists(void)
{
	Int16 i;
	Trpn item;
	CError err;
	
	lstedit_delete_lists();
	for (i=0;i<LIST_COUNT;i++) 
		if (StrLen(listPrefs.list[i])) {
			item = db_read_variable(listPrefs.list[i],&err);
			if (!err && item.type == list) {
				loadedList[i] = list_dup(item.u.listval);
			} else
				StrCopy(listPrefs.list[i],"");
			if (!err)
				rpn_delete(item);
		}
	CtlSetLabel(GetObjectPtr(popList1),listPrefs.list[0]);
	CtlSetLabel(GetObjectPtr(popList2),listPrefs.list[1]);
	CtlSetLabel(GetObjectPtr(popList3),listPrefs.list[2]);
	
	lstedit_upd_maxlines();
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_load
 * 
 * DESCRIPTION:  Load some values into table
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_tbl_load(void) PARSER;
static void
lstedit_tbl_load(void)
{
	Int16 i;
	TablePtr table;
	UInt16 rows;

	table = GetObjectPtr(tblList);
	rows = TblGetNumberOfRows(table);

	if (firstLine + rows >= maxLines)
		firstLine = maxLines - rows;
	if (maxLines < rows)
		firstLine = 0;
	for (i=0;i<rows;i++)
		TblSetItemInt(table,i,0,firstLine + i + 1);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_init
 * 
 * DESCRIPTION:  Constructor for List Editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_init(void) PARSER;
static void
lstedit_init(void)
{
	Int16 i;
	TablePtr table;
	UInt16 rows;
	UInt16 size;
	
	size = sizeof(listPrefs);
	if (PrefGetAppPreferences(APP_ID,PREF_LSTEDIT,&listPrefs,&size,
				  false)!=PREF_LSTEDIT_VERSION) {
		listPrefs.list[0][0] = '\0';
		listPrefs.list[1][0] = '\0';
		listPrefs.list[2][0] = '\0';
	}
	
	table = GetObjectPtr(tblList);
	rows = TblGetNumberOfRows(table);
	
	for (i=0;i<rows;i++) {
		TblSetItemStyle(table,i,0,customTableItem);
		TblSetItemStyle(table,i,1,customTableItem);
		TblSetItemStyle(table,i,2,customTableItem);
		TblSetItemStyle(table,i,3,customTableItem);
	}

	TblSetColumnSpacing(table,1,3);
	TblSetColumnSpacing(table,2,3);

	TblSetColumnUsable(table,0,true);
	TblSetColumnUsable(table,1,true);
	TblSetColumnUsable(table,2,true);
	TblSetColumnUsable(table,3,true);
	
	TblSetCustomDrawProcedure(table,0,lstedit_tbl_contents_num);
	TblSetCustomDrawProcedure(table,1,lstedit_tbl_contents);
	TblSetCustomDrawProcedure(table,2,lstedit_tbl_contents);
	TblSetCustomDrawProcedure(table,3,lstedit_tbl_contents);

	firstLine = 0;

	tblSelection.col = tblSelection.row = -1;

	if (showAns) {
		StrCopy(listPrefs.list[0],"ans");
		showAns = false;
	}
	lstedit_load_lists();
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_invert
 * 
 * DESCRIPTION:  Invert a particular cell in a table
 *
 * PARAMETERS:   tbl - table pointer
 *               row,col - which cell to invert
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_tbl_invert(TablePtr tbl, Int16 row, Int16 col) PARSER;
static void
lstedit_tbl_invert(TablePtr tbl, Int16 row, Int16 col)
{
	Int16 rows;
	RectangleType bounds;

	if (row == -1)
		return;

	rows = TblGetNumberOfRows(tbl);
	
	if (row >= firstLine && row < firstLine + rows) {
		TblGetItemBounds(tbl,row-firstLine,col,&bounds);
		WinInvertRectangle(&bounds,0);
	}
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_scroll
 * 
 * DESCRIPTION:  Scroll the table specified number of rows
 *
 * PARAMETERS:   tbl - pointer to table
 *               delta - how many lines (+down, -up) to scroll
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_tbl_scroll(TablePtr tbl,Int16 delta) PARSER;
static void
lstedit_tbl_scroll(TablePtr tbl,Int16 delta)
{
	RectangleType r;
	UInt16 i,rows;
	Int16 oldfirstline;

	rows = TblGetNumberOfRows(tbl);

	oldfirstline = firstLine;
	firstLine +=delta;
	
	if (firstLine < 0)
		firstLine = 0;
	lstedit_tbl_load();
	delta = firstLine - oldfirstline;
	if (delta > 0)
		i = delta;
	else
		i = -delta;

	if (i < rows) {
		TblGetBounds(tbl,&r);
		WinScrollRectangle(&r, (delta < 0) ? winDown : winUp, 
				   TblGetRowHeight(tbl, 0) * i, &r);
		while (i--) 
			TblMarkRowInvalid(tbl, (delta < 0) ? i : rows-i-1);
	} else
		TblMarkTableInvalid(tbl);
	TblRedrawTable(tbl);
	
	/* If selection was offscreen, put onscreen */
	if (tblSelection.row >= firstLine 
	    && tblSelection.row < firstLine + rows) {
		if (tblSelection.offscreen)
			lstedit_tbl_invert(tbl,tblSelection.row,
					   tblSelection.col);
		tblSelection.offscreen = false;
	} else
		tblSelection.offscreen = true;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_destroy
 * 
 * DESCRIPTION:  Destructor for List Editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_destroy(void) PARSER;
static void
lstedit_destroy(void)
{
	lstedit_delete_lists();
	PrefSetAppPreferences(APP_ID,PREF_LSTEDIT,PREF_LSTEDIT_VERSION,&listPrefs,
			      sizeof(listPrefs),false);
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_long_upd
 * 
 * DESCRIPTION:  Update the label that shows contents of selected
 *               cell
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_long_upd(void) PARSER;
static void
lstedit_long_upd(void)
{
	char *longtext;
	FormPtr frm = FrmGetActiveForm();

	if (tblSelection.row != -1) {
		longtext = display_complex(loadedList[tblSelection.col-1]->item[tblSelection.row]);
		FrmHideObject(frm,FrmGetObjectIndex(frm,lblListLong));
		FrmCopyLabel(frm,lblListLong,longtext);
		FrmShowObject(frm,FrmGetObjectIndex(frm,lblListLong));
		MemPtrFree(longtext);
	}
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_deselect
 * 
 * DESCRIPTION:  Deselect a variable, clear label that shows contents
 *               of variable
 *
 * PARAMETERS:   tbl - pointer to the table
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_tbl_deselect(TablePtr tbl) PARSER;
static void
lstedit_tbl_deselect(TablePtr tbl)
{
	FormPtr frm = FrmGetActiveForm();

	if (tblSelection.row != -1) {
		FrmHideObject(frm,FrmGetObjectIndex(frm,lblListLong));
		FrmCopyLabel(frm,lblListLong,"");
		FrmShowObject(frm,FrmGetObjectIndex(frm,lblListLong));

		lstedit_tbl_invert(tbl,tblSelection.row,tblSelection.col);
		tblSelection.row = tblSelection.col = -1;
	}
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_tbl_select 
 * 
 * DESCRIPTION:  This function gets called when the user taps on the 
 *               particular cell. It tracks the pen until it is lifted
 *               and sets appropriately variables and label that shows
 *               contents of variable
 *
 * PARAMETERS:   tbl - table pointer
 *               row,col - item which user tapped
 *
 * RETURN:       true - user selected the cell
 *               false - user lifted pen outside the cell
 *      
 ***********************************************************************/
static Boolean
lstedit_tbl_select(TablePtr tbl,Int16 row,Int16 col) PARSER;
static Boolean
lstedit_tbl_select(TablePtr tbl,Int16 row,Int16 col)
{
	Boolean penDown;
	Boolean inrect = true;
	Coord x,y;
	RectangleType bounds;
	Int16 oldrow,oldcol;

	if (col == 0)
		return false;
	if (loadedList[col-1] == NULL 
	    || row + firstLine + 1 > loadedList[col-1]->size)
		return false;

	oldrow = tblSelection.row;
	oldcol = tblSelection.col;
	lstedit_tbl_invert(tbl,oldrow,oldcol);

	TblGetItemBounds(tbl,row,col,&bounds);
	WinInvertRectangle(&bounds,0);
	do {
		EvtGetPen(&x,&y,&penDown);
		if (RctPtInRectangle(x,y,&bounds) && !inrect) {
			inrect = true;
			WinInvertRectangle(&bounds,0);
		} else if (!RctPtInRectangle(x,y,&bounds) && inrect) {
			inrect = false;
			WinInvertRectangle(&bounds,0);
		}		
	}while(penDown);

	if (!inrect && oldrow != -1) 
		lstedit_tbl_invert(tbl,oldrow,oldcol);

	if (inrect) {
		tblSelection.row = row + firstLine;
		tblSelection.col = col;
		tblSelection.offscreen = false;
		lstedit_long_upd();
	}
	return inrect;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_reselect
 * 
 * DESCRIPTION:  Shows a popup dialog with existing list and 'New' and 
 *               'Delete'. Creates new list, delete new list or just
 *               selects a new list
 *
 * PARAMETERS:   controlId - popuptrigger that was pressed
 *
 * RETURN:       true - user selected something
 *               false - user didn't select anything
 *      
 ***********************************************************************/
static Boolean
lstedit_reselect(Int16 controlId) PARSER;
static Boolean
lstedit_reselect(Int16 controlId)
{
	RectangleType bounds;
	FormPtr frm = FrmGetActiveForm();
	ListPtr slst;
	dbList *varlist;
	char **values,*lname;
	char loct1[MAX_RSCLEN],loct2[MAX_RSCLEN],loct3[MAX_RSCLEN];
	Int16 i;
	CError err;

	lname = listPrefs.list[controlId - popList1];

	FrmGetObjectBounds(frm,FrmGetObjectIndex(frm,controlId),
			   &bounds);
	slst = GetObjectPtr(lstList);
	LstSetPosition(slst,bounds.topLeft.x,bounds.topLeft.y);
	
	varlist = db_get_list(list);
	values = MemPtrNew(sizeof(*values) * (3+varlist->size));

	loct1[0] = loct2[0] = loct3[0] = ' ';
	SysCopyStringResource(loct1+1,strLocalNew);
	SysCopyStringResource(loct2+1,strLocalDelete);
	SysCopyStringResource(loct3+1,strLocalSaveAs);
	values[0] = loct1;
	values[1] = loct2;
	values[2] = loct3;

	for (i=0;i<varlist->size;i++)
		values[i+3] = varlist->list[i];
	LstSetListChoices(slst,values,varlist->size + 3);
	LstSetHeight(slst,varlist->size<8?varlist->size + 3:10);
	LstSetSelection(slst,0);
	for (i=0;i<varlist->size;i++) {
		if (StrCompare(varlist->list[i],lname) == 0)
			LstSetSelection(slst,i+3);
	}
	/* Set incrementel search */
	LstGlueSetIncrementalSearch(slst, true);

	i = LstPopupList(slst);
	if (i != noListSelection) {
		if (i > 2) {
			StrCopy(lname,varlist->list[i-3]);
			lstedit_tbl_deselect(GetObjectPtr(tblList));
		} else if (i == 2 && StrLen(lname)) {
			/* Save as */
			char text[MAX_FUNCNAME+1];
			if (varmgr_get_varstring(text,"List Name")) {
				StrCopy(lname,text);
				err = lstedit_save_lists();
				if (err) {
					alertErrorMessage(err);
					i = noListSelection;
				}
			}
		} else if (i == 1 && StrLen(lname)) {
			/* Delete */
			if (!FrmCustomAlert(altConfirmDelete,"list",lname,NULL)) {
				db_delete_record(lname);
				StrCopy(lname,"");
				lstedit_tbl_deselect(GetObjectPtr(tblList));
			} else
				i = noListSelection;
		} else if (i == 0) {
			/* New */
			char text[MAX_FUNCNAME+1];
			Complex cplx;
			cplx.real = cplx.imag = 0.0;
			if (varmgr_get_varstring(text,"List Name") &&
				varmgr_get_complex(&cplx,"List Item")) {
				StrCopy(lname,text);
				if (loadedList[controlId - popList1])
					MemPtrFree(loadedList[controlId - popList1]);
				loadedList[controlId - popList1] = list_new(1);
				loadedList[controlId - popList1]->item[0] = cplx;
				err = lstedit_save_lists();
				if (err) {
					alertErrorMessage(err);
					i = noListSelection;
				}

				tblSelection.offscreen = true;
				tblSelection.row = 0;
				tblSelection.col = controlId - popList1 + 1;
			} else 
				i = noListSelection;
		}
	}

	MemPtrFree(values);
	db_delete_list(varlist);

	return i != noListSelection;
}

/***********************************************************************
 *
 * FUNCTION:     lstedit_update_scrollbar
 * 
 * DESCRIPTION:  Update main scrollbar, should be called always
 *               when number of rows in table or position is changed
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
lstedit_update_scrollbar(void) PARSER;
static void
lstedit_update_scrollbar(void)
{
	TablePtr table;
	UInt16 rows;
	
	table = GetObjectPtr(tblList);
	rows = TblGetNumberOfRows(table);
	
	if (firstLine == 0 && maxLines <= rows) 
		SclSetScrollBar(GetObjectPtr(sclList),firstLine,0,0,0);
	else 
		SclSetScrollBar(GetObjectPtr(sclList),firstLine,0,
				maxLines - rows,
				rows);
}

/***********************************************************************
 *
 * FUNCTION:     ListEditHandleEvent
 * 
 * DESCRIPTION:  Event handler for List editor
 *
 * PARAMETERS:   
 *
 * RETURN:       
 *      
 ***********************************************************************/
Boolean 
ListEditHandleEvent(EventPtr event)
{
	Boolean    handled = false;
	FormPtr    frm=FrmGetActiveForm();
	Int16 controlId;
	Int16 row,col,rows,i;
	Int16 delta;
	Complex cplx;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	case tblEnterEvent:
		row = event->data.tblEnter.row;
		col = event->data.tblEnter.column;
		if (lstedit_tbl_select(GetObjectPtr(tblList),row,col))
			;
		handled = true;
		break;
	case keyDownEvent:
		switch (event->data.keyDown.chr) {
		case pageDownChr:
			lstedit_tbl_scroll(GetObjectPtr(tblList),9);
			lstedit_update_scrollbar();
			handled = true;
			break;
		case pageUpChr:
			lstedit_tbl_scroll(GetObjectPtr(tblList),-9);
			lstedit_update_scrollbar();
			handled = true;
			break;
		}
		if (!handled \
		    && ((event->data.keyDown.chr >= '0' \
			 && event->data.keyDown.chr <= 'z') \
			|| (event->data.keyDown.chr == '.' \
			    || event->data.keyDown.chr == ','))) {
			handled = true;
			col = tblSelection.col;
			if (col == -1)
				break;
			cplx.real = 0.0;
			cplx.imag = 0.0;
			EvtAddEventToQueue(event);
			if (varmgr_get_complex(&cplx,"List value")) {
				List *lst;
				Int16 i;
				CError err;

				lst = list_new(loadedList[col-1]->size+1);
				if (!lst) {
					alertErrorMessage(c_memory);
					break;
				}

				for (i=0;i<lst->size-1;i++)
					lst->item[i]=loadedList[col-1]->item[i];
				lst->item[i] = cplx;
				list_delete(loadedList[col-1]);
				loadedList[col-1] = lst;

				err = lstedit_save_lists();
				if (err) {
					alertErrorMessage(err);
					break;
				}

				lstedit_upd_maxlines();
				rows = TblGetNumberOfRows(GetObjectPtr(tblList));
				delta = (lst->size - rows) - firstLine;
				if (delta <= 0) {
					tblSelection.offscreen = true;
					TblMarkTableInvalid(GetObjectPtr(tblList));
				}
				lstedit_tbl_scroll(GetObjectPtr(tblList),delta);
				lstedit_update_scrollbar();
			}
		}
		break;
	case sclRepeatEvent:
		delta = event->data.sclRepeat.newValue - event->data.sclRepeat.value;
		lstedit_tbl_scroll(GetObjectPtr(tblList),delta);
		handled = false;
		break;
	case ctlSelectEvent:
		controlId=event->data.ctlSelect.controlID;
		switch (controlId) {
		case btnListInsert:
			handled = true;

			col = tblSelection.col;
			row = tblSelection.row;
			if (col == -1 || row == -1)
				break;
			cplx.real = 0.0;
			cplx.imag = 0.0;
			if (varmgr_get_complex(&cplx,"List value")) {
				List *lst, *oldlst;
				Int16 i;
				CError err;

				lst = list_new(loadedList[col-1]->size+1);
				if (!lst) {
					alertErrorMessage(c_memory);
					break;
				}

				for (i=0;i<row;i++)
					lst->item[i]=loadedList[col-1]->item[i];
				lst->item[row] = cplx;
				for (i=row;i<loadedList[col-1]->size;i++)
					lst->item[i+1] = loadedList[col-1]->item[i];
				oldlst = loadedList[col-1];
				loadedList[col-1] = lst;
				err = lstedit_save_lists();
				if (err) {
					alertErrorMessage(err);
					loadedList[col-1] = oldlst; /* restore original one */
					list_delete(lst);
				}
				else
					list_delete(oldlst);

				lstedit_upd_maxlines();
				TblMarkTableInvalid(GetObjectPtr(tblList));
				tblSelection.offscreen = true;
				lstedit_tbl_scroll(GetObjectPtr(tblList),0);
				lstedit_update_scrollbar();
			}
			break;
		case btnListDelete:
			handled = true;
			
			col = tblSelection.col;
			row = tblSelection.row;
			if (col == -1 || row == -1)
				break;
			if (loadedList[col-1]->size == 1)
				break;
			for (i=row+1;i<loadedList[col-1]->size;i++)
				loadedList[col-1]->item[i-1] = loadedList[col-1]->item[i];
			--loadedList[col-1]->size;
			if (tblSelection.row >= loadedList[col-1]->size)
				--tblSelection.row;

			lstedit_save_lists(); /* should succeed since list gets smaller */

			lstedit_upd_maxlines();
			lstedit_long_upd();
			TblMarkTableInvalid(GetObjectPtr(tblList));
			tblSelection.offscreen = true;
			lstedit_tbl_scroll(GetObjectPtr(tblList),0);
			lstedit_update_scrollbar();
			break;
		case btnListLong:
			handled = true;

			col = tblSelection.col;
			row = tblSelection.row;
			if (col == -1 || row == -1)
				break;
			if (varmgr_get_complex(&loadedList[col - 1]->item[row],
					       "List value")) {
				lstedit_save_lists(); /* should succeed since no change in list size */
				row -= firstLine;
				if (! tblSelection.offscreen) {
					lstedit_long_upd();
					/* We have to get this item redrawn,
					   but remain selected if possible */
					TblMarkRowInvalid(GetObjectPtr(tblList),row);
					tblSelection.offscreen = true;
					lstedit_tbl_scroll(GetObjectPtr(tblList),0);
				}
			}
			break;
		case btnListDone:
			lstedit_destroy();
			FrmReturnToForm(0);
			handled = true;			
			break;
		case popList1:
		case popList2:
		case popList3:
			if (lstedit_reselect(controlId)) {
				lstedit_load_lists();
				lstedit_tbl_load();
				lstedit_update_scrollbar();

				TblMarkTableInvalid(GetObjectPtr(tblList));
				TblRedrawTable(GetObjectPtr(tblList));

				lstedit_tbl_scroll(GetObjectPtr(tblList),0);
				lstedit_long_upd();
			}
			handled = true;
			break;
		}
		break;
	case frmUpdateEvent:
		break;
	case frmCloseEvent:
		lstedit_destroy();
		handled = false;
		break;
	case frmOpenEvent:
		lstedit_init();
		lstedit_tbl_load();
		lstedit_update_scrollbar();
		FrmDrawForm(frm);
		handled=true;
		break;
	}
	  
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:     lstedit_popup_ans
 * 
 * DESCRIPTION:  Popup List editor, set first column to variable 'ans'
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
lstedit_popup_ans(void)
{
	showAns = true;
	FrmPopupForm(frmListEdit);
}
