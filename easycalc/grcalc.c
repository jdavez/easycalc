/*   
 * $Id: grcalc.c,v 1.21 2007/12/19 13:49:13 cluny Exp $
 * 
 * Scientific Calculator for Palms.
 *   Copyright (C) 2001 Ondrej Palkovsky
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
#include "grcalc.h"
#include "graph.h"
#include "grsetup.h"
#include "grprefs.h"
#include "grtaps.h"
#include "funcs.h"
#include "integ.h"
#include "varmgr.h"
#include "mathem.h"
#include "display.h"
#include "MathLib.h"
#include "stack.h"

#ifdef HANDERA_SDK
#include "Vga.h"
#endif

static enum {
	cl_none,
	cl_min,
	cl_max,
	cl_value
}grState;

static enum {
	cp_zero = 0,
	cp_value,
	cp_min,
	cp_max,
	cp_dydx1,
	cp_dydx2,
	cp_integ,
	cp_intersect,
	cp_pdydx,
	cp_pdydt,
	cp_pdxdt,
	cp_odrdfi
}grFuncType;

static const Int16 grcFunc[] = {cp_zero,cp_value,cp_min,cp_max,
				cp_dydx1,cp_dydx2,cp_integ,cp_intersect};
static const Int16 grcParam[] = {cp_pdydx,cp_pdydt,cp_pdxdt};
static const Int16 grcPolar[] = {cp_odrdfi};

static Int16 grSelFunction,grSelFunction2;
static double grFuncMin;
static double grFuncMax;
static double grFuncValue;

/***********************************************************************
 *
 * FUNCTION:     grcalc_select_funcname
 * 
 * DESCRIPTION:  Present to user list of functions and let him select
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       -1 - error, user pressed cancel or no function on graph
 *               0-MAX_GRFUNCS - index to graphCurves table
 *      
 ***********************************************************************/
static Int16 grcalc_select_funcname(void) GRAPH;
static Int16
grcalc_select_funcname(void)
{
	FormPtr frm;
	ListPtr list;
	Int16 button;
	char *descr[MAX_GRFUNCS];
	Int16 nums[MAX_GRFUNCS];
	Int16 count,i;
	
	count = grsetup_fn_descr_arr(descr,nums);
	if (count == 0)
		return -1;

	frm = FrmInitForm(frmGrcSel);
#ifdef HANDERA_SDK
	if (handera)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	list = FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,lstGrcSel));
	LstSetListChoices(list,descr,count);
	button = FrmDoDialog(frm);

	i = LstGetSelection(list);
	FrmDeleteForm(frm);
	if (button == btnGrcSelCancel) 
		return -1;
	if (i == noListSelection)
		return -1;
	return nums[i];
}

/***********************************************************************
 *
 * FUNCTION:     grcalc_select_func
 * 
 * DESCRIPTION:  The initial step when the user presses the 'C' button
 *               and the function type 'Func' is displayed
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - user selected everything ok
 *               false - action cancelled
 *      
 ***********************************************************************/
static Boolean grcalc_select(void) GRAPH;
static Boolean
grcalc_select(void)
{
	Int16 i;
	Int16 list;
	const Int16 *ccs;

	if (graphPrefs.functype == graph_func) {
		list = lstGraphCalcFunc;
		ccs = grcFunc;
	} else if (graphPrefs.functype == graph_polar) {
		list = lstGraphCalcPolar;
		ccs = grcPolar;
	} else {
		list = lstGraphCalcParam;
		ccs = grcParam;
	}

	i = LstPopupList(GetObjectPtr(list));
	if (i == noListSelection)
		return false;

	grFuncType = ccs[i];

	if (CtlGetValue(GetObjectPtr(btnGraphTrack)))
		grSelFunction=grtaps_trackSelected();
	else{
		grSelFunction = grcalc_select_funcname();
		if (grSelFunction == -1)
			return false;
		grtaps_track_track(grSelFunction);
	}

	switch (grFuncType) {
	case cp_value:
		if (!varmgr_get_double(&grFuncValue,"Value of Function"))
			return false;
	case cp_integ:
	case cp_zero:
	case cp_min:
	case cp_max:
	case cp_intersect:
		if (FrmAlert(altGrcSelectLeft) == 1)
			return false;
		grState = cl_min;
		break;
	case cp_dydx1:
	case cp_dydx2:
	case cp_pdydx:
	case cp_pdydt:
	case cp_pdxdt:
	case cp_odrdfi:
		if (FrmAlert(altGrcSelectValue) == 1)
			return false;
		grState = cl_value;
		break;
	}
	return true;
}

/***********************************************************************
 *
 * FUNCTION:     grcalc_calculate
 * 
 * DESCRIPTION:  This function is called when all necessary
 *               data is fetched from the user and calculates
 *               and displays result to the user.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void grcalc_calculate(void) GRAPH;
static void
grcalc_calculate(void)
{
	double result;
	CodeStack *fcstack,*fcstack2;
	char *text,acdescr[MAX_RSCLEN];

	if (grState != cl_value && grFuncMin == grFuncMax) {
		FrmAlert(altGrcBoundsErr);
		return;
	} 
	if (grState != cl_value && grFuncMin > grFuncMax) {
		result = grFuncMin;
		grFuncMin = grFuncMax;
		grFuncMax = result;
	}

	if (grFuncType == cp_intersect) {
		grSelFunction2 = grcalc_select_funcname();
		if (grSelFunction2 == -1)
			return;
		if (grSelFunction == grSelFunction2) {
			FrmAlert(altGrcFuncErr);
			return;
		}
	}

	/* I wonder if I should display it at all */
	if (grFuncType == cp_zero || grFuncType == cp_min
	    || grFuncType == cp_max || grFuncType == cp_value)
		if (FrmAlert(altComputeCofirm) == 1)
			return; /* User cancelled operation */

	fcstack = graphCurves[grSelFunction].stack1;
	if (grFuncType == cp_intersect)
		fcstack2 = graphCurves[grSelFunction2].stack1;
	else
		fcstack2 = graphCurves[grSelFunction].stack2;

	switch (grFuncType) {
	case cp_intersect:
		result = integ_intersect(grFuncMin,grFuncMax,fcstack,
					 fcstack2,DEFAULT_ERROR,NULL);
		break;
	case cp_zero:
		result = integ_zero(grFuncMin,grFuncMax,0.0,fcstack,
				    DEFAULT_ERROR,MATH_FZERO,NULL);
		break;
	case cp_min:
		result = integ_zero(grFuncMin,grFuncMax,0.0,fcstack,
				    DEFAULT_ERROR,MATH_FMIN,NULL);
		break;
	case cp_max:
		result = integ_zero(grFuncMin,grFuncMax,0.0,fcstack,
				    DEFAULT_ERROR,MATH_FMAX,NULL);
		break;
	case cp_integ:
		result = integ_romberg(grFuncMin,grFuncMax,fcstack,
				       DEFAULT_ROMBERG,NULL);
//		result = integ_simps(grFuncMin,grFuncMax,fcstack,
//				       DEFAULT_ERROR);
		break;
	case cp_value:
		result = integ_zero(grFuncMin,grFuncMax,grFuncValue,fcstack,
				    DEFAULT_ERROR,MATH_FVALUE,NULL);
		break;
	case cp_dydx2:
		result = integ_derive2(grFuncValue,fcstack,DEFAULT_ERROR,NULL);
		break;
	case cp_dydx1:
	case cp_pdxdt:
	case cp_odrdfi:
		result = integ_derive1(grFuncValue,fcstack,DEFAULT_ERROR,NULL);
		break;
	case cp_pdydx:
		result = integ_derive1(grFuncValue,fcstack2,DEFAULT_ERROR,NULL);
		result /= integ_derive1(grFuncValue,fcstack,DEFAULT_ERROR,NULL);
		break;
	case cp_pdydt:
		result = integ_derive1(grFuncValue,fcstack2,DEFAULT_ERROR,NULL);
		break;
	default:
		result = NaN;
		break;
	}

	if (finite(result)) {
		/* The default error is lower, but this may
		   display better values */
		/* Save the result as 'graphres' variable */
		db_save_real("graphres", result);

		/* Round the result with respect to computing error */
		result /= 1E-5;
		result = round(result) * 1E-5;

		/* Set the cross to result where applicable */
		switch (grFuncType) {
		case cp_intersect:
		case cp_zero:
		case cp_min:
		case cp_max:
		case cp_value:
			grtaps_track_manual(result, track_set);
			break;
		}

		text = display_real(result);
		/* Fetch description of a function */
		SysStringByIndex(strGrcDescription, grFuncType,
		                 acdescr, MAX_RSCLEN - 1);
		FrmCustomAlert(altComputeResult, acdescr, text, NULL);
		MemPtrFree(text);
	} else
		FrmAlert(altCompute);
}

/***********************************************************************
 *
 * FUNCTION:     grcalc_init
 * 
 * DESCRIPTION:  Initialize/reset this module
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
grcalc_init(void)
{
	grState = cl_none;
}

/***********************************************************************
 *
 * FUNCTION:     grcalc_control
 * 
 * DESCRIPTION:  This things gets called when the 'C' button on the graph
 *               screen is pressed. 
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - this module waits for some more input
 *               false - this module is inactive
 *      
 ***********************************************************************/
Boolean
grcalc_control(void)
{
	switch (grState) {
	case cl_none:
		grcalc_select();
		break;
	case cl_min:
		grFuncMin = grtaps_track_last_value();
		if (FrmAlert(altGrcSelectRight) == 0) 
			grState = cl_max;
		else {
			grState = cl_none;
			//grtaps_track_pause();
		}
		break;
	case cl_max:
		grFuncMax = grtaps_track_last_value();
		grcalc_calculate();
		grState = cl_none;
		//grtaps_track_pause();
		break;
	case cl_value:
		grFuncValue = grtaps_track_last_value();
		grcalc_calculate();
		grState = cl_none;
		//grtaps_track_pause();
		break;
	}

	return grState != cl_none;
}


