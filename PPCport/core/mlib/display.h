/*
 *   $Id: display.h,v 1.6 2011/02/28 22:07:47 mapibid Exp $
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

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "Skin.h"
#include "konvert.h"

typedef enum {
    disp_decimal=10,
    disp_hexa=16,
    disp_octal=8,
    disp_binary=2
} Tbase;

typedef enum {
    disp_normal,
    disp_sci,
    disp_eng
} Tdisp_mode;

typedef enum {
    pen_selzone,
    pen_movzone,
    pen_trackpt,
    pen_centerwide
} Tpen_mode;

typedef struct {
    Int8 decPoints;
    Boolean stripZeros;
    Tdisp_mode mode;
    Tbase base;
    Boolean forceInteger;
    Boolean cvtUnits;
    Tpen_mode penMode;
} TdispPrefs;

TCHAR * display_real(double number) MLIB;
TCHAR * display_complex(Complex number) MLIB;
TCHAR * display_integer(UInt32 number,Tbase mode) MLIB;
TCHAR * display_default(Trpn rpn,Boolean complete,rpntype *resultType) MLIB;
TCHAR * display_list(List *list) MLIB;

#ifndef _DISPLAY_C_
extern TdispPrefs dispPrefs;
#endif

#endif
