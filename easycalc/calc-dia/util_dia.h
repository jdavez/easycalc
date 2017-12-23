/* File name: util_dia.h

   Copyright 2006 Ton van Overbeek

   This is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

 *
 * Summary
 *  Utility definitions for PalmResize DIA support.
 *
 */

#ifndef _UTIL_DIA_H_
#define _UTIL_DIA_H_

#define SafeMemPtrNew          MemPtrNew
#define SafeMemPtrFree(p)      if ((p) != NULL) MemPtrFree((p))

#endif
