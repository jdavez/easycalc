/*****************************************************************************
 * EasyCalc -- a scientific calculator
 * Copyright (C) 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/
#ifndef CORE_GLOBALS_H
#define CORE_GLOBALS_H 1


/*************/
/* Key codes */
/*************/
#define NB_KEYS   BUTTON_COUNT
// Keep KEY_SHIFT the smallest one in list, just after KEY_NONE
#define KEY_CLR     -1
#define KEY_BCKSPC  -2
#define KEY_EXE     -3
#define KEY_DOT     -4
#define KEY_CTRZONE -5
#define KEY_CTRWIDE -6
#define KEY_GPREFS  -7
#define KEY_GCONF   -8
#define KEY_GCALC   -9
#define KEY_GSETX   -10
#define KEY_ZONEIN  -11
#define KEY_ZONEOUT -12
#define KEY_WIDEIN  -13
#define KEY_WIDEOUT -14
#define KEY_NORMZON -15
#define KEY_SHIFT   -16
#define KEY_NONE    -17

/*******************/
/* Values for skey */
/*******************/
#define NOSKEY		-1
#define ANNUNBASE	-2
#define RESULT_AREA -99
#define ZONE_AREA	-100
#define WVIEW_AREA	-101

/*********************/
/* Annunciator codes */
/*********************/
#define NB_ANNUN    35
#define ANN_SHIFT   0
#define ANN_DEG     1
#define ANN_RAD     2
#define ANN_GRAD    3
#define ANN_DEC     4
#define ANN_OCT     5
#define ANN_BIN     6
#define ANN_HEX     7
#define ANN_SCRL    8
#define ANN_SCRR    9
#define ANN_S1      10
#define ANN_S2      11
#define ANN_S3      12
#define ANN_SG      13
#define ANN_VAR     14
#define ANN_UFCT    15
#define ANN_CFCT    16
#define ANN_RESMENU 17
#define ANN_HSTMENU 18
#define ANN_SELZONE 19
#define ANN_MOVGRPH 20
#define ANN_TRACKPT 21
#define ANN_CTRWIDE 22
#define ANN_ZONMAXY 23
#define ANN_ZONMINY 24
#define ANN_ZONMINX 25
#define ANN_ZONMAXX 26
#define ANN_WONZ_XR 27
#define ANN_WONZ_YR 28
#define ANN_PARAM0  29
#define ANN_PARAM   30
#define ANN_YVALUE  31
#define ANN_XVALUE  32
#define ANN_RVALUE  33
#define ANN_CURVENB 34

/******************************/
/* Annunciator value          */
/* Note: UNCH means unchanged */
/******************************/
#define ANNVAL_UNCH -1
#define ANNVAL_DEG  0
#define ANNVAL_RAD  1
#define ANNVAL_GRAD 2
#define ANNVAL_DEC  0
#define ANNVAL_OCT  1
#define ANNVAL_BIN  2
#define ANNVAL_HEX  3
#define ANNVAL_S1   0
#define ANNVAL_S2   1
#define ANNVAL_S3   2
#define ANNVAL_SG   3
#define ANNVAL_SCR_NONE  0
#define ANNVAL_SCR_LEFT  1
#define ANNVAL_SCR_RIGHT 2
                                // Effect on display zone           Effect on wide view
#define ANNVAL_PEN_SELZONE 0    // Drag = define zone + upd(x0,x,y) = define zone + upd(x0,x,y)
                                // Tap = set(x,y)                   = set(x,y)
#define ANNVAL_PEN_MOVZONE 1    // Drag = set(x,y)                  = set(x,y)
                                //        + move graph in zone        + move graph in wide view
                                // Tap = set(x,y)                   = set(x,y) + set zone center
#define ANNVAL_PEN_TRACKPT 2    // Drag = upd(x0,x,Yn(x))           = set(x,Yn(x)) + deselect
                                //        + define calc select zone
                                // Tap = set(x,Yn(x)) + deselect    = set(x,Yn(x)) + deselect
#define ANNVAL_PEN_CTRWIDE 3    // Same as MOVZONE                  = move zone center
                                //                                    + set(x,y)
                                // Same as MOVZONE                  = Same as MOVZONE


/***********************/
/* Variable data types */
/***********************/

/******************/
/* Emulator state */
/******************/

/* FLAGS
 * Note: flags whose names start with VIRTUAL_ are named here for reference
 * only; they are actually handled by virtual_flag_handler(). Setting or
 * clearing them in 'flags' has no effect.
 * Flags whose names are the letter 'f' followed by two digits have no
 * specified meaning according to the HP-42S manual; they are either user flags
 * or reserved.
 */
typedef union {
    char farray[100];
    struct {
        char audio_enable;
    } f;
} flags_struct;
extern flags_struct flags;


/****************/
/* More globals */
/****************/
extern int pending_command;
extern bool mode_shift;
extern bool mode_running;
extern int (*mode_interruptible)(int);
extern bool mode_stoppable;
extern bool mode_getkey;
extern int remove_program_catalog;
extern int mode_plainmenu;
extern int mode_transientmenu;


/* Keystroke buffer - holds keystrokes received while
 * there is a program running.
 */
extern int keybuf_head;
extern int keybuf_tail;
extern int keybuf[16];


/*********************/
/* Utility functions */
/*********************/



#endif