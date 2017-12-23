/*
 *   $Id: grtaps.h,v 1.1 2011/02/28 22:08:16 mapibid Exp $
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

#ifndef _GRTAPS_H_
#define _GRTAPS_H_

typedef enum {
	track_reset,
	track_add,
	track_set,
	track_redraw
}TtrackAction;

typedef enum {
	align_left,
	align_center,
	align_right
} Ttxtalign;

Boolean grtaps_tap(Int16 x,Int16 y) GRAPH;
Boolean grtaps_track_but(void) GRAPH;
void grtaps_print_val(char *name, double value,
                      Coord x, Coord y, Coord xmin, Coord xmax,
                      Ttxtalign align, WinDrawOperation mode) GRAPH;
void grtaps_track_manual(double value,TtrackAction action) GRAPH;
void grtaps_track_pause(void) GRAPH;
void grtaps_track_start(void) GRAPH;
void grtaps_track_reset(double start) GRAPH;
void grtaps_track_track(Int16 track) GRAPH;
double grtaps_track_last_value(void) GRAPH;
Int16 grtaps_trackSelected(void) GRAPH;

#endif
