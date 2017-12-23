/*
 *   $Id: graph.cpp,v 1.1 2011/02/28 22:07:18 mapibid Exp $
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
 *       Added code for display of trig mode and its selection
 *       on the Graph screen.
 *
 *  2003-05-19 - Arno Welzel <arno.welzel@gmx.net>
 *       Added code to handle Sony Clie specific output using
 *               Sony the HR lib
*/

#include "stdafx.h"
#define _GRAPH_CPP_

#include "compat/PalmOS.h"

//#include "clie.h"
#include "core/mlib/defuns.h"
#include "compat/MathLib.h"
#include "core/mlib/konvert.h"
#include "core/mlib/funcs.h"
#include "core/mlib/calcDB.h"
//#include "calcrsc.h"
#include "core/prefs.h"
#include "core/calc.h"
#include "core/mlib/mathem.h"
#include "core/mlib/stack.h"
#include "core/varmgr.h"
//#include "main.h"
#include "core/grsetup.h"
#include "core/Graph.h"
#include "system - UI/Skin.h"
//#include "grcalc.h"

//#ifdef SUPPORT_DIA
//#include "DIA.h"
//#endif

static struct {
    Boolean active;
    Boolean complete;
    int position;
    int funcnum;
    double start;
    double stop;
    double step;
} graphState;

Graph zoneGraph;
Graph viewGraph;

Tgraph_curves graphCurves[MAX_GRFUNCS];

TCHAR *lstGraphCalc[] = {
    _T("$$GRZERO"),
    _T("$$GRVALUE"),
    _T("$$GRMIN"),
    _T("$$GRMAX"),
    _T("$$GRDDX"),
    _T("d2/dx"),
    _T("$$GRINTEG"),
    _T("$$GRINTERSECT"),
    _T("dr/dt"),
    _T("dy/dx"),
    _T("dy/dt"),
    _T("dx/dt")
};

t_grFuncType grcFunc[GRCFUNC_SZ] = {cp_zero, cp_value, cp_min, cp_max,
                          cp_dydx1, cp_dydx2, cp_integ, cp_intersect};
t_grFuncType grcPolar[GRCPOLAR_SZ] = {cp_zero, cp_value, cp_min, cp_max, cp_odrdfi};
t_grFuncType grcParam[GRCPARAM_SZ] = {cp_pdydx, cp_pdydt, cp_pdxdt};

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
void graph_init_cache (void) {
    Int16 i;
    for (i=0 ; i<MAX_GRFUNCS ; i++) {
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
void graph_clean_cache (void) {
    Int16 i;
    for (i=0 ; i<MAX_GRFUNCS ; i++) {
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
static void graph_compile_cache (void) {
    int        i;
    TCHAR     *name;
    CodeStack *stack;
    CError     err;

    graph_clean_cache();
    if ((graphPrefs.functype == graph_func) ||
        (graphPrefs.functype == graph_polar)) {
        for (i=0 ; i<MAX_GRFUNCS ; i++) {
            name = grsetup_get_fname(i);
            if (StrLen(name) == 0)
                continue;
            stack = db_read_function(name, &err);
            if (err)
                continue;
            /* Lookup variables to speed up execution */
            stack_fix_variables(stack);
            graphCurves[i].stack1 = stack;
        }
    } else {
        /* Parametric */
        for (i=0 ; i<MAX_GRFUNCS ; i++) {
            name = grsetup_get_fname(i*2);
            if (StrLen(name) == 0)
                continue;
            stack = db_read_function(name, &err);
            if (err)
                continue;
            /* Lookup variables to speed up execution */
            stack_fix_variables(stack);
            graphCurves[i].stack1 = stack;

            name  = grsetup_get_fname(i*2 + 1);
            if (StrLen(name) == 0)
                continue;
            stack = db_read_function(name, &err);
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
 * FUNCTION:     graph_draw_stop, graph_draw_resume
 *
 * DESCRIPTION:  Stop/resume drawing functions
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
void graph_draw_resume (void) {
    graphState.active = true;
}

void graph_draw_stop (void) {
    graphState.active = false;
}

void graph_draw_complete (void) {
    graphState.active = false;
    graphState.complete = true;
}

bool is_graph_active (void) {
    return (graphState.active);
}

bool is_graph_complete (void) {
    return (graphState.complete);
}

/***********************************************************************
 *
 * FUNCTION:     graph_get_zoneOnView
 *
 * DESCRIPTION:  Calculate coordinates of the zone in view reference.
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
void graph_get_zoneOnView (long *top, long *left, long *bottom, long *right) {
    // Assumes being called only after view graph has been drawn on screen
    *top = viewGraph.graph_ygr2scr(graphPrefs.ymax);
    *left = viewGraph.graph_xgr2scr(graphPrefs.xmin);
    *bottom = viewGraph.graph_ygr2scr(graphPrefs.ymin);
    *right = viewGraph.graph_xgr2scr(graphPrefs.xmax);
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
Boolean Graph::graph_is_onscreen (double x, double y) {
    if ((x >= xmin) && (x <= xmax)
        && (y >= ymin) && (y <= ymax))
        return (true);
    return (false);
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
Coord Graph::graph_xgr2scr (double x) {
    if (graphPrefs.logx) {
        x = (log10(x) - ScrPrefs.logxmin) / ScrPrefs.logxtrans;
        return ((Coord) (round(x)));
    }
    return ((Coord) (round((x - xmin) / ScrPrefs.xtrans)));
}

Coord Graph::graph_ygr2scr (double y) {
    if (graphPrefs.logy) {
        y = (log10(y) - ScrPrefs.logymin) / ScrPrefs.logytrans;
        return ((Coord) (round(y)));
    }
    return ((Coord) (round((y - ymin) / ScrPrefs.ytrans)));
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
double Graph::graph_xscr2gr (Coord scr_x) {
    double res;

    if (graphPrefs.logx) {
        res = ScrPrefs.logxmin + ((double) scr_x) * ScrPrefs.logxtrans;
        res = pow(10.0, res);
    } else
        res = xmin + ((double) scr_x) * ScrPrefs.xtrans;

    return (res);
}

double Graph::graph_yscr2gr (Coord scr_y) {
    double res;

    if (graphPrefs.logy) {
        res = ScrPrefs.logymin + ((double) scr_y) * ScrPrefs.logytrans;
        res = pow(10.0, res);
    } else
        res = ymin + ((double) scr_y) * ScrPrefs.ytrans;
    return (res);
}

/***********************************************************************
 *
 * FUNCTION:     graph_isec_y
 *
 * DESCRIPTION:  Find an intersection of a line with a vertical line
 *               - it is used when drawing lines towards a point outside of
 *               the drawing area
 *
 * PARAMETERS:   x1,y1 - point 1
 *               x2,y2 - point 2
 *               x3 - the vertical line
 * RETURN:       y3 - the point on the vertical line
 *
 ***********************************************************************/
double Graph::graph_isec_y (double x1, double y1, double x2, double y2, double x3) {
    double sidea, sideb, parta;

    sidea = y2 - y1;
    sideb = x2 - x1;
    parta = x2 - x3;

    return (y2 - (sidea * parta) / sideb);
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
void Graph::graph_outscreen (double *newx, double *newy,
                             double onx, double ony, double offx, double offy) {
    double tmp;

    tmp = graph_isec_x(onx, ony, offx, offy, ymin);
    if ((offy < ymin) && (tmp >= xmin) && (tmp <= xmax)) {
        *newx = tmp;
        *newy = ymin;
        return;
    }
    tmp = graph_isec_x(onx, ony, offx, offy, ymax);
    if ((offy > ymax) && (tmp >= xmin) && (tmp <= xmax)) {
        *newx = tmp;
        *newy = ymax;
        return;
    }

    tmp = graph_isec_y(onx, ony, offx, offy, xmin);
    if ((offx < xmin) && (tmp >= ymin) && (tmp <= ymax)) {
        *newx = xmin;
        *newy = tmp;
        return;
    }
    tmp = graph_isec_y(onx, ony, offx, offy, xmax);
    if ((offx > xmax) && (tmp >= ymin) && (tmp <= ymax)) {
        *newx = xmax;
        *newy = tmp;
        return;
    }

    if (offx > onx)
        *newx = onx + ScrPrefs.xtrans;
    else
        *newx = onx - ScrPrefs.xtrans;

    if (offy > ony)
        *newy = ony + ScrPrefs.ytrans;
    else
        *newy = ony - ScrPrefs.ytrans;

    if((isinf(offy) > 0) && finite(offx))
        *newy = ymax;
    else if ((isinf(offy) < 0) && finite(offx))
        *newy = ymin;
    else if ((isinf(offx) > 0) && finite(offy))
        *newx = xmax;
    else if ((isinf(offx) < 0) && finite(offy))
        *newx = xmin;
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
void Graph::graph_new_point (Skin *skin, Coord scr_x, double x, double y, Int16 funcnum) {
    Coord scr_y;
    Boolean ison = graph_is_onscreen(x, y);

    switch (graphPrefs.grType[funcnum]) {
        case 0: // Dots
            if (ison) {
                if (scr_x < 0)   scr_x = graph_xgr2scr(x);
                scr_y = graph_ygr2scr(y);
                skin->drawline(scr_x, scr_y-1, scr_x, scr_y+1);
                skin->drawline(scr_x-1, scr_y, scr_x+1, scr_y);
            }
            break;

        case 2: // Surface
            if (!isnan(y)) {
                Coord scr_y0;

                if ((x > xmax) || (x < xmin))
                    break;  /* polar, parametric or DRAW_AT_ONCE !=1 */

                if (y > ymax){
                    if (ymax < 0)
                        break;
                    if (ymin > 0)
                        scr_y0 = 0;
                    else   scr_y0 = ScrPrefs.y0 + 1;
                    scr_y = ScrPrefs.ymax;
                } else if (y < ymin) {
                    if (ymin > 0)
                        break;
                    if (ymax < 0)
                        scr_y0 = ScrPrefs.ymax;
                    else   scr_y0 = ScrPrefs.y0 - 1;
                    scr_y = 0;
                } else {
                    scr_y = graph_ygr2scr(y);
                    if (ymax < 0)
                        scr_y0 = ScrPrefs.ymax;
                    else if (ymin > 0)
                        scr_y0 = 0;
                    else   scr_y0 = ScrPrefs.y0 + (int) (sgn(y));
                }

                if (scr_x < 0)   scr_x = graph_xgr2scr(x);
                scr_y = graph_ygr2scr(y);
                skin->drawline(scr_x, scr_y0, scr_x, scr_y);
            }
            break;

        default: // Line
            {
            double oldx = graphCurves[funcnum].x;
            double oldy = graphCurves[funcnum].y;

            if (!graphCurves[funcnum].valid) {
                if (ison) {
                    if (scr_x < 0)   scr_x = graph_xgr2scr(x);
                    scr_y = graph_ygr2scr(y);
                    skin->drawline(scr_x, scr_y, scr_x, scr_y);
                }
                graphCurves[funcnum].valid = true;
            } else if (graphCurves[funcnum].valid) {
                if (ison) {
                    if (graphCurves[funcnum].ison) {
                        if (scr_x < 0)   scr_x = graph_xgr2scr(x);
                        scr_y = graph_ygr2scr(y);
                        skin->drawline(graphCurves[funcnum].scr_x, graphCurves[funcnum].scr_y,
                                       scr_x, scr_y);
                    } else {
                        /* Only second point on screen */
                        double newx, newy;
                        graph_outscreen(&newx, &newy, x, y, oldx, oldy);
                        if (scr_x < 0)   scr_x = graph_xgr2scr(x);
                        scr_y = graph_ygr2scr(y);
                        skin->drawline(graph_xgr2scr(newx), graph_ygr2scr(newy),
                                       scr_x, scr_y);
                    }
                } else if (graphCurves[funcnum].ison) {
                    /* Only first point on screen */
                    double newx, newy;
                    graph_outscreen(&newx, &newy, oldx, oldy, x, y);
                    scr_x = graph_xgr2scr(newx);
                    scr_y = graph_ygr2scr(newy);
                    skin->drawline(graphCurves[funcnum].scr_x, graphCurves[funcnum].scr_y,
                                   scr_x, scr_y);
                    // We have a discontinuity, do not forget to draw last point
                    skin->finishline(graphPrefs.colors[funcnum]);
                }
            }
            break;
        }
    }

    // Keep track of current point as last point drawn
    graphCurves[funcnum].x = x;
    graphCurves[funcnum].y = y;
    graphCurves[funcnum].scr_x = scr_x;
    graphCurves[funcnum].scr_y = scr_y;
    graphCurves[funcnum].ison = ison;
}

/* Initialize cache variables to
 * avoid this time-expensive computation in the future */
void Graph::cacheTrans (void) {
    ScrPrefs.xtrans = (xmax - xmin) / ((double) (ScrPrefs.width - 1));
    ScrPrefs.ytrans = (ymax - ymin) / ((double) (ScrPrefs.height - 1));
    if (graphPrefs.logx) {
        ScrPrefs.logxtrans = (log10(xmax) - log10(xmin))
                             / ((double) (ScrPrefs.width));
        ScrPrefs.logxmin = log10(xmin);
    }
    if (graphPrefs.logy) {
        ScrPrefs.logytrans = (log10(ymax) - log10(ymin))
                             / ((double) (ScrPrefs.height));
       ScrPrefs.logymin = log10(ymin);
    }
}

//static void graph_grid (RectangleType *bounds) {
//    double tmpx, tmpy, ystart, xscale, yscale;
//    Coord  x, y;
//
//    xscale = xscale;
//    yscale = yscale;
//
//    if ((xscale == 0.0) || (yscale == 0.0))
//        return;
//
//    if ((((ymax - ymin) / yscale) > ((ScrPrefs.ymin - ScrPrefs.ymax) / 3))
//        || (((xmax - xmin) / xscale) > ((ScrPrefs.xmax - ScrPrefs.xmin) / 3)))
//        return;
//
//    graph_setcolor(-2);
//
//    tmpy = graph_yscr2gr(bounds->topLeft.y + bounds->extent.y);
//    ystart = ceil(tmpy / yscale) * yscale;
//    tmpx = graph_xscr2gr(bounds->topLeft.x);
//    tmpx = ceil(tmpx / xscale) * xscale;
//    for ( ; tmpx<=xmax ; tmpx+=xscale) {
//        x = graph_xgr2scr(tmpx);
//        for (tmpy=ystart ; tmpy<=ymax ; tmpy+=yscale) {
//            y = graph_ygr2scr(tmpy);
//            if (gHrMode != hrNone) {
//                /* Draw crosses in hi res ... */
//                skin->drawline(x, y-1, x, y+1);
//                skin->drawline(x-1, y, x+1, y);
//            } else
//                /* otherwise just dots */
//                skin->drawline(x, y, x, y);
//        }
//    }
//
//    graph_unsetcolor();
//}

//#ifndef SUPPORT_DIA
//#define HanderaCoord(x) (x)
//#endif

/* Initializes Graph gadget and shows the basic grid */
void Graph::graph_init_screen (Skin *skin, int width, int height) {
    if (graphPrefs.logx && (xmin <= 0.0))
        xmin = MIN_LOG_VAL;
    if (graphPrefs.logy && (ymin <= 0.0))
        ymin = MIN_LOG_VAL;

//    ScrPrefs.xmin = 0;
//    ScrPrefs.ymin = 0;
    ScrPrefs.xmax = (ScrPrefs.width = width) - 1;
    ScrPrefs.ymax = (ScrPrefs.height = height) - 1;
    cacheTrans();

    /* Draw axes */
    int x = graph_xgr2scr(0.0);
    int y = ScrPrefs.y0 = graph_ygr2scr(0.0);
    if (graphPrefs.grEnable[graphPrefs.functype][6]) { /* axes drawing enabled */
        skin->selPen(6);
        /* y axis */
        if ((x >= 0) && (x <= ScrPrefs.xmax)) {
            skin->drawline(x, 0, x, ScrPrefs.ymax);
            skin->finishline(graphPrefs.colors[6]);
        }
        /* x axis */
        if ((y >= 0) && (y <= ScrPrefs.ymax)) {
            skin->drawline(0, y, ScrPrefs.xmax, y);
            skin->finishline(graphPrefs.colors[6]);
        }
    }
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
Boolean Graph::graph_get_vals (int fnum, double param, double *x, double *y, double *r) {
    if (graphPrefs.functype == graph_param) {
        if (!graphCurves[fnum].stack1 ||
            !graphCurves[fnum].stack2)
            return (false);
        func_get_value(graphCurves[fnum].stack1, param, y, NULL);
        func_get_value(graphCurves[fnum].stack2, param, x, NULL);
        return (finite(*x) && finite(*y));
    } else if (graphPrefs.functype == graph_func) {
        if (!graphCurves[fnum].stack1)
            return (false);
        *x = param;
        func_get_value(graphCurves[fnum].stack1, param, y, NULL);

        return (finite(*y) != 0);
    } else { /* graph_pol */
        double radius;

        if (!graphCurves[fnum].stack1)
            return (false);
        func_get_value(graphCurves[fnum].stack1, math_rad_to_user(param), &radius, NULL);
        if (r != NULL)
            *r = radius;
        if (!finite(radius))
            return (false);
        *x = radius * cos(param);
        *y = radius * sin(param);

        return (true);
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
//void Graph::graph_zoom_out (void) {
//    double width = xmax - xmin;
//    double height = ymax - ymin;
//
//    xmin -= width / 2;
//    xmax += width / 2;
//
//    ymin -= height / 2;
//    ymax += height / 2;
//}


/***********************************************************************
 *
 * FUNCTION:     graph_draw_start
 *
 * DESCRIPTION:  Initialize the drawing procedure, that later occurs
 *               in background (using nilEvent in the Palm version,
 *               and a separate thread in other versions).
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
void Graph::graph_draw_start (Skin *skin, int width, int height, TgrPrefs *pgr) {
    int i;

    xmin = pgr->xmin;
    xmax = pgr->xmax;
    ymin = pgr->ymin;
    ymax = pgr->ymax;
    for (i=0 ; i<MAX_GRFUNCS ; i++)
        graphCurves[i].valid = false;

    graph_compile_cache();
    graph_init_screen(skin, width, height);

    graphState.active = true;
    graphState.complete = false;
    graphState.position = 0;
    graphState.funcnum = 0;

    if (graphPrefs.functype == graph_func) {
        graphState.start = xmin;
        graphState.stop = xmax;
        graphState.step = (xmax - xmin) / ScrPrefs.xmax;
    }
    else if (graphPrefs.functype == graph_polar) {
        graphState.start = graphPrefs.fimin;
        graphState.stop  = graphPrefs.fimax;
        graphState.step  = graphPrefs.fistep;
    }
    else if (graphPrefs.functype == graph_param) {
        graphState.start = graphPrefs.tmin;
        graphState.stop  = graphPrefs.tmax;
        graphState.step  = graphPrefs.tstep;
    }

//    grcalc_init();
}

/***********************************************************************
 *
 * FUNCTION:     graph_draw_incr
 *
 * DESCRIPTION:  Draws a piece of a function (called from nilEvent
 *               in the original Palm version)
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
void Graph::graph_draw_incr (Skin *skin) {
    Int32 i;
    double start;
    double x,y;

    if (graphState.funcnum >= MAX_GRFUNCS) {
        graph_draw_complete();
        return;
    }

    if (graphState.position == 0) { // At start, do some checks + inits
        /* Check, if we have all functions to draw graph */
        if (!graphPrefs.grEnable[graphPrefs.functype][graphState.funcnum]
            || (graphCurves[graphState.funcnum].stack1 == NULL)
            || ((graphPrefs.functype == graph_param)
                && (graphCurves[graphState.funcnum].stack2 == NULL))) {
            graphState.funcnum++;
            return;
        }
        skin->selPen(graphState.funcnum);
    }

    // Draw a small piece of graph and return
    if (graphPrefs.functype == graph_func) {
        for (i=0 ;
             ((i<DRAW_AT_ONCE) && (graphState.position<=ScrPrefs.xmax));
             i+=graphPrefs.speed, graphState.position+=graphPrefs.speed) {
            graph_get_vals(graphState.funcnum,
                           graph_xscr2gr(graphState.position),
                           &x, &y, NULL);
            graph_new_point(skin, graphState.position, x, y, graphState.funcnum);
        }
        if (graphState.position > ScrPrefs.xmax) { /* Curve finished */
            skin->finishline(graphPrefs.colors[graphState.funcnum]); // Draw last point
//            /* Redraw labels. They may have been overwritten */
//            graph_draw_axes_labels(&stdbounds);
            graphState.position = 0;
            graphState.funcnum++;
        }
    } else { /* Parametric and polar */
        start = graphState.start;
        for (i=0 ;
             ((i<DRAW_AT_ONCE) && (start<=graphState.stop)) ;
             i+=graphPrefs.speed, graphState.position+=graphPrefs.speed) {
            start = graphState.start + graphState.step * graphState.position;
            if (start > graphState.stop) {
                /* Draw slightly behind end on polar graphs
                 * to compensate for rounding */
                if (graphPrefs.functype == graph_polar)
                start -= graphState.step / 2;
                else
                    break; /* Do not draw behind end on parametric */
            }
            graph_get_vals(graphState.funcnum,
                           start,
                           &x, &y, NULL);
            graph_new_point(skin, -1, x, y, graphState.funcnum);
        }
        if (start >= graphState.stop) { /* Curve finished */
            skin->finishline(graphPrefs.colors[graphState.funcnum]); // Draw last point
            graphState.position = 0;
            graphState.funcnum++;
        }
    }
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_pol
 *
 * DESCRIPTION:  Tracking of a polar graph
 *
 * PARAMETERS:   x,y - first tap
 *
 * RETURN:       true - Could calculate values
 *               false - No result
 *
 ***********************************************************************/
Boolean Graph::grtaps_track_pol (int *x, int *y, double *r_x, double *r_y, double *th, double *r, int curve_nb) {
    double realx, realy;
    double angle, radius;
    int dx, dy;
    CodeStack *stack;

    stack = graphCurves[curve_nb].stack1;
    if (!stack)
        return (false);

    angle = (double)(*x) * (graphPrefs.fimax - graphPrefs.fimin) / (double) (ScrPrefs.width);
    angle = graphPrefs.fimin
            + round(angle / graphPrefs.fistep) * graphPrefs.fistep;
    if (finite(angle)) {
        func_get_value(stack, angle, &radius, NULL);

        realx = radius * cos(angle);
        realy = radius * sin(angle);
        dx = graph_xgr2scr(realx);
        dy = graph_ygr2scr(realy);
    } else {
        dx = dy = -1;
        realx = realy = NaN;
    }

    *x = dx;
    *y = dy;
    *th = angle;
    *r = radius;
    *r_x = realx;
    *r_y = realy;
    if (dy == -1)
        return (false);
    return (true);
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_param
 *
 * DESCRIPTION:  Tracking of a parametric graph
 *
 * PARAMETERS:   x,y - first tap
 *
 * RETURN:       true - Could calculate values
 *               false - No result
 *
 ***********************************************************************/
Boolean Graph::grtaps_track_param (int *x, int *y, double *r_x, double *r_y, double *t, int curve_nb) {
    double realx, realy;
    double param;
    int dx, dy;
    CodeStack *stack1, *stack2;

    stack1 = graphCurves[curve_nb].stack1;
    stack2 = graphCurves[curve_nb].stack2;
    if (!stack1 || !stack2)
        return (false);

    param = (double)(*x) * (graphPrefs.tmax - graphPrefs.tmin) / (double) (ScrPrefs.width);
    param = graphPrefs.tmin
            + round(param / graphPrefs.tstep) * graphPrefs.tstep;
    if (finite(param)) {
        func_get_value(stack1, param, &realy, NULL);
        func_get_value(stack2, param, &realx, NULL);
        dx = graph_xgr2scr(realx);
        dy = graph_ygr2scr(realy);
    } else {
        dx = dy = -1;
        realx = realy = NaN;
    }

    *x = dx;
    *y = dy;
    *t = param;
    *r_x = realx;
    *r_y = realy;
    if (dy == -1)
        return (false);
    return (true);
}

/***********************************************************************
 *
 * FUNCTION:     grtaps_track_func
 *
 * DESCRIPTION:  Tracking of a normal function
 *
 * PARAMETERS:   x,y - first tap
 *
 * RETURN:       true - Could calculate values
 *               false - No result
 *
 ***********************************************************************/
Boolean Graph::grtaps_track_func (int *x, int *y, double *r_x, double *r_y, int curve_nb) {
    double realx, realy;
    CodeStack *stack;
    int dy;

    stack = graphCurves[curve_nb].stack1;
    if (!stack)
        return (false);

    realx = graph_xscr2gr(*x);
    func_get_value(stack, realx, &realy, NULL);
    dy = graph_ygr2scr(realy);
    if (!finite(realy))
        dy = -1;

    *y = dy;
    *r_x = realx;
    *r_y = realy;
    if (dy == -1)
        return (false);
    return (true);
}
