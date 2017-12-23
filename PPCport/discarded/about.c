/*
 * $Id: about.c,v 1.1 2009/10/17 13:49:23 mapibid Exp $
 *
 * Copyright 2006 Ton van Overbeek
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Summary
 *  Resizable about and help form routines for EasyCalc.
 *
 * Author
 *  Ton van Overbeek, ton@v-overbeek.nl
 *
 */

#include <PalmOS.h>
#include "calcrsc.h"
#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static UInt16 returnFormID = frmBasic;
static Char *emptyTitle = "";

static void updateScrollers(FormType *form, FieldType *fld)
{
	UInt16 upix, downix;
	Boolean enableUp, enableDown;

	enableUp = FldScrollable(fld, winUp);
	enableDown = FldScrollable(fld, winDown);
	upix = FrmGetObjectIndex(form, abthlpUp);
	downix = FrmGetObjectIndex(form, abthlpDn);
	FrmUpdateScrollers(form, upix, downix, enableUp, enableDown);
}

static void helpScroll(FormType *form, WinDirectionType dir)
{
	UInt16 linestoscroll;
	FieldType *fld;

	fld = FrmGetObjectPtr(form, FrmGetObjectIndex(form, abthlpText));
	linestoscroll = FldGetVisibleLines(fld) - 1;
	FldScrollField(fld, linestoscroll, dir);
	updateScrollers(form, fld);
}

static void releaseResource(FormType *form)
{
	FieldType *fld;
	MemHandle abthlpH, titleH;
	Char *title;

	fld = FrmGetObjectPtr(form, FrmGetObjectIndex(form, abthlpText));
	abthlpH = FldGetTextHandle(fld);
	FldSetTextHandle(fld, 0);
	DmReleaseResource(abthlpH);
	if ((title = (Char *)FrmGetTitle(form))) {
		titleH = MemPtrRecoverHandle((MemPtr) title);
		FrmSetTitle(form, emptyTitle);
		MemHandleUnlock(titleH);
		DmReleaseResource(titleH);
	}
}

Boolean aboutEventHandler(EventPtr event)
{
	Boolean handled = false;
	FormType *form = FrmGetActiveForm();
	FieldType *fld;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	case frmOpenEvent:
	case frmUpdateEvent:
		FrmDrawForm(form);
		fld = FrmGetObjectPtr(form, FrmGetObjectIndex(form, abthlpText));
		updateScrollers(form, fld);
		handled = true;
		break;

	case ctlSelectEvent:
		if (event->data.ctlEnter.controlID == abthlpOk) {
			FrmGotoForm(returnFormID);
			handled = true;
		}
		break;

	case frmCloseEvent:
		releaseResource(form);
		break;

	case ctlRepeatEvent:
		if (event->data.ctlRepeat.controlID == abthlpUp) {
			helpScroll(form, winUp);
		}
		else if (event->data.ctlRepeat.controlID == abthlpDn) {
			helpScroll(form, winDown);
		}
		break;

	case keyDownEvent:
		if (EvtKeydownIsVirtual(event)) {
			switch (event->data.keyDown.chr) {
			case vchrPageUp:
			case vchrRockerUp:
				helpScroll(form, winUp);
				break;

			case vchrPageDown:
			case vchrRockerDown:
				helpScroll(form, winDown);
				break;
			}
		}
		break;

	default:
		break;
	}

	return handled;
}

void doAbout(UInt16 aboutStringID)
{
	MemHandle aboutH;
	FormType *form;
	FieldType *fld;

	aboutH = DmGetResource(strRsc, aboutStringID);
	form = FrmInitForm(frmAbout);
	fld = FrmGetObjectPtr(form, FrmGetObjectIndex(form, abthlpText));
	FldSetTextHandle(fld, aboutH);
	FldSetInsertionPoint(fld, 0);
	returnFormID = FrmGetActiveFormID();
	if (returnFormID == 0)
	returnFormID = frmBasic;
	FrmGotoForm(frmAbout);
}

void doHelp(UInt16 hlpTitleID, UInt16 hlpTextID)
{
	MemHandle titleH, helpH;
	FormType *frm;
	FieldType *fld;

	titleH = DmGetResource(strRsc, hlpTitleID);
	helpH = DmGetResource(strRsc, hlpTextID);
	frm = FrmInitForm(frmHelp);
	FrmSetTitle(frm, MemHandleLock(titleH));
	fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, abthlpText));
	FldSetTextHandle(fld, helpH);
	FldSetInsertionPoint(fld, 0);
        returnFormID = FrmGetActiveFormID();
	FrmGotoForm(frmHelp);
}
