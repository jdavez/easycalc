/*   
 *   $Id: graph.h,v 1.9 2007/12/18 09:06:37 cluny Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999 Ondrej Palkovsky
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
#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "grprefs.h"

#define DRAW_AT_ONCE   5

typedef struct {
	double x,y;
	Boolean valid;
	CodeStack *stack1,*stack2;
}Tgraph_curves;

Boolean GraphFormHandleEvent(EventPtr event) GRAPH;
double graph_xscr2gr(Coord x) GRAPH;
double graph_yscr2gr(Coord y) GRAPH;
Coord graph_xgr2scr(double x) GRAPH;
Coord graph_ygr2scr(double y) GRAPH;
Boolean graph_get_vals(Int16 fnum,double param,double *x,double *y) GRAPH;
void graph_setcolor(Int16 funcnum) GRAPH;
void graph_unsetcolor(void) GRAPH;

extern Tgraph_curves graphCurves[MAX_GRFUNCS];
#endif
