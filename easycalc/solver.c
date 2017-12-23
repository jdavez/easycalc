/*
 *   $Id: solver.c,v 1.15 2007/09/28 01:23:25 tvoverbe Exp $
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
#include <LstGlue.h>
#include <MathLib.h>
#include <string.h>
#include "konvert.h"
#include "calcDB.h"
#include "defuns.h"
#include "display.h"
#include "stack.h"
#include "calc.h"
#include "calcrsc.h"
#include "varmgr.h"
#include "calc.h"
#include "solver.h"
#include "funcs.h"
#include "integ.h"
#include "mathem.h"
#include "prefs.h"
#include "dbutil.h"

#ifdef HANDERA_SDK
#include "Vga.h"
#endif
#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

#define DEFAULT_MIN -1E5
#define DEFAULT_MAX 1E5

static struct {
    char **list;
    int size;
}varitems;
static char HORIZ_ELLIPSIS;
static Int16 selectedLine = -1;
static Int16 selectedWorksheet = -1;

static double worksheet_min;
static double worksheet_max;
static double worksheet_prec;
static char worksheet_title[MAX_WORKSHEET_TITLE+1];
static char *worksheet_note = NULL;

static DmOpenRef gDB;
char oldparam[MAX_FUNCNAME+1];

struct dbworksheet {
	double min,max,prec;
	char title[MAX_WORKSHEET_TITLE+1];
	char equation[0];
};

#define VARNAME_WIDTH  42
/***********************************************************************
 *
 * FUNCTION:     slv_draw_item
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
static void slv_draw_item(Int16 itemNum,RectangleType *bounds,
			     Char **itemText) IFACE;
static void
slv_draw_item(Int16 itemNum,RectangleType *bounds,
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

	WinDrawChars(itemText[itemNum],StrLen(itemText[itemNum]),
		     bounds->topLeft.x,bounds->topLeft.y);
	
	item = db_read_variable(varitems.list[itemNum],&err);
	if (!err) {
		text = display_default(item,false);
		rpn_delete(item);
	} else {
		text = MemPtrNew(MAX_RSCLEN);
		SysCopyStringResource(text,strUndefined);
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

Int16 slv_db_close() IFACE;
Int16 
slv_db_close()
{
	return DmCloseDatabase(gDB);
}


/***********************************************************************
 *
 * FUNCTION:     slv_add_variable
 * 
 * DESCRIPTION:  Adds variable name to the 'varitems' variable list
 *               (list of variables of the equation)
 *
 * PARAMETERS:   varname
 *
 * RETURN:       
 *      
 ***********************************************************************/
static void
slv_add_variable(char *varname) IFACE;
static void
slv_add_variable(char *varname)
{
	Int16 i,j;
	
	if (is_constant(varname))
		return;
	for (i=0;i<varitems.size && StrCompare(varname,varitems.list[i])>=0;
	     i++) {
		if (StrCompare(varname,varitems.list[i]) == 0)
			return;
	}
	/* Move all items one position forward */
	for (j=varitems.size;j>i;j--)
		varitems.list[j] = varitems.list[j-1];
	varitems.list[i] = MemPtrNew(StrLen(varname)+1);
	StrCopy(varitems.list[i],varname);
	varitems.size++;
}

static const Int16 general_controls[] = {slvEquation,
					 slvVarList,
					 slvModify,
					 slvUpdate,
					 slvDelete,
					 slvOptions,
					 slvNote,
					 slvHelp,
					 0};

/***********************************************************************
 *
 * FUNCTION:     slv_update_worksheet
 * 
 * DESCRIPTION:  This is called when there is a change of active worksheet
 *               - sets all internal variables to worksheet values
 *               - shows/hide display controls, sets texts etc.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void slv_update_worksheet(void) IFACE;
static void
slv_update_worksheet(void)
{
	Int16 i;
	FormPtr frm;
	struct dbworksheet *record;
	MemHandle recordHandle;
	FieldPtr eqfld;
	char *noteptr;

	frm = FrmGetActiveForm();

	if (selectedWorksheet == -1) {
		for (i=0;general_controls[i];i++)
			FrmHideObject(frm,FrmGetObjectIndex(frm,
							    general_controls[i]));
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvSolve));
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvCalculate));
		StrCopy(worksheet_title,"");
		CtlSetLabel(GetObjectPtr(slvWorksheet),worksheet_title);
		return;
	}
	
	recordHandle = DmQueryRecord(gDB,selectedWorksheet);
	record = MemHandleLock(recordHandle);
	StrCopy(worksheet_title,record->title);
	worksheet_min = record->min;
	worksheet_max = record->max;
	worksheet_prec = record->prec;

	eqfld = GetObjectPtr(slvEquation);
	FldDelete(eqfld,0, FldGetTextLength(eqfld));
	FldInsert(eqfld, record->equation,StrLen(record->equation));

	noteptr = record->equation + StrLen(record->equation) + 1;
	StrCopy(worksheet_note,noteptr);

	MemHandleUnlock(recordHandle);

	CtlSetLabel(GetObjectPtr(slvWorksheet),worksheet_title);

	for (i=0;general_controls[i];i++)
		FrmShowObject(frm,FrmGetObjectIndex(frm,
						    general_controls[i]));
	FrmSetFocus(frm,FrmGetObjectIndex(frm,slvEquation));
	selectedLine = -1;
}

/***********************************************************************
 *
 * FUNCTION:     slv_init_varlist
 * 
 * DESCRIPTION:  Initialize the on-screen list with names of variables
 *               values get computed on-the-fly while drawing list
 *
 * PARAMETERS:   None
 *
 * RETURN:       If 1, we shall redraw the List
 *      
 ***********************************************************************/
static Int16
slv_init_varlist(void) IFACE;
static Int16
slv_init_varlist(void)
{
	CodeStack *stack=NULL;
	ListPtr list;
	char *equation;
	CError err;
	Int16 varcount,i;
	FormPtr frm;

	if (selectedWorksheet == -1) {
		LstSetListChoices(GetObjectPtr(slvVarList),NULL,0);
		return -1;
	}

	frm = FrmGetActiveForm();
	equation = FldGetTextPtr(GetObjectPtr(slvEquation));

	if (!equation) {
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvSolve));
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvCalculate));

		LstSetListChoices(GetObjectPtr(slvVarList),NULL,0);
		selectedLine = -1;
		return 0;
	}

	StrCopy(oldparam,parameter_name);
	StrCopy(parameter_name,"**");
	stack = text_to_stack(equation,&err);
	StrCopy(parameter_name,oldparam);
	if (err) {
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvSolve));
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvCalculate));

		selectedLine = -1;
		LstSetListChoices(GetObjectPtr(slvVarList),NULL,0);
		return 0;
	}

	/* Now get the number of variables froms tack */
	varcount = 0;
	for (i=0;i<stack->size;i++) {
	    if (stack->stack[i].type == variable ||
		stack->stack[i].type == litem)
		varcount++;
	}
	varitems.list = MemPtrNew(varcount * sizeof(*varitems.list));
	varitems.size = 0;
	
	for (i=0;i<stack->size;i++) {
		if (stack->stack[i].type == variable)
			slv_add_variable(stack->stack[i].u.varname);
		else if (stack->stack[i].type == litem)
			slv_add_variable(stack->stack[i].u.litemval.name);
	}

	list = GetObjectPtr(slvVarList);
	LstSetListChoices(list,varitems.list,varitems.size);
	LstSetDrawFunction(list,slv_draw_item);

	/* Now distinguish if it is equation or not */
	if (stack->stack[0].type == function &&
	    stack->stack[0].u.funcval.offs == FUNC_EQUAL) {
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvCalculate));
		FrmShowObject(frm,FrmGetObjectIndex(frm,slvSolve));
	} else {
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvSolve));
		FrmShowObject(frm,FrmGetObjectIndex(frm,slvCalculate));
	}

	stack_delete(stack);
	return 0;
}

/***********************************************************************
 *
 * FUNCTION:     slv_solve_real
 * 
 * DESCRIPTION:  Solve the equation using fzero() for variable varname
 *               and store the result in the variable
 *
 * PARAMETERS:   equation,varname
 *
 * RETURN:       err
 *      
 ***********************************************************************/
static CError slv_solve_real(char *equation,char *varname) IFACE;
static CError
slv_solve_real(char *equation,char *varname)
{
	CodeStack *stack;
	CError err;
	double result;
	
	StrCopy(oldparam,parameter_name);
	StrCopy(parameter_name,varname);
	stack = text_to_stack(equation,&err);
	StrCopy(parameter_name,oldparam);
	if (err)
		return err;
	
	/* Now we expect, that FUNC_EQUAL is the first item */
	/* And we should definitely check funcval.num istead, but
	 * this is not unique yet */
	if (stack->stack[0].type != function ||
	    stack->stack[0].u.funcval.offs != FUNC_EQUAL) {
		stack_delete(stack);
		return c_syntax;
	}
	/* Now change '=' to '-' */
	stack->stack[0].u.funcval.offs = FUNC_MINUS;
	stack->stack[0].u.funcval.num = FUNC_MINUS;
	/* And now let's solve it */
	result = integ_zero(worksheet_min, worksheet_max,0.0,
			    stack,worksheet_prec,MATH_FZERO,NULL);
	if (!finite(result)) {
		stack_delete(stack);
		return c_compimp;
	}
	/* Round the result somewhat */
//	result /= 1E-5;
//	result = round(result) * 1E-5;
	/* And now save the result */
	stack_delete(stack);
	db_save_real(varname,result);

	return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     slv_solve
 * 
 * DESCRIPTION:  This is called when button 'solve' is pressed
 *               and it solves equation for the selected variable
 *
 * PARAMETERS:   selection - selected variable
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void slv_solve(Int16 selection) IFACE;
static void slv_solve(Int16 selection)
{
	CError err;

	wait_draw();
	err = slv_solve_real(FldGetTextPtr(GetObjectPtr(slvEquation)),
			     varitems.list[selection]);
	wait_erase();
	if (!err) 
		FrmUpdateForm(slvForm,frmUpdateVars);
	else
		alertErrorMessage(err);
}

/***********************************************************************
 *
 * FUNCTION:     slv_calculate
 * 
 * DESCRIPTION:  Calculate result of the expression
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
slv_calculate(void) IFACE;
static void
slv_calculate(void)
{
	CodeStack *stack;
	char *equation;
	CError err;
	Trpn result;
	char *text;

	equation = FldGetTextPtr(GetObjectPtr(slvEquation));
	if (!equation)
		return;
	
	StrCopy(oldparam,parameter_name);
	StrCopy(parameter_name,"**");
	stack = text_to_stack(equation,&err);
	StrCopy(parameter_name,oldparam);
	if (!err) {
		if (stack->stack[0].type == function && 
		    stack->stack[0].u.funcval.offs == FUNC_EQUAL)
			return;
		
		err = stack_compute(stack);
		if (!err) {
			result = stack_pop(stack);
			text = display_default(result,false);
			FrmCustomAlert(altSolverResult,text,NULL,NULL);
			db_write_variable("solvres",result);
			MemPtrFree(text);
			rpn_delete(result);
		}
		stack_delete(stack);
	}
	if (err)
		alertErrorMessage(err);
}

/***********************************************************************
 *
 * FUNCTION:     slv_destroy_varlist
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
slv_destroy_varlist(void) IFACE;
static void
slv_destroy_varlist(void)
{
	Int16 i;

	if (!varitems.list)
	  return;
	
	for (i=0;i<varitems.size;i++) 
		MemPtrFree(varitems.list[i]);
	MemPtrFree(varitems.list);
	varitems.list = NULL;
	varitems.size = 0;
}


/***********************************************************************
 *
 * FUNCTION:     slv_modify
 * 
 * DESCRIPTION:  Popup variable modification dialog
 *
 * PARAMETERS:   i - selected variable
 *
 * RETURN:       true - var changed, false - otherwise
 *      
 ***********************************************************************/
static Boolean
slv_modify(Int16 i) IFACE;
static Boolean
slv_modify(Int16 i)
{
	char tmptxt[MAX_RSCLEN];

	SysCopyStringResource(tmptxt,varVarModTitle);
	return varmgr_edit(varitems.list[i],tmptxt,
			   variable,false,NULL);
}


/***********************************************************************
 *
 * FUNCTION:     slv_compare_record
 * 
 * DESCRIPTION:  Compare 2 records in solver database - used for 
 *               binary search in solver DB
 *
 * PARAMETERS:   
 *
 * RETURN:       
 *      
 ***********************************************************************/
static Int16 slv_compare_record(struct dbworksheet *rec1,
				struct dbworksheet *rec2,
				Int16 unesedInt16,SortRecordInfoPtr unused1,
				SortRecordInfoPtr unused2,
				MemHandle appInfoH) MLIB;
static Int16 
slv_compare_record(struct dbworksheet *rec1,struct dbworksheet *rec2,
		   Int16 unesedInt16,SortRecordInfoPtr unused1,
		   SortRecordInfoPtr unused2,
		   MemHandle appInfoH)
{
	return StrCompare(rec1->title,rec2->title);
}

/***********************************************************************
 *
 * FUNCTION:     slv_db_open
 * 
 * DESCRIPTION:  Open the solver database, create new one if needed
 *
 * PARAMETERS:   
 *
 * RETURN:       0 - success, err - otherwise
 *      
 ***********************************************************************/
static Int16 slv_db_open() IFACE;
static Int16 
slv_db_open()
{
	return open_db(SOLVERDBNAME, SOLVER_DB_VERSION, LIB_ID, SOLVERDBTYPE,
	               &gDB);
}

/***********************************************************************
 *
 * FUNCTION:     slv_worksheet_name
 * 
 * DESCRIPTION:  Get name of worksheet on the current database position
 *
 * PARAMETERS:   pos - position in the database
 *
 * RETURN:       MemPtrNew()'d name of the database
 *      
 ***********************************************************************/
static char * slv_worksheet_name(Int16 pos) IFACE;
static char *
slv_worksheet_name(Int16 pos)
{
	MemHandle recordHandle;
	struct dbworksheet *record;
	char *result;
	
	recordHandle = DmQueryRecord(gDB,pos);
	record = MemHandleLock(recordHandle);
	result = MemPtrNew(StrLen(record->title) + 1);
	StrCopy(result,record->title);
	MemHandleUnlock(recordHandle);
	
	return result;
}

/***********************************************************************
 *
 * FUNCTION:     slv_ask_name
 * 
 * DESCRIPTION:  Ask the user for the new name of the worksheet
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       NULL - if user tapped cancel, name otherwise
 *      
 ***********************************************************************/
static char * slv_ask_name(void) IFACE;
static char *
slv_ask_name(void)
{
	char *result=NULL,*fldtext;
	FormPtr frm;
	FieldPtr field;
	Int16 button;
	Boolean res;

	frm = FrmInitForm(slvTitleForm);
#ifdef HANDERA_SDK
	if (handera)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	FrmSetFocus(frm,FrmGetObjectIndex(frm,slvTitle));
	field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,slvTitle));
	
	while (1) {
		button = FrmDoDialog(frm);
		if (button == slvTitleCancel) {
			res = false;
			break;
		}
		fldtext = FldGetTextPtr(field);
		if (fldtext && StrLen(fldtext)) {
			result = MemPtrNew(StrLen(fldtext)+1);
			StrCopy(result,fldtext);
			res = true;
			break;
		}
	}

	FrmDeleteForm(frm);
	if (res)
		return result;
	else
		return NULL;
}

/***********************************************************************
 *
 * FUNCTION:     slv_save_worksheet_vars
 * 
 * DESCRIPTION:  Create new worksheet and save it to database. If the 
 *               itemnum is specified, replace the worksheet number 'itemnum'
 *               by the worksheet being saved
 *
 * PARAMETERS:   
 *
 * RETURN:       New record (worksheet) position in datbase
 *      
 ***********************************************************************/
static Int16 slv_save_worksheet_vars(Int16 itemnum,char *title,char *equation,
				     double min,double max,double prec,
				     char *note) IFACE;
static Int16
slv_save_worksheet_vars(Int16 itemnum,char *title,char *equation,
		   double min,double max,double prec,
		   char *note)
{
	struct dbworksheet *newrecord,*newrecordptr;
	MemHandle myRecordMemHandle;
	UInt16 recordNumber,size;

	if (equation)
		size = sizeof(*newrecord)+StrLen(equation)+1+\
			StrLen(note)+1;
	else
		size = sizeof(*newrecord)+1+StrLen(note)+1;
	newrecord = MemPtrNew(size);
	StrCopy(newrecord->title,title);
	if (equation)
		StrCopy(newrecord->equation,equation);
	else
		newrecord->equation[0] = '\0';
	StrCopy(newrecord->equation+StrLen(newrecord->equation)+1,
		note);
	newrecord->min = min;
	newrecord->max = max;
	newrecord->prec = prec;
	
	if (itemnum != -1) {
		/* Delete existing record */
		DmRemoveRecord(gDB,itemnum);
	}
	/* Create a new one */
	recordNumber = DmFindSortPosition(gDB,newrecord,0,
					  (DmComparF *)slv_compare_record,0);
	myRecordMemHandle = DmNewRecord(gDB,&recordNumber,size);
	newrecordptr = MemHandleLock(myRecordMemHandle);
	DmWrite(newrecordptr,0,newrecord,size);
	MemHandleUnlock(myRecordMemHandle);	
	DmReleaseRecord(gDB,recordNumber,true); /* Now the recordNumber contains real index */
	MemPtrFree(newrecord);
	return recordNumber;
}

/***********************************************************************
 *
 * FUNCTION:     slv_save_worksheet
 * 
 * DESCRIPTION:  Saves current selected worksheet to database
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       New worksheet position in database
 *      
 ***********************************************************************/
static Int16 slv_save_worksheet(void) IFACE;
static Int16 
slv_save_worksheet(void)
{
	char *eq;

	if (selectedWorksheet == -1)
		return -1;

	eq = FldGetTextPtr(GetObjectPtr(slvEquation));
	
	return slv_save_worksheet_vars(selectedWorksheet,worksheet_title,
				       eq,worksheet_min,worksheet_max,
				       worksheet_prec,worksheet_note);
}

/***********************************************************************
 *
 * FUNCTION:     slv_new_worksheet
 * 
 * DESCRIPTION:  Create a new blank worksheet with given name. If such
 *               worksheet already exist, do not overwrite it and instead
 *               switch to the old one
 *
 * PARAMETERS:   name of the new worksheet
 *
 * RETURN:       Worksheet position in database
 *      
 ***********************************************************************/
static Int16 slv_new_worksheet(char *name) IFACE;
static Int16
slv_new_worksheet(char *name)
{
	UInt16 recordNumber;
	MemHandle myRecordMemHandle;
	struct dbworksheet *newrecord,*newrecordptr;
	UInt16 size;
	
	size = sizeof(*newrecord)+2;
	newrecord = MemPtrNew(size);
	StrCopy(newrecord->title,name);
	newrecord->equation[0] = '\0';
	newrecord->equation[1] = '\0';
	newrecord->min = DEFAULT_MIN;
	newrecord->max = DEFAULT_MAX;
	newrecord->prec = DEFAULT_ERROR;

	recordNumber = DmFindSortPosition(gDB,newrecord,0,
					  (DmComparF *)slv_compare_record,0);
	if (recordNumber > 0) {  /* We might modify the record */
		struct dbworksheet *record;	       
		MemHandle recordMemHandle;
		Int16 foundIt;
		
		recordMemHandle = DmQueryRecord(gDB,recordNumber - 1);
		record = MemHandleLock(recordMemHandle);
		foundIt = StrCompare(newrecord->title,record->title) == 0;
		MemHandleUnlock(recordMemHandle);
		if (foundIt)	{
                        /* Return position */
			MemPtrFree(newrecord);
			return recordNumber - 1;
		}
	}
	myRecordMemHandle = DmNewRecord(gDB,&recordNumber,size);
	newrecordptr = MemHandleLock(myRecordMemHandle);
	DmWrite(newrecordptr,0,newrecord,size);
	MemHandleUnlock(myRecordMemHandle);	
	DmReleaseRecord(gDB,recordNumber,true); /* Now the recordNumber contains real index */
	MemPtrFree(newrecord);
	return recordNumber;
}

/***********************************************************************
 *
 * FUNCTION:     slv_comp_text
 * 
 * DESCRIPTION:  Compute text and expect it executes into double
 *
 * PARAMETERS:   text <- input, value <- output
 *
 * RETURN:       error
 *      
 ***********************************************************************/
static CError slv_comp_text(char *text,double *value) IFACE;
static CError 
slv_comp_text(char *text,double *value)
{
	CError err;
	CodeStack *stack;
	
	stack = text_to_stack(text,&err);
	if (!err) {
		err = stack_compute(stack);
		if (!err)
			err=stack_get_val(stack,(void *)value,real);
		stack_delete(stack);		  
	}
	return err;
}

			    

/***********************************************************************
 *
 * FUNCTION:     slv_comp_field
 * 
 * DESCRIPTION:  Computes the 'real' value of a text field
 *
 * PARAMETERS:   objectID - id of the field
 *
 * RETURN:       CError - 0 on success
 *               *value - the computed value
 *      
 ***********************************************************************/
static CError slv_comp_field(Int16 objectID,double *value,
			     FormPtr frm) IFACE;
static CError
slv_comp_field(Int16 objectID,double *value,FormPtr frm)
{
	char *text;
	CError err;
	FieldPtr field;
	
	field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,objectID));
	text=FldGetTextPtr(field);

	err = slv_comp_text(text,value);

	if (err)
		FrmAlert(altGraphBadVal);

	return err || !finite(*value);
}


/***********************************************************************
 *
 * FUNCTION:     slv_set_field
 * 
 * DESCRIPTION:  Set the display field to the number value 
 *
 * PARAMETERS:   objectID - Id of the control
 *               newval - value
 *               frm - form pointer
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void slv_set_field(UInt16 objectID,double newval,FormPtr frm) IFACE;
static void
slv_set_field(UInt16 objectID,double newval,FormPtr frm)
{
	FieldPtr pole;
	char *text;

	pole = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,objectID));
	
	text = display_real(newval);
	
	FldDelete(pole,0,FldGetTextLength(pole));
	FldInsert(pole,text,StrLen(text));
	MemPtrFree(text);
}


/***********************************************************************
 *
 * FUNCTION:     slv_options
 * 
 * DESCRIPTION:  Handle the 'Options' dialog of the worksheet
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - user pressed OK, false - otherwise
 *      
 ***********************************************************************/
static Boolean slv_options(void) IFACE;
static Boolean 
slv_options(void)
{
	FormPtr frm;
	FieldPtr field;
	Int16 button;
	Boolean res;
	double min,max,prec;
	CError err;

	frm = FrmInitForm(slvOptionsForm);
#ifdef HANDERA_SDK
	if (handera)
        	VgaFormModify(frm, vgaFormModify160To240);
#endif
	FrmSetFocus(frm,FrmGetObjectIndex(frm,slvOptionsTitle));
	field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,slvOptionsTitle));
	FldDelete(field,0,FldGetTextLength(field));
	FldInsert(field,worksheet_title,StrLen(worksheet_title));

	slv_set_field(slvOptionsMin,worksheet_min,frm);
	slv_set_field(slvOptionsMax,worksheet_max,frm);
	slv_set_field(slvOptionsPrec,worksheet_prec,frm);
	
	while (1) {
		button = FrmDoDialog(frm);
		if (button == slvOptionsCancel) {
			res = false;
			break;
		}
		err = slv_comp_field(slvOptionsMin,&min,frm);
		if (err) 
			continue;
		err = slv_comp_field(slvOptionsMax,&max,frm);
		if (err)
			continue;
		err = slv_comp_field(slvOptionsPrec,&prec,frm);
		if (err)
			continue;
		if (slvOptionsMin > slvOptionsMax) {
			FrmAlert(altGraphBadVal);
			continue;
		}
		if (StrLen(FldGetTextPtr(field)) == 0)
			continue;
		worksheet_min = min;
		worksheet_max = max;
		worksheet_prec = prec;
		StrCopy(worksheet_title,FldGetTextPtr(field));
		res = true;
		break;
	}
	selectedWorksheet = slv_save_worksheet();
	FrmUpdateForm(slvForm,frmUpdateWorksheet);

	FrmDeleteForm(frm);
	return res;
}

/***********************************************************************
 *
 * FUNCTION:     slv_select_worksheet
 * 
 * DESCRIPTION:  Handle the worksheet selection popup menu
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       New selected worksheet number
 *      
 ***********************************************************************/
static Int16 slv_select_worksheet(void) IFACE;
static Int16 
slv_select_worksheet(void)
{
	char **namelist;
	Int16 size,i,sel;
	ListPtr list;
	char *newname;
	char newstring[MAX_RSCLEN];

	newstring[0] = ' ';
	SysCopyStringResource(newstring+1,strNewWorksheet);


	size = DmNumRecords(gDB);
	namelist = MemPtrNew(sizeof(*namelist)*(size+1));
	namelist[0] = newstring;
	for (i=0;i<size;i++)
		namelist[i+1] = slv_worksheet_name(i);

	list = GetObjectPtr(slvWorksheetList);
	/* The list is sorted alphabetically */
	LstSetListChoices(list,namelist,size+1);
	LstMakeItemVisible(list,selectedWorksheet+1);
	LstSetSelection(list,selectedWorksheet+1);
	if (size < 8)
		LstSetHeight(list,size+1);
	else
		LstSetHeight(list,9);
	LstGlueSetIncrementalSearch(list, true);
	sel = LstPopupList(list);

	/* Free allocated space */
	for (i=1;i<size+1;i++)
		MemPtrFree(namelist[i]);
	MemPtrFree(namelist);

	if (sel == noListSelection)
		return -1;

	if (sel == 0) {
		newname = slv_ask_name();
		if (!newname)
			return -1;
		i = slv_new_worksheet(newname);
		MemPtrFree(newname);
		return i;
	}

	return sel-1;
}

/***********************************************************************
 *
 * FUNCTION:     slv_note
 * 
 * DESCRIPTION:  Handle the 'Note' dialog
 *
 * PARAMETERS:   
 *
 * RETURN:       
 *      
 ***********************************************************************/
static void slv_note(void) IFACE;
static void
slv_note(void)
{
	FormPtr frm;
	FieldPtr field;
	char *fldtext;

	frm = FrmInitForm(slvNoteForm);
#ifdef HANDERA_SDK
	if (handera)
        	VgaFormModify(frm, vgaFormModify160To240);
#endif
	FrmSetFocus(frm,FrmGetObjectIndex(frm,slvNoteField));
	field = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,slvNoteField));
	FldInsert(field,worksheet_note,StrLen(worksheet_note));
	
	FrmDoDialog(frm);
	fldtext = FldGetTextPtr(field);
	if (fldtext)
		StrCopy(worksheet_note,fldtext);

	FrmDeleteForm(frm);
	
}

/***********************************************************************
 *
 * FUNCTION:     slv_update_help
 * 
 * DESCRIPTION:  Called when user selects line in the variable list,
 *               show help text if available in the 'Note'
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void slv_update_help(void) IFACE;
static void 
slv_update_help(void)
{
	FormPtr frm;
	char *hlpstring;
	char *tmp,*colon;
	char *varname;
	Int16 i,size;

	if (selectedWorksheet == -1)
		return;

	frm = FrmGetActiveForm();

	i = LstGetSelection(GetObjectPtr(slvVarList));
	if (i == noListSelection) {
		FrmHideObject(frm,FrmGetObjectIndex(frm,slvHelp));
		FrmCopyLabel(frm,slvHelp,"");
		FrmShowObject(frm,FrmGetObjectIndex(frm,slvHelp));
		return;
	}
	varname = varitems.list[i];
	
	for (tmp=worksheet_note;tmp;tmp=StrChr(tmp,'\n')) {
		if (*tmp == '\n')
			tmp++;
		colon = StrChr(tmp,':');
		if (!colon)
			continue;
		if ((colon-tmp) != StrLen(varname))
			continue;
		if (memcmp(tmp,varname,StrLen(varname)) == 0) {
			colon++;
			if (*colon == ' ')
				colon++;
			tmp = StrChr(colon,'\n');
			if (tmp)
				size = tmp-colon;
			else
				size = StrLen(colon);
			if (size > MAX_WORKSHEET_HELP)
				size = MAX_WORKSHEET_HELP;
			hlpstring = MemPtrNew(size+1);
			memcpy(hlpstring,colon,size);
			hlpstring[size] = '\0';
			FrmHideObject(frm,FrmGetObjectIndex(frm,slvHelp));
			FrmCopyLabel(frm,slvHelp,hlpstring);
			FrmShowObject(frm,FrmGetObjectIndex(frm,slvHelp));
			MemPtrFree(hlpstring);
			return;
		}
	}
	FrmHideObject(frm,FrmGetObjectIndex(frm,slvHelp));
	FrmCopyLabel(frm,slvHelp,"");
	FrmShowObject(frm,FrmGetObjectIndex(frm,slvHelp));
}

/***********************************************************************
 *
 * FUNCTION:     slv_write_txt
 * 
 * DESCRIPTION:  Append text to the database at the current offset,
 *               increase offset
 *
 * PARAMETERS:   record - database record where to write
 *               text - the text
 *               offset - offset from where we should start writing
 *
 * RETURN:       
 *      
 ***********************************************************************/
static void
slv_write_txt(MemPtr record, char *text,UInt16 *offset) IFACE;
static void
slv_write_txt(MemPtr record, char *text,UInt16 *offset)
{
	DmWrite(record,*offset,text,StrLen(text));
	*offset += StrLen(text);
}

/***********************************************************************
 *
 * FUNCTION:     slv_append_real
 * 
 * DESCRIPTION:  Append real number in text representation to the string
 *
 * PARAMETERS:   str - where to print, num - what to print
 *
 * RETURN:       
 *      
 ***********************************************************************/
static void
slv_append_real(char *str,double num) IFACE;
static void
slv_append_real(char *str,double num)
{
	char *text;
	
	text = display_real(num);
	StrCat(str,text);
	MemPtrFree(text);
}

/***********************************************************************
 *
 * FUNCTION:     slv_memo_import
 * 
 * DESCRIPTION:  Import worksheet from the text identified by 
 *               parameter. Note: this is not called from 'Solver',
 *               that's why we have to open/close the Solver database.
 *
 * PARAMETERS:   text
 *
 * RETURN:       
 *      
 ***********************************************************************/
Boolean
slv_memo_import(char *text)
{
	char title[MAX_WORKSHEET_TITLE+1];
	char *tmp,*lineend;
	double min,max,prec;
	CError err;
	Int16 res;

	/* Get the title */
	tmp = StrChr(text,'\n');
	if (tmp-text > MAX_WORKSHEET_TITLE)
		return false;
	memcpy(title,text,tmp-text);
	title[tmp-text] = '\0';
	text = tmp+1;
	/* Now we should get the numbers */
	lineend = StrChr(text,'\n');
	if (!lineend)
		return false;
	/* Skip the colon */
	text++;
	/* Skip the spaces */
	for (;*text == ' ';text++)
		;
	/* Get min value */
	tmp = StrChr(text,' ');
	if (tmp && tmp < lineend) {
		*tmp = '\0';
		err = slv_comp_text(text,&min);
		if (err)
			return false;
		text = tmp+1;
		/* Get max value */
		for (;*text == ' ';text++)
			;
		tmp = StrChr(text,' ');
		if (!tmp || lineend < tmp)
			return false;
		*tmp = '\0';
		err = slv_comp_text(text,&max);
		if (err)
			return false;
		text = tmp+1;
		/* Get prec value */
		for (;*text == ' ';text++)
			;
		tmp = StrChr(text,' ');
		if (!tmp || lineend < tmp)
			tmp = lineend;
		*tmp = '\0';
		err = slv_comp_text(text,&prec);
		if (err)
			return false;
		if (min >= max)
			return false;
	} else {
		min = DEFAULT_MIN;
		max = DEFAULT_MAX;
		prec = DEFAULT_ERROR;
	}
	text = lineend+1;
	tmp = StrChr(text,'\n');
	if (!tmp) 
		tmp = "";
	else {
		tmp[0] = '\0';
		tmp++;
	}
	/* Now we have equation in 'text' and not in 'tmp'(or \0 if no note) */
	slv_db_open();
	res = slv_save_worksheet_vars(-1,title,text,min,max,prec,tmp);
	slv_db_close();
	if (res == -1)
		return false;
	return true;
}

/***********************************************************************
 *
 * FUNCTION:     slv_create_initial_note
 * 
 * DESCRIPTION:  Create Note to the worksheet that contains a template
 *               with variable names and ':' so that the user can see,
 *               how to comment variables
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
slv_create_initial_note(void)
{
	Int16 i;

	if (!varitems.size)
		return;

	SysCopyStringResource(worksheet_note,strDescriptionVar);

	for (i=0; i<varitems.size;i++) {
		StrCat(worksheet_note,varitems.list[i]);
		StrCat(worksheet_note,": \n");
	}
}

/***********************************************************************
 *
 * FUNCTION:     slv_export_memo
 * 
 * DESCRIPTION:  Export worksheet to MemoPad
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - OK, false - otherwise
 *      
 ***********************************************************************/
static Boolean slv_export_memo(void) IFACE;
static Boolean
slv_export_memo(void)
{
	DmOpenRef MemopadDB;
	MemPtr recP;
	MemHandle recH;
	UInt16 offset = 0;
	Char null = '\0';
	UInt16 category;
	UInt16 attr;
	UInt16 index = dmMaxRecordIndex;
	UInt16 size;
	char configuration[100];
	char *equation;

	equation = FldGetTextPtr(GetObjectPtr(slvEquation));
	if (!equation)
		return false;

	StrCopy(configuration,":");
	slv_append_real(configuration,worksheet_min);
	StrCat(configuration," ");
	slv_append_real(configuration,worksheet_max);
	StrCat(configuration," ");
	slv_append_real(configuration,worksheet_prec);

	size = StrLen(worksheet_title) + 1 \
		+ StrLen(configuration) + 1 \
		+ StrLen(equation) + 1 \
		+ StrLen(worksheet_note) \
		+ 1;
	
	if((MemopadDB = DmOpenDatabaseByTypeCreator('DATA', 'memo', 
						    dmModeReadWrite)) == NULL)
		return false;
	
	recH = DmNewRecord(MemopadDB, &index,size );
	if(recH != NULL) { 
		recP = MemHandleLock(recH); 
		slv_write_txt(recP,worksheet_title,&offset);
		slv_write_txt(recP,"\n",&offset);
		slv_write_txt(recP,configuration,&offset);
		slv_write_txt(recP,"\n",&offset);
		slv_write_txt(recP,equation,&offset);
		slv_write_txt(recP,"\n",&offset);
		slv_write_txt(recP,worksheet_note,&offset);
		DmWrite(recP,offset,&null,1);
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
	DmCloseDatabase(MemopadDB);

	if (recH)
		return true;
	return false;
}

/***********************************************************************
 *
 * FUNCTION:     SolverHandleEvent
 * 
 * DESCRIPTION:  Event handler for Definition manager main window
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean 
SolverHandleEvent(EventPtr event)
{
	Boolean handled = false;
	FormPtr frm;
	Int16 controlID;
	Int16 i;
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
		 list = GetObjectPtr(slvVarList);
		 switch (event->data.keyDown.chr) {
		 case pageUpChr: 
			 if (selectedWorksheet == -1)
				 break;
			 FrmHideObject(frm,FrmGetObjectIndex(frm,slvVarList));
			 LstScrollList(list,winUp,LstGetVisibleItems(list)-1);
			 FrmShowObject(frm,FrmGetObjectIndex(frm,slvVarList));
			 handled = true;
			 break;
		 case pageDownChr:
			 if (selectedWorksheet == -1)
				 break;
			 FrmHideObject(frm,FrmGetObjectIndex(frm,slvVarList));
			 LstScrollList(list,winDown,LstGetVisibleItems(list)-1);
			 FrmShowObject(frm,FrmGetObjectIndex(frm,slvVarList));
			 handled = true;
			 break;
		 case '\n':
			 FrmUpdateForm(slvForm,frmUpdateVars);
			 handled = true;
			 break;
		 }
		 break;
	 }
	case lstSelectEvent:
		if (selectedLine == event->data.lstSelect.selection) {
			if (slv_modify(selectedLine))
				FrmUpdateForm(slvForm,frmUpdateVars);
		} else
			selectedLine = event->data.lstSelect.selection;
		slv_update_help();
		handled = true;
		break;
	 case ctlSelectEvent:
		controlID = event->data.ctlSelect.controlID;
		handled = true;
		switch (controlID) {
		case slvWorksheet:
			slv_save_worksheet();
			i = slv_select_worksheet();
			if (i != -1) {
				selectedWorksheet = i;
				FrmUpdateForm(slvForm,frmUpdateWorksheet);
			}
			break;
		case slvUpdate:
			FrmUpdateForm(slvForm,frmUpdateVars);
			break;
		case slvDone:
			calcPrefs.solverWorksheet = selectedWorksheet;
			slv_save_worksheet();
			slv_destroy_varlist();
			slv_db_close();
			MemPtrFree(worksheet_note);
			FrmReturnToForm(0);
			break;
		case slvModify:
			i = LstGetSelection(GetObjectPtr(slvVarList));
			if (i != noListSelection) 
				if (slv_modify(i))
					FrmUpdateForm(slvForm,frmUpdateVars);
			break;
		case slvSolve:
			i = LstGetSelection(GetObjectPtr(slvVarList));
			if (i != noListSelection)
				slv_solve(i);
			break;
		case slvCalculate:
			slv_calculate();
			break;
		case slvDelete:
			SysCopyStringResource(tmptxt,strWorksheet);
			if (FrmCustomAlert(altConfirmDelete,tmptxt,
					   worksheet_title,NULL)==0) {
				DmRemoveRecord(gDB,selectedWorksheet);
				selectedWorksheet = -1;
				FrmUpdateForm(slvForm,frmUpdateWorksheet);
			}
			break;
		case slvOptions:
			slv_options();
			break;
		case slvNote:
			slv_note();
			slv_update_help();
			break;
		default:
			handled = false;
			break;
		}
		break;
	 case frmOpenEvent:
		ChrHorizEllipsis(&HORIZ_ELLIPSIS);
		frm = FrmGetActiveForm();

		worksheet_note = MemPtrNew(MAX_WORKSHEET_NOTE);
		selectedWorksheet = calcPrefs.solverWorksheet;
		slv_db_open();
		slv_update_worksheet();
		slv_init_varlist();
		slv_update_help();

		FrmDrawForm(frm);
		
		handled = true;
		break;		
	case frmCloseEvent:
		calcPrefs.solverWorksheet = selectedWorksheet;
		slv_save_worksheet();
		slv_destroy_varlist();
		slv_db_close();
		MemPtrFree(worksheet_note);
		/* FrmHandleEvent must handle this event itself */
		handled = false;
		break;

	case menuEvent:		  
		switch (event->data.menu.itemID) {
		case slvExport:
			if (slv_export_memo())
				FrmAlert(slvExportOK);
			else
				FrmAlert(slvExportError);
			handled = true;
			break;
		}
		break;
	case frmUpdateEvent:
		if (event->data.frmUpdate.updateCode == frmUpdateWorksheet) {
			slv_update_worksheet();
			slv_destroy_varlist();
			if (slv_init_varlist() == 0)
				LstDrawList(GetObjectPtr(slvVarList));
			slv_update_help();
			handled = true;
		}
		if (event->data.frmUpdate.updateCode == frmUpdateVars) {
			slv_destroy_varlist();
			if (slv_init_varlist() == 0) {
				LstDrawList(GetObjectPtr(slvVarList));
				if (!StrLen(worksheet_note)) 
					slv_create_initial_note();
			}

			slv_update_help();
			handled = true;
		}
		break;
	}
	
	return handled;
}
