/*
 *   $Id: fp.h,v 1.6 2007/08/10 18:54:06 tvoverbe Exp $
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

#ifndef _FP_H_
#define _FP_H_

void fp_setup_flpoint(void) PARSER;
void fp_print_double(char *strP, double value) PARSER;
void fp_print_g_double(char *s, double value, Int16 prec) PARSER;
void fp_set_base(Tbase base) PARSER; 
TdispPrefs fp_set_prefs(TdispPrefs prefs) PARSER;
Tdisp_mode fp_set_dispmode(Tdisp_mode newmode) PARSER;

extern char flSpaceChar, flPointChar;

#endif
