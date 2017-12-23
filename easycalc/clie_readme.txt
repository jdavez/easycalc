The files clie.c, clie.h and clie-util.c now support both PalmOS 5 hires
and Sony PalmOS 4 hires. Sony PalmOS 5 uses the generic PalmOS 5 hires code.

The text below by Arno Welzel is now mostly obsolete. It is left to preserve
the history.

Ton van Overbeek, 2006-08
------------------------------------------------------------------------------
Changes for EasyCalc 1.20/hr
Arno Welzel / 2003-05


1. Sony Clie SDK 3.0

All changes are based on the Sony Clie SDK 3.0. This SDK may be
obtained at http://www.us.sonypdadev.com/develop_tool/sdk_30.html

IMPORTANT: The header files are not usable "out of the box" with
prc-tools! Some files have to be modified in the following way,
e.h. Libraries/SonyHRLib.h:

  typedef enum tagHRTrapNumEnum
  {
    HRTrapGetAPIVersion = sysLibTrapCustom,
    HRTrapWinClipRectangle,
    HRTrapWinCopyRectangle,
  ...
  }
  
This has to be changed to:
  
  //typedef enum tagHRTrapNumEnum
  //{
    #define HRTrapGetAPIVersion (sysLibTrapCustom+0) //= sysLibTrapCustom,
    #define HRTrapWinClipRectangle (sysLibTrapCustom+1) //
    #define HRTrapWinCopyRectangle (sysLibTrapCustom+2) //
  ...
  // }

I provide a version of these files, which are already changed.
  

2. Makefile

The makefile modified to include the Sony SDK header files, which
should be available in the path /PalmDev/clie-sdk. Additionally,
a new file clie.c with its header clie.h has to be included in
the build:

CFLAGS=-Wall -Wno-switch -O2 -I./include -I. -I/PalmDev/clie-sdk/include  -I/PalmDev/clie-sdk/include/Libraries  -I/PalmDev/clie-sdk/include/System

CALC_OBJS = calc.o clie.o mlib/calcDB.o mlib/fl_num.o mlib/fp.o mlib/funcs.o \
       mlib/guess.o mlib/konvert.o mlib/mathem.o mlib/stack.o prefs.o \
       mlib/MathLib.o mlib/display.o mlib/history.o mlib/complex.o \
       finance.o result.o ansops.o defmgr.o mlib/meqstack.o \
       mlib/txtask.o \
       main.o memo.o varmgr.o $(TARGET)-sections.o


3. New files

For a number of functions to handle the Clie specific stuff there is a new
file clie.c with a header clie.h.


4. Changes

calc.c

- new global variable palmOS50 as an indicator for OS 5.0 (this is neccessary
  to do the proper coordinate handling for the result display)

- gadget_bounds() contains adjustments for high resolution display on Sony Clie

- StartApplication() contains additional initialization code for Sony Clie HR mode

graph.c

- A number of Win..() calls where replaced with clie_..() calls (see clie.c), which
  handle high resolution modes as well
  
grtaps.c

- A number of Win..() calls where replaced with clie_..() calls (see clie.c), which
  handle high resolution modes as well

result.c

- A number of Win..() calls where replaced with clie_..() calls (see clie.c), which
  handle high resolution modes as well
