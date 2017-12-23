/*
 *   $Id: segment.h,v 1.7 2006/09/12 19:40:55 tvoverbe Exp $
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


#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#define MLIB_SECTION __attribute__ ((section ("mlib")))
#define MLIB MLIB_SECTION

#define BASEFUNC_SECTION __attribute__ ((section ("basefunc")))
#define BASEFUNC BASEFUNC_SECTION

#define IFACE_SECTION __attribute__ ((section ("iface")))
#define IFACE IFACE_SECTION

#define SPECFUN_SECTION __attribute__ ((section ("specfun")))
#define SPECFUN SPECFUN_SECTION

#define GRAPH_SECTION __attribute__ ((section ("graph")))
#define GRAPH GRAPH_SECTION

#define NEWFUNC_SECTION __attribute__ ((section ("newfunc")))
#define NEWFUNC NEWFUNC_SECTION

#define PARSER_SECTION __attribute__ ((section ("parser")))
#define PARSER PARSER_SECTION

#define MATFUNC_SECTION __attribute__ ((section ("matfunc")))
#define MATFUNC MATFUNC_SECTION

#endif
