/*****************************************************************************
 * EasyCalc -- a scientific calculator
 * Copyright (C) 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * The name and many features come from
 * - EasyCalc on Palm:
 *
 * It also is reusing elements from
 * - Free42:  Thomas Okken
 * for its adaptation to the PocketPC world.
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
/* core_globals.cpp : contains global calc state & procedures.
 *****************************************************************************/

#include "stdafx.h"
#include "core_globals.h"

#include <stdlib.h>

/* Flags */
flags_struct flags;

int pending_command = 0;
bool mode_shift = false;
bool mode_running = false;
int (*mode_interruptible)(int) = NULL;
bool mode_stoppable;
bool mode_getkey;
int remove_program_catalog = 0;
int mode_plainmenu;
int mode_transientmenu;

/* Keystroke buffer - holds keystrokes received while
 * there is a program running.
 */
int keybuf_head = 0;
int keybuf_tail = 0;
int keybuf[16];
