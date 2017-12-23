/*
 *   $Id: grprefs.h,v 1.1 2011/02/28 22:07:18 mapibid Exp $
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

#ifndef _GRPREFS_H_
#define _GRPREFS_H_

#include "compat/segment.h"
#include "core/mlib/defuns.h"

#define MIN_LOG_VAL 1E-5

typedef enum {
    graph_func,
    graph_polar,
    graph_param,
    nb_func_types
} Tfunctype;

typedef struct {
    double xmin, ymin;
    double xmax, ymax;
    double xscale, yscale;
    double fimin, fimax, fistep;
    double tmin, tmax, tstep;
    Tfunctype functype;
    TCHAR funcFunc[MAX_GRFUNCS][MAX_FUNCNAME+1];
    TCHAR funcPol[MAX_GRFUNCS][MAX_FUNCNAME+1];
    TCHAR funcPar[MAX_GRFUNCS][2][MAX_FUNCNAME+1]; // Warning: compared to original EasyCalc
                                                   // code, this is inverted:
                                                   // 0 -> y
                                                   // 1 -> x
    Boolean logx,logy;
    Boolean saved_reducePrecision;
    Int16 speed;
    IndexedColorType colors[9];
    Boolean grEnable[nb_func_types][9];
    Int8 grType[6];
} TgrPrefs;

//Boolean GraphPrefsHandleEvent(EventPtr event) IFACE;
CError grpref_comp_field(TCHAR *text, double *value);
Int16 grpref_verify_values (void) IFACE;
CError set_axis(Functype *func, CodeStack *stack) GRAPH;

extern TgrPrefs graphPrefs;
extern TgrPrefs viewPrefs;
extern RGBColorType graphRGBColors[];
extern const TdispPrefs grDPrefs;

#endif
