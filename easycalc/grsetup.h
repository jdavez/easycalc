/*
 *   $Id: grsetup.h,v 1.7 2006/10/16 18:58:37 tvoverbe Exp $
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

#ifndef _GRSETUP_H_
#define _GRSETUP_H_

char * grsetup_get_fname(Int16 row) IFACE;
const char * grsetup_fn_descr(Int16 i) IFACE;
Boolean  GraphSetupHandleEvent(EventPtr event);
Int16 grsetup_fn_descr_arr(char **descr, Int16 *nums);

#endif
