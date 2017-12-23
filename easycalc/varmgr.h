/*   
 * $Id: varmgr.h,v 1.6 2006/09/12 19:40:55 tvoverbe Exp $
 * 
 * Scientific Calculator for Palms.
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

#ifndef _VARMGR_H_
#define _VARMGR_H_

#define MAX_LIST_LENGTH 13
#define MAX_EDIT_TITLE  30

char * history_popup(void) IFACE;
char * varmgr_popup(rpntype type) IFACE;
char * varmgr_popup_builtin(void) IFACE;
Boolean VarmgrEntryHandleEvent(EventPtr event) IFACE;
Boolean varmgr_edit(const char *varname,char *title,rpntype type,
		    Boolean editname,const char *parameter) IFACE;
Boolean varmgr_get_double(double *value,char *title) IFACE;
Boolean varmgr_get_complex(Complex *value,char *title) IFACE;
Boolean varmgr_get_varstring(char *varname,char *title) IFACE;
#endif
