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
#ifndef CORE_MAIN_H
#define CORE_MAIN_H 1

#include "EasyCalc.h"
#include "system - UI/Skin.h"


/*******************/
/* Keyboard repeat */
/*******************/
extern int repeating;
extern int repeating_shift;
extern int repeating_key;

/**********************************/
/* Shell/Core interface functions */
/**********************************/
int core_repeat() MAIN_SECT;
void core_keytimeout1() MAIN_SECT;
void core_keytimeout2() MAIN_SECT;
int core_keydown(int key, int *enqueued, int *repeat) MAIN_SECT;
int core_keyup(Skin *skin, void *hWnd_p) MAIN_SECT;

/*******************/
/* Other functions */
/*******************/

#endif
