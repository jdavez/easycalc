/*
 *   $Id: result.h,v 1.7 2006/09/12 19:40:55 tvoverbe Exp $
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

#ifndef _RESULT_H_
#define _RESULT_H_

#include "display.h"

typedef enum {
	COPYRESULT=0,	  
	VARSAVEAS,
	DEFMGR,
	GUESSIT,
	TODEGREE,
	TORADIAN,
	TOGONIO,
	TOCIS,
	ENGDISPLAY,
	TODEGREE2
}resSelection;
#define SELECTION_COUNT 9

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
	
void result_copy(void) IFACE;
void result_init(Int16 id) IFACE;
void result_destroy(void) IFACE;
void result_clear_arrowflags(void) IFACE;
void result_draw(void) IFACE;
Boolean result_track(Coord x,Coord y) IFACE;
void result_set(Trpn item) IFACE;
void result_set_text(char *text,rpntype type) IFACE;
void result_set_pow(char *text) IFACE;
void result_popup(void) IFACE;
void result_error(CError errcode) IFACE;
#endif
