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
#include "StdAfx.h"

#include "compat/PalmOS.h"

unsigned long nan[2]={0xffffffff, 0x7fffffff};

UInt32 TimGetSeconds (void) {
    SYSTEMTIME syst_time;
    FILETIME   file_time;
    UInt32     t;

    GetSystemTime(&syst_time);
    SystemTimeToFileTime(&syst_time, &file_time);
    // We got the number of 100-nanosecond intervals since January 1, 1601
    // Convert to seconds since then.
    // 1 s = 1000000 µs = 10000000 * 100 ns
    t = (UInt32) (((ULARGE_INTEGER*) &file_time)->QuadPart / 10000000L);
    return (t);
}
