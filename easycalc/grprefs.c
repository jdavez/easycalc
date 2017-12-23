/*
 *   $Id: grprefs.c,v 1.29 2007/12/16 12:24:50 cluny Exp $
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

#include "grprefs.h"
#include "calcrsc.h"
#include "defuns.h"
#include "MathLib.h"
#include "stack.h"
#include "prefs.h"
#include "calc.h"
#include "display.h"
#include "mathem.h"
#include "fp.h"
#include "main.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#ifdef SUPPORT_DIA_HANDERA
#include "Vga.h"
#endif
#endif

/* Local structure - values in function mode */
TgrPrefs graphPrefs;
/* Colors of functions and background, axis, grid */
RGBColorType graphRGBColors[] = {
	{0,255,0,0},
	{0,0,200,0},
	{0,0,0,255},
	{0,128,128,0},
	{0,0,128,128},
	{0,128,0,128},
	{0,0,0,0},       /* axis */
	{0,128,128,128},  /* grid */
	{0,255,255,255} /* background */
};

/* Gray colors: 6 funcs + axis + grid + backgrnd */
IndexedColorType funcolors[9]={15,11,7,13,9,5,15,8,0}; 

/* Display mode for preferences */
static const TdispPrefs grDPrefs = { 9,true,disp_normal,disp_decimal,false,false};

/***********************************************************************
 *
 * FUNCTION:     grpref_init
 * 
 * DESCRIPTION:  Initialize graphical preferences, reading the appropriate
 *               preference from system
 *               Initialize color table
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
grpref_init(void)
{
	UInt16 size;
	Int16 i;
#ifdef SUPPORT_DIA
	FormType *frm;
	RectangleType natbounds;
	RectangleType stdbounds;
#endif

	size=sizeof(graphPrefs);
	if (PrefGetAppPreferences(APP_ID,PREF_GRAPH,&graphPrefs,&size,
							  true)!=PREF_GRAPH_VERSION) {
		graphPrefs.xmin = -7.0;
		graphPrefs.xmax = 7.0;
		graphPrefs.ymin = -7.0;
		graphPrefs.ymax = 7.0;
		graphPrefs.xscale = 1.0;
		graphPrefs.yscale = 1.0;
		graphPrefs.fimin = 0.0;
		graphPrefs.fimax = M_PIl*2.0;
		graphPrefs.fistep = M_PIl*0.02;
		graphPrefs.tmin = 0.0;
		graphPrefs.tmax = 10.0;
		graphPrefs.tstep = 0.1;
		graphPrefs.functype = graph_func;
		for (i=0;i<MAX_GRFUNCS;i++)
		  graphPrefs.funcFunc[i][0]='\0';
		for (i=0;i<MAX_GRFUNCS;i++)
		  graphPrefs.funcPol[i][0]='\0';
		for (i=0;i<MAX_GRFUNCS;i++) {
			graphPrefs.funcPar[i][0][0]='\0';
			graphPrefs.funcPar[i][1][0]='\0';
		}
		graphPrefs.logx = false;
		graphPrefs.logy = false;
		graphPrefs.speed = 1;
		for (i=0; i<9; i++){
			/* Note: grEnable[8] enables axis labels,
			 *       colors[8] is the background color
			 */
			graphPrefs.grEnable[i] = true;
			if (colorDisplay)
				graphPrefs.colors[i] = WinRGBToIndex(&graphRGBColors[i]);
			if (grayDisplay)
				graphPrefs.colors[i] = funcolors[i];
		}
		for (i=0; i<6; i++)
			graphPrefs.grType[i] = 1;
	}
#ifdef SUPPORT_DIA
	/* Set up ScrPrefs to correspond the square graph gadget. When
	 * opening the graph form with the DIA minimized in either portrait
	 * or landscape the DIA code will do "The Right Thing (TM)" */
	frm = FrmInitForm(frmGraph);
#ifdef SUPPORT_DIA_HANDERA
	if (GetDIAHardware() == DIA_HARDWARE_HANDERA)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	gadget_bounds(frm, graphGadget, &natbounds, &stdbounds);
	FrmDeleteForm(frm);
	ScrPrefs.xmin = natbounds.topLeft.x;
	ScrPrefs.ymax = natbounds.topLeft.y;
	ScrPrefs.xmax = natbounds.topLeft.x + natbounds.extent.x - 1;
	ScrPrefs.ymin = natbounds.topLeft.y + natbounds.extent.y - 1;
	ScrPrefs.xtrans = (double)(ScrPrefs.xmax - ScrPrefs.xmin) /
	                          (graphPrefs.xmax - graphPrefs.xmin);
	ScrPrefs.ytrans = (double)(ScrPrefs.ymin - ScrPrefs.ymax) /
	                          (graphPrefs.ymax - graphPrefs.ymin);
#endif
}

/***********************************************************************
 *
 * FUNCTION:     grpref_close
 * 
 * DESCRIPTION:  Saves graph preferences
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
grpref_close(void)
{
#ifdef SUPPORT_DIA
	FormType *frm;
	RectangleType natbounds;
	RectangleType stdbounds;
	Coord dx, dy;

	/* Before saving the graph preferences, reload the graph form
	 * to get the virgin square rectangle of the graph gadget.
	 * Use this size to normalize the graph preferences in case we are
	 * on a device with a Dynamic Input Area */
	frm = FrmInitForm(frmGraph);
#ifdef SUPPORT_DIA_HANDERA
	if (GetDIAHardware() == DIA_HARDWARE_HANDERA)
		VgaFormModify(frm, vgaFormModify160To240);
#endif
	gadget_bounds(frm, graphGadget, &natbounds, &stdbounds);
	FrmDeleteForm(frm);
	dx = natbounds.topLeft.x + natbounds.extent.x - 1 - ScrPrefs.xmax;
	dy = natbounds.topLeft.y + natbounds.extent.y - 1 - ScrPrefs.ymin;
	if (dx)
		graphPrefs.xmax += dx / ScrPrefs.xtrans;
	if (dy)
		graphPrefs.ymin -= dy / ScrPrefs.ytrans;
#endif 
	PrefSetAppPreferences(APP_ID,PREF_GRAPH,PREF_GRAPH_VERSION,&graphPrefs,
						  sizeof(graphPrefs),true);
}

/***********************************************************************
 *
 * FUNCTION:     grpref_compl_field
 * 
 * DESCRIPTION:  Computes the 'real' value of a text field
 *
 * PARAMETERS:   objectID - id of the field
 *
 * RETURN:       CError - 0 on success
 *               *value - the computed value
 *      
 ***********************************************************************/
static CError grpref_comp_field(Int16 objectID,double *value) IFACE;
static CError
grpref_comp_field(Int16 objectID,double *value)
{
	char *text;
	CError err;
	CodeStack *stack;
	
	text=FldGetTextPtr(GetObjectPtr(objectID));
	
	stack = text_to_stack(text,&err);
	if (!err) {
		err = stack_compute(stack);
		if (!err)
		  err=stack_get_val(stack,(void *)value,real);
		stack_delete(stack);		  
	}

	return err || !finite(*value);
}

/***********************************************************************
 *
 * FUNCTION:     grpref_save_values
 * 
 * DESCRIPTION:  Gets text from fields, computes them and saves them
 *               into corresponding global structure
 *
 * PARAMETERS:   None
 *
 * RETURN:       1 - success
 *               0 - error while computing
 *      
 ***********************************************************************/
static Int16 grpref_save_values() IFACE;
static Int16
grpref_save_values()
{
	double xmin,xmax,ymin,ymax,xscale,yscale;
	double t1,t2,t3;
	Tfunctype functype;	
	
	if (grpref_comp_field(grPrefXmin,&xmin))
		return 0;
	if (grpref_comp_field(grPrefXmax,&xmax))
		return 0;
	if (grpref_comp_field(grPrefYmin,&ymin))
		return 0;
	if (grpref_comp_field(grPrefYmax,&ymax))
		return 0;
	if (grpref_comp_field(grPrefXscale,&xscale))
		return 0;
	if (grpref_comp_field(grPrefYscale,&yscale))
		return 0;

	if (xscale < 0.0 || yscale < 0.0)
		return 0;

	if (xmin>=xmax || ymin>=ymax)
	  return 0;

	if (CtlGetValue(GetObjectPtr(ckbGrPrefLogX)) && xmin <= 0.0)
		return 0;
	if (CtlGetValue(GetObjectPtr(ckbGrPrefLogY)) && ymin <= 0.0)
		return 0;
	
	if (CtlGetValue(GetObjectPtr(grPrefFunc)))
		functype = graph_func;
	else {
		if (grpref_comp_field(grPrefField1,&t1))
			return 0;
		if (grpref_comp_field(grPrefField2,&t2))
			return 0;
		if (grpref_comp_field(grPrefField3,&t3))
			return 0;
		if (CtlGetValue(GetObjectPtr(grPrefPol))) {
			if (t1>t2 || (t2-t1)/t3 > 10000 || t3 <= 0) /* check that min<max and step is reasonable */
				return 0;
			graphPrefs.fimin  = math_user_to_rad(t1);
			graphPrefs.fimax  = math_user_to_rad(t2);
			graphPrefs.fistep = math_user_to_rad(t3);
			functype = graph_polar;
		}
		else {
			if (t1>t2 || (t2-t1)/t3 > 10000 || t3 <= 0) /* check that min<max and step is reasonable */
				return 0;
			graphPrefs.tmin = t1;
			graphPrefs.tmax = t2;
			graphPrefs.tstep = t3;
			functype = graph_param;
		}
	}	
	graphPrefs.functype = functype;
	graphPrefs.xmin=xmin; graphPrefs.xmax=xmax;
	graphPrefs.ymin=ymin; graphPrefs.ymax=ymax;
	graphPrefs.xscale = xscale;graphPrefs.yscale = yscale;
	graphPrefs.logx = CtlGetValue(GetObjectPtr(ckbGrPrefLogX));
	graphPrefs.logy = CtlGetValue(GetObjectPtr(ckbGrPrefLogY));
	return 1;
}

/***********************************************************************
 *
 * FUNCTION:     grpref_set_field
 * 
 * DESCRIPTION:  Sets a text in a field to a value
 *
 * PARAMETERS:   objectID - id of the field
 *               newval - value that should be printed into field
 *
 * RETURN:       
 *      
 ***********************************************************************/
static void grpref_set_field(UInt16 objectID,double newval) IFACE;
static void
grpref_set_field(UInt16 objectID,double newval)
{
	FieldPtr pole = GetObjectPtr(objectID);	  
	char *text;
	
	text = MemPtrNew(15);
	fp_print_g_double(text, newval, 5);
	
	FldDelete(pole, 0, FldGetTextLength(pole));
	FldInsert(pole, text, StrLen(text));
	MemPtrFree(text);
}

/***********************************************************************
 *
 * FUNCTION:     grpref_set_fields
 * 
 * DESCRIPTION:  Sets the basic xmin/xmax/ymin/ymax text values
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grpref_set_fields(void) IFACE;
static void
grpref_set_fields(void)
{
	grpref_set_field(grPrefXmin,graphPrefs.xmin);
	grpref_set_field(grPrefXmax,graphPrefs.xmax);
	grpref_set_field(grPrefYmin,graphPrefs.ymin);
	grpref_set_field(grPrefYmax,graphPrefs.ymax);
	grpref_set_field(grPrefXscale,graphPrefs.xscale);
	grpref_set_field(grPrefYscale,graphPrefs.yscale);
}

/***********************************************************************
 *
 * FUNCTION:     grpref_show_fields
 * 
 * DESCRIPTION:  Shows additinal input fields, used by Polar/Parameter 
 *               functions
 *
 * PARAMETERS:   ftyp - function type.
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grpref_show_fields(Tfunctype ftyp) IFACE;
static void
grpref_show_fields(Tfunctype ftyp)
{
	FormPtr frm = FrmGetActiveForm();

	FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefLabel1));
	FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefLabel2));
	FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefLabel3));
	FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefField1));
	FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefField2));
	FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefField3));	
	if (ftyp == graph_polar) {
		const Char *unittxt = trigmode[calcPrefs.trigo_mode].trigtext;

		FrmCopyLabel(frm,grPrefLabel1u,unittxt);
		FrmCopyLabel(frm,grPrefLabel2u,unittxt);
		FrmCopyLabel(frm,grPrefLabel3u,unittxt);
		FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefLabel1u));
		FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefLabel2u));
		FrmShowObject(frm,FrmGetObjectIndex(frm,grPrefLabel3u));
	}
}

/***********************************************************************
 *
 * FUNCTION:     grpref_hide_fields
 * 
 * DESCRIPTION:  Hides additional fields used by polar/parametric graphs
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grpref_hide_fields(void) IFACE;
static void
grpref_hide_fields(void)
{
	FormPtr frm = FrmGetActiveForm();

	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefLabel1));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefLabel2));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefLabel3));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefField1));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefField2));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefField3));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefLabel1u));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefLabel2u));
	FrmHideObject(frm,FrmGetObjectIndex(frm,grPrefLabel3u));
}

/***********************************************************************
 *
 * FUNCTION:     grpref_set_names
 * 
 * DESCRIPTION:  Set names of the 3 additional fields
 *
 * PARAMETERS:   txt1,2,3 - names of the fields
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grpref_set_names(char *txt1,char *txt2,char *txt3) IFACE;
static void
grpref_set_names(char *txt1,char *txt2,char *txt3)
{
	FormPtr frm = FrmGetActiveForm();

	FrmCopyLabel(frm,grPrefLabel1,txt1);
	FrmCopyLabel(frm,grPrefLabel2,txt2);
	FrmCopyLabel(frm,grPrefLabel3,txt3);
}

/***********************************************************************
 *
 * FUNCTION:     grpref_pol_setup
 * 
 * DESCRIPTION:  Setup fields corresponding to polar graphs
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grpref_pol_setup(void) IFACE;
static void
grpref_pol_setup(void)
{
	grpref_set_names("t-min:","t-max:","t-step:");
	grpref_set_field(grPrefField1,math_rad_to_user(graphPrefs.fimin));
	grpref_set_field(grPrefField2,math_rad_to_user(graphPrefs.fimax));
	grpref_set_field(grPrefField3,math_rad_to_user(graphPrefs.fistep));
	grpref_show_fields(graph_polar);
}

/***********************************************************************
 *
 * FUNCTION:     grpref_par_setup
 * 
 * DESCRIPTION:  Setup fields corresponding to parametric graphs
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void grpref_par_setup(void) IFACE;
static void
grpref_par_setup(void)
{
	grpref_set_names("T-min:","T-max:","T-step:");
	grpref_set_field(grPrefField1,graphPrefs.tmin);
	grpref_set_field(grPrefField2,graphPrefs.tmax);
	grpref_set_field(grPrefField3,graphPrefs.tstep);
	grpref_show_fields(graph_param);
}

/***********************************************************************
 *
 * FUNCTION:     GraphPrefsHandleEvent
 * 
 * DESCRIPTION:  Event handler for graph preferences
 *
 * PARAMETERS:   event
 *
 * RETURN:       handled
 *      
 ***********************************************************************/
Boolean
GraphPrefsHandleEvent(EventPtr event)
{
	Boolean handled=false;
	Int16 controlID;
	FormPtr frm;
	static TdispPrefs oldprefs;
	double tmp;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	 case frmCloseEvent:
		fp_set_prefs(oldprefs);
		handled = false;
		break;
	 case frmOpenEvent:
		frm = FrmGetActiveForm();
		
		oldprefs = fp_set_prefs(grDPrefs);
		
		grpref_set_fields();
		switch (graphPrefs.functype) {
		 default:
		 case graph_func:
			controlID = grPrefFunc;
			break;
		 case graph_polar:
			controlID = grPrefPol;
			grpref_pol_setup();
			break;
		 case graph_param:
			controlID = grPrefPar;
			grpref_par_setup();
			break;
		}
		CtlSetValue(GetObjectPtr(controlID),1);
		CtlSetValue(GetObjectPtr(ckbGrPrefLogX),graphPrefs.logx);
		CtlSetValue(GetObjectPtr(ckbGrPrefLogY),graphPrefs.logy);

		FrmDrawForm(frm);

		handled=true;
		break;
	 case ctlSelectEvent:
		controlID=event->data.ctlSelect.controlID;
		switch (controlID) {
		case ckbGrPrefLogX:
			if (event->data.ctlSelect.on) {
				grpref_comp_field(grPrefXmin,&tmp);
				if (tmp <= 0.0) 
					grpref_set_field(grPrefXmin,MIN_LOG_VAL);
			}
			handled = false;
			break;
		case ckbGrPrefLogY:
			if (event->data.ctlSelect.on) {
				grpref_comp_field(grPrefYmin,&tmp);
				if (tmp <= 0.0) 
					grpref_set_field(grPrefYmin,MIN_LOG_VAL);
			}
			handled = false;
			break;
		 case grPrefFunc:
		 case grPrefPol:
		 case grPrefPar:
			grpref_hide_fields();
			if (controlID==grPrefPol)
				grpref_pol_setup();
			else if (controlID==grPrefPar)
				grpref_par_setup();
			handled = true;
			break;
		 case grPrefDefault:
			grpref_set_field(grPrefXmin,-7.0);
			grpref_set_field(grPrefXmax,7.0);
			grpref_set_field(grPrefYmin,-7.0);
			grpref_set_field(grPrefYmax,7.0);
			grpref_set_field(grPrefXscale,1.0);
			grpref_set_field(grPrefYscale,1.0);
			if (CtlGetValue(GetObjectPtr(grPrefPol))) {
				grpref_set_field(grPrefField1,0.0);
				grpref_set_field(grPrefField2,math_rad_to_user(2.0*M_PIl));
				grpref_set_field(grPrefField3,math_rad_to_user(0.02*M_PIl));
			}
			else if (CtlGetValue(GetObjectPtr(grPrefPar))) {
				grpref_set_field(grPrefField1,0.0);
				grpref_set_field(grPrefField2,10.0);
				grpref_set_field(grPrefField3,0.1);
			}
			handled=true;
			break;
		 case grPrefTrig:
			grpref_set_field(grPrefXmin,-2.0*M_PIl);
			grpref_set_field(grPrefXmax,2.0*M_PIl);
			grpref_set_field(grPrefYmin,-2.0*M_PIl);
			grpref_set_field(grPrefYmax,2.0*M_PIl);
			grpref_set_field(grPrefXscale,M_PIl/2.0);
			grpref_set_field(grPrefYscale,M_PIl/2.0);
			handled=true;
			break;			
		 case prefOK:
			if (grpref_save_values()) {
				fp_set_prefs(oldprefs);
				FrmReturnToForm(calcPrefs.form);
				FrmUpdateForm(calcPrefs.form,frmUpdateVars);
			}
			else 
				FrmAlert(altGraphBadVal);
			handled=true;
			break;
		 case prefCancel:
			FrmReturnToForm(calcPrefs.form);
			dispPrefs = oldprefs;
			handled=true;
			break;
		}
	}	
	  
	return handled;
}

CError
set_axis(Functype *func,CodeStack *stack)
{
	List *lst,*lst2;
	float xscale=graphPrefs.xscale;
	float yscale=graphPrefs.yscale;
	int i;
	CError err;

	if (func->paramcount > 2) 
		return c_badargcount;
	else if (func->paramcount == 2) {
		err = stack_get_val(stack,&lst2,list);
		if (err)
			return err;
		if (lst2->size!=2){
			MemPtrFree(lst2);
			return c_baddim;
		}
		if (lst2->item[0].imag!=0.0 || lst2->item[1].imag!=0.0){
			MemPtrFree(lst2);
			return c_badarg;
		}
		xscale=lst2->item[0].real;
		yscale=lst2->item[1].real;
		MemPtrFree(lst2);
		if(xscale<=0.0 || yscale<=0.0) return c_badarg;
	}
	
	err = stack_get_val(stack,&lst,list);
	if (err)
		return err;
	
	if (lst->size!=4){
		MemPtrFree(lst);
		return c_baddim;
	}
		
	for (i=0;i<4;i++)
	  if (lst->item[i].imag != 0.0){
		MemPtrFree(lst);
		return c_badarg;
	}
	
	if(lst->item[0].real>=lst->item[1].real || lst->item[2].real>=lst->item[3].real){
		MemPtrFree(lst);
		return c_badarg;
	}
	
	graphPrefs.xmin=lst->item[0].real;
	graphPrefs.xmax=lst->item[1].real;
	graphPrefs.ymin=lst->item[2].real;
	graphPrefs.ymax=lst->item[3].real;
	if (func->paramcount == 2) {
		graphPrefs.xscale=xscale;
		graphPrefs.yscale=yscale;
		graphPrefs.grEnable[7] = true;
	}
	else
		graphPrefs.grEnable[7] = false;

	
	err = stack_add_val(stack,&lst,list);
	MemPtrFree(lst);
	
	return err;
}