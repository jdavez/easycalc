 /*   
 * $Id: prefs.h,v 1.9 2006/09/12 19:40:55 tvoverbe Exp $
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

#ifndef _PREFS_H_
#define _PREFS_H_

#include "segment.h"
#include "calcrsc.h"
#include "konvert.h"
#include "defuns.h"
#include "display.h"

typedef struct {
	UInt16 form;     /* last form */
	UInt16 btnRow;  /* Selected btnrow on scientific form */
	UInt16 insertPos;     /* last insert position */
	UInt16 selPosStart, selPosEnd;
	Boolean finBegin;  /* Begin/end selector in financial form */
	
	Boolean matchParenth; /* Add ')' to functions ending with '(' */
	Ttrigo_mode trigo_mode;
	
	TdispPrefs dispPrefs;
	Boolean insertHelp; /* Insert help strings where appropriate */
	Boolean acceptPalmPref; /* Accept Palm settings about number formatting */
	Boolean reducePrecision;
	Boolean dispScien;
	Int16 solverWorksheet;
	char input[MAX_INPUT_LENGTH+1];
}tPrefs;

void prefs_read_preferences() IFACE;
void  prefs_save_preferences() IFACE;
Boolean PreferencesHandleEvent(EventPtr event) IFACE;

extern tPrefs calcPrefs;
#endif
