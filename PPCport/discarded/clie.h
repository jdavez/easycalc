/*
 * $Id: clie.h,v 1.1 2009/11/02 17:26:58 mapibid Exp $
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

#ifndef _CLIE_H_
#define _CLIE_H_

enum { hrNone = 0, hrSony, hrPalm };
extern UInt16 gHrMode;
extern Int16 gSonyFactor;

extern UInt16 gHrLibApiVersion; // HRLib version
extern UInt16 gHrLibRefNum;     // HRLib reference number
extern Boolean gHrLibIsV2;      // True when HR Lib is ver.2 or later

extern UInt16 GetHRLibVersion(UInt16 *hrLibRefNum);
extern void clie_drawpixel(Coord x, Coord y);
extern void clie_drawline(Coord x1, Coord y1, Coord x2, Coord y2);
extern void clie_invertline(Coord x1, Coord y1, Coord x2, Coord y2);
extern void clie_cliprectangle(RectangleType *rP);
extern void clie_eraserectangle(RectangleType *rP, UInt16 cornerDiam);
extern WinHandle clie_createoffscreenwindow(Coord width, Coord height, WindowFormatType format, UInt16 *error);
extern void clie_copyrectangle(WinHandle srcWin, WinHandle dstWin, RectangleType *srcRect, Coord destX, Coord destY, WinDrawOperation mode);
extern void clie_setclip(RectangleType *rP);
extern void clie_getclip(RectangleType *rP);
extern void clie_drawrectangleframe(FrameType frame, const RectangleType *rP);
extern void clie_getwindowextent(Coord *extentX, Coord *extentY);
extern void clie_drawbitmap(BitmapPtr bitmapP, Coord x, Coord y);
extern void clie_drawchars(const Char *chars, Int16 len, Coord x, Coord y);

#endif
