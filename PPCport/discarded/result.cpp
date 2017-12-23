/*
 *   $Id: result.cpp,v 1.2 2009/10/17 13:49:23 mapibid Exp $
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

#include "StdAfx.h"
#include "compat/PalmOS.h"
#ifdef SONY_SDK
#include <SonyCLIE.h>
#endif

#ifdef SUPPORT_DIA
#include "DIA.h"
#else
#define HanderaAdjustFont( font )	( font )
#endif

//#include "clie.h"
#include "calcrsc.h"
#include "konvert.h"
#include "stack.h"
#include "funcs.h"
#include "defuns.h"
#include "result.h"
#include "prefs.h"
#include "display.h"
#include "ansops.h"
//#include "calc.h"
#include "fp.h"
//#include "lstedit.h"
//#include "mtxedit.h"

#define BOLDRES	1

/* This handle gets freed from the PalmOS at the end,
 * or it would have to be called from some other routine than AppHandler */
WinHandle txtWindow = NULL;
Coord position;
RectangleType resultBounds;


/***********************************************************************
 *
 * FUNCTION:     result_popup
 * 
 * DESCRIPTION:  Popup a list with operations related to the result field
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
result_popup(void)
{
}


/***********************************************************************
 *
 * FUNCTION:    result_copy
 * 
 * DESCRIPTION: Copy contents of Result field to clipboard
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *      
 ***********************************************************************/
void
result_copy(void)
{
}

/***********************************************************************
 *
 * FUNCTION:     result_set
 * 
 * DESCRIPTION:  set new result to display, automatically convert
 *               given item
 *
 * PARAMETERS:   Trpn item - the object to display
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void result_set(Trpn item) IFACE;
void
result_set(Trpn item)
{
}

/***********************************************************************
 *
 * FUNCTION:     result_set_text
 * 
 * DESCRIPTION:  set text result on display
 *
 * PARAMETERS:   text - text to display
 *               type - type of the displayed text
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
result_set_text(TCHAR *text,rpntype type) IFACE;
void
result_set_text(TCHAR *text,rpntype type)
{
}

/***********************************************************************
 *
 * FUNCTION:     result_set_pow
 * 
 * DESCRIPTION:  set formatted text on display,
 *               DOESN'T CHANGE ANSTYPE!
 *
 * PARAMETERS:   text - text to display
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
result_set_pow(TCHAR *text) IFACE;
void
result_set_pow(TCHAR *text)
{
}

/***********************************************************************
 *
 * FUNCTION:     result_init
 * 
 * DESCRIPTION:  Initialize resultField gadget, allocate txtWindow
 *
 * PARAMETERS:   id - resource id of the gadget
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void result_init(Int16 id) IFACE;
void
result_init(Int16 id)
{
	UInt16 err;
	UInt16 size=0;
	Int16 version;
	void *prefData;
	Coord dx, dy;
	FormType *frm = FrmGetActiveForm();
	
	FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, id), &resultBounds);
	resultBounds.topLeft.x *= gSonyFactor;
	resultBounds.topLeft.y *= gSonyFactor;
	resultBounds.extent.x *= gSonyFactor;
	resultBounds.extent.y *= gSonyFactor;
	if (gHrMode == hrPalm) {
		WinHandle oldwin;
		UInt16 save;

		/* make sure we have a valid draw window */
		oldwin = WinSetDrawWindow(WinGetDisplayWindow());
		save = WinSetCoordinateSystem(kCoordinatesNative);
		dx = WinScaleCoord(resultBounds.extent.x, true);
		dy = WinScaleCoord(resultBounds.extent.y, true);
		WinSetCoordinateSystem(save);
		WinSetDrawWindow(oldwin);
	} else {
		dx = resultBounds.extent.x;
		dy = resultBounds.extent.y;
	}

	if (!txtWindow) {
		txtWindow = clie_createoffscreenwindow(dx, dy, nativeFormat, &err);
		ErrFatalDisplayIf(err,"Error while creating txtWindow.");	
		position = 0;
		/* Read the stored text last time used in this gadget */
		version=PrefGetAppPreferences(APP_ID,PREF_RESULT,NULL,&size,false);
		if (version==PREF_RES_VERSION) {
			prefData = MemPtrNew(size);
			PrefGetAppPreferences(APP_ID,PREF_RESULT,prefData,&size,false);
			resultPrefs = *(TresultPrefs *)prefData;
			
			if (resultPrefs.formatType == plaintext)
				result_set_text((TCHAR *)prefData+sizeof(resultPrefs),
						resultPrefs.ansType);
			else if (resultPrefs.formatType == powtext)
				result_set_pow((TCHAR *)prefData+sizeof(resultPrefs));
			else {
				result_set_text((TCHAR *)prefData+sizeof(resultPrefs),
						resultPrefs.ansType);
				resultPrefs.formatType = fmtnumber;
				result_print_num(displayedText);
			}
			MemPtrFree(prefData);
		} else 
			result_set_text("",notype);
	}
}

/***********************************************************************
 *
 * FUNCTION:     result_destroy
 * 
 * DESCRIPTION:  Free all structures asociated with resultField
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void result_destroy(void) IFACE;
void
result_destroy(void)
{
	void *data;
	
	if (!txtWindow) /* The gadget was not initialized */
	  return;

	WinDeleteWindow(txtWindow,false);
	txtWindow = NULL;
	
	data = MemPtrNew(sizeof(resultPrefs)+StrLen(displayedText)+1);
	*(TresultPrefs *)data = resultPrefs;
	StrCopy((TCHAR *)data+sizeof(resultPrefs),displayedText);
	
	PrefSetAppPreferences(APP_ID,PREF_RESULT,PREF_RES_VERSION,
						  data,StrLen(displayedText)+1+sizeof(resultPrefs),
						  false);
	MemPtrFree(data);
	if (displayedText)
	  MemPtrFree(displayedText);
}

static Boolean larr_drawn=false,rarr_drawn=false;

void result_clear_arrowflags(void) IFACE;
void
result_clear_arrowflags(void)
{
	larr_drawn = rarr_drawn = false;
}

/***********************************************************************
 *
 * FUNCTION:     result_draw
 * 
 * DESCRIPTION:  Redraw a gadget, depending on the position variable
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void result_draw(void) IFACE;
void
result_draw(void)
{
	RectangleType tmpbounds;
	MemHandle bmpRes;
	BitmapType *bmpPtr;
	Coord x, y, width,height;
	RectangleType re;
	WinHandle oldwindow;

	if (gHrMode == hrPalm) {
		UInt16 save = WinSetCoordinateSystem(kCoordinatesNative);

		tmpbounds.topLeft.x = WinScaleCoord(position, true);
		tmpbounds.topLeft.y = 0;
		tmpbounds.extent.x =  WinScaleCoord(resultBounds.extent.x, true);
		tmpbounds.extent.y =  WinScaleCoord(resultBounds.extent.y, true);
		x = WinScaleCoord(resultBounds.topLeft.x, true);
		y = WinScaleCoord(resultBounds.topLeft.y, true);
		WinSetCoordinateSystem(save);
	} else {
		tmpbounds.topLeft.x = position;
		tmpbounds.topLeft.y = 0;
		tmpbounds.extent.x =  resultBounds.extent.x;
		tmpbounds.extent.y =  resultBounds.extent.y;
		x = resultBounds.topLeft.x;
		y = resultBounds.topLeft.y;
	}
	clie_copyrectangle(txtWindow,NULL,&tmpbounds, x, y, winPaint);

	re.extent.x = 3;
	re.extent.y = 5;
	re.topLeft.y = resultBounds.topLeft.y/gSonyFactor+4;
	
	oldwindow = WinSetDrawWindow(txtWindow);
	WinGetWindowExtent(&width,&height);
	if (gHrMode == hrPalm) {
		UInt16 save = WinSetCoordinateSystem(kCoordinatesNative);

		width = WinUnscaleCoord(width, true);
		height = WinUnscaleCoord(height, true);
		WinSetCoordinateSystem(save);
	}
	WinSetDrawWindow(oldwindow);
	
	if (position>3*gSonyFactor && !larr_drawn) {
		bmpRes = DmGetResource(bitmapRsc,bmpLArrow);
		bmpPtr = MemHandleLock(bmpRes);
		clie_drawbitmap(bmpPtr,
				resultBounds.topLeft.x-4*gSonyFactor,
				resultBounds.topLeft.y+4*gSonyFactor);
		MemHandleUnlock(bmpRes);
		DmReleaseResource(bmpRes);
		larr_drawn = true;
	}
	if (position <=3*gSonyFactor && larr_drawn) {
		re.topLeft.x = resultBounds.topLeft.x/gSonyFactor-4;
		WinEraseRectangle(&re,0);
		larr_drawn = false;
	}
	
	if (position>=width-resultBounds.extent.x-3*gSonyFactor && rarr_drawn) {
		re.topLeft.x = (resultBounds.topLeft.x+resultBounds.extent.x)/gSonyFactor+1;
		WinEraseRectangle(&re,0);
		rarr_drawn = false;
	}
	if (position<width-resultBounds.extent.x-3*gSonyFactor && !rarr_drawn) {
		bmpRes = DmGetResource(bitmapRsc,bmpRArrow);
		bmpPtr = MemHandleLock(bmpRes);
		clie_drawbitmap(bmpPtr,
				resultBounds.topLeft.x+resultBounds.extent.x+1*gSonyFactor,
			 	resultBounds.topLeft.y+4*gSonyFactor);
		MemHandleUnlock(bmpRes);
		DmReleaseResource(bmpRes);
		rarr_drawn = true;
	}							
}

/***********************************************************************
 *
 * FUNCTION:     result_track
 * 
 * DESCRIPTION:  track the pen movement over result gadget
 *
 * PARAMETERS:   x,y - starting pen position
 *
 * RETURN:       true - pen movement handled by this gadget
 *               false - the tap wasn't for this gadget
 *      
 ***********************************************************************/
Boolean result_track(Coord x,Coord y) IFACE;
Boolean
result_track(Coord x,Coord y)
{
	Coord width,height;
	Coord oldx,oldy;
	Boolean penDown;
	Int16 diff;
	WinHandle oldwindow;

	x *= gSonyFactor; y *= gSonyFactor;
	oldx = x; oldy = y;
	if (!RctPtInRectangle(x,y,&resultBounds))
	  return false;

#ifdef SPECFUN_ENABLED
	/* Popup list editor if the displayed thing is a list */
	if (resultPrefs.ansType == list) {
		lstedit_popup_ans();
		return true;
	} else if (resultPrefs.ansType == matrix || 
		   resultPrefs.ansType == cmatrix) {
		mtxedit_popup_ans();
		return true;
	}
#endif
	
	oldwindow = WinSetDrawWindow(txtWindow);
	WinGetWindowExtent(&width,&height);
	if (gHrMode == hrPalm) {
		UInt16 save = WinSetCoordinateSystem(kCoordinatesNative);

		width = WinUnscaleCoord(width, true);
		height = WinUnscaleCoord(height, true);
		WinSetCoordinateSystem(save);
	}
	WinSetDrawWindow(oldwindow);

	do {
		EvtGetPen(&x,&y,&penDown);
		x *= gSonyFactor; y *= gSonyFactor;
		if (oldx!=x && RctPtInRectangle(x,y,&resultBounds)) {
			diff = oldx-x;
			if (position+diff>=0 &&
				position+diff+resultBounds.extent.x<=width) {
				position+=diff;
				result_draw();
			}
			oldx = x;
			oldy = y;
		}		
	}while(penDown);
	return true;
}

static void
result_clear(Coord width) IFACE;
static void
result_clear(Coord width)
{
	UInt16 err;
	Coord dx, dy;
	
	if (txtWindow)
		WinDeleteWindow(txtWindow,false);
	if (gHrMode == hrPalm) {
		WinHandle oldwin;
		UInt16 save;

		/* Make sure we have a valid draw window */
		oldwin = WinSetDrawWindow(WinGetDisplayWindow());
		save = WinSetCoordinateSystem(kCoordinatesNative);
		dx = WinScaleCoord(width, true);
		dy = WinScaleCoord(resultBounds.extent.y, true);
		WinSetCoordinateSystem(save);
		WinSetDrawWindow(oldwin);
	} else {
		dx = width;
		dy = resultBounds.extent.y;
	}
	txtWindow = clie_createoffscreenwindow(dx,dy,nativeFormat,&err); 
	ErrFatalDisplayIf(err,"Cannot create window."); 
}

/***********************************************************************
 *
 * FUNCTION:     result_print
 * 
 * DESCRIPTION:  Prints a given text on display
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
result_print(TCHAR *text)
{
}

/***********************************************************************
 *
 * FUNCTION:     result_print_pow
 * 
 * DESCRIPTION:  Formats a a^b in a nice way on the display
 *
 * PARAMETERS:   text to display, should contain a '^' character
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
result_print_pow(TCHAR *text)
{
}

/***********************************************************************
 *
 * FUNCTION:     result_print_num
 * 
 * DESCRIPTION:  Formats a generic real/int number, adds
 *               spacing to display e.g. 1111 1111, or 100.222,333 444
 *
 * PARAMETERS:   text to display
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void
result_print_num(TCHAR *text)
{
}
  
void
result_error(CError errcode)
{
}
