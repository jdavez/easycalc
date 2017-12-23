/* File name: config.h

   Copyright 2006 Ton van Overbeek

   This is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

 *
 * Summary
 *  Config file for PalmResize DIA support in EasyCalc.
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef SONY_SDK
#define SUPPORT_DIA_SONY
#endif

#ifdef HANDERA_SDK
#define SUPPORT_DIA_HANDERA
#endif

#endif
