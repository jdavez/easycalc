/*
 *   $Id: result.c,v 1.34 2008/02/08 13:04:10 cluny Exp $
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
#ifdef SONY_SDK
#include <SonyCLIE.h>
#endif

#ifdef SUPPORT_DIA
#include "DIA.h"
#else
#define HanderaAdjustFont( font )	( font )
#endif

#include "clie.h"
#include "calcrsc.h"
#include "konvert.h"
#include "stack.h"
#include "funcs.h"
#include "defuns.h"
#include "result.h"
#include "prefs.h"
#include "display.h"
#include "ansops.h"
#include "calc.h"
#include "fp.h"
#include "lstedit.h"
#include "mtxedit.h"

#define BOLDRES	1

/* This handle gets freed from the PalmOS at the end,
 * or it would have to be called from some other routine than AppHandler */
WinHandle txtWindow = NULL;
Coord position;
RectangleType resultBounds;
char *displayedText=NULL;

TresultPrefs resultPrefs;

static void result_print_num(char *text) IFACE;
static void result_print_pow(char *text) IFACE;
static void result_print(char *text) IFACE;


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
result_popup(void) IFACE;
void
result_popup(void)
{
	Int16 selection,i;
	Int16 count=0;
	resSelection choices[SELECTION_COUNT];
	char *description[SELECTION_COUNT];
	ListPtr list; 
	Tdisp_mode oldmode;
	
	choices[count++] = COPYRESULT;
	choices[count++] = DEFMGR;
	if (resultPrefs.ansType != notype) {
		choices[count++] = VARSAVEAS;
		if (resultPrefs.ansType == real ||
		    resultPrefs.ansType == complex) {
			choices[count++] = GUESSIT;
			choices[count++] = ENGDISPLAY;
			if (calcPrefs.trigo_mode == degree) {
				choices[count++] = TODEGREE;
				choices[count++] = TODEGREE2;
			} else if (calcPrefs.trigo_mode == radian)
				choices[count++] = TORADIAN;
		}
		if (resultPrefs.ansType==complex) {
			choices[count++] = TOGONIO;
			choices[count++] = TOCIS;
		}
	}

	for (i=0;i<count;i++) {
		description[i] = MemPtrNew(MAX_RSCLEN);
		SysStringByIndex(strMenuDescription,choices[i],
				 description[i],MAX_RSCLEN-1);
	}
	

	list = GetObjectPtr(calcResList);
	LstSetListChoices(list,description,count);
	LstSetHeight(list,count);
	selection=LstPopupList(list);

	for (i=0;i<count;i++) 
	  MemPtrFree(description[i]);
	
	if (selection==noListSelection)
	  return;
	
	switch (choices[selection]) {
	case COPYRESULT:
		result_copy();
		break;
	case VARSAVEAS:
		FrmPopupForm(varEntryForm);
		break;
	case DEFMGR:
		FrmPopupForm(defForm);
		break;
	case TODEGREE:
		ans_redisplay("todeg(ans)");
		break;
	case TODEGREE2:
		ans_redisplay("todeg2(ans)");
		break;
	case GUESSIT:
		ans_guess();
		break;
	case TOGONIO:
		ans_redisplay("togonio(ans)");
		break;
	case TOCIS:
		ans_redisplay("tocis(ans)");
		break;
	case TORADIAN:
		ans_redisplay("torad(ans)");
		break;
	case ENGDISPLAY:
		oldmode = fp_set_dispmode(disp_eng);
		ans_redisplay("ans");
		fp_set_dispmode(oldmode);
		break;
	}
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
void result_copy(void) IFACE;
void
result_copy(void)
{
	if (displayedText)
	  ClipboardAddItem(clipboardText,displayedText,StrLen(displayedText));
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
	char *text;
	CError err;
	Boolean freeitem = false;
	
	if (item.type==variable || item.type == litem) {
		err = rpn_eval_variable(&item,item);
		if (err) {
			result_error(err);
			return;
		}
		/* Do not display complete matrices */
		if (((item.type == matrix || item.type == cmatrix)
		     && item.u.matrixval->cols * item.u.matrixval->rows > 9)
		    || (item.type == list && item.u.listval->size > 6))
			text = display_default(item,false);
		else 
			text = display_default(item,true);
		resultPrefs.ansType = item.type;
		resultPrefs.dispBase = dispPrefs.base;
		freeitem = true;
	}
	else {
		resultPrefs.ansType = item.type;
		resultPrefs.dispBase = dispPrefs.base;
		if (((item.type == matrix || item.type == cmatrix )
		     && item.u.matrixval->cols * item.u.matrixval->rows > 9)
		    || (item.type == list && item.u.listval->size > 6))
			text = display_default(item,false);
		else
			text = display_default(item,true);
	}
	/* Save the text for later use or Copy&Paste */
	if (displayedText)
		MemPtrFree(displayedText);
	displayedText = MemPtrNew(StrLen(text)+1);
	StrCopy(displayedText,text); 

	if (item.type == integer || item.type == real) {
		resultPrefs.formatType = fmtnumber;
		result_print_num(displayedText);
	} else {
		resultPrefs.formatType = plaintext;
		result_print(displayedText);
	}

	MemPtrFree(text);
	if (freeitem)
		rpn_delete(item);
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
result_set_text(char *text,rpntype type) IFACE;
void
result_set_text(char *text,rpntype type)
{
	if (type != notype)
		resultPrefs.ansType = type;
	resultPrefs.formatType = plaintext;
	
	if (displayedText)
		MemPtrFree(displayedText);
	displayedText = MemPtrNew(StrLen(text)+1);
	StrCopy(displayedText,text); 
	result_print(displayedText);
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
result_set_pow(char *text) IFACE;
void
result_set_pow(char *text)
{
	resultPrefs.formatType = powtext;
	
	if (displayedText)
		MemPtrFree(displayedText);
	displayedText = MemPtrNew(StrLen(text)+1);
	StrCopy(displayedText,text); 
	result_print_pow(displayedText);
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
				result_set_text((char *)prefData+sizeof(resultPrefs),
						resultPrefs.ansType);
			else if (resultPrefs.formatType == powtext)
				result_set_pow((char *)prefData+sizeof(resultPrefs));
			else {
				result_set_text((char *)prefData+sizeof(resultPrefs),
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
	StrCopy((char *)data+sizeof(resultPrefs),displayedText);
	
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
result_print(char *text)
{
	Coord width,chrwidth;	
	FontID oldfont;
	WinHandle oldwindow;
	
	/* Set the position for printing to 0 */
#ifdef SONY_SDK
	if (gHrMode == hrSony)
		oldfont = HRFntSetFont(gHrLibRefNum, hrLargeFont+5*BOLDRES);
	else
#endif
		oldfont = FntSetFont(HanderaAdjustFont(largeFont+5*BOLDRES));
	chrwidth = FntCharsWidth(text,StrLen(text));
	if (chrwidth + 5*gSonyFactor < resultBounds.extent.x)
	  width = resultBounds.extent.x;
	else
	  width = chrwidth + 5*gSonyFactor;
	
	result_clear(width);
	
	oldwindow = WinSetDrawWindow(txtWindow);
	WinEraseWindow();
	clie_drawchars(text,StrLen(text),width-chrwidth - 2*gSonyFactor, 0);
	WinSetDrawWindow(oldwindow);
	
	position = 0;
#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRFntSetFont(gHrLibRefNum, oldfont);
	else 
#endif
		FntSetFont(oldfont);
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
result_print_pow(char *text)
{
	char *base,*expon;
	Coord textwidth,width;
	FontID oldfont;
	WinHandle oldwindow;
	Boolean fin=false;

#ifdef SONY_SDK
	if (gHrMode == hrSony)
		oldfont = HRFntSetFont(gHrLibRefNum, hrLargeFont+5*BOLDRES);
	else
#endif
		oldfont = FntSetFont(HanderaAdjustFont(largeFont+5*BOLDRES));
	textwidth=2;

	base=text;

	do {
		expon = StrChr(base,'^');	
		if (!expon) {
			expon = StrChr(base,NULL);
			fin=true; 
		}
#ifdef SONY_SDK
		if (gHrMode == hrSony)
			HRFntSetFont(gHrLibRefNum, hrLargeFont+5*BOLDRES);
		else
#endif
			FntSetFont(HanderaAdjustFont(largeFont+5*BOLDRES));
		textwidth += FntCharsWidth(base, expon-base);
		if (fin) break;
		expon++;
#ifdef SONY_SDK
		if (gHrMode == hrSony)
			HRFntSetFont(gHrLibRefNum, hrStdFont+BOLDRES);
		else
#endif
			FntSetFont(HanderaAdjustFont(stdFont+BOLDRES));
		if (expon[0] == '(') {
			expon++;
			base=StrChr(expon, ')');
			if (StrChr(expon, '(') && base > StrChr(expon, '('))
				base = StrChr(base+1, ')'); 
			textwidth += FntCharsWidth(expon, base - expon);
			if (++base == NULL) fin = true;
		} else {
			base = StrChr(expon, '*');
			if (!base) base = StrChr(expon, '/');
			if (!base) {
				base = StrChr(expon, NULL);
				fin = true;
			}
			textwidth += FntCharsWidth(expon, base-expon);
		}
	} while (!fin);

	if (textwidth + 8 < resultBounds.extent.x)
		width = resultBounds.extent.x;
	else
		width = textwidth + 8;
	
	result_clear(width);

	oldwindow = WinSetDrawWindow(txtWindow);
	WinEraseWindow();

	fin = false;
	base = text;

	do {
		expon = StrChr(base, '^');	
		if (!expon) {
			expon = StrChr(base, NULL);
			fin = true; 
		}
#ifdef SONY_SDK
		if (gHrMode == hrSony)
			HRFntSetFont(gHrLibRefNum, hrLargeFont+5*BOLDRES);
		else
#endif
			FntSetFont(HanderaAdjustFont(largeFont+5*BOLDRES));
		clie_drawchars(base, expon - base, width - textwidth, 2);
		textwidth -= FntCharsWidth(base, expon - base);
		if (fin) break;
		expon++;
#ifdef SONY_SDK
		if (gHrMode == hrSony)
			HRFntSetFont(gHrLibRefNum, hrStdFont+BOLDRES);
		else
#endif
			FntSetFont(HanderaAdjustFont(stdFont+BOLDRES));
		if (expon[0] == '(') {
			expon++;
			base = StrChr(expon, ')');
			if (StrChr(expon, '(') && base > StrChr(expon, '('))
				base = StrChr(base + 1, ')'); 
			clie_drawchars(expon, base - expon, width - textwidth, 0);
			textwidth -= FntCharsWidth(expon, base - expon);
			if (++base == NULL) fin=true;
		} else {
			base = StrChr(expon, '*');
			if (!base) base = StrChr(expon, '/');
			if (!base) {
				base = StrChr(expon, NULL);
				fin = true;
			}
			clie_drawchars(expon, base - expon, width - textwidth, 0);
			textwidth -= FntCharsWidth(expon, base-expon);
		}
	} while (!fin);

	position = 0;
#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRFntSetFont(gHrLibRefNum, oldfont);
	else 
#endif
		FntSetFont(oldfont);
	WinSetDrawWindow(oldwindow);
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
result_print_num(char *text)
{
	char *result;
	Int16 spacing;
	Int16 i,j,k;
	Int16 endint;

	if (resultPrefs.dispBase == disp_octal || 
	    (resultPrefs.dispBase != disp_hexa && StrChr(text,'E')) ||
	    StrChr(text,'r')) {
		result_print(text);
		return;
	}
	if (resultPrefs.dispBase == disp_decimal)
		spacing = 3;
	else
		spacing = 4;

	result = MemPtrNew(StrLen(text) * 2);
	k = 0;

	/* Handle the negative sign */
	if (text[0] == '-') {
		result[k++] = text[0];
		text++;
	}

	/* Find postion of decimal point */
	for (i=0;text[i];i++)
		if (text[i] == '.' || text[i] == ',')
			break;
	
	/* In engineer notation it may end with somehitng other than
	 * number */
	if (text[i-1] > '9' && (text[i-1]<'A' || text[i-1]>'F'))
		endint = i-1;
	else
		endint = i;
	/* Handle the integer part of text */
	for (j=0;j < endint;j++) {
		if (j>0 && ((endint-j) % spacing) == 0) 
 			result[k++] = flSpaceChar;
		result[k++] = text[j];
	}
	/* Move the engineering suffix, if exists */
	if (endint != i)
		result[k++] = text[endint];
	/* Move decimal point */
	result[k++] = text[i];

	/* Handle the after-dot part of number */
	if (text[i] != '\0') { /* It was not an integer num */
		for (j=i+1;text[j];j++) {
			if ((j-i-1) != 0 && (j-i-1) % spacing == 0)
				result[k++] = ' ';
			result[k++] = text[j];
		}
		/* Handle the case, when there would be 
		 * a unit preceded by space and */
		if (result[k-2] == ' ' && result[k-1] > '9') {
			result[k-2] = result[k-1];
			k--;
		}
		result[k++] = '\0';
	} 
	result_print(result);
	MemPtrFree(result);
}
  
void
result_error(CError errcode)
{
	char *text;
	
	text = print_error(errcode);
	result_set_text(text,notype);
	MemPtrFree(text);
}
