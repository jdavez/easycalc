/*
 *   $Id: grprefs.h,v 1.15 2007/12/16 12:24:50 cluny Exp $
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

#include "segment.h"
#include "defuns.h"

#define MAX_GRFUNCS 6
#define MIN_LOG_VAL 1E-5

typedef enum {
	graph_func,
	graph_polar,
	graph_param
}Tfunctype;

typedef struct {
	double xmin,ymin;
	double xmax,ymax;
	double xscale,yscale;
	double fimin,fimax,fistep;
	double tmin,tmax,tstep;
	Tfunctype functype;
	char funcFunc[MAX_GRFUNCS][MAX_FUNCNAME+1];
	char funcPol[MAX_GRFUNCS][MAX_FUNCNAME+1];
	char funcPar[MAX_GRFUNCS][2][MAX_FUNCNAME+1];
	Boolean logx,logy;
	Int16 speed;
	IndexedColorType colors[9];
	Boolean grEnable[9];
	Int8 grType[6];
}TgrPrefs;

/* Screen preferences in pixels */
/* Please note, the ymax is de facto smaller then ymin on the screen */
typedef struct {
        /* The Y screen max is the smallest value */
        /*0,0   1,0                                  */
        /*0,1                                        */
        /*0,2[beginning of graph                     */

        Coord xmin,ymax;
        Coord xmax,ymin;
        /* Cached coeficients for graph2screen computations */
        /* CacheTrans MUST be called when any of the ScrPrefs
         * and graphPrefs attributes is changed */
        double xtrans,ytrans;
        /* cached logaritms, it is quite slow to calculate */
        double logxtrans,logytrans;
        double logxmin,logymin;
}TscrPrefs;

Boolean GraphPrefsHandleEvent(EventPtr event) IFACE;
void grpref_init(void) IFACE;
void grpref_close(void) IFACE;

extern TgrPrefs graphPrefs;
extern TscrPrefs ScrPrefs;
extern IndexedColorType graphColors[];

CError set_axis(Functype *func,CodeStack *stack) GRAPH;

#endif
