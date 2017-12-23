/*
 * $Id: clie-util.c,v 1.1 2009/11/02 17:26:58 mapibid Exp $
 *
 *   Support code for hi-res Clié support for EasyCalc.
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

/* The code in this file is derived from a message on the Sony developer forum
   by Shannon Pekary on 2003-01-07. See
   http://news.palmos.com/read/messages?id=107811#107811

   Code can only be run on 68k devices. It will fail on ARM devices, due to
   the access of Bitmap internals. Should be no problem for EasyCalc, since
   this code is only used on Sony PalmOS4 devices (=68k) with HRLibrary.
*/

#ifdef SONY_SDK

#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include <Bitmap.h>
#include <Window.h>
#include <MemoryMgr.h>
#include <BmpGlue.h>
#include <HostControl.h>
#include <ErrorBase.h>

/***********************************************************************

UtilV2BitmapFromV3

Purpose: Returns a newly created version 2 bitmap from a version
3 bitmap. This would primarily be used when wanting to
draw a high resolution bitmap on a sony screen and needing
to extract the hi-res bitmap from the current bitmap data.

We assume there is no color table and that this is an 8-bit
image only.

Preconditions:

Input: none

Output: none

Returns: none

************************************************************************/

static BitmapPtr GetNextBitmap (BitmapPtr pBitmap);

static BitmapPtr UtilV2BitmapFromV3(BitmapPtr pV3Bitmap)
{

	BitmapTypeV2* pNew;
	BitmapTypeV3* pBitmap = (BitmapTypeV3*) pV3Bitmap;
	UInt32 bitsSize;
	UInt16 bitsOffset;
	Boolean compressed = pBitmap->flags.compressed;

	bitsOffset = pBitmap->size;

	if (compressed) {
		bitsSize = *((UInt32*)((UInt8*)pBitmap + pBitmap->size));
		// bit size is the first word of compressed bits
		bitsOffset += 2;
		bitsSize -= 2;
		// V2 bitmaps use a short instead of a long for the compressed size
	} else {
		bitsSize = pBitmap->rowBytes * pBitmap->height;
	}

	if (bitsSize > 65500) {
		// V2 can't handle large bitmaps
		return (BitmapPtr)NULL;
	}

	pNew = (BitmapTypeV2*) MemPtrNew (bitsSize + sizeof (BitmapTypeV2));

	if (pNew) {
		MemMove (pNew, pBitmap, sizeof (BitmapType));
		// moves basic header values
		pNew->nextDepthOffset = 0;
		// terminate bitmap chain
		pNew->transparentIndex = pBitmap->transparentValue;
		pNew->compressionType = pBitmap->compressionType;
		pNew->reserved = 0;

		MemMove (pNew + 1, (UInt8*)pBitmap + bitsOffset, bitsSize);
	}

	return (BitmapPtr)pNew;
}

/***********************************************************************

SCExtractHiresBitmap

Purpose: Extracts a hires bitmap from the given bitmap. If
extraction is successful, this will return a new
version 2 bitmap that you should draw with and dispose.
If there is no hires bitmap, this will return NULL.

This would mainly be used to draw hi res on a sony device.

Preconditions:

Input: pBitmap The bitmap to search
color true to find a color bitmap

Output: none

Returns: none

************************************************************************/

BitmapPtr SCExtractHiresBitmap(BitmapPtr pBitmap)
{
	BitmapPtr pTempBitmap = NULL,
	previousBitmap = pBitmap;
	UInt32 screenDepth;

	WinScreenMode(winScreenModeGet, NULL, NULL, &screenDepth, NULL);

	pTempBitmap = pBitmap;
	do {
		do {
			pTempBitmap = GetNextBitmap (pTempBitmap);
		} while (pTempBitmap && pTempBitmap->version != 3);
		// we assume any version 3 bitmap is what we want

		if (pTempBitmap) {
			if (pTempBitmap->pixelSize == screenDepth ||
			    (pTempBitmap->pixelSize > screenDepth &&
			     pTempBitmap->pixelSize < 8)) {
				break; // found a match
			} else if (pTempBitmap->pixelSize > screenDepth) {
				pTempBitmap = previousBitmap;
				break; // use the last one we found
			}

			// else find again
			previousBitmap = pTempBitmap;
		}
	} while (pTempBitmap);

	if (!pTempBitmap) {
		pTempBitmap = previousBitmap; // use the most recent one found
	}

	if (pTempBitmap && pTempBitmap->version == 3) {
		pTempBitmap = UtilV2BitmapFromV3 (pTempBitmap);
	} else {
		pTempBitmap = NULL;
	}

	return pTempBitmap;
}

// returns the next bitmap in a bitmap family regardless of density.

static BitmapPtr GetNextBitmap (BitmapPtr pBitmap)
{
	BitmapPtr pTempBitmap;

	if (pBitmap->version < 3) {
		pTempBitmap = BmpGetNextBitmap (pBitmap);
		/* Check if dummy V1 bitmap, if yes return first V3 header */
		if (pTempBitmap->version == 1 &&
		    pTempBitmap->pixelSize == 0xFF &&
		    pTempBitmap->width == 0 &&
		    pTempBitmap->height == 0 &&
		    pTempBitmap->rowBytes == 0) {
			return (BitmapPtr)((char *)pTempBitmap + sizeof(BitmapTypeV1));
		}
	} else {
		UInt32 offset = ((BitmapTypeV3*)pBitmap)->nextBitmapOffset;

		if (offset) {
			return (BitmapPtr)(((char*)pBitmap) + offset);
		}
	}
	return NULL;
}

#endif
