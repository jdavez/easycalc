/*
 *   $Id: defuns.h,v 1.41 2007/09/28 01:23:25 tvoverbe Exp $
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
 *   You can contact me at 'ondrap@penguin.cz'.
*/

#ifndef _DEFUNS_H_
#define _DEFUNS_H_

#include "segment.h"
#include "konvert.h"

#define MAX_FP_NUMBER 64

/* Application specific things */
#define APP_ID        'OpCl'
#define LIB_ID        'OpCl'
#define DBNAME        "CalcDB-OpCl"
#define HISTORYDBNAME "CalcDB-H-OpCl"
#define SOLVERDBNAME  "CalcDB-S-OpCl"
#define DBTYPE        'Data'
#define SOLVERDBTYPE  'Slvr'
#define CARDNO        0

#define DBVERSION          5
#define HIST_DB_VERSION    6
#define SOLVER_DB_VERSION  1

#define PREF_DEFAULT           0
#define PREF_RESULT            1
#define PREF_GRAPH             3
#define PREF_LSTEDIT           4
#define PREF_MTXEDIT           5
#define PREF_DIA               6

/* Change this if you are changing contents of the preferences structure */
#define PREF_VERSION          44
#define PREF_RES_VERSION       2
#define PREF_GRAPH_VERSION     5
#define PREF_LSTEDIT_VERSION   1
#define PREF_MTXEDIT_VERSION   1
#define PREF_DIA_VERSION       1

/* Maximum length of normal strings copied from tSTR */
#define MAX_RSCLEN   30
#define MAX_RESULT   17

/* Rounding offset for reduce precision */
#define ROUND_OFFSET      13
#define PRECISION_TO_ZERO 1E-12

/* konvert.c constants */
#define MAX_RPNSIZE 105
#define MAX_OSTACK   45

#define NaN ((double)(3-3)/(3-3))

char * guess(Trpn item) MLIB;
char * print_error(CError err);

#endif
