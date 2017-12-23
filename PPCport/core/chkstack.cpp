/*   
 * $Id: chkstack.cpp,v 1.1 2009/10/17 13:47:14 mapibid Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000,2001 Ondrej Palkovsky
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
#include "StdAfx.h"

#include "compat/PalmOS.h"
#include "konvert.h"
//#include "calc.h"
//#include "calcrsc.h"

#define MARGIN 800UL /* stack margin */

static UInt32 stackBeg = 0;
static UInt32 stackEnd = 0;

void initChkStack(void)
{
	UInt16 dummy = 0;

#ifndef PALMOS
    /* Limited 32K stack for Non PalmOS (for now) */
    stackEnd = (UInt32)&dummy;  /* dummy is at offset 0x7D */
    stackBeg = stackEnd - 0x8000 + 0x7D;
#else
	if (palmOS3) {
		SysGetStackInfo((MemPtr *)&stackBeg, (MemPtr *)&stackEnd);
	}
	else {
		/* Limited 2K stack for PalmOS 2.0 */
		stackEnd = (UInt32)&dummy;  /* dummy is at offset 0x7D */
		stackBeg = stackEnd - 0x800 + 0x7D;
	}
#endif
}

Boolean chkStack(void)
{
	UInt32 stackaddr = (UInt32)&stackaddr;

	if (stackaddr > (stackBeg + MARGIN) && stackaddr < stackEnd)
		return false;
	else
		return true;
}
