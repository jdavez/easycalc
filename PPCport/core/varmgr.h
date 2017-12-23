/*   
 * $Id: varmgr.h,v 1.5 2011/02/28 22:07:18 mapibid Exp $
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

#include "Skin.h"

void history_popup(Skin *skin, void *hWnd) IFACE;
TCHAR *history_action(Skin *skin, void *hWnd_p, void *hWnd_calc, int selection);
void varmgr_popup(Skin *skin, void *hWnd, rpntype type);
TCHAR *varmgr_action(int selection);
void varmgr_listVar(Skin *skin, void *hWnd);
bool varmgr_listVar_action(TCHAR *text, void *hWnd_calc, bool saveasvar, bool fromList, bool valueAns);
void varmgr_popup_builtin(Skin *skin, void *hWnd);
TCHAR *varmgr_builtinAction(int selection);
bool varmgr_getVarDef (TCHAR *varname, TCHAR **varDef);
bool varmgr_getFctDef (TCHAR *fctname, TCHAR **fctDef, TCHAR *fctParam);
Boolean varmgr_edit_save(TCHAR *namefield, TCHAR *varfield, TCHAR *paramfield,
                         rpntype type, Boolean editname, void *hWnd_p) IFACE;
#endif
