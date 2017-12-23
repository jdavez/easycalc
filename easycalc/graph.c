/*
 *   $Id: graph.c,v 1.63 2007/12/19 13:49:13 cluny Exp $
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
 *  2001-09-31 - John Hodapp <bigshot@email.msn.com>
 *		 Added code for display of trig mode and its selection
 *		 on the Graph screen.
 *
 *  2003-05-19 - Arno Welzel <arno.welzel@gmx.net>
 *		 Added code to handle Sony Clie specific output using
 *               Sony the HR lib
*/


#include <PalmOS.h>
#ifdef SONY_SDK
#include <SonyCLIE.h>
#endif

#include "clie.h"
#include "defuns.h"
#include "MathLib.h"
#include "konvert.h"
#include "funcs.h"
#include "calcDB.h"
#include "calcrsc.h"
#include "prefs.h"
#include "calc.h"
#include "mathem.h"
#include "graph.h"
#include "grprefs.h"
#include "stack.h"
#include "grsetup.h"
#include "grtaps.h"
#include "varmgr.h"
#include "grcalc.h"
#include "main.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

static struct {
	Boolean active;
	Int32 position;
	Int16 funcnum;
	double start;
	double stop;
	double step;
}graphState;

TscrPrefs ScrPrefs;

Tgraph_curves graphCurves[MAX_GRFUNCS];

static RectangleType stdbounds; // bounds in standard coordinates

/***********************************************************************
 *
 * FUNCTION:     graph_setcolor
 * 
 * DESCRIPTION:  Sets foreground color to the color of funcnum and
 *               background to white
 *
 * PARAMETERS:   funcnum - (-1) - sets foreground color to black
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
graph_setcolor(Int16 funcnum)
{
	if (palmOS35)
		WinPushDrawState();

	if (grayDisplay || colorDisplay) {
		if (funcnum == -1) {
			WinSetBackColor(graphPrefs.colors[8]); /* background */
			WinSetForeColor(graphPrefs.colors[6]); /* axis */
        	}
		else if (funcnum == -2)
			WinSetForeColor(graphPrefs.colors[7]); /* grid */
		else
			WinSetForeColor(graphPrefs.colors[funcnum]); /* graph */
	}
}

/***********************************************************************
 *
 * FUNCTION:     graph_unsetcolor
 * 
 * DESCRIPTION:  Restores color settings after graph_setcolor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
graph_unsetcolor(void)
{
	if (palmOS35)
		WinPopDrawState();
}

/***********************************************************************
 *
 * FUNCTION:     graph_is_onscreen
 * 
 * DESCRIPTION:  If the coordinates fit on the screen
 *
 * PARAMETERS:   x,y - coordinates of the original function
 *
 * RETURN:       true - fits, false - outside
 *      
 ***********************************************************************/
static Boolean graph_is_onscreen(double x,double y) GRAPH;
static Boolean
graph_is_onscreen(double x,double y)
{
	if (x>=graphPrefs.xmin && x<=graphPrefs.xmax &&
		y>=graphPrefs.ymin && y<=graphPrefs.ymax)
	  return true;
	return false;
}

/***********************************************************************
 *
 * FUNCTION:     graph_xgr2scr
 * 
 * DESCRIPTION:  Convert the 'x' or 'y' coordinate from the function to
 *               screen coordinates
 * 
 * PARAMETERS:   x or y
 *
 * RETURN:       coordinate
 *      
 ***********************************************************************/
Coord 
graph_xgr2scr(double x)
{
	if (graphPrefs.logx) {
		x = log10(x) - ScrPrefs.logxmin;
		x *= ScrPrefs.logxtrans;

		return ScrPrefs.xmin + (Coord) round(x * ScrPrefs.xtrans);
	} 
	return ScrPrefs.xmin+(Coord)round((x-graphPrefs.xmin)*ScrPrefs.xtrans);
}

Coord 
graph_ygr2scr(double y)
{
	if (graphPrefs.logy) {
		y = log10(y) - ScrPrefs.logymin;
		y *= ScrPrefs.logytrans;
		y += graphPrefs.ymin;
	}
	return ScrPrefs.ymin-(Coord)round((y-graphPrefs.ymin)*ScrPrefs.ytrans);
}

/***********************************************************************
 *
 * FUNCTION:     graph_xscr2gr
 * 
 * DESCRIPTION:  Convert a screen 'x' (or y) coordinate to a graph coordinate
 *
 * PARAMETERS:   x or y
 *
 * RETURN:       coordinate
 *      
 ***********************************************************************/
double 
graph_xscr2gr(Coord x)
{
	double res;
	res =graphPrefs.xmin+(double)(x-ScrPrefs.xmin)/ScrPrefs.xtrans;

	if (graphPrefs.logx) {
		res -= graphPrefs.xmin;
		res /= ScrPrefs.logxtrans;
		res += ScrPrefs.logxmin;
		res = pow(10.0,res);
	}
	return res;
}

double 
graph_yscr2gr(Coord y)
{
	double res;
	res = graphPrefs.ymin+(double)(ScrPrefs.ymin-y)/ScrPrefs.ytrans;

	if (graphPrefs.logy) {
		res -= graphPrefs.ymin;
		res /= ScrPrefs.logytrans;
		res += ScrPrefs.logymin;
		res = pow(10.0,res);
	}
	return res;
}

/***********************************************************************
 *
 * FUNCTION:     graph_isec_y
 * 
 * DESCRIPTION:  Find an intersection of a line with a vertical line 
 *               - it is used when drawing lines to points outside of
 *               the drawing area
 *
 * PARAMETERS:   x1,y1 - point 1
 *               x2,y2 - point 2
 *               x3 - the vertical line
 * RETURN:       y3 - the point on the vertical line
 *      
 ***********************************************************************/
static double
graph_isec_y(double x1,double y1,double x2,double y2,double x3) GRAPH;
static double
graph_isec_y(double x1,double y1,double x2,double y2,double x3)
{
	double sidea,sideb,parta;

	sidea = y2-y1;
	sideb = x2-x1;
	parta = x2 - x3;

	return y2-(sidea*parta)/sideb;
}

#define graph_isec_x(x1,y1,x2,y2,y3) (graph_isec_y(y1,x1,y2,x2,y3))

/***********************************************************************
 *
 * FUNCTION:     graph_outscreen
 * 
 * DESCRIPTION:  Find a point, where a line should be drawn instead of
 *               the point outside of drawing area
 *
 * PARAMETERS:   onx,ony - points in the drawing area
 *               offx,offy - points outside drawing area
 *
 * RETURN:       newx,newy - new points on the border of drawing area
 *      
 ***********************************************************************/
static void
graph_outscreen(double *newx,double *newy,
		double onx,double ony,double offx,double offy) GRAPH;
static void
graph_outscreen(double *newx,double *newy,
		double onx,double ony,double offx,double offy)
{
	double tmp;

	tmp = graph_isec_x(onx,ony,offx,offy,graphPrefs.ymin);
	if (offy<graphPrefs.ymin && tmp>=graphPrefs.xmin && tmp<=graphPrefs.xmax) {
		*newx = tmp;
		*newy = graphPrefs.ymin;
		return;
	}
	tmp = graph_isec_x(onx,ony,offx,offy,graphPrefs.ymax);
	if (offy>graphPrefs.ymax && tmp>=graphPrefs.xmin && tmp<=graphPrefs.xmax) {
		*newx = tmp;
		*newy = graphPrefs.ymax;
		return;
	} 

	tmp = graph_isec_y(onx,ony,offx,offy,graphPrefs.xmin);
	if (offx<graphPrefs.xmin && tmp>=graphPrefs.ymin && tmp<=graphPrefs.ymax) {
		*newx = graphPrefs.xmin;
		*newy = tmp;
		return;
	}
	tmp = graph_isec_y(onx,ony,offx,offy,graphPrefs.xmax);
	if (offx>graphPrefs.xmax && tmp>=graphPrefs.ymin && tmp<=graphPrefs.ymax) {
		*newx = graphPrefs.xmax;
		*newy = tmp;
		return;
	} 

	tmp = (graphPrefs.xmax - graphPrefs.xmin)/(ScrPrefs.xmax-ScrPrefs.xmin);
      if(offx>onx)
		*newx=onx+tmp;
	else
		*newx=onx-tmp;

	tmp = (graphPrefs.ymax - graphPrefs.ymin)/(ScrPrefs.ymin-ScrPrefs.ymax);
      if(offy>ony)
		*newy=ony+tmp;
	else
		*newy=ony-tmp;

	if(isinf(offy)>0 && finite(offx))
		*newy=graphPrefs.ymax;
	else
	if(isinf(offy)<0 && finite(offx))
		*newy=graphPrefs.ymin;
	else
	if(isinf(offx)>0 && finite(offy))
		*newx=graphPrefs.xmax;
	else
	if(isinf(offx)<0 && finite(offy))
		*newx=graphPrefs.xmin;
	else{
		*newx = onx;
		*newy = ony;
	}
}

/***********************************************************************
 *
 * FUNCTION:     graph_new_point
 * 
 * DESCRIPTION:  Adds a new point to graph. Depending on the selected
 *               style this function chooses way of drawing, colors etc.
 *
 * PARAMETERS:   x,y - coordinates of a new point
 *               funcnum - function number, where the point belongs to
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void graph_new_point(double x,double y,Int16 funcnum) GRAPH;
static void
graph_new_point(double x,double y,Int16 funcnum)
{
	Boolean ison=graph_is_onscreen(x,y);

	switch (graphPrefs.grType[funcnum]) {

	  case 0:
		if(ison){
		  Coord xx=graph_xgr2scr(x);
		  Coord yy=graph_ygr2scr(y);
		  clie_drawline(xx,yy-1,xx,yy+1);
		  clie_drawline(xx-1,yy,xx+1,yy);
		}
		break;

	  case 2:
		if(!isnan(y))
		{
		   Coord yy,xx;
		   Coord y0=graph_ygr2scr(0.0);

		   if(x>graphPrefs.xmax || x<graphPrefs.xmin)
			break;	/* polar, parametric or DRAW_AT_ONCE !=1 */

		   if(y>graphPrefs.ymax){
			if(graphPrefs.ymax<0) break;
			if(graphPrefs.ymin>0) y0=ScrPrefs.ymin;
			else y0--;
			yy=ScrPrefs.ymax;
		   }
		   else if(y<graphPrefs.ymin){
			if(graphPrefs.ymin>0) break;
			if(graphPrefs.ymax<0) y0=ScrPrefs.ymax;
			else y0++;
			yy=ScrPrefs.ymin;
		   }
		   else{
			yy=graph_ygr2scr(y);
			if(graphPrefs.ymax<0) y0=ScrPrefs.ymax;
			else if(graphPrefs.ymin>0) y0=ScrPrefs.ymin;
			else y0-=sgn(y);
		   }

		   xx=graph_xgr2scr(x);
		   clie_drawline(xx,y0,xx,yy);
		}
		break;

	  default:
	    {
		double oldx = graphCurves[funcnum].x;
		double oldy = graphCurves[funcnum].y;
		
		if (!graphCurves[funcnum].valid && ison) {
			clie_drawline(graph_xgr2scr(x),graph_ygr2scr(y),
					  graph_xgr2scr(x),graph_ygr2scr(y));
		}	
		else if (graphCurves[funcnum].valid) {
			if (ison && graph_is_onscreen(oldx,oldy)) { 
				clie_drawline(graph_xgr2scr(oldx),graph_ygr2scr(oldy),
						  graph_xgr2scr(x),graph_ygr2scr(y));
			}
			else if (ison) { 
				/* First point on screen */
				double newx,newy;
				graph_outscreen(&newx,&newy,x,y,oldx,oldy);
				clie_drawline(graph_xgr2scr(x),graph_ygr2scr(y),
						  graph_xgr2scr(newx),graph_ygr2scr(newy));
			}
			else if (graph_is_onscreen(oldx,oldy)) { 
				/* Second point on screen */
				double newx,newy;
				graph_outscreen(&newx,&newy,oldx,oldy,x,y);
				clie_drawline(graph_xgr2scr(oldx),graph_ygr2scr(oldy),
						  graph_xgr2scr(newx),graph_ygr2scr(newy));
			}
		}
		break;
	    }

	}
	graphCurves[funcnum].x = x;
	graphCurves[funcnum].y = y;
	graphCurves[funcnum].valid = true;
}
/* Initialize cache variables to
 * avoid this time-expensive computation in the future */		
static void cacheTrans(void) GRAPH;
static void
cacheTrans(void)
{		
	ScrPrefs.xtrans=(double)(ScrPrefs.xmax-ScrPrefs.xmin)/(graphPrefs.xmax-graphPrefs.xmin);
	ScrPrefs.ytrans=(double)(ScrPrefs.ymin-ScrPrefs.ymax)/(graphPrefs.ymax-graphPrefs.ymin);
	if (graphPrefs.logx) {
	   ScrPrefs.logxtrans = (graphPrefs.xmax - graphPrefs.xmin) \
		/ (log10(graphPrefs.xmax) - log10(graphPrefs.xmin));
	   ScrPrefs.logxmin = log10(graphPrefs.xmin);
	}
	if (graphPrefs.logy) {
	   ScrPrefs.logytrans = (graphPrefs.ymax - graphPrefs.ymin) \
		/ (log10(graphPrefs.ymax) - log10(graphPrefs.ymin));
	   ScrPrefs.logymin = log10(graphPrefs.ymin);
	}
}

static void graph_grid(RectangleType *bounds) GRAPH;
static void 
graph_grid(RectangleType *bounds)
{
	double tmpx,tmpy,ystart,xscale,yscale;
	Coord x,y;

	xscale = graphPrefs.xscale;
	yscale = graphPrefs.yscale;

	if (xscale == 0.0 || yscale == 0.0)
		return;

	if ((graphPrefs.ymax-graphPrefs.ymin)/yscale > (ScrPrefs.ymin-ScrPrefs.ymax)/3
	    || (graphPrefs.xmax-graphPrefs.xmin)/xscale > (ScrPrefs.xmax-ScrPrefs.xmin)/3)
		return;
							 
	graph_setcolor(-2);

	tmpy = graph_yscr2gr(bounds->topLeft.y + bounds->extent.y);
	ystart = ceil(tmpy/yscale)*yscale;
	tmpx = graph_xscr2gr(bounds->topLeft.x);
	tmpx = ceil(tmpx/xscale)*xscale;
	for (;tmpx <= graphPrefs.xmax;tmpx+=xscale) {
		x = graph_xgr2scr(tmpx);
		for (tmpy=ystart;tmpy <= graphPrefs.ymax;tmpy+=yscale) {
			y = graph_ygr2scr(tmpy);
			if (gHrMode != hrNone) {
				/* Draw crosses in hi res ... */
				clie_drawline(x,y-1,x,y+1);
				clie_drawline(x-1,y,x+1,y);
			}
			else
				/* otherwise just dots */
				clie_drawline(x,y,x,y);
		}
	}

	graph_unsetcolor();
}

#ifndef SUPPORT_DIA
#define HanderaCoord(x)	(x)
#endif

static void graph_draw_axes_labels(RectangleType *stdbounds) GRAPH;
static void
graph_draw_axes_labels(RectangleType *stdbounds)
{
	Coord x, y, xstd, ystd;

	x = graph_xgr2scr(0.0);
	y = graph_ygr2scr(0.0);

	if (gHrMode == hrPalm) {
		UInt16 save = WinSetCoordinateSystem(kCoordinatesNative);
		xstd = WinUnscaleCoord(x, true);
		ystd = WinUnscaleCoord(y, true);
		WinSetCoordinateSystem(save);
	}
	else {
		xstd = x / gSonyFactor;
		ystd = y / gSonyFactor;
	}
	if (graphPrefs.grEnable[6] && graphPrefs.grEnable[8]) { /* axes and labels enabled */
		Coord dy1 = HanderaCoord(1);
		Coord dy10 = HanderaCoord(10);
		Coord dy15 = HanderaCoord(15);
		Coord dy26 = HanderaCoord(26);

		grtaps_print_val("y>",graphPrefs.ymin,
		                 xstd + 1,
		                 stdbounds->topLeft.y + stdbounds->extent.y - dy10,
		                 stdbounds->topLeft.x,
		                 stdbounds->topLeft.x + stdbounds->extent.x,
		                 align_left, winOverlay);
		grtaps_print_val("y<",graphPrefs.ymax,
		                 xstd + 1,
		                 stdbounds->topLeft.y - dy1,
		                 stdbounds->topLeft.x,
		                 stdbounds->topLeft.x + stdbounds->extent.x,
		                 align_left, winOverlay);
		ystd -= dy1;
		if (ystd < stdbounds->topLeft.y + dy15)
			ystd = stdbounds->topLeft.y + dy15;
		if (ystd > stdbounds->topLeft.y + stdbounds->extent.y - dy26)
			ystd = stdbounds->topLeft.y + stdbounds->extent.y - dy26;
		grtaps_print_val("x>",graphPrefs.xmin,
		                 stdbounds->topLeft.x + dy1,
		                 ystd,
		                 stdbounds->topLeft.x,
		                 stdbounds->topLeft.x + stdbounds->extent.x,
		                 align_left, winOverlay);
		grtaps_print_val("x<",graphPrefs.xmax,
		                 stdbounds->topLeft.x + stdbounds->extent.x,
		                 ystd,
		                 stdbounds->topLeft.x,
		                 stdbounds->topLeft.x + stdbounds->extent.x,
		                 align_right, winOverlay);
	}
}

/* Initializes Graph gadget and shows the basic grid */
static void graph_init_screen(FormPtr frm,UInt16 graphId) GRAPH;
static void 
graph_init_screen(FormPtr frm,UInt16 graphId)
{
	RectangleType natbounds; // bounds in native coordinates
	Coord x, y;
#ifdef SUPPORT_DIA
	Coord dx, dy;
#endif

	/* Check, if the bounds are correct */
	gadget_bounds(frm, graphId, &natbounds, &stdbounds);
#ifdef SUPPORT_DIA
	/* Adjust in case of window resize */
	dx = natbounds.topLeft.x + natbounds.extent.x - 1 - ScrPrefs.xmax;
	dy = natbounds.topLeft.y + natbounds.extent.y - 1 - ScrPrefs.ymin;
	if (dx)
		graphPrefs.xmax += dx / ScrPrefs.xtrans;
	if (dy)
		graphPrefs.ymin -= dy / ScrPrefs.ytrans;
#endif
	if (graphPrefs.logx && graphPrefs.xmin <= 0.0) 
		graphPrefs.xmin = MIN_LOG_VAL;
	if (graphPrefs.logy && graphPrefs.ymin <= 0.0) 
		graphPrefs.ymin = MIN_LOG_VAL;

	ScrPrefs.xmin=natbounds.topLeft.x;
	ScrPrefs.ymax=natbounds.topLeft.y;
	ScrPrefs.xmax=ScrPrefs.xmin+natbounds.extent.x-1;
	ScrPrefs.ymin=ScrPrefs.ymax+natbounds.extent.y-1;
	cacheTrans();

	/* Draw the drawing area */
	clie_drawrectangleframe(1,&natbounds);
	graph_setcolor(-1);
	clie_eraserectangle(&natbounds,0);

	/* Draw axes */
	x = graph_xgr2scr(0.0);
	y = graph_ygr2scr(0.0);
	if (graphPrefs.grEnable[6]) { /* axes drawing enabled */
		/* y axis */
		if (x>ScrPrefs.xmin && x<ScrPrefs.xmax)
			clie_drawline(x,ScrPrefs.ymax,x,ScrPrefs.ymin);
		/* x axis */
		if (y>ScrPrefs.ymax && y<ScrPrefs.ymin)
			clie_drawline(ScrPrefs.xmin,y,ScrPrefs.xmax,y);
	}

	graph_unsetcolor();
	
	/*Draw grid */
	if (graphPrefs.grEnable[7])
		graph_grid(&natbounds);

	/* Draw axes labels */
	graph_draw_axes_labels(&stdbounds);
}

/***********************************************************************
 *
 * FUNCTION:     graph_get_vals
 *
 * DESCRIPTION:  Return the coordinates on the resulting graph (in graph
 *               coordinates) of the function given by funcnum and
 *               from the parameter 'param'. 
 *
 * PARAMETERS:   fnum - the function number
 *               param - parameter value
 *
 * RETURN:       true - the resulting 'x' and 'y' coordinates are valid
 *               false - an error occured - e.g. function not defined etc.
 *      
 ***********************************************************************/
Boolean
graph_get_vals(Int16 fnum,double param,double *x,double *y)
{
	if (graphPrefs.functype == graph_param) {
		if (!graphCurves[fnum].stack1 ||
			!graphCurves[fnum].stack2)
		  return false;
		func_get_value(graphCurves[fnum].stack1,param,x,NULL);
		func_get_value(graphCurves[fnum].stack2,param,y,NULL);
		return (finite(*x) && finite(*y));
	}
	else if (graphPrefs.functype == graph_func) {
		if (!graphCurves[fnum].stack1)
		  return false;
		*x = param;
		func_get_value(graphCurves[fnum].stack1,param,y,NULL);

		return finite(*y);
	}
	else { /* graph_pol */
		double r;
		
		if (!graphCurves[fnum].stack1)
			return false;
		func_get_value(graphCurves[fnum].stack1,
		               math_rad_to_user(param),&r,NULL);
		if (!finite(r))
			return false;
		*x = r*cos(param);
		*y = r*sin(param);
		
		return true;
	}
}

/***********************************************************************
 *
 * FUNCTION:     graph_zoom_out
 *
 * DESCRIPTION:  Zoom out by making a window of a same center but
 *               twice as high and twice as wide
 *
 * PARAMETERS:   frm, objectId - id of the graph gadget
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void graph_zoom_out(FormPtr frm,Int16 objectId) GRAPH;
static void
graph_zoom_out(FormPtr frm,Int16 objectId)
{
	double width = graphPrefs.xmax - graphPrefs.xmin;
	double height = graphPrefs.ymax - graphPrefs.ymin;
	
	graphPrefs.xmin-=width/2;
	graphPrefs.xmax+=width/2;
	
	graphPrefs.ymin-=height/2;
	graphPrefs.ymax+=height/2;
	
	FrmUpdateForm(frmGraph,frmUpdateVars);
}


/***********************************************************************
 *
 * FUNCTION:     graph_init_cache
 * 
 * DESCRIPTION:  Initializes the field of precompiled functions with
 *               NULLs
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void graph_init_cache(void) GRAPH;
static void
graph_init_cache(void)
{
	Int16 i;
	for (i=0;i<MAX_GRFUNCS;i++) {
		graphCurves[i].stack1 = NULL;
		graphCurves[i].stack2 = NULL;
	}	  
}

/***********************************************************************
 *
 * FUNCTION:     graph_clean_cache
 * 
 * DESCRIPTION:  Frees memory occupied by precompiled functions
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void graph_clean_cache(void) GRAPH;
static void
graph_clean_cache(void)
{
	Int16 i;
	for (i=0;i<MAX_GRFUNCS;i++) {
		if (graphCurves[i].stack1) {
			stack_delete(graphCurves[i].stack1);
			graphCurves[i].stack1 = NULL;
		}
			if (graphCurves[i].stack2) {
			stack_delete(graphCurves[i].stack2);
			graphCurves[i].stack2 = NULL;
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:     graph_compile_cache
 * 
 * DESCRIPTION:  Compiles functions that should be drawn and saves
 *               for later use
 *
 * PARAMETERS:   None
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void graph_compile_cache(void) GRAPH;
static void
graph_compile_cache(void)
{
	int i;
	char *name;
	CodeStack *stack;
	CError err;
	
	graph_clean_cache();
	if (graphPrefs.functype==graph_func ||
		graphPrefs.functype==graph_polar) {
		for (i=0;i<MAX_GRFUNCS;i++) {
			name = grsetup_get_fname(i);
			if (StrLen(name)==0)
			  continue;
			stack = db_read_function(name,&err);
			if (err)
			  continue;
			/* Lookup variables to speed up execution */
			stack_fix_variables(stack);
			graphCurves[i].stack1 = stack;
		}
	} else {
		/* Parametric */
		for (i=0;i<MAX_GRFUNCS;i++) {
			name  = grsetup_get_fname(i*2);
			if (StrLen(name)==0)
			  continue;
			stack = db_read_function(name,&err);
			if (err)
			  continue;
			/* Lookup variables to speed up execution */
			stack_fix_variables(stack);
			graphCurves[i].stack1 = stack;
			
			name  = grsetup_get_fname(i*2+1);
			if (StrLen(name)==0)
			  continue;
			stack = db_read_function(name,&err);
			if (err)
			  continue;
			/* Lookup variables to speed up execution */
			stack_fix_variables(stack);
			graphCurves[i].stack2 = stack;
		}
	}		
}

/***********************************************************************
 *
 * FUNCTION:     graph_draw_start
 * 
 * DESCRIPTION:  Initialize the drawing procedure, that later occurs 
 *               using nilEvent in background
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void graph_draw_start(void) GRAPH;
static void
graph_draw_start(void)
{
	int i;
		
	for (i=0;i<MAX_GRFUNCS;i++) 
	  graphCurves[i].valid = false;
	
	graph_compile_cache();
	
	graph_init_screen(FrmGetActiveForm(),graphGadget);
	
	graphState.active = true;
	graphState.position = 0;
	graphState.funcnum = 0;
	if (graphPrefs.functype == graph_func) {
		graphState.start = graphPrefs.xmin;
		graphState.stop = graphPrefs.xmax;
		graphState.step = (graphPrefs.xmax - graphPrefs.xmin)/(ScrPrefs.xmax-ScrPrefs.xmin);
	} 
	else if (graphPrefs.functype == graph_polar) {
		graphState.start = graphPrefs.fimin;
		graphState.stop = graphPrefs.fimax;
		graphState.step = graphPrefs.fistep;
	}
	else if (graphPrefs.functype == graph_param) {
		graphState.start = graphPrefs.tmin;
		graphState.stop = graphPrefs.tmax;
		graphState.step = graphPrefs.tstep;
	}
	calc_nil_timeout(0);
}

/***********************************************************************
 *
 * FUNCTION:     graph_draw_stop, graph_draw_resum
 * 
 * DESCRIPTION:  Stop/resume drawing functions
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void graph_draw_resume() GRAPH;
static void
graph_draw_resume()
{
	graphState.active = true;
	calc_nil_timeout(0);
}

static void graph_draw_stop() GRAPH;
static void
graph_draw_stop()
{
	if (graphState.active)
	  calc_nil_timeout(evtWaitForever);
	graphState.active = false;
}

/***********************************************************************
 *
 * FUNCTION:     graph_draw
 * 
 * DESCRIPTION:  Draws a piece of a function, called from nilEvent
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
static void graph_draw(void) GRAPH;
static void
graph_draw(void)
{
	Int32 i;
	double start;
	double x,y;

	if (!graphState.active)
	  return;
	if (graphState.funcnum>=MAX_GRFUNCS) {
		graph_draw_stop();
		return;
	}

	/* Check, if we have all functions to draw graph */
	if (!graphPrefs.grEnable[graphState.funcnum] || !graphCurves[graphState.funcnum].stack1 ||
		(graphPrefs.functype==graph_param && !graphCurves[graphState.funcnum].stack2)) {
		graphState.funcnum++;
		return;
	}

	graph_setcolor(graphState.funcnum);
	/* Should we draw all points at once, or step slowly to the end? */

	if (graphPrefs.functype == graph_func) {
		for (i=0;i<DRAW_AT_ONCE;i+=graphPrefs.speed) {
			graph_get_vals(graphState.funcnum,
				       graph_xscr2gr(i+ScrPrefs.xmin+graphState.position),
				       &x,&y);
			graph_new_point(x,y,graphState.funcnum);
		}
		graphState.position += i;
		if (graphState.position > ScrPrefs.xmax) {
			/* Curve finished */
			/* Redraw labels. They may have been overwritten */
			graph_draw_axes_labels(&stdbounds);
			graphState.position = 0;
			graphState.funcnum++;
		}
	} else { /* Parametric and polar */
		start = graphState.start;
		for (i=0;i < DRAW_AT_ONCE && start <= graphState.stop;
		     i+=graphPrefs.speed) {
			start = graphState.start+graphState.step*(i+graphState.position);
			if (start > graphState.stop) {
			    /* Draw slightly behind end on polar graphs
			     * to compensate for rounding */
			    if (graphPrefs.functype == graph_polar)
				start -= graphState.step/2;
			    else
				break; /* Do not draw behind end on parametric */
			}

			graph_get_vals(graphState.funcnum,
				       start,
				       &x,&y);
			graph_new_point(x,y,graphState.funcnum);
		}
		graphState.position+=i;
		if (start >= graphState.stop) {
			/* Reached end of drawing area/parameters */
			/* Redraw labels. They may have been overwritten */
			graph_draw_axes_labels(&stdbounds);
			graphState.position = 0;
			graphState.funcnum++;
		}
	}
	graph_unsetcolor();
}

static void graph_select_speed(void) GRAPH;
static void
graph_select_speed(void)
{
	Int16 i,speed;
	switch (graphPrefs.speed) {
	case 1:	i = 0; break;
	case 2:	i = 1; break;
	case 4:	i = 2; break;
	case 7:
	default:i = 3; break;
	}
	LstSetSelection(GetObjectPtr(lstGraphSpeed),i);
	i = LstPopupList(GetObjectPtr(lstGraphSpeed));

	if (i != noListSelection) {
		switch (i) {
		case 0:	speed = 1;break;
		case 1:	speed = 2;break;
		case 2: speed = 4;break;
		case 3: 
		default: speed = 7;break;
		}
		if (speed < graphPrefs.speed)
			FrmUpdateForm(frmGraph,frmRedrawUpdateCode);
		graphPrefs.speed = speed;
	}
}

Boolean 
GraphFormHandleEvent(EventPtr event)
{
	Boolean    handled = false;
	FormPtr    frm=FrmGetActiveForm();
	Int16 controlId;
	double position;
	static Boolean oldreduceprecision;
	Tbase trig;
#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	case winEnterEvent:
		/* We still recieve nilEvent when in the menu, disable the
		 * nilEvent when in Menu */
		if (event->data.winEnter.enterWindow == 
			(WinHandle) FrmGetFormPtr(frmGraph))
		  graph_draw_resume();
		else 
		  graph_draw_stop();
		break;
	case nilEvent:
		graph_draw();
		handled = true;
		break;
	case penDownEvent:		  
		handled=grtaps_tap(event->screenX,event->screenY);
		break;
	case ctlSelectEvent:
		controlId=event->data.ctlSelect.controlID;
		  if (chooseForm(controlId)) {
			  handled=true;
			  break;
		  }
		handled=true;
		switch (controlId) {
		case btnPrefMode:
			trig=calcPrefs.trigo_mode;	
			switch (trig) {
			case 1:
				trig = 0;
				break;
			case 2:
				trig = 1;
				break;
			case 0:
				trig = 2;
				break;
			}
			calcPrefs.trigo_mode = trig;
			CtlSetLabel(GetObjectPtr(btnPrefMode),trigmode[trig].trigtext);
			FrmUpdateForm(frmGraph,frmUpdateVars);
			break;
		case btnGraphSpeed:
			graph_select_speed();
			break;
		case btnGraphCalc:
			/*if(graphState.funcnum>=MAX_GRFUNCS)	//avoid tracking while drawing */
				grcalc_control();
			break;
		case btnGraphZoomOut:
			graph_zoom_out(frm,graphGadget);
			break;
		case btnGraphPref:
			FrmPopupForm(grPrefForm);
			break;
		case btnGraphSetup:
			FrmPopupForm(grSetupForm);
			break;
		case btnGraphTrack:
			/*if(graphState.funcnum<MAX_GRFUNCS){	//avoid tracking while drawing
				grtaps_track_pause();
				break;
			}*/
			if (CtlGetValue(GetObjectPtr(btnGraphTrack))) {
				if (!grtaps_track_but()) 
					grtaps_track_pause();
				else 
					grtaps_track_start();
			} else {
				/* User unchecked the button */
				grtaps_track_manual(grtaps_track_last_value(),track_redraw);
				grtaps_track_pause();
			}
			break;
		case btnGraphTrackGoto:
			position = grtaps_track_last_value();
			if (graphPrefs.functype == graph_polar)
				position = math_rad_to_user(position);
			if (varmgr_get_double(&position,"Jump to position")) {
				grtaps_track_manual(position,track_set);
			}
			handled = true;
			break;
		default:
			handled=false;
		}
		break;
	 case frmUpdateEvent:
		if (event->data.frmUpdate.updateCode == frmRedrawUpdateCode) {
			FrmDrawForm(FrmGetActiveForm());
			graph_draw_start();
			if (CtlGetValue(GetObjectPtr(btnGraphTrack)))
				grtaps_track_pause();
			/* Cancel the numeric input, if there was any*/
			grcalc_init();
			handled = true;
		}
		if (event->data.frmUpdate.updateCode == frmUpdateVars) {
			trig = calcPrefs.trigo_mode;
			CtlSetLabel(GetObjectPtr(btnPrefMode),
				    trigmode[trig].trigtext);
			graph_draw_start();
			grtaps_track_reset(graphState.start);
			/* Cancel the numeric input, if there was any*/
			grcalc_init();
			handled = true;
		}
		break;
	case keyDownEvent:
		handled = true;
		if (CtlGetValue(GetObjectPtr(btnGraphTrack)) &&
		    !(event->data.keyDown.modifiers & poweredOnKeyMask)) {
			double step = graphState.step;

			if (graphPrefs.functype == graph_polar)
				step = math_rad_to_user(step);

			switch (event->data.keyDown.chr) {
			case vchrHard3:
				grtaps_track_manual(step*5,track_add);
				break;
			case vchrPageUp:
				grtaps_track_manual(step,track_add);
				break;
			case vchrPageDown:
				grtaps_track_manual(-step,track_add);
				break;
			case vchrHard2:
				grtaps_track_manual(-step*5,track_add);
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
			case ',':
			case '-':
				/* Automatically popup a 'Goto' window when
				 * number pressed 
				 */
				if (CtlGetValue(GetObjectPtr(btnGraphTrack))) {
					EvtAddEventToQueue(event);
					if (varmgr_get_double(&position,"Jump to position"))
						grtaps_track_manual(position,track_set);
				}
				break;
			default:
				handled = false;
				break;
			}
		}
		break;
	case frmCloseEvent:
		graph_draw_stop();
		graph_clean_cache();
		calcPrefs.reducePrecision = oldreduceprecision;
		handled = false;
		break;
	case frmOpenEvent:
		oldreduceprecision = calcPrefs.reducePrecision;
		calcPrefs.reducePrecision = false;

		calcPrefs.form=FrmGetActiveFormID();
		FrmDrawForm(frm);
		
		CtlSetValue(GetObjectPtr(btnGraph),1);
		trig = calcPrefs.trigo_mode;
		CtlSetLabel(GetObjectPtr(btnPrefMode),trigmode[trig].trigtext);
		graph_init_cache();
		graph_draw_start();
		
		grtaps_track_reset(graphState.start);
		grcalc_init();
				
		handled=true;
		break;
	 case menuEvent:
		if (chooseForm(event->data.menu.itemID)) {
			handled=true;
			break;
		  }
		handled = true;
		switch (event->data.menu.itemID) {
		 case btnGraphPref:
			FrmPopupForm(grPrefForm);
			break;
		 case btnGraphTable:
			FrmPopupForm(grTableForm);
			break;
		 case btnGraphSetup:
			FrmPopupForm(grSetupForm);
			break;
		 case tdBPref:
			FrmPopupForm(prefForm);
			break;
		 default:
			handled = false;
		}
		break;
	}
	  
	return handled;
}

