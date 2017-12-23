 /*
 * $Id: prefs.cpp,v 1.1 2009/10/17 13:47:14 mapibid Exp $
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

#include "stdafx.h"
#include "compat/PalmOS.h"

//#include "calcrsc.h"
#include "konvert.h"
//#include "calc.h"
#define _PREFS_C_
#include "prefs.h"
#include "calcDB.h"
#include "fp.h"
//#include "about.h"
#include "System - UI/EasyCalc.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

tPrefs calcPrefs;

void prefs_read_preferences (void) {
    read_state();
    fp_set_prefs(calcPrefs.dispPrefs);
}

void prefs_save_preferences (void) {
    calcPrefs.dispPrefs = dispPrefs;
    save_state();
}
