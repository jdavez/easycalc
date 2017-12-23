/*
 *   $Id: mtxedit.c,v 1.20 2006/09/12 19:40:55 tvoverbe Exp $
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
 *
 *   This editor works automatically with complex matrices and upon
 *   save tries to convert them to the real ones.
*/

#include <PalmOS.h>
#include <LstGlue.h>
#include <WinGlue.h>

#include "calcrsc.h"
#include "konvert.h"
#include "calc.h"
#include "defuns.h"
#include "segment.h"
#include "mtxedit.h"
#include "matrix.h"
#include "cmatrix.h"
#include "calcDB.h"
#include "stack.h"
#include "display.h"
#include "varmgr.h"
#include "matrix.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

CMatrix *matrixValue = NULL;
char matrixName[MAX_FUNCNAME+1];

char matrixDim[MATRIX_MAX][4];
char * matrixDimD[MATRIX_MAX];

static Int16 rowPosition, colPosition;
static Boolean showAns = false;

/***********************************************************************
 *
 * FUNCTION:     mtxedit_tbl_draw_num
 * 
 * DESCRIPTION:  Draw a bold number
 *
 * PARAMETERS:   num - nnumber to draw
 *               bounds - where should the number be drawn
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
mtxedit_tbl_draw_num(Int16 num, RectangleType *bounds) IFACE;
static void
mtxedit_tbl_draw_num(Int16 num, RectangleType *bounds)
{
	FontID oldfont;
	char text[5];

	oldfont = FntSetFont(boldFont);
	StrPrintF(text,"%d",num);
	WinDrawChars(text,StrLen(text),bounds->topLeft.x+3,
		     bounds->topLeft.y);
	FntSetFont(oldfont);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_tbl_contents
 * 
 * DESCRIPTION:  Callback function for drawing matrix table
 *
 * PARAMETERS:   See PalmOS Reference
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
mtxedit_tbl_contents(void *table,Int16 row,Int16 column, 
		     RectangleType *bounds) IFACE;
static void
mtxedit_tbl_contents(void *table,Int16 row,Int16 column, 
		     RectangleType *bounds)
{
	char *text;
	FontID oldfont;

	if (row == 0 && column == 0)
		return;

	if (matrixValue == NULL)
		return;

	if (row == 0) {
		if (column + colPosition - 1 < matrixValue->cols)
			mtxedit_tbl_draw_num(column + colPosition,bounds);
	} else if (column == 0) {
		if (row + rowPosition - 1 < matrixValue->rows)
			mtxedit_tbl_draw_num(row + rowPosition, bounds);
	} else if (row+rowPosition-1 < matrixValue->rows \
		   && column+colPosition-1 < matrixValue->cols) {

		oldfont = FntSetFont(stdFont);
		text = display_complex(MATRIX(matrixValue,
					      row+rowPosition-1,
					      column+colPosition-1));
		WinGlueDrawTruncChars(text,StrLen(text),
				  bounds->topLeft.x+2,bounds->topLeft.y,
				  bounds->extent.x-2);
		MemPtrFree(text);
		FntSetFont(oldfont);
	}
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_save_matrix
 * 
 * DESCRIPTION:  Save the loaded matrix to database
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void mtxedit_save_matrix(void) IFACE;
static void
mtxedit_save_matrix(void)
{
	Trpn item;
	CodeStack *stack;
	Matrix *m;

	stack = stack_new(1);
	if (StrLen(matrixName)) {
		/* Try to scale down to normal matrix */
		m = cmatrix_to_matrix(matrixValue);
		if (m) {
			stack_add_val(stack,&m,matrix);
			matrix_delete(m);
		} else
			stack_add_val(stack,&matrixValue,cmatrix);
		item = stack_pop(stack);
		db_write_variable(matrixName,item);
		rpn_delete(item);
	}
	stack_delete(stack);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_load_matrix
 * 
 * DESCRIPTION:  Load the matrix specified in matrixName to 
 *               variable matrixValue. If it doesn't exist, set
 *               matrixName to ""
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
mtxedit_load_matrix(void) IFACE;
static void
mtxedit_load_matrix(void)
{
	Trpn item;
	CError err;

	if (matrixValue != NULL) {
		cmatrix_delete(matrixValue);
		matrixValue = NULL;
	}

	if (StrLen(matrixName) == 0)
		return;

	item = db_read_variable(matrixName,&err);
	if (err) {
		StrCopy(matrixName,"");
		return;
	}

	if (item.type == matrix) 
		matrixValue = matrix_to_cmatrix(item.u.matrixval);
	else if (item.type == cmatrix)
		matrixValue = cmatrix_dup(item.u.cmatrixval);
	else {
		StrCopy(matrixName,"");
		rpn_delete(item);
		return;
	}
	rpn_delete(item);
	CtlSetLabel(GetObjectPtr(popMatrix),matrixName);

	rowPosition = 0;
	colPosition = 0;
}


/***********************************************************************
 *
 * FUNCTION:     mtxedit_dim_label
 * 
 * DESCRIPTION:  Set the popup list texts to reflect dimension of matrix
 *
 * PARAMETERS:   
 *
 * RETURN:       
 *      
 ***********************************************************************/
static void mtxedit_dimlabel(void) IFACE;
static void mtxedit_dimlabel(void)
{
	if (matrixValue !=NULL) {
		CtlSetLabel(GetObjectPtr(popMatrixRows),
			    matrixDim[matrixValue->rows-1]);
		CtlSetLabel(GetObjectPtr(popMatrixCols),
			    matrixDim[matrixValue->cols-1]);
	} else {
		CtlSetLabel(GetObjectPtr(popMatrixRows),"");
		CtlSetLabel(GetObjectPtr(popMatrixCols),"");
	}
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_init
 * 
 * DESCRIPTION:  Initialize the matrix editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void mtxedit_init(void) IFACE;
static void
mtxedit_init(void)
{
	Int16 i;
	TablePtr table;
	UInt16 rows;
	UInt16 size;

	table = GetObjectPtr(tblMatrix);
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
	
	TblSetCustomDrawProcedure(table,0,mtxedit_tbl_contents);
	TblSetCustomDrawProcedure(table,1,mtxedit_tbl_contents);
	TblSetCustomDrawProcedure(table,2,mtxedit_tbl_contents);
	TblSetCustomDrawProcedure(table,3,mtxedit_tbl_contents);

	rowPosition = colPosition = 0;

	if (showAns) {
		showAns = false;
		StrCopy(matrixName,"ans");
	} else {
		size = MAX_FUNCNAME + 1;
		if (PrefGetAppPreferences(APP_ID,PREF_MTXEDIT,&matrixName,&size,
					  false)!=PREF_MTXEDIT_VERSION)
			StrCopy(matrixName,"");
	}


	for (i=0;i < MATRIX_MAX;i++) {
		StrPrintF(matrixDim[i], "%d", i+1);
		matrixDimD[i] = matrixDim[i];
	}
	LstSetListChoices(GetObjectPtr(lstMatrixDim),matrixDimD,MATRIX_MAX);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_destroy
 * 
 * DESCRIPTION:  Destructor for matrix editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void mtxedit_destroy(void) IFACE;
static void
mtxedit_destroy(void)
{
	PrefSetAppPreferences(APP_ID,PREF_MTXEDIT,PREF_MTXEDIT_VERSION,&matrixName,
			      MAX_FUNCNAME+1,false);
	if (matrixValue != NULL) {
		cmatrix_delete(matrixValue);
		matrixValue = NULL;
	}
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_table_pendown
 * 
 * DESCRIPTION:  Handle tap on the cell of a table 
 *
 * PARAMETERS:   bounds - bounds of a cell
 *
 * RETURN:       true - user selected cell
 *               false - user lifted pen outside of the cell bounds
 *      
 ***********************************************************************/
static Boolean mtxedit_table_pendown(RectangleType *bounds) IFACE;
static Boolean
mtxedit_table_pendown(RectangleType *bounds)
{
    Boolean penDown;
    Boolean inrect = true;
    Coord x,y;
	
    WinInvertRectangle(bounds,0);
    do {
	EvtGetPen(&x,&y,&penDown);
	if (RctPtInRectangle(x,y,bounds) && !inrect) {
	    inrect = true;
	    WinInvertRectangle(bounds,0);
	} else if (!RctPtInRectangle(x,y,bounds) && inrect) {
	    inrect = false;
	    WinInvertRectangle(bounds,0);
	}		
    }while(penDown);
    if (inrect)
	WinInvertRectangle(bounds,0);
    return inrect;
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_select_dim
 * 
 * DESCRIPTION:  Handle when user selects a new size of matrix
 *               (reallocate data etc.)
 *
 * PARAMETERS:   ctlid - ctlid of a pressed popup(row or column change)
 *
 * RETURN:       true - user selected new dimension
 *               false - user didn't select
 *      
 ***********************************************************************/
static Boolean mtxedit_select_dim(Int16 ctlid) IFACE;
static Boolean 
mtxedit_select_dim(Int16 ctlid)
{
	Int16 i,j;
	CMatrix *nm;
	Int16 rows,cols;
	RectangleType bounds;
	ListPtr dimlst;

	if (matrixValue == NULL)
		return false;
	
	rows = matrixValue->rows;
	cols = matrixValue->cols;
	
	dimlst = GetObjectPtr(lstMatrixDim);

	if (ctlid == popMatrixRows) {
		LstSetSelection(dimlst,rows-1);
		LstMakeItemVisible(dimlst,rows - 1);
	} else {
		LstSetSelection(dimlst,cols-1);
		LstMakeItemVisible(dimlst,cols - 1);
	}
	FrmGetObjectBounds(FrmGetActiveForm(),
			   FrmGetObjectIndex(FrmGetActiveForm(),ctlid),
			   &bounds);
	LstSetPosition(dimlst,bounds.topLeft.x,bounds.topLeft.y);

	i = LstPopupList(dimlst);
	if (i == noListSelection) 
		return false;
	
	if (ctlid == popMatrixRows) 
		nm = cmatrix_new(i+1,cols);
	else
		nm = cmatrix_new(rows,i+1);

	if (!nm)
	    return false;

	for (i=0;i<matrixValue->rows && i < nm->rows; i++)
		for (j=0;j<matrixValue->cols && j<nm->cols;j++)
			MATRIX(nm,i,j) = MATRIX(matrixValue,i,j);
	cmatrix_delete(matrixValue);
	matrixValue = nm;

	return true;
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_select
 * 
 * DESCRIPTION:  Handle the user tap on the matrix selector
 *               allows creating new matrix, deleting etc.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - new matrix loaded
 *               false - no change
 *      
 ***********************************************************************/
static Boolean mtxedit_select(void) IFACE;
static Boolean
mtxedit_select(void)
{
	ListPtr slst;
	dbList *matrices,*cmatrices;
	char loct1[MAX_RSCLEN],loct2[MAX_RSCLEN],loct3[MAX_RSCLEN];
	char **values;
	Int16 i,j,k,l,compsize;

	slst = GetObjectPtr(lstMatrix);
	
	matrices = db_get_list(matrix);
	cmatrices = db_get_list(cmatrix);

	compsize = 3 + matrices->size + cmatrices->size;
	values = MemPtrNew(sizeof(*values) * compsize);

	loct1[0] = loct2[0] = loct3[0] = ' ';
	SysCopyStringResource(loct1+1,strLocalNew);
	SysCopyStringResource(loct2+1,strLocalDelete);
	SysCopyStringResource(loct3+1,strLocalSaveAs);
	values[0] = loct1;
	values[1] = loct2;
	values[2] = loct3;

	for (j=0,i=3;j<matrices->size;i++,j++)
		values[i] = matrices->list[j];
	/* Insert sort cmatrices */
	for (j=0;j < cmatrices->size;j++,i++) {
		for (k=3;k < i && StrCompare(cmatrices->list[j],values[k]) > 0;k++)
			;
		for (l=i;l > k;l--)
			values[l] = values[l-1];
		values[k] = cmatrices->list[j];
	}
		     
	LstSetListChoices(slst,values,compsize);
	LstSetHeight(slst,compsize < 10 ? compsize : 10);
	LstSetSelection(slst,0);
	for (i=3;i<compsize;i++) {
		if (StrCompare(values[i],matrixName) == 0)
			LstSetSelection(slst,i);
	}
	/* Set incremental search */
	LstGlueSetIncrementalSearch(slst, true);

	i = LstPopupList(slst);
	
	if (i != noListSelection) {
		if (i > 2) {
			mtxedit_save_matrix();
			StrCopy(matrixName,values[i]);
		} else if (i == 2 && StrLen(matrixName)) {
			char text[MAX_FUNCNAME+1];
			if (varmgr_get_varstring(text,"List Name")) {
				mtxedit_save_matrix();
				StrCopy(matrixName,text);
				mtxedit_save_matrix();
			}
		} else if (i == 1 && StrLen(matrixName)) {
			/* Delete */
			if (!FrmCustomAlert(altConfirmDelete,"matrix",
					    matrixName,NULL)) {
				db_delete_record(matrixName);
				StrCopy(matrixName,"");
				cmatrix_delete(matrixValue);
				CtlSetLabel(GetObjectPtr(popMatrix),"");
				matrixValue = NULL;
			} else
				i = noListSelection;
		} else if (i == 0) {
			char text[MAX_FUNCNAME+1];

			if (varmgr_get_varstring(text,"Matrix Name")) {
				mtxedit_save_matrix();

				StrCopy(matrixName, text);
				if (matrixValue != NULL)
					cmatrix_delete(matrixValue);
				matrixValue = cmatrix_new(1,1);
				mtxedit_save_matrix();
			}
		}
	}
	MemPtrFree(values);
	db_delete_list(matrices);
	db_delete_list(cmatrices);	     

	return i != noListSelection;
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_tbl_scroll
 * 
 * DESCRIPTION:  Handle up/down scrolls of matrix table
 *
 * PARAMETERS:   tbl - Pointer to matrix table
 *               delta - how many lines to scroll
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
mtxedit_tbl_scroll(TablePtr tbl,Int16 delta)
{
	RectangleType r;
	Int16 i,rows;
	Int16 oldfirstline;

	if (matrixValue == NULL)
		return;

	rows = TblGetNumberOfRows(tbl) - 1;

	oldfirstline = rowPosition;
	rowPosition +=delta;
	
	if (rowPosition > matrixValue->rows - rows)
		rowPosition = matrixValue->rows - rows;

	if (rowPosition < 0)
		rowPosition = 0;

	delta = rowPosition - oldfirstline;
	if (delta > 0)
		i = delta;
	else
		i = -delta;

	if (i < rows) {
		TblGetBounds(tbl,&r);
		r.topLeft.y += TblGetRowHeight(tbl,0);
		r.extent.y -= TblGetRowHeight(tbl,0);
		WinScrollRectangle(&r, (delta < 0) ? winDown : winUp, 
				   TblGetRowHeight(tbl, 0) * i, &r);
		while (i--) 
			TblMarkRowInvalid(tbl, (delta < 0) ? i +1: rows-i);
	} else
		TblMarkTableInvalid(tbl);

	i = colPosition;
	if (colPosition + 3 > matrixValue->cols)
		colPosition = matrixValue->cols - 3;
	if (colPosition < 0)
		colPosition = 0;
	if (i != colPosition)
		TblMarkTableInvalid(tbl);

	TblRedrawTable(tbl);
}	

/***********************************************************************
 *
 * FUNCTION:     mtxedit_update_scrollbar
 * 
 * DESCRIPTION:  Update the scrollbra position acording to a real
 *               position of a table
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
mtxedit_update_scrollbar(void)
{
	TablePtr table;
	UInt16 rows;
	FormPtr frm;

	frm = FrmGetActiveForm();
	table = GetObjectPtr(tblMatrix);
	rows = TblGetNumberOfRows(table) - 1;
	
	if (matrixValue == NULL 
	    || (rowPosition == 0 && matrixValue->rows <= rows)) 
		SclSetScrollBar(GetObjectPtr(sclMatrix),rowPosition,0,0,0);
	else 
		SclSetScrollBar(GetObjectPtr(sclMatrix),rowPosition,0,
				matrixValue->rows - rows,
				rows);
	if (matrixValue == NULL || colPosition == 0) 
		FrmHideObject(frm,FrmGetObjectIndex(frm,btnMatrixLeft));
	else
		FrmShowObject(frm,FrmGetObjectIndex(frm,btnMatrixLeft));

	if (matrixValue == NULL || colPosition >= matrixValue->cols - 3)
		FrmHideObject(frm,FrmGetObjectIndex(frm,btnMatrixRight));
	else
		FrmShowObject(frm,FrmGetObjectIndex(frm,btnMatrixRight));
}


Boolean
MatrixHandleEvent(EventPtr event)
{
	Boolean handled = false;
	FormPtr frm;
	Int16 controlID;
	Int16 row,col,delta;
	RectangleType bounds;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	case tblEnterEvent:
		row = event->data.tblEnter.row;
		col = event->data.tblEnter.column;
		if (matrixValue != NULL && row > 0 && col > 0 \
		    && row+rowPosition-1<matrixValue->rows \
		    && col+colPosition-1<matrixValue->cols) {

			TblGetItemBounds(GetObjectPtr(tblMatrix),
					 row,col,&bounds);
			if (mtxedit_table_pendown(&bounds)) {
				row += rowPosition - 1;
				col += colPosition - 1;
				if (varmgr_get_complex(&MATRIX(matrixValue,
							       row,col),
						      "Matrix Item")) {
					TblMarkRowInvalid(GetObjectPtr(tblMatrix),
							  row - rowPosition + 1);
					TblRedrawTable(GetObjectPtr(tblMatrix));
				}
			}
		}
		handled = true;
		break;
	case keyDownEvent:
		switch (event->data.keyDown.chr) {
		case pageDownChr:
			mtxedit_tbl_scroll(GetObjectPtr(tblMatrix),8);
			mtxedit_update_scrollbar();
			handled = true;
			break;
		case pageUpChr:
			mtxedit_tbl_scroll(GetObjectPtr(tblMatrix),-8);
			mtxedit_update_scrollbar();
			handled = true;
			break;
		}
		break;
	case sclRepeatEvent:
		delta = event->data.sclRepeat.newValue - event->data.sclRepeat.value;
		mtxedit_tbl_scroll(GetObjectPtr(tblMatrix),delta);
		handled = false;
		break;
	case ctlSelectEvent:
		controlID = event->data.ctlSelect.controlID;
		switch (controlID) {
		case btnMatrixLeft:
			if (matrixValue != NULL) {
				colPosition -= 3;
				if (colPosition < 0)
					colPosition = 0;
				mtxedit_update_scrollbar();
				TblMarkTableInvalid(GetObjectPtr(tblMatrix));
				TblRedrawTable(GetObjectPtr(tblMatrix));
			}
			handled = true;
			break;
		case btnMatrixRight:
			if (matrixValue != NULL) {
				colPosition += 3;
				if (colPosition + 3 > matrixValue->cols)
					colPosition = matrixValue->cols - 3;
				if (colPosition < 0)
					colPosition = 0;
				mtxedit_update_scrollbar();
				TblMarkTableInvalid(GetObjectPtr(tblMatrix));
				TblRedrawTable(GetObjectPtr(tblMatrix));
			}
			handled = true;
			break;
		case popMatrixRows:
		case popMatrixCols:
			if (mtxedit_select_dim(controlID)) {
				mtxedit_dimlabel();
				mtxedit_tbl_scroll(GetObjectPtr(tblMatrix),0);
				mtxedit_update_scrollbar();
				TblMarkTableInvalid(GetObjectPtr(tblMatrix));
				TblRedrawTable(GetObjectPtr(tblMatrix));
			}
			handled = true;
			break;
		case btnMatrixDone:
			mtxedit_save_matrix();
			mtxedit_destroy();
			FrmReturnToForm(0);
			handled = true;
			break;
		case popMatrix:
			if (mtxedit_select()) {
				mtxedit_load_matrix();
				mtxedit_dimlabel();
				mtxedit_update_scrollbar();
				TblMarkTableInvalid(GetObjectPtr(tblMatrix));
				TblRedrawTable(GetObjectPtr(tblMatrix));
			}
			handled = true;
			break;
		}
		break;
	case frmOpenEvent:
		frm = FrmGetActiveForm();
		mtxedit_init();
		mtxedit_load_matrix();
		mtxedit_dimlabel();
		mtxedit_update_scrollbar();
		FrmDrawForm(frm);
		
		handled = true;
		break;		
	case frmCloseEvent:
		mtxedit_save_matrix();
		mtxedit_destroy();
	        /* FrmHandleEvent must handle this event itself */
		handled = false;
		break;
	}
	
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_popup_ans
 * 
 * DESCRIPTION:  Popup a matrix editor, but show matrix stored in 
 *               'ans' variable
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
mtxedit_popup_ans(void)
{
	showAns = true;
	FrmPopupForm(frmMatrix);
}
