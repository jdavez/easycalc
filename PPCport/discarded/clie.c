/*
 * $Id: clie.c,v 1.1 2009/11/02 17:26:58 mapibid Exp $
 *
 *   Hi-res support routines for Palm and Sony Clié for EasyCalc.
 *   Copyright (C) 2003 Arno Welzel.
 *   Copyright (C) 2006 Ton van Overbeek
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

#include <PalmOS.h>
#include "konvert.h"
#include "calc.h"
#include "clie.h"

UInt16 gHrMode;
Int16 gSonyFactor = 1;

#ifdef SONY_SDK
#include <SonyCLIE.h>

UInt16 gHrLibApiVersion; // HRLib version
UInt16 gHrLibRefNum;     // HRLib reference number
Boolean gHrLibIsV2;      // True when HR Lib is ver.2 or later
#endif

/***********************************************************************
 *
 * FUNCTION:     GetHRLibVersion
 * 
 * DESCRIPTION:  Determine, if the Sony HR Lib is available
 *
 * PARAMETERS:   Pointer to variable to store lib reference handle
 *
 * RETURN:       0 if no HR lib is available, otherwise lib version
 *      
 ***********************************************************************/

UInt16 GetHRLibVersion(UInt16 *hrLibRefNumP)
{
#ifdef SONY_SDK
	Err error = errNone;
	UInt16 refNum;
	UInt16 version;

	if((error = SysLibFind(sonySysLibNameHR, &refNum)) != errNone)
	{
		if(error == sysErrLibNotFound)
		{ /* couldn't find lib */
			error = SysLibLoad( 'libr', sonySysFileCHRLib, &refNum );
		}
	}

	if(!error )
	{
		// There's HR Library. Let's get API version
		*hrLibRefNumP = refNum;
		HRGetAPIVersion( refNum, &version ); // Assume this api success
	} else
	{
		// No HRLib
		version = 0;
	}

	return version;
#else
	return 0;
#endif
}

/***********************************************************************
 *
 * FUNCTION:     clie_drawpixel
 * 
 * DESCRIPTION:  Clie compatible substitude for WinDrawLine
 *
 * PARAMETERS:   See WinDrawLine()
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/

void clie_drawpixel(Coord x, Coord y)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinDrawPixel(gHrLibRefNum, x, y);
	else 
#endif
	if(gHrMode == hrPalm) {
		save = WinSetCoordinateSystem(kCoordinatesNative);
		WinDrawPixel(x, y);
		WinSetCoordinateSystem(save);
	}
	else
	if (palmOS35)
		WinDrawPixel(x, y);
	else
		WinDrawLine(x, y, x, y);
}

/***********************************************************************
 *
 * FUNCTION:     clie_drawline
 * 
 * DESCRIPTION:  Clie compatible substitude for WinDrawLine
 *
 * PARAMETERS:   See WinDrawLine()
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/

void clie_drawline(Coord x1, Coord y1, Coord x2, Coord y2)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinDrawLine(gHrLibRefNum, x1, y1, x2, y2);
	else 
#endif
	if(gHrMode == hrPalm) {
		save = WinSetCoordinateSystem(kCoordinatesNative);
		WinDrawLine(x1, y1, x2, y2);
		WinSetCoordinateSystem(save);
	}
	else
		WinDrawLine(x1, y1, x2, y2);
}

/***********************************************************************
 *
 * FUNCTION:     clie_invertline
 * 
 * DESCRIPTION:  Clie compatible substitude for WinInvertLine
 *
 * PARAMETERS:   See WinInvertLine()
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/

void clie_invertline(Coord x1, Coord y1, Coord x2, Coord y2)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinInvertLine(gHrLibRefNum, x1, y1, x2, y2);
	else 
#endif
        if(gHrMode == hrPalm) {
                save = WinSetCoordinateSystem(kCoordinatesNative);
                WinInvertLine(x1, y1, x2, y2);
                WinSetCoordinateSystem(save);
        }
        else
		WinInvertLine(x1, y1, x2, y2);
}

/***********************************************************************
 *
 * FUNCTION:     clie_cliprectangle
 * 
 * DESCRIPTION:  Clie compatible substitude for WinClipRectangle
 *
 * PARAMETERS:   Coordinates of rectangle
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/

void clie_cliprectangle(RectangleType *rP)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinClipRectangle(gHrLibRefNum, rP);
	else 
#endif
	if (gHrMode == hrPalm) {
		save = WinSetCoordinateSystem(kCoordinatesNative);
		WinClipRectangle(rP);
		WinSetCoordinateSystem(save);
	}
	else
		WinClipRectangle(rP);
}

/***********************************************************************
 *
 * FUNCTION:     clie_eraserectangle
 * 
 * DESCRIPTION:  Clie compatible substitude for WinEraseRectangle
 *
 * PARAMETERS:   see WinEraseRectangle()
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/

void clie_eraserectangle(RectangleType *rP, UInt16 cornerDiam)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinEraseRectangle(gHrLibRefNum, rP, cornerDiam);
	else 
#endif
	if (gHrMode == hrPalm) {
		save = WinSetCoordinateSystem(kCoordinatesNative);
		WinEraseRectangle(rP, cornerDiam);
		WinSetCoordinateSystem(save);
	}
	else
		WinEraseRectangle(rP, cornerDiam);
}

/***********************************************************************
 *
 * FUNCTION:     clie_createoffscreenwindow
 * 
 * DESCRIPTION:  Clie compatible substitude for WinCreateOffscreenWindow
 *
 * PARAMETERS:   Width, hight, format, pointer for error return code
 *
 * RETURN:       Window handle
 *      
 ***********************************************************************/

WinHandle clie_createoffscreenwindow(Coord width, Coord height, WindowFormatType format, UInt16 *error)
{
#ifdef SONY_SDK
	if(gHrMode == hrSony)
		return HRWinCreateOffscreenWindow(gHrLibRefNum, width, height, format, error);
	else 
#endif
		return WinCreateOffscreenWindow(width, height, format, error);
}

/***********************************************************************
 *
 * FUNCTION:     clie_copyrectangle
 * 
 * DESCRIPTION:  Clie compatible substitude for WinCopyRectangle
 *
 * PARAMETERS:   see WinCopyRectangle()
 *
 * RETURN:       nothing
 *      
 ***********************************************************************/

void clie_copyrectangle(WinHandle srcWin, WinHandle dstWin, RectangleType *srcRect, Coord destX, Coord destY, WinDrawOperation mode)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinCopyRectangle(gHrLibRefNum, srcWin, dstWin, srcRect, destX, destY, mode);
	else 
#endif
        if (gHrMode == hrPalm) {
                save = WinSetCoordinateSystem(kCoordinatesNative);
		WinCopyRectangle(srcWin, dstWin, srcRect, destX, destY, mode);
                WinSetCoordinateSystem(save);
        }
        else
		WinCopyRectangle(srcWin, dstWin, srcRect, destX, destY, mode);
}

/***********************************************************************
 *
 * FUNCTION:     clie_setclip
 * 
 * DESCRIPTION:  Clie compatible substitude for WinSetClip
 *
 * PARAMETERS:   see WinSetClip()
 *
 * RETURN:       nothing
 *      
 ***********************************************************************/

void clie_setclip(RectangleType *rP)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinSetClip(gHrLibRefNum, rP);
	else 
#endif
        if (gHrMode == hrPalm) {
                save = WinSetCoordinateSystem(kCoordinatesNative);
                WinSetClip(rP);
                WinSetCoordinateSystem(save);
        }
        else
		WinSetClip(rP);
}

/***********************************************************************
 *
 * FUNCTION:     clie_getclip
 * 
 * DESCRIPTION:  Clie compatible substitude for WinGetClip
 *
 * PARAMETERS:   see WinGetClip()
 *
 * RETURN:       nothing
 *      
 ***********************************************************************/

void clie_getclip(RectangleType *rP)
{
	UInt16 save;

#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinGetClip(gHrLibRefNum, rP);
	else 
#endif
        if (gHrMode == hrPalm) {
                save = WinSetCoordinateSystem(kCoordinatesNative);
                WinGetClip(rP);
                WinSetCoordinateSystem(save);
        }       
        else
		WinGetClip(rP);
}

/***********************************************************************
 *
 * FUNCTION:     clie_drawrectangleframe
 * 
 * DESCRIPTION:  Clie compatible substitude for WinDrawRectangleFrame
 *
 * PARAMETERS:   see WinDrawRectangleFrame()
 *
 * RETURN:       nothing
 *      
 ***********************************************************************/

void clie_drawrectangleframe(FrameType frame, const RectangleType *rP)
{
	UInt16 save;
#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinDrawRectangleFrame(gHrLibRefNum, frame, (RectangleType *)rP);
	else
#endif
	if (gHrMode == hrPalm) {
		save = WinSetCoordinateSystem(kCoordinatesNative);
		WinDrawRectangleFrame(frame, rP);
		WinSetCoordinateSystem(save);
	}
	else
		WinDrawRectangleFrame(frame, rP);
}

/***********************************************************************
 *
 * FUNCTION:     clie_drawbitmap
 * 
 * DESCRIPTION:  Clie compatible substitude for WinDrawBitmap
 *
 * PARAMETERS:   see WinDrawBitmap()
 *
 * RETURN:       nothing
 *      
 ***********************************************************************/
BitmapPtr SCExtractHiresBitmap(BitmapPtr pBitmap);

void clie_drawbitmap(BitmapPtr bitmapP, Coord x, Coord y)
{
#ifdef SONY_SDK
	if (gHrMode == hrSony) {
		BitmapPtr V2bitmapP = SCExtractHiresBitmap(bitmapP);

		if (V2bitmapP) {
			HRWinDrawBitmap(gHrLibRefNum, V2bitmapP, x, y);
			MemPtrFree(V2bitmapP);
		} else {
			WinDrawBitmap(bitmapP, x, y);
		}
	} else 
#endif
		WinDrawBitmap(bitmapP, x, y);
}

/***********************************************************************
 *
 * FUNCTION:     clie_drawchars
 * 
 * DESCRIPTION:  Clie compatible substitude for WinDrawChars
 *
 * PARAMETERS:   see WinDrawChars()
 *
 * RETURN:       nothing
 *      
 ***********************************************************************/

void clie_drawchars(const Char *chars, Int16 len, Coord x, Coord y)
{
#ifdef SONY_SDK
	if(gHrMode == hrSony) 
		HRWinDrawChars(gHrLibRefNum, chars, len, x, y);
	else 
#endif
		WinDrawChars(chars, len, x, y);
}
