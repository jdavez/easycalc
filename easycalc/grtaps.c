/*
 *   $Id: grtaps.c,v 1.34 2007/12/19 13:49:13 cluny Exp $
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
*/

#include <PalmOS.h>
#ifdef SONY_SDK
#include <SonyCLIE.h>
#endif
#ifdef SUPPORT_DIA
#include "DIA.h"
#else
#define HanderaCoord( x )	( x )
#define HanderaAdjustFont( font )	( font )
#endif

#include "clie.h"
#include "MathLib.h"
#include "grtaps.h"
#include "calcrsc.h"
#include "grprefs.h"
#include "graph.h"
#include "calc.h"
#include "mathem.h"
#include "display.h"
#include "fp.h"
#include "grsetup.h"
#include "funcs.h"

#define BOLDRES	1

static Int16 trackSelected = 0;
static double lastParam = 0.0;
static Coord crossX = -1, crossY = -1;
static double oldr = NaN;
static double oldangle = NaN;
static double oldparam = NaN;
static double oldrealx = NaN;
static double oldrealy = NaN;
//static const TdispPrefs grNumPrefs = { 5, true, disp_normal, disp_decimal, false, false};

/***********************************************************************
 *
 * FUNCTION:     EvtGetPenNat
 *
 * DESCRIPTION:  As EvtGetPen, but returns native screen coordinates.
 *
 * PARAMETERS:   Same as EvtGetPen().
 *
 * RETURN:       Nothing.
 *
 ***********************************************************************/
static void
EvtGetPenNat(Coord *x, Coord *y, Boolean *penDown)
{
	EvtGetPen(x, y, penDown);
	if (gHrMode == hrPalm) {
		UInt16 save = WinSetCoordinateSystem(kCoordinatesNative);
		*x = WinScaleCoord(*x, true);
		*y = WinScaleCoord(*y, true);
		WinSetCoordinateSystem(save);
	}
	else {
		*x *= gSonyFactor;
		*y *= gSonyFactor;
	}
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_rectangle
 * 
 * DESCRIPTION:  Draws a rectangle for use with 'Zoom in' operation
 *
 * PARAMETERS:   corners of the rectangle
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
grtaps_rectangle(Int16 x1,Int16 y1,Int16 x2,Int16 y2) GRAPH;
static void
grtaps_rectangle(Int16 x1,Int16 y1,Int16 x2,Int16 y2)
{
	if (x1<0 || x2<0 || y1<0 || y2<0)
	  return;
	
	clie_invertline(x1,y1,x2,y1);
	clie_invertline(x1,y2,x2,y2);
	clie_invertline(x1,y1,x1,y2);
	clie_invertline(x2,y1,x2,y2);
	clie_invertline(x1,y1,x2,y2);
	clie_invertline(x2,y1,x1,y2);
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_clear_edges
 * 
 * DESCRIPTION:  Used when moving screen, clears the edges, that should 
 *               be left empty, when the contents of the screen is moved
 *
 * PARAMETERS:   bounds - bounds of draw area
 *               mx,my - coordinates, where the contents is moved
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
grtaps_clear_edges(Coord mx,Coord my,RectangleType *bounds) GRAPH;
static void
grtaps_clear_edges(Coord mx,Coord my,RectangleType *bounds)
{
	RectangleType tmp,tmp2;
	
	tmp = *bounds;
	tmp.topLeft.x+=mx;
	tmp.topLeft.y+=my;
	clie_cliprectangle(&tmp);

	tmp2 = *bounds;
	tmp2.extent.x = tmp.topLeft.x - bounds->topLeft.x;
	clie_eraserectangle(&tmp2,0);

	tmp2 = *bounds;
	tmp2.extent.y = tmp.topLeft.y - bounds->topLeft.y;
	clie_eraserectangle(&tmp2,0);
	
	tmp2 = *bounds;
	tmp2.topLeft.x = tmp.topLeft.x+tmp.extent.x;
	clie_eraserectangle(&tmp2,0);
	
	tmp2 = *bounds;
	tmp2.topLeft.y = tmp.topLeft.y+tmp.extent.y;
	clie_eraserectangle(&tmp2,0); 
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_move
 * 
 * DESCRIPTION:  Started to fetch pen moves and move the drawing screen
 *               contents accordingly
 *
 * PARAMETERS:   begx,begy - the original user tap
 *               bounds - bounds of the drawing area
 *
 * RETURN:       true/false - if the user ended within the bounds
 *               and the screen should be moved
 *      
 ***********************************************************************/
static Boolean
grtaps_move(Coord begx,Coord begy,RectangleType *bounds) GRAPH;
static Boolean
grtaps_move(Coord begx,Coord begy,RectangleType *bounds)
{
	Boolean penDown,moved;
	Coord x,y,dx,dy;
	Coord oldx=begx,oldy=begy;
	WinHandle orgwindow;
	UInt16 err;
	RectangleType orgbounds,rs;
	double xmin,ymin;
	
	orgwindow = clie_createoffscreenwindow(bounds->extent.x,bounds->extent.y,
										 nativeFormat,&err);
	/* Note: orgwindow may be NULL if there is insufficient memory for the
       offscreen window. Then we need to copy inside the screen window */
	if (orgwindow)
		clie_copyrectangle(NULL,orgwindow,bounds,0,0,winPaint);
	clie_getclip(&rs);
	clie_setclip(bounds);
	orgbounds = *bounds;
	if (orgwindow)
		orgbounds.topLeft.x = orgbounds.topLeft.y = 0;
	
	graph_setcolor(-1);
	do {
		EvtGetPenNat(&x, &y, &penDown);
		if ((oldx!=x || oldy!=y) && RctPtInRectangle(x,y,bounds)) {
			dx = (orgwindow ? x-begx : x-oldx);
			dy = (orgwindow ? y-begy : y-oldy);
			clie_copyrectangle(orgwindow,NULL,&orgbounds,
							 bounds->topLeft.x + dx,
							 bounds->topLeft.y + dy,winPaint);
			grtaps_clear_edges(dx,dy,bounds);
			oldx=x;oldy=y;
		}
	} while(penDown);
	graph_unsetcolor();

	xmin = graph_xscr2gr((bounds->topLeft.x)-(x-begx));
	ymin = graph_yscr2gr((bounds->topLeft.y+bounds->extent.y-1)-(y-begy));
	if (!RctPtInRectangle(x,y,bounds)
	    || (graphPrefs.logx && xmin <= 0.0)
	    || (graphPrefs.logy && ymin <= 0.0)) {
		/* Restore original plot from offscreen window if possible,
		   otherwise force a redraw by setting moved to true */
		if (orgwindow) {
			clie_copyrectangle(orgwindow,NULL,&orgbounds,bounds->topLeft.x,
				 bounds->topLeft.y,winPaint);
			moved = false;
		}
		else {
			moved = true;
		}
	}
	else {
		graphPrefs.xmax=graph_xscr2gr((bounds->topLeft.x+bounds->extent.x-1)-(x-begx));
		graphPrefs.ymax=graph_yscr2gr((bounds->topLeft.y)-(y-begy));
		graphPrefs.xmin = xmin;
		graphPrefs.ymin = ymin;
		moved = true;
	}
	
	clie_setclip(&rs);
	if (orgwindow)
		WinDeleteWindow(orgwindow,false);
	
	return moved;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_zoom_in
 * 
 * DESCRIPTION:  Fetch pen moves, draw a rectangle on the screen
 *               according to pen moves and zoom in, if the user
 *               lift the pen within bounds
 *
 * PARAMETERS:   begx,begy - first tap, one of the corners
 *               bounds - bounds of drawing area
 *
 * RETURN:       true - zoom ok
 *               false - did not zoom
 *      
 ***********************************************************************/
static Boolean
grtaps_zoom_in(Coord begx,Coord begy,RectangleType *bounds) GRAPH;
static Boolean
grtaps_zoom_in(Coord begx,Coord begy,RectangleType *bounds)
{
	Boolean penDown;
	Coord x,y;
	Coord oldx=-1,oldy=-1;
	double tmp;
	
	do {
		EvtGetPenNat(&x, &y, &penDown);
		if ((oldx != x || oldy != y) && RctPtInRectangle(x, y, bounds)) {
			grtaps_rectangle(begx, begy, x, y);
			grtaps_rectangle(begx, begy, oldx, oldy);
			oldx = x; oldy = y;
		}
	} while(penDown);
	/* Remove the rectangle from screen */
	grtaps_rectangle(begx, begy, oldx, oldy);
	if (!RctPtInRectangle(x,y,bounds)) {
		/* User wanted to cancel the zoom */
		return false;
	}
	
	if (oldx==begx || oldy==begy)
		return false;
	
	if (begx > oldx) {
		tmp = begx; begx = oldx; oldx = tmp;
	}

	/* The Y axis is inversed on the screen */
	if (begy < oldy) {
		tmp = begy; begy = oldy; oldy = tmp;
	}       	
	/* begx - Left corner of the box, oldx - right */
	/* begy - bottom corner, oldy - top corner */
	
	graphPrefs.xmax=graph_xscr2gr(oldx);
	graphPrefs.xmin=graph_xscr2gr(begx);
	graphPrefs.ymax=graph_yscr2gr(oldy);
	graphPrefs.ymin=graph_yscr2gr(begy);

	return true;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_but
 * 
 * DESCRIPTION:  The handler for the 'T' button, popups a list of 
 *               functions, that can be tracked
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - user selected the func, tracking should begin
 *               false - user canceled operation
 *      
 ***********************************************************************/
Boolean
grtaps_track_but(void)
{
	ListPtr list;
	char *descr[MAX_GRFUNCS];
	Int16 nums[MAX_GRFUNCS];
	Int16 count,i;

	count = grsetup_fn_descr_arr(descr,nums);
	if (count==0)
		return false;
	list = GetObjectPtr(graphTrackList);
	LstSetListChoices(list,descr,count);
	LstSetHeight(list,count);

	for (i=0;i<count;i++)
		if (nums[i] == trackSelected)
			break;
	if (i == count)
		i = noListSelection;
	LstSetSelection(list,i);

	i = LstPopupList(list);
	if (i == noListSelection)
	  return false;
	trackSelected = nums[i];
	
	return true;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_cross
 * 
 * DESCRIPTION:  Draw a tracking cross on the screen
 *
 * PARAMETERS:   x,y - coordinates of the center of cross
 *               bounds - bounds of drawing area
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
grtaps_cross(Int16 x,Int16 y,RectangleType *bounds) GRAPH;
static void
grtaps_cross(Int16 x,Int16 y,RectangleType *bounds)
{
	if (x != -1) 
		clie_invertline(x, bounds->topLeft.y,
		                x, bounds->topLeft.y + bounds->extent.y - 1);
	if (y != -1) 
		clie_invertline(bounds->topLeft.x, y,
		                bounds->topLeft.x + bounds->extent.x - 1, y);	
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_print_val
 * 
 * DESCRIPTION:  Print a specified 'double' on the location, clear
 *               the location before printing
 *
 * PARAMETERS:   name - text, that should precede the number
 *               value - number to be drawn
 *               x,y - coordinates
 *               xmin,xmax - keep text between xmin and xmax
 *               align - alignment w.r.t. coordinates
 *               mode - drawing mode
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
grtaps_print_val(char *name, double value, Coord x, Coord y, Coord xmin, Coord xmax,
                 Ttxtalign align, WinDrawOperation mode)
{
	char text[MAX_FP_NUMBER+10];
	Char *numtxt;
	Coord txtwidth;
	WinDrawOperation oldmode;

#ifdef SUPPORT_DIA
	FntSetFont(HanderaAdjustFont(FntGetFont()));
#endif
	
	StrCopy(text,name);
	numtxt = text + StrLen(text);
	//if (finite(value)) {
		fp_print_g_double(numtxt, value, 4);
		txtwidth = FntCharsWidth(text, StrLen(text));
		if (align == align_center) {
			x -= txtwidth / 2;
		}
		else if (align == align_right) {
			x -= txtwidth;
		}
		if (x < xmin)
			x = xmin;
		if ((x + txtwidth) > xmax)
			x = xmax - txtwidth;
		oldmode = WinSetDrawMode(mode);
		WinPaintChars(text, StrLen(text), x, y);
		WinSetDrawMode(oldmode);
	//}
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_print_pol, grtaps_print_param, grtaps_print_coords
 * 
 * DESCRIPTION:  Prints r & angle or x & y respectively on
 *               given places of the screen
 *
 * PARAMETERS:   r,angle,x,y - values
 *               bounds - bounds of drawing area
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
grtaps_print_pol(double r, double angle,RectangleType *bounds) GRAPH;
static void
grtaps_print_pol(double r, double angle,RectangleType *bounds)
{
	Coord dx1 = HanderaCoord(1);
	Coord dy = HanderaCoord(11);

	if (graphPrefs.grEnable[8])
		dy += HanderaCoord(7);
	grtaps_print_val("r:", r, bounds->topLeft.x + dx1,
	                 bounds->topLeft.y + bounds->extent.y - dy,
	                 bounds->topLeft.x,
	                 bounds->topLeft.x + bounds->extent.x,
	                 align_left, winInvert);
	
	grtaps_print_val("fi:", angle, bounds->topLeft.x + bounds->extent.x,
	                 bounds->topLeft.y + bounds->extent.y - dy,
	                 bounds->topLeft.x,  
	                 bounds->topLeft.x + bounds->extent.x, 
	                 align_right, winInvert);
}

static void
grtaps_print_param(double t,RectangleType *bounds) GRAPH;
static void
grtaps_print_param(double t,RectangleType *bounds)
{
	Coord dx1 = HanderaCoord(1), dy = HanderaCoord(11);

	if (graphPrefs.grEnable[8])
		dy += HanderaCoord(7);
	grtaps_print_val("T:", t, bounds->topLeft.x + dx1,
	                 bounds->topLeft.y + bounds->extent.y - dy,
	                 bounds->topLeft.x,  
	                 bounds->topLeft.x + bounds->extent.x, 
	                 align_left, winInvert);
}

static void
grtaps_print_coords(double realx, double realy,RectangleType *bounds) GRAPH;
static void
grtaps_print_coords(double realx, double realy,RectangleType *bounds)
{
	Coord dx1 = HanderaCoord(1);
	Coord dy7 = HanderaCoord(7);
	Coord y;

	y = bounds->topLeft.y;
	if (graphPrefs.grEnable[8])
		y += dy7;
	grtaps_print_val("x:", realx, bounds->topLeft.x + dx1, y,
	                 bounds->topLeft.x,  
	                 bounds->topLeft.x + bounds->extent.x, 
	                 align_left, winInvert);

	grtaps_print_val("y:", realy, bounds->topLeft.x + bounds->extent.x, y,
	                 bounds->topLeft.x,  
	                 bounds->topLeft.x + bounds->extent.x, 
	                 align_right, winInvert);
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_manual
 *
 * DESCRIPTION:  manually (not by pen-tap) set the tracking cross
 *
 * PARAMETERS:   value - value to which set the cross
 *               action - track_reset - the screen was redrawn,
 *                                      reset the coordinates so
 *                                      the next track will not clear
 *                                      the lastly drawn cross
 *                        track_set - set a cross to a value and
 *                                    draw it
 *                        track_add - moves a cross by value
 *                        track_redraw - redraws cross and labels after
 *                                       screen resize
 * 
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
grtaps_track_manual(double value, TtrackAction action)
{
	double realx, realy, r;
	RectangleType natbounds;
	RectangleType stdbounds;

	if (graphPrefs.functype == graph_polar) {
		value = math_user_to_rad(value);
	}

	if (action == track_reset) {
		lastParam = value;
		crossX = crossY = -1;
		oldr = oldangle = oldparam = oldrealx = oldrealy = NaN;
		return;
	}
	else {
		FontID oldfont;
#ifdef SONY_SDK
		if (gHrMode == hrSony)
			oldfont = HRFntSetFont(gHrLibRefNum, hrStdFont+BOLDRES);
		else
#endif
			oldfont = FntSetFont(HanderaAdjustFont(stdFont+BOLDRES));
		gadget_bounds(FrmGetActiveForm(), graphGadget, &natbounds, &stdbounds);
		
		if (action == track_set)
			lastParam = value;
		else if (action == track_add)
			lastParam += value;

		if (action != track_redraw) {
			/* Erase previously drawn cross + text */
			grtaps_cross(crossX, crossY, &natbounds);
			grtaps_print_coords(oldrealx, oldrealy, &stdbounds);
			if (graphPrefs.functype == graph_param)
				grtaps_print_param(oldparam, &stdbounds);
			else if (graphPrefs.functype == graph_polar)
				grtaps_print_pol(oldr, oldangle, &stdbounds);
		}

		graph_get_vals(trackSelected, lastParam, &realx, &realy);
		crossX = finite(realx) ? graph_xgr2scr(realx) : -1;
		crossY = finite(realy) ? graph_ygr2scr(realy) : -1;
		if (crossX < natbounds.topLeft.x 
		    || crossX > natbounds.topLeft.x + natbounds.extent.x)
			crossX = -1;
		if (crossY < natbounds.topLeft.y 
		    || crossY > natbounds.topLeft.y + natbounds.extent.y)
			crossY = -1;
		grtaps_cross(crossX, crossY, &natbounds);
		grtaps_print_coords(realx, realy, &stdbounds);
		oldrealx = realx; oldrealy = realy;
		if (graphPrefs.functype == graph_param) {
			grtaps_print_param(lastParam, &stdbounds);
			oldparam = lastParam;
		}
		else if (graphPrefs.functype == graph_polar) {
			// func_get_value(graphCurves[trackSelected].stack1,
			//                lastParam,&r,NULL);
			r = sqrt(realx * realx + realy * realy);
			grtaps_print_pol(r, math_rad_to_user(lastParam), &stdbounds);
			oldr = r;
			oldangle = math_rad_to_user(lastParam);
		}
#ifdef SONY_SDK
		if(gHrMode == hrSony) 
			HRFntSetFont(gHrLibRefNum, oldfont);
		else 
#endif
			FntSetFont(oldfont);
	}
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_pol
 * 
 * DESCRIPTION:  Tracking of a polar graph
 *
 * PARAMETERS:   x,y - first tap
 *               bounds
 *
 * RETURN:       true - user lifted pen within bounds
 *               false - outside bounds
 *      
 ***********************************************************************/
static Boolean
grtaps_track_pol(Coord x, Coord y,
                 RectangleType *natbounds, RectangleType *stdbounds) GRAPH;
static Boolean
grtaps_track_pol(Coord x, Coord y,
                 RectangleType *natbounds, RectangleType *stdbounds)
{
	double realx, realy;
	double r;	
	double angle_rad; /* angle in radians */
	double angle_usr; /* angle in user units: rad/deg/grad */
	Coord oldx, oldy, dx, dy;
	CodeStack *stack;
	Boolean penDown;	

	stack = graphCurves[trackSelected].stack1;
	if (!stack)
		return false;

	oldx = oldy = -1;
	realx = realy = r = angle_rad = angle_usr = NaN;
	do {
		EvtGetPenNat(&x, &y, &penDown);
		if ((oldx != x || oldy != y) && RctPtInRectangle(x, y, natbounds)) {
			realx = graph_xscr2gr(x);
			realy = graph_yscr2gr(y);
			angle_rad = angle_usr = NaN;
			r = hypot(realx, realy);
			if (r != 0.0) {
				if (realx >= 0.0 && realy >= 0.0)
					angle_rad = asin(realy / r);
				else if (realx <= 0.0)
					angle_rad = M_PIl - asin(realy / r);
				else
					angle_rad = 2 * M_PIl + asin(realy / r);
				angle_usr = math_rad_to_user(angle_rad);

				func_get_value(stack, angle_usr, &r, NULL);

				realx = r * cos(angle_rad);
				realy = r * sin(angle_rad);
				dx = graph_xgr2scr(realx);
				dy = graph_ygr2scr(realy);
				if (!finite(angle_rad) || !RctPtInRectangle(dx, dy, natbounds)) {
					dy = -1;				
					dx = -1;
				}				  
			}
			else {
				dx = dy = -1;
				realx = realy = NaN;
			}
			grtaps_cross(dx, dy, natbounds);
			grtaps_cross(crossX, crossY, natbounds);
			grtaps_print_coords(oldrealx, oldrealy, stdbounds);
			grtaps_print_coords(realx, realy, stdbounds);
			grtaps_print_pol(oldr, oldangle, stdbounds);
			grtaps_print_pol(r, angle_usr, stdbounds);
			oldx = x; oldy = y;
			crossX = dx; crossY = dy;
			oldrealx = realx; oldrealy = realy;
			oldr = r; oldangle = angle_usr;
		}		
	} while(penDown);	   	
	
	if (RctPtInRectangle(x, y, natbounds)) {
		lastParam = angle_rad;
		return true;
	}
	else
		return false;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_param
 *
 * DESCRIPTION:  Tracking of a parametric graph
 *
 * PARAMETERS:   x,y - first tap
 *               bounds
 *
 * RETURN:       true - user lifted pen within bounds
 *               false - outside bounds
 *
 ***********************************************************************/
static Boolean
grtaps_track_param(Coord x, Coord y,
                   RectangleType *natbounds, RectangleType *stdbounds) GRAPH;
static Boolean
grtaps_track_param(Coord x, Coord y,
                   RectangleType *natbounds, RectangleType *stdbounds)
{
	double realx, realy;
	double param;
	Coord oldx, oldy, dx, dy;
	CodeStack *stack1, *stack2;
	double x2param;
	Boolean penDown;

	stack1 = graphCurves[trackSelected].stack1;
	stack2 = graphCurves[trackSelected].stack2;
	if (!stack1 || !stack2)
		return false;

	oldx = oldy = -1;
	realx = realy = param = NaN;
	x2param = (graphPrefs.tmax - graphPrefs.tmin) / (double)natbounds->extent.x;
	do {
		EvtGetPenNat(&x, &y, &penDown);
		if (oldx != x && RctPtInRectangle(x, y, natbounds)) {
			param = (double)(x - natbounds->topLeft.x) * x2param;
			param = graphPrefs.tmin
			        + round(param / graphPrefs.tstep) * graphPrefs.tstep;
			if (finite(param)) {
				func_get_value(stack1, param, &realx, NULL);
				func_get_value(stack2, param, &realy, NULL);
				dx = graph_xgr2scr(realx);
				dy = graph_ygr2scr(realy);
				if (dx < natbounds->topLeft.x ||
					dx > natbounds->topLeft.x + natbounds->extent.x)
					dx = -1;
				if (dy < natbounds->topLeft.y ||
					dy > natbounds->topLeft.y + natbounds->extent.y)
					dy = -1;
			}
			else {
				dx = dy = -1;
				realx = realy = NaN;
			}
			grtaps_cross(dx, dy, natbounds);
			grtaps_cross(crossX, crossY, natbounds);
			grtaps_print_coords(oldrealx, oldrealy, stdbounds);
			grtaps_print_coords(realx, realy, stdbounds);
			grtaps_print_param(oldparam, stdbounds);
			grtaps_print_param(param, stdbounds);
			oldx = x; oldy = y;
			crossX = dx; crossY = dy;
			oldrealx = realx; oldrealy = realy;
			oldparam = param;
		}
	} while (penDown);

	if (RctPtInRectangle(x, y, natbounds)) {
		lastParam = param;
		return true;
	}
	else
		return false;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_func
 * 
 * DESCRIPTION:  Tracking of a normal function
 *
 * PARAMETERS:   x,y - first tap
 *               bounds
 *
 * RETURN:       true/false - user lifted inside/outside of bounds
 *      
 ***********************************************************************/
static Boolean
grtaps_track_func(Coord x, Coord y,
                  RectangleType *natbounds, RectangleType *stdbounds) GRAPH;
static Boolean
grtaps_track_func(Coord x,Coord y,
                  RectangleType *natbounds, RectangleType *stdbounds)
{
	double realx, realy;
	CodeStack *stack;
	Boolean penDown;	

	stack = graphCurves[trackSelected].stack1;
	if (!stack)
		return false;

	realx = realy = NaN;
	do {
		EvtGetPenNat(&x, &y, &penDown);
		if (crossX != x && RctPtInRectangle(x, y, natbounds)) {
			realx = graph_xscr2gr(x);
			func_get_value(stack, realx, &realy, NULL);
			y = graph_ygr2scr(realy);
			if (!finite(realy) || !RctPtInRectangle(x, y, natbounds))
				y = -1;
			grtaps_cross(x, y, natbounds);
			grtaps_cross(crossX, crossY, natbounds);
			grtaps_print_coords(oldrealx, oldrealy, stdbounds);
			grtaps_print_coords(realx, realy, stdbounds);
			crossX = x; crossY = y;
			oldrealx = realx; oldrealy = realy;
		}
	} while(penDown);	   	
	
	if (RctPtInRectangle(x, y, natbounds)) {
		lastParam = realx;
		return true;
	}
	else
		return false;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_tap
 * 
 * DESCRIPTION:  Generic handler started when user taps on the drawing 
 *               gadget. 
 *
 * PARAMETERS:   x,y - coords, where user tapped
 *
 * RETURN:       true - handled, user tapped inside gadget
 *               false - should be handled by somone else
 *      
 ***********************************************************************/
Boolean
grtaps_tap(Int16 x,Int16 y)
{	
	RectangleType natbounds;
	RectangleType stdbounds;

	if (gHrMode == hrPalm) {
		UInt16 save = WinSetCoordinateSystem(kCoordinatesNative);
		x = WinScaleCoord(x, true);
		y = WinScaleCoord(y, true);
		WinSetCoordinateSystem(save);
	}
	else {
		x *= gSonyFactor; y *= gSonyFactor;
	}

	gadget_bounds(FrmGetActiveForm(), graphGadget, &natbounds, &stdbounds);

	if (RctPtInRectangle(x, y, &natbounds)) {
		/* Is zoom-in checked? */
		if (CtlGetValue(GetObjectPtr(btnGraphZoomIn))) {
			if (grtaps_zoom_in(x, y, &natbounds)) {
				CtlSetValue(GetObjectPtr(btnGraphZoomIn), 0);
				FrmUpdateForm(frmGraph, frmUpdateVars);
			}			
		}	
		/* Is tracking checked? */
		else if (CtlGetValue(GetObjectPtr(btnGraphTrack))) {
			FontID oldfont;
#ifdef SONY_SDK
			if (gHrMode == hrSony)
				oldfont = HRFntSetFont(gHrLibRefNum, hrStdFont+BOLDRES);
			else
#endif
				oldfont = FntSetFont(HanderaAdjustFont(stdFont+BOLDRES));
			if (graphPrefs.functype == graph_func)
				grtaps_track_func(x, y, &natbounds, &stdbounds);
			else if (graphPrefs.functype == graph_polar)
				grtaps_track_pol(x, y, &natbounds, &stdbounds);
			else if (graphPrefs.functype == graph_param)
				grtaps_track_param(x, y, &natbounds, &stdbounds);
#ifdef SONY_SDK
			if(gHrMode == hrSony) 
				HRFntSetFont(gHrLibRefNum, oldfont);
			else 
#endif
				FntSetFont(oldfont);
		}
		else /* Move the graph */
		  if (grtaps_move(x, y, &natbounds)) 
			FrmUpdateForm(frmGraph, frmUpdateVars);
		return true;
	}
	return false;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_start
 * 
 * DESCRIPTION:  Start tracking a function, show a cross
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void 
grtaps_track_start(void)
{
	FormPtr frm = FrmGetActiveForm();
	
	grtaps_track_manual(0, track_redraw);
	FrmShowObject(frm, FrmGetObjectIndex(frm, btnGraphTrackGoto));
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_pause
 * 
 * DESCRIPTION:  Pause tracking, although the cross is still displayed
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
grtaps_track_pause(void)
{
	FormPtr frm = FrmGetActiveForm();
	
	CtlSetValue(GetObjectPtr(btnGraphTrack), false);
	FrmHideObject(frm, FrmGetObjectIndex(frm, btnGraphTrackGoto));
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_reset
 * 
 * DESCRIPTION:  There was probably a forced screen redraw, reset internal 
 *               tracking variables and stop tracking
 *
 * PARAMETERS:   start - where should the tracking start (where should
 *                       the cross appear when grtaps_track_start() is
 *                       called)
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
grtaps_track_reset(double start)
{
	FormPtr frm = FrmGetActiveForm();

	grtaps_track_manual(start, track_reset);
	CtlSetValue(GetObjectPtr(btnGraphTrack), false);
	FrmHideObject(frm, FrmGetObjectIndex(frm, btnGraphTrackGoto));
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_track
 * 
 * DESCRIPTION:  Start tracking of a particular function
 *
 * PARAMETERS:   track - index to graphCurves table
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
grtaps_track_track(Int16 track)
{
	trackSelected = track;
	CtlSetValue(GetObjectPtr(btnGraphTrack), true);
	grtaps_track_start();
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_last_value
 * 
 * DESCRIPTION:  Return the last position of a cross
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       The parameter value on the last cross position
 *      
 ***********************************************************************/
double
grtaps_track_last_value(void)
{
	return lastParam;
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_trackSelected
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       The index of the function selected for tracking
 *      
 ***********************************************************************/
Int16
grtaps_trackSelected(void)
{
	return trackSelected;
}
