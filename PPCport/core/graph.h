/*
 *   $Id: graph.h,v 1.1 2011/02/28 22:07:18 mapibid Exp $
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
/* graph.h : Graph object for handling multiple drawing on screen.
 *****************************************************************************/
#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "core/grprefs.h"

#define DRAW_AT_ONCE 10

#define GRCFUNC_SZ  8
#define GRCPOLAR_SZ 5
#define GRCPARAM_SZ 3

typedef enum {
    cp_zero = 0,    // Find a zero between min & max
    cp_value,       // Find a point at which fct returns 0 between min & max
    cp_min,         // Find minimum output over [min, max]
    cp_max,         // Find minimum output over [min, max]
    cp_dydx1,       // Derived value at selected input x value
    cp_dydx2,       // Derived of derived at selected input x
    cp_integ,       // Integration of fct between min & max
    cp_intersect,   // Intersection point between 2 curves over min & max
    cp_odrdfi,      // Value of dr/dth in polar mode at a given th
    cp_pdydx,       // value of dy/dx at point t in parametric mode at a given t
    cp_pdydt,       // Value of dy/dt in parametric mode at a given t
    cp_pdxdt,       // Value of dx/dt in parametric mode at a given t
    cp_end
} t_grFuncType;

typedef struct {
    double  x, y;
    Boolean valid;
    Boolean ison;
    Coord   scr_x, scr_y;
    CodeStack *stack1, *stack2;
} Tgraph_curves;

/* Screen preferences in pixels */
typedef struct {
//        Coord xmin, ymin;
        Coord xmax, ymax;
        Coord y0;
        int   width, height;
        /* Cached coeficients for graph2screen computations */
        /* CacheTrans MUST be called when any of the ScrPrefs
         * and graphPrefs attributes is changed */
        double xtrans,ytrans;
        /* cached logaritms, it is quite slow to calculate */
        double logxtrans,logytrans;
        double logxmin,logymin;
} TscrPrefs;

class Graph {
protected:
    double xmin, ymin;
    double xmax, ymax;

    double graph_isec_y(double x1, double y1, double x2, double y2, double x3);
    void graph_outscreen(double *newx, double *newy,
                         double onx, double ony, double offx, double offy);
    void graph_new_point(Skin *skin, Coord scr_x, double x, double y, Int16 funcnum);
    void cacheTrans(void);
    void graph_init_screen(Skin *skin, int width, int height);

public:
    TscrPrefs ScrPrefs;

    Boolean graph_is_onscreen(double x, double y);
    double graph_xscr2gr(Coord x);
    double graph_yscr2gr(Coord y);
    Coord graph_xgr2scr(double x);
    Coord graph_ygr2scr(double y);
    Boolean graph_get_vals(int fnum, double param, double *x, double *y, double *r);
    void graph_zoom_out(void);
    //void graph_setcolor(Int16 funcnum);
    //void graph_unsetcolor(void);
    void graph_draw_start(Skin *skin, int width, int height, TgrPrefs *pgr);
    void graph_draw_resume(void);
    void graph_draw_stop(void);
    bool is_graph_active(void);
    bool is_graph_complete(void);
    void graph_draw_incr(Skin *skin);
    Boolean grtaps_track_pol(int *x, int *y, double *r_x, double *r_y, double *th, double *r, int curve_nb);
    Boolean grtaps_track_param(int *x, int *y, double *r_x, double *r_y, double *t, int curve_nb);
    Boolean grtaps_track_func(int *x, int *y, double *r_x, double *r_y, int curve_nb);
};

void graph_init_cache(void);
void graph_clean_cache(void);
void graph_draw_resume(void);
void graph_draw_stop(void);
void graph_draw_complete(void);
bool is_graph_active(void);
bool is_graph_complete(void);
void graph_get_zoneOnView(long *top, long *left, long *bottom, long *right);

#ifndef _GRAPH_CPP_
extern Graph zoneGraph;
extern Graph viewGraph;
extern Tgraph_curves graphCurves[];
extern TCHAR *lstGraphCalc[];
extern t_grFuncType grcFunc[GRCFUNC_SZ];
extern t_grFuncType grcPolar[GRCPOLAR_SZ];
extern t_grFuncType grcParam[GRCPARAM_SZ];
#endif

#endif
