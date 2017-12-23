#pragma once
#if defined(POCKETPC2003_UI_MODEL) || defined(STANDARDSHELL_UI_MODEL) || defined (SMARTPHONE2003_UI_MODEL)
#include "resourceppc.h"
#endif

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
#ifndef EASYCALC_H
#define EASYCALC_H 1


#ifndef BCD_MATH
#include <math.h>
#endif

#define SHELL1_SECT
#define SHELL2_SECT
#define FILESYS_SECT
#define MAIN_SECT
#define COMMANDS1_SECT
#define COMMANDS2_SECT
#define COMMANDS3_SECT
#define COMMANDS4_SECT
#define COMMANDS5_SECT
#define COMMANDS6_SECT
#define DISPLAY_SECT
#define GLOBALS_SECT
#define HELPERS_SECT
#define KEYDOWN_SECT
#define LINALG1_SECT
#define LINALG2_SECT
#define MATH1_SECT
#define MATH2_SECT
#define PHLOAT_SECT
#define STO_RCL_SECT
#define TABLES_SECT
#define VARIABLES_SECT
#define BCD_SECT

#if defined(_WINDOWS) && !defined(__GNUC__)
#define int8 __int64
#define uint8 unsigned __int64
#define LL(x) x

#else

#define int8 long long
#define uint8 unsigned long long
#define LL(x) x##LL

/* NOTE: In my Linux build, all I have to do is DECLARE sincos(); glibc 2.3.3
 * has it (for C99, I suppose) so I don't have to DEFINE it. On other Unixes
 * (e.g. MacOS X), it may not be provided by the standard libraries; in this
 * case, define the NO_SINCOS symbol, here or in the Makefile.
 * For the Palm build, we don't even need the declaration, since sincos() is
 * provided by MathLib.
 */
#ifndef PALMOS
extern "C" void sincos(double x, double *sinx, double *cosx) HELPERS_SECT;
#endif
//#define NO_SINCOS 1

#endif

// Version of the state file
#define STATE_VERSION_ORIG 1
#define STATE_VERSION      5

/* Magic number and version number for the state file.
 */
#define EASYCALC_MAGIC 0x4543414C
#define EASYCALC_VERSION 1

/* Forms .. */
#define varEntryForm        1
#define defForm             2
#define altAnsProblem       3
#define altGuessNotFound    4
#define altBadVariableName  5
#define altConfirmDelete    6
#define altConfirmOverwrite 7
#define altCompute          8
#define altBadParameter     9
#define altNoStateFile      10
#define altNoLayoutFile     11
#define altErrorGifFile     12
#define altWriteStateFile   13
#define altSolverResult     14
#define altGraphBadVal      15
#define altMemoImportError  16
#define altBadGifFile       17
#define altMemAllocError    18
#define altCalcNeedTrackPt  19
#define altCalcNeedCurveSel 20
#define altCalcNeedSelect   21
#define altComputeResult    22
#define altGrcFuncErr       23
#define FrmAlert FrmPopupForm
#define FrmAlertMain(a) FrmPopupForm(a, NULL)
int FrmPopupForm(int formNb, void *hWnd_p);
int FrmCustomAlert(int formNb, const TCHAR *s1, const TCHAR *s2, const TCHAR *s3, void *hWnd_p);

#include "compat/PalmOS.h"
// Structure for talking with the ask()panel.
typedef struct {
    TCHAR *asktxt;
    TCHAR *defaultvalue;
    TCHAR *answertxt;
} t_asktxt_param;

Boolean popupAskTxt(t_asktxt_param *param);

#ifndef _EASYCALC_C_
#include "StateManager.h"
#include "compat/Lang.h"
extern LibLang      *libLang;
extern int           g_systUserLangId;       // Detected language of the system user
extern HWND          g_hWnd;                 // current main window handle
extern HWND          g_hwndE;                // Handle to the edit control input area.
extern void         *g_cedit2_obj;           // Input area subclassing object.
extern StateManager *stateMgr;
extern int           ann_state[NB_ANNUN];    // Annunciators state
#endif

#include "core/mlib/konvert.h"
void *subclassEdit(HWND hWnd_p, HWND hWndE);
void unsubclassEdit(HWND hWndE, void *cedit2_obj);
void read_state (void);
void save_state (void);
void shell_graphAction (int key);
void shell_beeper(int frequency, int duration);
void shell_annunciators(int skin, int shf, int angle_mode, int int_mode, int pen_mode, int run);
void shell_scroll_annunciators(int scroll);
void shell_powerdown();
void alertErrorMessage(CError err);
void ErrFatalDisplayIf(int cond, TCHAR *msg);

void shell_clBracket(void);

void LstEditSetLabel(int listNb, TCHAR *label);
void LstEditSetRow(int rowNb, int value, TCHAR *cell1, TCHAR *cell2, TCHAR *cell3);
void LstEditSetListChoices(int listNb, TCHAR **values, int size);
void LstEditDeselect(void);

void MtxEditSetRowLabel(int rowNb, int value);
void MtxEditSetRowValue(int rowNb, int colNb, TCHAR *cell);

void SlvEnableAll(Boolean enable);
void SlvSetExeButton(int cmd);
void SlvEqFldSet(TCHAR *eq);
TCHAR *SlvEqFldGet(void);
void SlvVarSetRow(int rowNb, TCHAR *label, TCHAR *value);
void SlvVarHelp(TCHAR *text);

void FinSetRow(int rowNb, TCHAR *label, TCHAR *value);
void FinShowValue(int rowNb, TCHAR *value);

void GraphConfSetRow (int rowNb, TCHAR *celltext);

void wait_draw(void);
void wait_erase(void);

#endif
