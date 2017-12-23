/*
 *   $Id: grtable.c,v 1.24 2006/11/20 03:05:07 tvoverbe Exp $
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
#include "calcrsc.h"

#include "segment.h"
#include "konvert.h"
#include "calc.h"
#include "display.h"
#include "calcDB.h"
#include "stack.h"
#include "fp.h"
#include "varmgr.h"
#include "funcs.h"

#include "grtable.h"
#include "grsetup.h"
#include "grprefs.h"
#include "graph.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static double firstVisible;
static double rowStep;
static Int16 firstCol;

static const TdispPrefs grtabDPrefs = { 5,true,disp_normal,disp_decimal,false,false};

/***********************************************************************
 *
 * FUNCTION:     grtable_tbl_contents
 *
 * DESCRIPTION:  Draw a contents of cells in the table, called as a
 *               callback function. This funtion puts in the first 
 *               row function names and in the first column
 *               parameter values.
 *
 * PARAMETERS:   See PalmOS reference
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
grtable_tbl_contents(void *table,Int16 row,Int16 column, 
					 RectangleType *bounds) GRAPH;
static void
grtable_tbl_contents(void *table,Int16 row,Int16 column, 
					 RectangleType *bounds)
{
	FontID oldfont;
	char *text;
	const char *text2;
	double val;
	RectangleType rp;
	CodeStack *stack;
	
	WinGetClip(&rp);
	WinSetClip(bounds);
	
	if (row == 0) {
		oldfont = FntSetFont(boldFont);
		if (column == 0) {
			if (graphPrefs.functype == graph_func)
				text = "x";
			else if (graphPrefs.functype == graph_polar)
				text = "t";
			else 
				text = "T";
			WinDrawChars(text, 1, bounds->topLeft.x, bounds->topLeft.y);
		}
		else {
			text2 = grsetup_fn_descr(column+firstCol-1);
			WinDrawChars(text2, StrLen(text2), bounds->topLeft.x,
			             bounds->topLeft.y);
			
		}		  
		FntSetFont(oldfont);
	}
	else if (column == 0) {
		text = display_real(firstVisible+rowStep*(row-1));
		WinDrawChars(text,StrLen(text),bounds->topLeft.x,
					 bounds->topLeft.y);
		MemPtrFree(text);
	}
	else  {
		if (graphPrefs.functype == graph_param) {
			if ((column+firstCol-1) % 2) 
			  stack = graphCurves[(column+firstCol-1)/2].stack2;
			else
			  stack = graphCurves[(column+firstCol-1)/2].stack1;
		}
		else /* func and polar */
			stack = graphCurves[column+firstCol-1].stack1;
		if (stack) {
			func_get_value(stack,firstVisible+rowStep*(row-1),
				       &val,NULL);
			text = display_real(val);
			WinDrawChars(text,StrLen(text),bounds->topLeft.x,bounds->topLeft.y);
			MemPtrFree(text);
		}
	}
	WinSetClip(&rp);
}

/***********************************************************************
 *
 * FUNCTION:     grtable_init_tbl
 *
 * DESCRIPTION:  Initialize the table, called once from FrmOpenEvent
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void grtable_init_tbl(void) GRAPH;
static void
grtable_init_tbl(void)
{
	Int16 i;
	TablePtr table;
	UInt16 rows;
#ifdef SUPPORT_DIA
	Int16 height;

	height = HanderaCoord(FntLineHeight());
#endif
	table = GetObjectPtr(grTableTable);
	rows = TblGetNumberOfRows(table);
	
	for (i=0;i<rows;i++) {
#ifdef SUPPORT_DIA
		if (GetDIAHardware() == DIA_HARDWARE_HANDERA) {
			TblSetRowHeight(table, i, height);
		}
#endif
		TblSetItemStyle(table,i,0,customTableItem);
		TblSetItemStyle(table,i,1,customTableItem);
		TblSetItemStyle(table,i,2,customTableItem);
	}
	TblSetColumnUsable(table,0,true);
	TblSetColumnUsable(table,1,true);
	TblSetColumnUsable(table,2,true);
	
	TblSetCustomDrawProcedure(table,0,grtable_tbl_contents);
	TblSetCustomDrawProcedure(table,1,grtable_tbl_contents);
	TblSetCustomDrawProcedure(table,2,grtable_tbl_contents);
	
	if (graphPrefs.functype == graph_func) {
		firstVisible = (graphPrefs.xmax+graphPrefs.xmin)/2;
//		rowStep = (graphPrefs.xmax - graphPrefs.xmin)/140.0;
		rowStep = graphPrefs.xscale;
	}
	else if (graphPrefs.functype == graph_polar) {
		firstVisible = graphPrefs.fimin;
		rowStep = graphPrefs.fistep;
	}
	else {
		firstVisible = graphPrefs.tmin;
		rowStep = graphPrefs.tstep;
	}
	firstCol = 0;
}

/***********************************************************************
 *
 * FUNCTION:     grtable_scroll
 *
 * DESCRIPTION:  Scroll a table according to parameters and redraw
 *               the resulting table
 *
 * PARAMETERS:   dir - winUp or winDown - which direction to scroll
 *               count - how many lines to scroll
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void grtable_scroll(WinDirectionType dir,Int16 count) GRAPH;
void 
grtable_scroll(WinDirectionType dir,Int16 count)
{
	TablePtr table;
	
	if (dir==winDown) 
	  firstVisible+=rowStep*count;
	else
	  firstVisible-=rowStep*count;
	
	table = GetObjectPtr(grTableTable);
	TblMarkTableInvalid(table);
	TblRedrawTable(table);
}

static void grtable_goto(void) GRAPH;
static void 
grtable_goto(void)
{
	if (varmgr_get_double(&firstVisible,"Go directly to value")) {
		TblMarkTableInvalid(GetObjectPtr(grTableTable));
		TblRedrawTable(GetObjectPtr(grTableTable));
	}
}

static void grtable_step(void) GRAPH;
static void 
grtable_step(void)
{
	if (varmgr_get_double(&rowStep,"Set step value")) {
		TblMarkTableInvalid(GetObjectPtr(grTableTable));
		TblRedrawTable(GetObjectPtr(grTableTable));
	}
}

/***********************************************************************
 *
 * FUNCTION:     GraphTableHandleEvent
 * 
 * DESCRIPTION:  Event handler for graphic graph Table view
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean 
GraphTableHandleEvent(EventPtr event)
{
	Boolean    handled = false;
	FormPtr    frm=FrmGetActiveForm();
	Int16 controlId;
	static TdispPrefs oldprefs;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	case tblEnterEvent:
		handled = true;
		break;
	case keyDownEvent:
		switch (event->data.keyDown.chr) {
		case pageDownChr:
			grtable_scroll(winDown,8);
			handled = true;
			break;
		case pageUpChr:
			grtable_scroll(winUp,8);
			handled = true;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		case ',':
		case '-':
		      /* Automatically popup a 'Goto' window when
		       * number pressed 
		       */
			EvtAddEventToQueue(event);
			grtable_goto();
			handled = true;
			break;
		}
		break;
	case ctlSelectEvent:
		controlId=event->data.ctlSelect.controlID;
		switch (controlId) {
		case grTableDone:
			FrmReturnToForm(0);
			fp_set_prefs(oldprefs);
			handled = true;			
			break;
		case grTableGoto:
			grtable_goto();
			handled = true;
			break;
		case grTableStep:
			grtable_step();
			handled = true;
			break;
		 case grTableLeft:
			firstCol--;
			if (firstCol>=0) {
				TblMarkTableInvalid(GetObjectPtr(grTableTable));
				TblRedrawTable(GetObjectPtr(grTableTable));
			}
			else
			  firstCol++;
			handled = true;
			break;
		 case grTableRight:
			firstCol++;
			if ((firstCol<MAX_GRFUNCS*2-1 && graphPrefs.functype==graph_param)
				|| firstCol<MAX_GRFUNCS-1) {
				TblMarkTableInvalid(GetObjectPtr(grTableTable));
				TblRedrawTable(GetObjectPtr(grTableTable));
			}
			else
			  firstCol--;
			handled = true;
			break;
/*			
		 case grSetupDown:
			grsetup_scroll(winDown);
			handled = true;
			break;
		 case grSetupUp:
			grsetup_scroll(winUp);
            handled = true;
			break;
*/
		}
		break;
	 case frmUpdateEvent:
		break;
	 case frmCloseEvent:
		fp_set_prefs(oldprefs);
		handled = false;
		break;
	 case frmOpenEvent:
		oldprefs = fp_set_prefs(grtabDPrefs);
		
		grtable_init_tbl();

		FrmDrawForm(frm);
		handled=true;
		break;
	}
	  
	return handled;
}
