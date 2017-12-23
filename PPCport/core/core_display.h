/*****************************************************************************
 * EasyCalc -- a scientific calculator
 * Copyright (C) 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/
#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H 1

//#include "EasyCalc.h"
#include "system - UI/skin.h"
#include "compat/PalmOS.h"
#include "core/mlib/display.h"
#include "core/prefs.h"

typedef enum {
	plaintext,
	powtext,
	fmtnumber
}printType;

typedef struct {
	rpntype ansType;
	printType formatType;
	Tbase dispBase;
}TresultPrefs;
	
#define MENULEVEL_COMMAND   0
#define MENULEVEL_ALPHA     1
#define MENULEVEL_TRANSIENT 2
#define MENULEVEL_PLAIN     3
#define MENULEVEL_APP       4

#ifndef _CORE_DISPLAY_C_
extern TCHAR *resMenuDesc[];     // Array of strings for the result actions menu.
extern TCHAR *strErrCodes[];     // Error strings
extern TresultPrefs resultPrefs; // Description of result in result screen

#endif

void squeak(void);

void result_copy(Skin *skin) IFACE;
void result_set(Skin *skin, void *hWnd_p, Trpn item) IFACE;
void result_set_text(Skin *skin, void *hWnd_p, TCHAR *text,rpntype type) IFACE;
void result_set_pow(Skin *skin, void *hWnd_p, TCHAR *text) IFACE;
void result_popup(Skin *skin, void *hWnd_p) IFACE;
void result_action(Skin *skin, void *hWnd_p, void *hWnd_calc, int selection);
void result_error(Skin *skin, void *hWnd_p, CError errcode) IFACE;
TCHAR *print_error(CError err);
int input_exec (Skin *skin, void *hWnd_p);

#endif
