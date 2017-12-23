#pragma once

class Main
{
public:
    Main(void);
    ~Main(void);
};

/*   
 * $Id: Main.h,v 1.5 2009/12/24 16:35:58 mapibid Exp $
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
*/

#ifndef _MAIN_H_
#define _MAIN_H_

#include "Skin.h"

// Number of defined buttons in Main.cpp
#define BUTTON_COUNT 187

#ifndef _MAIN_C_
extern const struct {
        const char *trigtext;
} trigmode[];
#endif

//Boolean MainFormHandleEvent(EventPtr event);
#include "core/mlib/konvert.h"
Boolean main_btnrow_click(Skin *skin, int btnid);
CError main_input_exec (TCHAR *inp, Trpn *result);
void main_insert(Skin *skin, void *hwnd_edit, const TCHAR *text, Boolean operatr, Boolean func,
                 Boolean nostartparen, const TCHAR *helptext);

#endif