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
#ifndef PALM_OS_H
#define PALM_OS_H 1

#include <types.h>

#define int2 short
#define uint2 unsigned short
#define int4 int
#define uint4 unsigned int
#define uint unsigned int

#define Int8    INT8
#define UInt8   UINT8
#define Int16   INT16
#define UInt16  UINT16
#define Int32   INT32
#define UInt32  UINT32
#define Char    TCHAR

#define Boolean bool
#define Coord   int
#define Err     int
#define FieldPtr    int
#define IndexedColorType int

#define StrCopy      _tcscpy
#define StrLen       _tcslen
#define StrNCompare  _tcsncmp
#define StrCompare   _tcscmp
#define StrCat       _tcscat
#define StrChr       _tcschr
#define StrPrintF    _stprintf
#define StrIToA(s,i) _itot(i,s,10)
// No _tcsicoll / _wcsicoll /_stricoll in MSVC for Pocket PC unfortunately,
// so for now, using _tcsicmp.
#define StrCaselessCompare _tcsicmp

#include "MemoryManager.h"
typedef void *WinHandle;
typedef void *ListPtr;

#define noListSelection -1

typedef struct PointType {
  Coord x;
  Coord y;
} PointType;

typedef struct RectangleType {
  PointType topLeft;
  PointType extent;
} RectangleType;
typedef RectangleType *RectanglePtr;

typedef struct RGBColorType {
  UInt8 index;
  UInt8 r;
  UInt8 g;
  UInt8 b;
} RGBColorType;

#define WinRGBToIndex(c)  ((DWORD) (((BYTE) ((c)->r) | ((WORD) ((c)->g) << 8)) | (((DWORD) (BYTE) ((c)->b)) << 16)))

#include "DataManager.h"
#include "FloatManager.h"
#include "Preferences.h"

#include <stdlib.h>
#define SysRandom(x) ((x == 0) ? rand() : (srand(x), rand()))
#define sysRandomMax RAND_MAX
extern unsigned long nan[];
#define NaN (*((double *) nan))

UInt32 TimGetSeconds (void);

void ErrFatalDisplayIf(int cond, TCHAR *msg); // Declared in Easycalc.cpp

#define SYS_TRAP(a)

#endif