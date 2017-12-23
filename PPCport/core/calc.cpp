/*
 * $Id: calc.cpp,v 1.1 2009/10/17 13:47:14 mapibid Exp $
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
#include "stdafx.h"

#include "compat/PalmOS.h"
#include "system - UI/EasyCalc.h"
#include "core/mlib/display.h"
#include "core/mlib/calcDB.h"
#include "core/mlib/fp.h"
#include "core/mlib/history.h"
#include "core/chkstack.h"
#include "core/prefs.h"
#ifdef GRAPHS_ENABLED
#include "grprefs.h"
// TODO #include "grsetup.h"
// TODO #include "graph.h"
// TODO #include "grtable.h"
#endif /* GRAPH_ENABLED */


Err StartApplication (void) {
#ifdef PALMOS
    /* Get version of PalmOS */
    palmOS3 = calc_rom_greater(3,0);  /* for stack check */
    palmOS35 = calc_rom_greater(3,5); /* for color/grayscale support */
#endif

    /* Get stack boundaries */
    initChkStack();

    /* Initialize all modules */
    if (mlib_init_mathlib())
        return 1;

    /* This also opens DBs */
    prefs_read_preferences();

    fp_setup_flpoint();

#ifdef GRAPHS_ENABLED
// TODO    grpref_init();
#endif

    return 0;
}

void StopApplication (void) {
    /* close all modules */
// TODO    result_destroy();

    mlib_close_mathlib();

    prefs_save_preferences();

    db_close();

    history_close();
#ifdef GRAPHS_ENABLED
// TODO    grpref_close();
#endif
}
