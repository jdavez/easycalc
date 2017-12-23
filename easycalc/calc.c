/*   
 * $Id: calc.c,v 1.40 2007/12/18 09:06:36 cluny Exp $
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
 *  Antonio Fiol Bonnín" <Antonio.FIOL@enst-bretagne.fr> submitted patches 
 *   for the financial part.
 *
 *  2003-05-19 - Arno Welzel - added code for Sony Clie support
*/

#include <PalmOS.h>
#include <StringMgr.h>
#include <stdio.h>
#include "about.h"
#ifdef SONY_SDK
#include <SonyCLIE.h>
#endif
#ifdef HANDERA_SDK
#include "Vga.h"
#endif

#include "clie.h"
#include "defuns.h"
#include "calc.h"
#include "calcDB.h"
#include "result.h"
#include "defmgr.h"
#include "calcrsc.h"    
#include "graph.h"
#include "finance.h"
#include "varmgr.h"
#include "prefs.h"
#include "memo.h"
#include "main.h"
#include "history.h"
#include "fp.h"
#include "lstedit.h"
#include "mtxedit.h"
#include "chkstack.h"

#ifdef SPECFUN_ENABLED
#include "solver.h"
#endif

#ifdef GRAPHS_ENABLED
#include "grprefs.h"
#include "grsetup.h"
#include "graph.h"
#include "grtable.h"
#endif /* GRAPH_ENABLED */

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

Boolean palmOS3=false;
Boolean palmOS35=false;
Boolean handera=false;
Boolean colorDisplay=false;
Boolean grayDisplay=false;

void *GetObjectPtr(UInt16 objectID)
{
	FormPtr frm;
	
	frm=FrmGetActiveForm();
	return (FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,objectID)));
}

void
gadget_bounds(FormType *frm, Int16 gadget,
              RectangleType *natbounds, RectangleType *stdbounds)
{
	UInt16 index = FrmGetObjectIndex(frm, gadget);

	FrmGetObjectBounds(frm, index, stdbounds);
	RctCopyRectangle(stdbounds, natbounds);
	
	// If Sony HR Lib is available, adjust coordinates for high resolution
#ifdef SONY_SDK
	if(gHrMode == hrSony) {
		natbounds->topLeft.x = natbounds->topLeft.x*2 +2;
		natbounds->topLeft.y *= 2;
		natbounds->extent.x *= 2;
		natbounds->extent.y *= 2;
	}
#endif
	if(gHrMode == hrPalm) {
		UInt16 save;

		save = WinSetCoordinateSystem(kCoordinatesNative);
		WinScaleRectangle(natbounds);
		WinSetCoordinateSystem(save);
		natbounds->topLeft.x+=2;
	}
	natbounds->extent.x++;
	natbounds->extent.y++;
}

static void 
calc_init_application()
{
	prefs_read_preferences();
}

/* Change to the next screen */
void
GotHardKey(void)
{
	UInt16 formId = 0;
	
	switch (FrmGetActiveFormID()) {
	case frmBasic:
	case frmBasicS:
	    formId = frmScientific;
	    break;
	case frmScientific:
	    formId = frmInteger;
	    break;
	case frmInteger:
#ifdef GRAPHS_ENABLED
	    formId = frmGraph;
#else
	    if (calcPrefs.dispScien)
		formId = frmBasicS;
	    else
		formId = frmBasic;
#endif
	    break;
	case frmGraph:
	    if (calcPrefs.dispScien)
		formId = frmBasicS;
	    else
		formId = frmBasic;
	    break;
	default:
	}
	if (formId)
		ChangeForm(formId);
}

/* Generic button action - actions for buttons  on all screens */
Int16
chooseForm(Int16 button)
{
	Boolean handled = true;

	switch (button) {
	case hlpMain2:
		doHelp(hlpMain2T, hlpMain2);
		break;
	case hlpFunc:
		doHelp(hlpFuncT, hlpFunc);
		break;
	case mitCredits:
		doHelp(strCreditsT, strCredits);
		break;
	case mitAbout:
		doAbout(hlpMain1);
		break;
	case btnInteger:
		ChangeForm(frmInteger);
		break;
	case btnBasic:
		if (calcPrefs.dispScien) 
			ChangeForm(frmBasicS);
		else
			ChangeForm(frmBasic);
		break;
	case btnScientific:
		ChangeForm(frmScientific);
		break;
#ifdef GRAPHS_ENABLED		
	case btnGraph:
		ChangeForm(frmGraph);
		break;
#endif		
	default:
		handled = false;
	}
	return handled;
}

Boolean
calc_rom_greater(UInt16 major, UInt16 minor)
{
	UInt32        romVersion;
	UInt16        maj;
	UInt16        min;

	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	maj = sysGetROMVerMajor(romVersion);
	min = sysGetROMVerMinor(romVersion);
	if (maj > major)
		return true;
	else if ((maj == major) && (min >= minor))
		return true;
	
	return false;
}

static Err 
StartApplication(void)
{
	Err error;
	UInt32 version, attr;

	/* Get version of PalmOS */
	palmOS3 = calc_rom_greater(3,0);  /* for stack check */
	palmOS35 = calc_rom_greater(3,5); /* for color/grayscale support */

	/* Get stack boundaries */
	initChkStack();

	/* Are we on a Handera ? */
#ifdef HANDERA_SDK
	if (FtrGet(TRGSysFtrID, TRGVgaFtrNum, &version) == errNone)
		handera = true;
#endif

	gHrMode = hrNone;
	FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version);
	if (version >= 4) {
		WinScreenGetAttribute(winScreenDensity, &attr);
		if (attr > kDensityLow)
			gHrMode = hrPalm;
	}

#ifdef SONY_SDK
	gHrLibApiVersion = GetHRLibVersion(&gHrLibRefNum); // HR Lib available?
	if (gHrLibApiVersion && gHrMode == hrNone) {
		UInt32 width,height,depth;

		HROpen(gHrLibRefNum); // Open HR Lib

		width = hrWidth;
		height = hrHeight;
		depth = 8;
		/* Following call fails for grayscale HiRes devices.
		 * Taken care of below in the 'if (palmOS35)' part */
		HRWinScreenMode(gHrLibRefNum, winScreenModeSet, &width, &height, &depth, NULL);
		gHrMode = hrSony;
		gSonyFactor = 2;
	}
#endif
	if (palmOS35){
		WinScreenMode(winScreenModeGet,NULL,NULL,NULL,&colorDisplay);
		if (!colorDisplay){
			UInt32 depth=4;
#ifdef SONY_SDK
			if (gHrMode == hrSony){
				/* Set Sony Hires grayscale devices to HiRes */
				UInt32 width = hrWidth;
				UInt32 height = hrHeight;
				if (!HRWinScreenMode(gHrLibRefNum, winScreenModeSet, &width, &height, &depth, NULL))
				grayDisplay = true;
			}
			else
#endif
			if (!WinScreenMode(winScreenModeSet,NULL,NULL,&depth,NULL))
				grayDisplay=true;
		}
	}

	/* Initialize all modules */
	if (mlib_init_mathlib())
		return 1;

#ifdef SUPPORT_DIA
	InitializeResizeSupport(resize_index);
	LoadResizePrefs(APP_ID, PREF_DIA);
#endif

	error = db_open();
	ErrFatalDisplayIf(error, "Can't open CalcDB");
	
	error = history_open();
	ErrFatalDisplayIf(error, "Can't open History DB");
	
	calc_init_application(); /* also calls FrmGotoForm */
	
	fp_setup_flpoint();

#ifdef GRAPHS_ENABLED	
	grpref_init();
#endif

	return 0;
}

static void 
StopApplication(void)
{	
	FrmSaveAllForms();
	FrmCloseAllForms();

	/* close all modules */
	result_destroy();
	
	mlib_close_mathlib();
	
	prefs_save_preferences();
	
	db_close();
	
	history_close();
#ifdef GRAPHS_ENABLED	
	grpref_close();
#endif	
#ifdef SUPPORT_DIA
	SaveResizePrefs(APP_ID, PREF_DIA, PREF_DIA_VERSION);
	TerminateResizeSupport();
#endif
#ifdef SONY_SDK
	if (gHrMode == hrSony) {
		/* Set screen back to defaults and close HR library */
		HRWinScreenMode(gHrLibRefNum, winScreenModeSetToDefaults,
		                NULL, NULL, NULL, NULL);
		HRClose(gHrLibRefNum);
	}
#endif
}

char *
print_error(CError err)
{
	char *res;

	res = MemPtrNew(MAX_RSCLEN);
	SysStringByIndex(strErrCodes,err,res,MAX_RSCLEN-1);
	return res;
}

void alertErrorMessage(CError err)
{
	Char *txt;

	txt = print_error(err);
	FrmCustomAlert(altErrorMsg, txt, NULL, NULL);
	MemPtrFree(txt);
}

void ChangeForm(Int16 formID)
{
	Int16 activefrm = FrmGetActiveFormID();
	
	if (activefrm==formID)
	  return;
	FrmGotoForm(formID);       
}

static WinHandle saved_window;

void
wait_draw(void)
{
	RectangleType rec,frame;
	UInt16 error;

	frame.topLeft.x = 50;
	frame.topLeft.y = 50;
	frame.extent.x = 60;
	frame.extent.y = 20;

	rec.topLeft.x = 48;
	rec.topLeft.y = 48;
	rec.extent.x = 64;
	rec.extent.y = 24;

	saved_window = WinCreateOffscreenWindow(rec.extent.x,rec.extent.y,
						nativeFormat,&error);

	/* Save the screen */
	WinCopyRectangle(NULL,saved_window,&rec,0,0,winPaint);
	
	WinEraseRectangle(&rec,2);
	WinDrawRectangleFrame(boldRoundFrame,&frame);
	WinDrawChars("Please wait...",14,55,55);
}

void
wait_erase(void)
{
	RectangleType rec;
		
	rec.topLeft.x = 0;
	rec.topLeft.y = 0;
	rec.extent.x = 64;
	rec.extent.y = 24;

	WinCopyRectangle(saved_window,NULL,&rec,48,48,winPaint);
	WinDeleteWindow(saved_window,0);
}


static Boolean 
ApplicationHandleEvent(EventPtr event)
{
	FormPtr  frm;
	Int16    formId;
	Boolean  handled = false;
	
	if (event->eType == frmLoadEvent) {
		formId = event->data.frmLoad.formID;
		// Load the form resource specified in the event then activate it,
		// except for the About and Help forms which are already loaded.
		if (formId != frmAbout && formId != frmHelp)
			frm = FrmInitForm(formId);
		else
			frm = FrmGetFormPtr(formId);
		FrmSetActiveForm(frm);
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmDispatchEvent each time it is called
		switch (formId) {
#ifdef GRAPHS_ENABLED			
		case grSetupForm:
			FrmSetEventHandler(frm,GraphSetupHandleEvent);
			break;
		case grPrefForm:
			FrmSetEventHandler(frm,GraphPrefsHandleEvent);
			break;
		case grTableForm:
			FrmSetEventHandler(frm,GraphTableHandleEvent);
			break;
		case frmGraph:
#ifdef SUPPORT_DIA
			SetResizePolicy(formId);
#endif
			FrmSetEventHandler(frm,GraphFormHandleEvent);
			break;
#endif /* GRAPHS_ENABLED */
		case frmAbout:
		case frmHelp:
#ifdef SUPPORT_DIA
			SetResizePolicy(formId);
#endif
			FrmSetEventHandler(frm,aboutEventHandler);
			break;
		case prefForm:
			FrmSetEventHandler(frm,PreferencesHandleEvent);
			break;
		case frmInteger:
		case frmBasic:
		case frmBasicS:
		case frmScientific:
			FrmSetEventHandler(frm, MainFormHandleEvent);
			break;
		case defForm:
			FrmSetEventHandler(frm, DefmgrHandleEvent);
			break;
		case finForm:
			FrmSetEventHandler(frm, FinHandleEvent);
			break;
		case varEntryForm:
			FrmSetEventHandler(frm,VarmgrEntryHandleEvent);
			break;
		case memoImportForm:
			FrmSetEventHandler(frm,MemoImportHandleEvent);
			break;
#ifdef SPECFUN_ENABLED
		case slvForm:
			FrmSetEventHandler(frm, SolverHandleEvent);
			break;
		case frmListEdit:
			FrmSetEventHandler(frm,ListEditHandleEvent);
			break;
		case frmMatrix:
			FrmSetEventHandler(frm,MatrixHandleEvent);
			break;
#endif
		}	
		handled = true;	       
	}
	
	return handled;
}

Int32 evtTimeout = evtWaitForever;

void
calc_nil_timeout(Int32 timeout)
{
	evtTimeout = timeout;
}

/* Allow some hardkeys to track graphs through */
Boolean
hard_key_to_pass_through(EventPtr event)
{
	return (FrmGetActiveFormID() == frmGraph
		&& CtlGetValue(GetObjectPtr(btnGraphTrack))
		&& TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr)
		&& ((event->data.keyDown.chr == vchrHard2)
		    || (event->data.keyDown.chr == vchrHard3)));
}

static void 
EventLoop(void)
{
	EventType  event;
	UInt16      error;
	
	do {
		EvtGetEvent(&event, evtTimeout);

		/* Only the button assigned to EasyCalc should switch modes */
		if (hard_key_to_pass_through(&event) || ! SysHandleEvent(&event)) {
			if (event.eType == keyDownEvent
			    && TxtCharIsHardKey(event.data.keyDown.modifiers, event.data.keyDown.chr)
			    && ! (event.data.keyDown.modifiers & poweredOnKeyMask)
			    && ! (hard_key_to_pass_through(&event)))
				GotHardKey();
			else if (! MenuHandleEvent(0, &event, &error))
				if (! ApplicationHandleEvent(&event))
					FrmDispatchEvent(&event);
		}
	} while (event.eType != appStopEvent);
}

UInt32
PilotMain(UInt16 launchCode, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err err=0;
	
	if (launchCode == sysAppLaunchCmdNormalLaunch) { 
		if ((err = StartApplication()) == 0) {
			EventLoop();
			StopApplication();
		}
	}
#ifdef SUPPORT_DIA
	else if (launchCode == sysAppLaunchCmdNotify) {
		HandleResizeNotification(((SysNotifyParamType *)cmdPBP)->notifyType);
	}
#endif
	else
		return sysErrParamErr;
	
	return err;
}
