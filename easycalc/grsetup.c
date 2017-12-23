/*
 *   $Id: grsetup.c,v 1.34 2007/12/18 09:06:37 cluny Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000 Ondrej Palkovsky
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
#include <string.h>

#include "clie.h"
#include "calcDB.h"
#include "konvert.h"
#include "grsetup.h"
#include "grprefs.h"
#include "graph.h"
#include "varmgr.h"
#include "calcrsc.h"
#include "calc.h"
#include "stack.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static Int16 firstVisible = 0;
static Int16 selectedRow;
static Boolean grSetupOpen = false;

static char *tblFuncs[] = {"Y1","Y2","Y3","Y4","Y5","Y6"};
static char *tblPols[] = {"r1","r2","r3","r4","r5","r6"};
static char *tblPars[] = {"X1","Y1","X2","Y2","X3","Y3","X4","Y4","X5","Y5","X6","Y6"};

enum {
	COL_FLIST = 0,
	COL_FUNC = 1
};

static void draw_grType(FormType *frm) IFACE;

static const char *
grsetup_param_name(void) IFACE;
static const char *
grsetup_param_name(void)
{
	if (graphPrefs.functype == graph_func)
		return "x";
	else if (graphPrefs.functype == graph_polar)
		return "t";
	else /* Parametric */
		return "t";
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_fn_descr_arr
 * 
 * DESCRIPTION:  Setup the array with names and positions of 
 *               existing user selected functions
 *
 * PARAMETERS:   descr[MAX_GRFUNCS] - array that will contain 
 *                                    descriptions of funcs
 *               nums[MAX_GRFUNCS] - positions to table of selected functions
 *
 * RETURN:       count of selected functions
 *      
 ***********************************************************************/
Int16 
grsetup_fn_descr_arr(char **descr, Int16 *nums)
{
	Int16 i,count;

	for (i=0,count=0;i<MAX_GRFUNCS;i++) {
		if (graphPrefs.functype == graph_param &&
		    StrLen(grsetup_get_fname(i*2)) &&
		    StrLen(grsetup_get_fname(i*2+1))) {
			descr[count] = (char *)grsetup_fn_descr(i*2);
			nums[count] = i;
			count++;
		} else if (graphPrefs.functype != graph_param &&
		           StrLen(grsetup_get_fname(i))) {
			descr[count] = (char *)grsetup_fn_descr(i);
			nums[count] = i;
			count++;
		}
	}
	return count;
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_fn_descr
 * 
 * DESCRIPTION:  Return name of function, that is selected by the user
 *               on the i'th position in the list
 *
 * PARAMETERS:   i - position of a function in the list
 *
 * RETURN:       Pointer to static function name
 *      
 ***********************************************************************/
const char *
grsetup_fn_descr(Int16 i) IFACE;
const char *
grsetup_fn_descr(Int16 i)
{
	if (graphPrefs.functype == graph_func)
		return tblFuncs[i];
	else if (graphPrefs.functype == graph_polar)
		return tblPols[i];
	else 
		return tblPars[i];
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_def_name
 * 
 * DESCRIPTION:  Return name of function, that should be created specially
 *               for graphing purposes (the 'User' selection)
 *
 * PARAMETERS:   row - the row number in a grsetup table
 *
 * RETURN:       Pointer to static function name 
 *      
 ***********************************************************************/
static const char * grsetup_def_name(Int16 row) IFACE;
static const char *
grsetup_def_name(Int16 row)
{
	static char result[MAX_FUNCNAME+1];
	char tmp[2];
	
	StrCopy(tmp,"1");
	switch (graphPrefs.functype) {
	 case graph_func:
		StrCopy(result,"z_grafun");
		tmp[0]+=(char)row;
		StrCat(result,tmp);
		break;
	 case graph_polar:
		StrCopy(result,"z_grapol");
		tmp[0]+=(char)row;
		StrCat(result,tmp);
		break;
	 case graph_param:
		StrCopy(result,"z_grapar");
		StrCat(result,row%2?"b":"a");
		tmp[0]+=(char)row/2;
		StrCat(result,tmp);
		break;
	}
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_get_fname
 * 
 * DESCRIPTION:  Get a name of a function for the given row, that is
 *               already saved in Preferences
 *
 * PARAMETERS:   row - number of row in the displayed table
 *
 * RETURN:       !!! Pointer to a graphPrefs structure, so
 *               functions can modify the name directly
 *      
 ***********************************************************************/
char * grsetup_get_fname(Int16 row) IFACE;
char *
grsetup_get_fname(Int16 row)
{
	switch (graphPrefs.functype) {
	case graph_func:
		return graphPrefs.funcFunc[row];
	case graph_polar:
		return graphPrefs.funcPol[row];
	case graph_param:
	default:
		return graphPrefs.funcPar[row/2][row%2];
	}
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_tbl_contents
 * 
 * DESCRIPTION:  Callback function for drawing the function name or
 *               contents of the user defined function
 *
 * PARAMETERS:   see PalmOS Reference
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
grsetup_tbl_contents(void *table,Int16 row,Int16 column, 
					 RectangleType *bounds) IFACE;
static void
grsetup_tbl_contents(void *table,Int16 row,Int16 column, 
					 RectangleType *bounds)
{
	char *name;
	const char *usrname;
	char *text;
	CError err;
	RectangleType rp;
	
	row += firstVisible;
	name = grsetup_get_fname(row); /* name is now a pointer to graphPrefs */
	usrname = grsetup_def_name(row);

	WinGetClip(&rp);
	WinSetClip(bounds);
	if (StrLen(name)==0)
	  ; /* Do not draw nothing */
	else if (StrCompare(usrname,name)==0) {
		/* Special graph function, show contents */
		err = db_func_description(usrname,&text,NULL);
		if (!err) {
			WinDrawChars(text,StrLen(text),bounds->topLeft.x,bounds->topLeft.y);
			MemPtrFree(text);			
		}
		else 
		  /* Somone probably deleted it, reset the value */
		  StrCopy(name,"");
	} else {
		/* Normal function, show ordinary name */
		char tmpname[MAX_FUNCNAME+5];
		StrCopy(tmpname,name);
		StrCat(tmpname,"()");
		WinDrawChars(tmpname,StrLen(tmpname),bounds->topLeft.x,bounds->topLeft.y);
	}
	WinSetClip(&rp);
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_tbl_name
 * 
 * DESCRIPTION:  Display the name ('Y1','r1' etc.) of a function,
 *               use a function color if on color display
 *
 * PARAMETERS:   See PalmOS Reference for callback functions
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
grsetup_tbl_name(void *table,Int16 row,Int16 column, 
				 RectangleType *bounds) IFACE;
static void
grsetup_tbl_name(void *table,Int16 row,Int16 column, 
				  RectangleType *bounds)
{
	char text[6];
	Coord x;

	StrCopy(text,grsetup_fn_descr(row+firstVisible));
	StrCat(text,":");
	x = bounds->topLeft.x+bounds->extent.x-FntCharsWidth(text,StrLen(text))-2;
	WinDrawChars(text,StrLen(text),x,bounds->topLeft.y);
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_tbl_init
 * 
 * DESCRIPTION:  Initialize the on-screen table 
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grsetup_tbl_init(void) IFACE;
static void
grsetup_tbl_init(void)
{
	Int16 i;
	TablePtr table;
	UInt16 rows;
        FormPtr frm = FrmGetActiveForm();
	Int16 height;

	table = GetObjectPtr(grSetupTable);
	rows = TblGetNumberOfRows(table);
	height = TblGetRowHeight(table, 0);
	if (handera)
		height = height * 3 / 2 + 1;

	for (i=0;i<rows;i++) {
		TblSetRowHeight(table,i,height);
		TblSetItemStyle(table,i,0,customTableItem);
		TblSetItemStyle(table,i,1,customTableItem);
	}
	TblSetColumnUsable(table,0,true);
	TblSetColumnUsable(table,1,true);

	TblSetCustomDrawProcedure(table,0,grsetup_tbl_name);
	TblSetCustomDrawProcedure(table,1,grsetup_tbl_contents);

	/* We will effectively remember the last state of graph_param */
	/* because others do not have scrolling table */
	if (graphPrefs.functype != graph_param)
		firstVisible = 0;

	CtlSetValue(GetObjectPtr(ckbGrAxes), graphPrefs.grEnable[6]);
	CtlSetValue(GetObjectPtr(ckbGrGrid), graphPrefs.grEnable[7]);
	CtlSetValue(GetObjectPtr(ckbGrAxesLabels), graphPrefs.grEnable[8]);

	/* Setup the up/down pointers, grType and grColor buttons*/
	if (grayDisplay || colorDisplay) {
		for (i = grColor1; i <= grBgndLbl; i++)
			FrmShowObject(frm, FrmGetObjectIndex(frm, i));
	}
	if (graphPrefs.functype == graph_param) {
		FrmShowObject(frm,FrmGetObjectIndex(frm,grSetupDown));
		FrmShowObject(frm,FrmGetObjectIndex(frm,grSetupUp));
		if (grayDisplay || colorDisplay) {
			for (i = grColor2; i <= grColor6; i += 2)
				FrmHideObject(frm, FrmGetObjectIndex(frm, i));
		}
	} else {
		for (i = ckbGrfun2; i <= grType6; i += 2)
			FrmShowObject(frm, FrmGetObjectIndex(frm, i));
	}
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_handle_pendown
 * 
 * DESCRIPTION:  Simulates a button in all cells of table
 *
 * PARAMETERS:   bounds of a cell in table, where the user tapped
 *
 * RETURN:       true - user lifted the pen inside bounds of table 
 *                      (button pressed)
 *               false - user lifted pen outside bounds of a field
 *      
 ***********************************************************************/
Int16
grsetup_handle_pendown(Int16 row,Int16 col)
{
	Boolean penDown;
	Boolean inrect;
	Int16 redraw=0;
	Coord x,y;
	Int16 row2,row1=row;
	RectangleType bounds,oldbounds;
	TablePtr tbl = GetObjectPtr(grSetupTable);

	TblGetItemBounds(tbl,row,col,&oldbounds);
#ifdef SUPPORT_DIA
	/* Height for customTableItem is fixed at 11, so we  have to adjust it for Handera */
	oldbounds.extent.y = HanderaCoord(oldbounds.extent.y);
#endif
	WinInvertRectangle(&oldbounds,0);

	do {
		EvtGetPen(&x,&y,&penDown);
		inrect=false;
		if(redraw) redraw++;
		
		for(row2=TblGetNumberOfRows(tbl)-1;(row2>=0 && !inrect);row2--){
			TblGetItemBounds(GetObjectPtr(grSetupTable),row2,col,&bounds);
#ifdef SUPPORT_DIA
			bounds.extent.y = HanderaCoord(bounds.extent.y);
#endif
			if(RctPtInRectangle(x,y,&bounds))
				inrect=true;
		}
		
		if(inrect){
		  row2++;
		  if(row2!=row1){
			if(row1!=row)WinInvertRectangle(&oldbounds,0);
			if(row2!=row)WinInvertRectangle(&bounds,0);
			row1=row2;
			oldbounds=bounds;
		  }		  
		}else{ 
			if(graphPrefs.functype==graph_param){
					if(firstVisible && y<bounds.topLeft.y){
						firstVisible=0;
						redraw=1;
					}else if(!firstVisible){
						TblGetBounds(tbl,&bounds);
#ifdef SUPPORT_DIA
						bounds.extent.y = HanderaCoord(bounds.extent.y);
#endif
						if(y>bounds.topLeft.y+bounds.extent.y){
							firstVisible=TblGetNumberOfRows(tbl);
							redraw=1;
						}
					}
					if(redraw==1){
						row1=row=-1;
						FrmDrawForm(FrmGetActiveForm());
						draw_grType(FrmGetActiveForm());
					}
			}
			if(row1!=row){
				WinInvertRectangle(&oldbounds,0);
				row1=row;
			}
		}
	}while(penDown);

	if (inrect && row2!=row)
		WinInvertRectangle(&bounds,0);

	if(!redraw){
		TblGetItemBounds(tbl,row,col,&oldbounds);
#ifdef SUPPORT_DIA
		bounds.extent.y = HanderaCoord(bounds.extent.y);
#endif*/
		WinInvertRectangle(&oldbounds,0);
	}
	
	return row2;
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_func_tap
 * 
 * DESCRIPTION:  Triggered when user tapped on the function name, show
 *               a varmgr_edit window
 *
 * PARAMETERS:   row - where the function name was pressed
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
grsetup_func_tap(Int16 row) IFACE;
static void
grsetup_func_tap(Int16 row)
{
	char usrname[MAX_FUNCNAME+1]; /* grsetup_def_name returns static variable,
								  * that can change e.g. on form closure
								  * (and update on Debug ROM).
								  * DO not store a pointer
								  */
	char tmptxt[MAX_RSCLEN];
	char *grname;
	
	grname = grsetup_get_fname(row);	
	StrCopy(usrname,grsetup_def_name(row));
	
	if (StrLen(grname)==0) {
		SysCopyStringResource(tmptxt,grSetupStrFunc);
		if (varmgr_edit(usrname,tmptxt,function,false,
						grsetup_param_name())) {
			StrCopy(grname,usrname);
			FrmUpdateForm(grSetupForm,frmUpdateVars);
		}
		return;
	}		
	if (StrCompare(usrname,grname)==0) {
		SysCopyStringResource(tmptxt,grSetupStrFunc);
		if (varmgr_edit(usrname,tmptxt,function,false,
						grsetup_param_name())) {
			StrCopy(grname,usrname);
			FrmUpdateForm(grSetupForm,frmUpdateVars);
		}
	}
	else {
		SysCopyStringResource(tmptxt,varFuncModTitle);
		varmgr_edit(grname,tmptxt,function,false,
					grsetup_param_name());
	}
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_scroll
 * 
 * DESCRIPTION:  Scrolls a table (parameter functions do not fit on 1 
 *               screen)
 *
 * PARAMETERS:   dir - which direction shall we scroll
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
grsetup_scroll(WinDirectionType dir) IFACE;
static void
grsetup_scroll(WinDirectionType dir)
{
	TablePtr table;		
	Int16 tmp;
	
	if (graphPrefs.functype!=graph_param)
	  return;
	table = GetObjectPtr(grSetupTable);
	if (dir == winUp) {
		tmp=firstVisible - TblGetNumberOfRows(table);
		if (tmp<0)
		  tmp = 0;
	}
	else /* winDown */ {
		tmp = firstVisible + TblGetNumberOfRows(table);
		if (tmp > TblGetNumberOfRows(table))
		  tmp = TblGetNumberOfRows(table);
	}
	/* Don't redraw if we did not move */
	if (tmp==firstVisible)
	  return;
	firstVisible = tmp;
	TblMarkTableInvalid(table);
	TblRedrawTable(table);
}

/***********************************************************************
 *
 * FUNCTION:     grsetup_flist_tap
 * 
 * DESCRIPTION:  User tapped the 'X:'/'Y:' etc. Show a popup list of 
 *               defined functions with 'Off' and 'User' at the beginning
 *               Make a table searchable, Off and User have space as 
 *               a 1st character to allow incremental search
 *
 * PARAMETERS:   row, where the 'button' was pressed
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grsetup_flist_tap(Int16 row) IFACE;
static void
grsetup_flist_tap(Int16 row)
{
	dbList *dblist;
	char **displist;
	ListPtr list;
	RectangleType bounds;
	char *fname;
	Int16 i,pos=0;
	char tmptxt[MAX_RSCLEN],txtuser[MAX_RSCLEN],txtnone[MAX_RSCLEN];

	dblist = db_get_list(function);
	displist = MemPtrNew(sizeof(*displist)*(dblist->size+2));
	
	/* displist will contain dynamic and static items, but the dynamic
	 * will be released by db_delete_list with dblist */
	txtnone[0] = ' ';
	SysCopyStringResource(txtnone+1,strLocalDelete);
	txtuser[0] = ' ';
	SysCopyStringResource(txtuser+1,grSetupUser);	
	displist[0] = txtnone;
	displist[1] = txtuser;	
	memcpy((void *)&displist[2],(void *)dblist->list,dblist->size*sizeof(*displist));

	/* Fname is actually pointer to graphPref structure */
	fname = grsetup_get_fname(row);
	if (StrLen(fname)==0)
	  pos = 0;
	else if (StrCompare(fname,grsetup_def_name(row))==0)
	  pos = 1;
	else
	  for (i=0;i<dblist->size;i++)
		if (StrCompare(fname,dblist->list[i])==0)
		  pos = i+2;
	
	TblGetBounds(GetObjectPtr(grSetupTable),&bounds);
	list = GetObjectPtr(grSetupList);
	LstGlueSetIncrementalSearch(list, true);
	LstSetListChoices(list,displist,dblist->size+2);
	LstSetSelection(list,pos);
	LstMakeItemVisible(list,pos);
	LstSetPosition(list,bounds.topLeft.x,bounds.topLeft.y);
	LstSetHeight(list,dblist->size+2>6?6:dblist->size+2);
	
	i = LstPopupList(list);
	
	if (i!=noListSelection && i!=pos) {
		if (i==0){
		  StrCopy(fname,"");
		  db_delete_record(grsetup_def_name(row));
		}
		else if (i==1) { /* The 'User' function get always edit window */
			SysCopyStringResource(tmptxt,grSetupStrFunc);
			varmgr_edit(grsetup_def_name(row),tmptxt,function,false,
						grsetup_param_name());
			StrCopy(fname,grsetup_def_name(row));
		}
		else
		  StrCopy(fname,displist[i]);
		FrmUpdateForm(grSetupForm,frmUpdateVars);
	}
	
	db_delete_list(dblist);
	MemPtrFree(displist);
}

#ifndef SUPPORT_DIA
#define HanderaCoord(x)	(x)
#endif

static void
draw_grType(FormType *frm)
{
	Int8 i,k;
	Coord x, y, x1, x2, x3, y1, y2, y3;
	RectangleType bounds;

	for (i=0; i<6; i++) {
		if (graphPrefs.functype != graph_param || i % 2 == 0) {
			FrmGetObjectBounds(frm, FrmGetObjectIndex(frm,grType1+i), &bounds);
			graph_setcolor(-1);
			WinEraseRectangle(&bounds, 0);
			graph_unsetcolor();
			WinDrawRectangleFrame(roundFrame, &bounds);

			x = bounds.topLeft.x;
			y = bounds.topLeft.y + bounds.extent.y - 1;
			x1 = x + HanderaCoord(3);
			x2 = x + HanderaCoord(6);
			x3 = x + HanderaCoord(9);
			y1 = y - HanderaCoord(1);
			y2 = y - HanderaCoord(3);
			y3 = y - HanderaCoord(5);
			if (graphPrefs.functype == graph_param) {
				k = (firstVisible + i) / 2;
			}
			else
				k = i;

			CtlSetValue(GetObjectPtr(ckbGrfun1+i), graphPrefs.grEnable[k]);

			graph_setcolor(k);
			switch (graphPrefs.grType[k]) {
			case 0:
				x1++; x3--;
				WinDrawLine(x1-1, y2, x1+1, y2);
				WinDrawLine(x1 ,y2-1, x1, y2+1);
				WinDrawLine(x3-1, y2, x3+1, y2);
				WinDrawLine(x3,y2-1, x3, y2+1);
				break;
			case 2:
				WinDrawLine(x1, y1, x1, y1);
				WinDrawLine(x2, y1, x2, y2);
				WinDrawLine(x3, y1, x3, y3);	
				break;
			default:
				WinDrawLine(x1, y2, x3, y2);
				break;
			}
			if (grayDisplay || colorDisplay) {
				/* Paint the line color button */
				FrmGetObjectBounds(frm, FrmGetObjectIndex(frm,grColor1+i), &bounds);
				WinDrawRectangle(&bounds, 0);
			}
			graph_unsetcolor();
		}
	}
	for (i=0; i<3; i++) {
		if (grayDisplay || colorDisplay) {
			graph_setcolor(i == 1 ? -2 : -1); 
			FrmGetObjectBounds(frm, FrmGetObjectIndex(frm,grAxes+i), &bounds);
			if (i == 2 ) {
				WinEraseRectangle(&bounds, 0);
			} else {
				WinDrawRectangle(&bounds, 0);
			}
			graph_unsetcolor();
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:     GraphSetupHandleEvent
 * 
 * DESCRIPTION:  Event handler for graphic Setup form, where
 *               the functions, that should be drawn are selected
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean 
GraphSetupHandleEvent(EventPtr event)
{
	Boolean    handled = false;
	FormPtr    frm=FrmGetActiveForm();
	Int16 controlId;
	Int16 row=0;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	 case tblEnterEvent:
	  {
		Int16 col,row2;
		TablePtr tbl = GetObjectPtr(grSetupTable);
		row = event->data.tblSelect.row;
		col = event->data.tblSelect.column;

		row+=firstVisible;
		row2=grsetup_handle_pendown(row-firstVisible,col);
		if(row-firstVisible<TblGetNumberOfRows(tbl))
			TblMarkRowInvalid(tbl,row-firstVisible);
		TblMarkRowInvalid(tbl,row2);
		row2+=firstVisible;

		if (row2==row) {
			selectedRow = row;
			switch (col) {
			 case COL_FLIST:
				grsetup_flist_tap(selectedRow);
				break;
			 case COL_FUNC:
				grsetup_func_tap(selectedRow);
				break;
			}
		}else if(row2>=firstVisible){
			CError err;
			CodeStack *stack;
			char param[MAX_FUNCNAME+1];
			char *value=grsetup_get_fname(row);
			char *value2=grsetup_get_fname(row2);
			
			StrCopy(param,value2);
			if(StrCompare(value,grsetup_def_name(row)))
				StrCopy(value2,value);
			else StrCopy(value2,grsetup_def_name(row2));
			if(StrCompare(param,grsetup_def_name(row2)))
				StrCopy(value,param);
			else StrCopy(value,grsetup_def_name(row));
			
			err =db_func_description(grsetup_def_name(row),&value,param);
			if(!err){
				stack = text_to_stack(value,&err);
				if(!err){
					StrCopy(parameter_name,param);
					err =db_func_description(grsetup_def_name(row2),&value2,param);
					db_write_function(grsetup_def_name(row2),stack,value);
					stack_delete(stack);
				}
				MemPtrFree(value);
			}else{
				err =db_func_description(grsetup_def_name(row2),&value2,param);
				db_delete_record(grsetup_def_name(row2));
			}
			
			if(!err){
				stack = text_to_stack(value2,&err);
				if(!err){
					StrCopy(parameter_name,param);
					db_write_function(grsetup_def_name(row),stack,value2);
					stack_delete(stack);
				}
				MemPtrFree(value2);
			}else
				db_delete_record(grsetup_def_name(row));
				
			TblRedrawTable(tbl);
		}	
		handled = true;
		break;
	  }
	 case keyDownEvent:
		switch (event->data.keyDown.chr) {
		 case pageDownChr:
			grsetup_scroll(winDown);
			handled = true;
			break;
		 case pageUpChr:
			grsetup_scroll(winUp);
			handled = true;
			break;
		}
		break;
	 case ctlSelectEvent:
		controlId=event->data.ctlSelect.controlID;
		switch (controlId) {
		 case grSetupDone:
			graphPrefs.grEnable[6] = CtlGetValue(GetObjectPtr(ckbGrAxes));
			graphPrefs.grEnable[7] = CtlGetValue(GetObjectPtr(ckbGrGrid));
			graphPrefs.grEnable[8] = CtlGetValue(GetObjectPtr(ckbGrAxesLabels));
			grSetupOpen = false;
			FrmReturnToForm(0);
			FrmUpdateForm(frmGraph, frmRedrawUpdateCode);
			handled = true;
			break;
		 case grSetupDown:
			grsetup_scroll(winDown);
			draw_grType(frm);
			handled = true;
			break;
		 case grSetupUp:
			grsetup_scroll(winUp);
			draw_grType(frm);
			handled = true;
			break;
		 case ckbGrfun1:
		 case grType1:
		 case grColor1:
			if(graphPrefs.functype == graph_param)
				row = firstVisible / 2;
			else
				row = 0;
			break;
		 case ckbGrfun2:
		 case grType2:
		 case grColor2:
			row = 1;
			break;
		 case ckbGrfun3:
		 case grType3:
		 case grColor3:
			if (graphPrefs.functype == graph_param)
				row = firstVisible / 2 + 1;
			else
				row = 2;
			break;
		 case ckbGrfun4:
		 case grType4:
		 case grColor4:
			row = 3;
			break;
		 case ckbGrfun5:
		 case grType5:
		 case grColor5:
			if (graphPrefs.functype == graph_param)
				row = firstVisible / 2 + 2;
			else
				row = 4;
			break;
		 case ckbGrfun6:
		 case grType6:
		 case grColor6:
			row = 5;
			break;
		 case grBgnd:
		 case grAxes:
		 case grGrid:
			row = controlId - grColor1; 
			break;
		}

		if (controlId >= ckbGrfun1 && controlId <= ckbGrfun6) {
			graphPrefs.grEnable[row] = CtlGetValue(GetObjectPtr(controlId));
			handled=true;
		}
		else if (controlId >= grType1 && controlId <= grType6) {
			graphPrefs.grType[row]++;
			if(graphPrefs.grType[row]>2)
			   graphPrefs.grType[row]=0;
			draw_grType(frm);
			handled=true;
		}
		else if (controlId >= grColor1 && controlId <= grBgnd) {
			IndexedColorType col = graphPrefs.colors[row];
			RGBColorType rgb;
			Char *picktitle;

			graph_setcolor(-2);

			picktitle = MemPtrNew(MAX_RSCLEN);
			SysStringByIndex(grColorElem, row, picktitle, MAX_RSCLEN);
			if (UIPickColor(&col, (colorDisplay? &rgb : NULL),
			                UIPickColorStartPalette, picktitle, NULL)) {
				graphPrefs.colors[row] = col;
			}

			graph_unsetcolor();
			MemPtrFree(picktitle);
			handled = true;
		}
		break;

	 case frmUpdateEvent:
		if (event->data.frmUpdate.updateCode == frmUpdateVars) {
			TablePtr tbl;
			
			tbl = GetObjectPtr(grSetupTable);
			TblMarkRowInvalid(tbl,selectedRow-firstVisible);
			TblRedrawTable(tbl);
			handled = true;
		}		  
		break;
	 case winEnterEvent:
		if (grSetupOpen &&
		    event->data.winEnter.enterWindow == (WinHandle) frm) 
			draw_grType(frm);
		break;
	 case frmCloseEvent:
		grSetupOpen = false;
		handled = false;
		break;
	 case frmOpenEvent:
		grSetupOpen = true;
		grsetup_tbl_init();
		FrmDrawForm(frm);
		draw_grType(frm);
		handled=true;
		break;
	}
	  
	return handled;
}
