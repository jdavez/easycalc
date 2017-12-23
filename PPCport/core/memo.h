/*
 *   $Id: memo.h,v 1.1 2011/02/28 22:07:18 mapibid Exp $
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
 *
*/

#ifndef _MEMO_H_
#define _MEMO_H_

//#define MAX_DUMP 4000
#define MAX_DUMP 61000
#define EQUATIONS 999

Boolean memo_import_memo(Skin *skin, void *hWnd_p, const TCHAR *filename, Boolean unicode);
void memo_dump(rpntype type, const TCHAR *filename, Boolean unicode);

#endif
