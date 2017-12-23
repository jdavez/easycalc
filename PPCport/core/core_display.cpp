/*****************************************************************************
 * EasyCalc -- a scientific calculator
 * Copyright (C) 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * The name and many features come from
 * - EasyCalc on Palm:
 *
 * It also is reusing elements from
 * - Free42:  Thomas Okken
 * for its adaptation to the PocketPC world.
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
/* core_display.cpp : procedures for painting the calculator screen.
 *****************************************************************************/

#include "stdafx.h"

#include "compat/PalmOS.h"
#include "core/core_globals.h"
#include "core/main.h"
#include "core/mlib/display.h"
#include "core/mlib/stack.h"
#include "core/ansops.h"
#include "core/mlib/fp.h"
#include "core/mlib/funcs.h"
#include "core/Main.h"
#include "Skin.h"
#include "EasyCalc.h"
#define _CORE_DISPLAY_C_
#include "core_display.h"
#include "Easycalc.h"


/*-------------------------------------------------------------------------------
 - Constants.                                                                   -
 -------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------
 - Type declarations.                                                           -
 -------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------
 - Global variables.                                                            -
 -------------------------------------------------------------------------------*/
/* Menu strings to be translated */
TCHAR *resMenuDesc[] =
                  {_T("$$MNCOPY"),
                   _T("$$MNSAVE AS"),
                   _T("$$MNDATAMGR"),
                   _T("$$MNGUESSIT"),
                   _T("$$MNDEGREE"),
                   _T("$$MNRADIAN"),
                   _T("$$MNCIS"),
                   _T("$$MNGON"),
                   _T("$$ENGDISPL"),
                   _T("$$SCIDISPL"),
                   _T("$$NORMDISPL"),
                   _T("$$MNDEGREE2"),
                   _T("$$MNGRAD")
                  };
/* Error strings to be translated */
TCHAR *strErrCodes[] =
                  {_T("$$ERRNO ERROR"),
                   _T("$$ERRSYNTAX ERROR"),
                   _T("$$ERRBAD FUNCTION"),
                   _T("$$ERRMISSING ARGUMENT"),
                   _T("$$ERRBAD ARGUMENT TYPE"),
                   _T("$$ERRINTERNAL ERROR"),
                   _T("$$ERRBAD MODE SELECTED"),
                   _T("$$ERRNO RESULT"),
                   _T("$$ERRDIVISION BY 0"),
                   _T("$$ERRNO SUCH VARIABLE"),
                   _T("$$ERRBAD RESULT"),
                   _T("$$ERRTOO DEEP RECURSION"),
                   _T("$$ERRIMPOSSIBLE CALCULATION"),
                   _T("$$ERROUT OF RANGE"),
                   _T("$$ERRBAD ARG COUNT"),
                   _T("$$ERRBAD DIMENSION"),
                   _T("$$ERRSINGULAR"),
                   _T("$$ERRINTERRUPTED"),
                   _T("Not enough memory"),
                   _T("Stack low")
                  };

/*-------------------------------------------------------------------------------
 - Module variables.                                                            -
 -------------------------------------------------------------------------------*/
TresultPrefs resultPrefs;


/*-------------------------------------------------------------------------------
 - Forward declarations.                                                        -
 -------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------
 - Procedures.                                                                  -
 -------------------------------------------------------------------------------*/
/********************************************************************************
 * FUNCTION: squeak()                                                           *
 * Emit a sound if audio is enabled.                                            *
 ********************************************************************************/
void squeak() {
    if (flags.f.audio_enable)   shell_beeper(1835, 125);
}


/***********************************************************************
 *
 * FUNCTION:     result_popup
 *
 * DESCRIPTION:  Popup a list with operations related to the result field
 *
 * PARAMETERS:   hWnd is a pointer at an OS specific structure describing
 *               the result popup list object.
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
resSelection respopup_choices[SELECTION_COUNT];
void result_popup (Skin *skin, void *hWnd) {
    int count = 0;

    // Create the list of actions function of the result type
    respopup_choices[count++] = COPYRESULT;
    respopup_choices[count++] = DEFMGR;
    if (resultPrefs.ansType != notype) {
        respopup_choices[count++] = VARSAVEAS;
        if ((resultPrefs.ansType == real) ||
            (resultPrefs.ansType == complex)) {
            respopup_choices[count++] = GUESSIT;
            // Mapi: provide eng / sci / norm conversions all time
            //if (dispPrefs.mode != disp_eng)
                respopup_choices[count++] = ENGDISPLAY;
            //if (dispPrefs.mode != disp_sci)
                respopup_choices[count++] = SCIDISPLAY;
            //if (dispPrefs.mode != disp_normal)
                respopup_choices[count++] = NORMDISPLAY;
            // Mapi: provide trigo conversion functions all time
            //if (calcPrefs.trigo_mode == degree) {
                respopup_choices[count++] = TODEGREE;
                respopup_choices[count++] = TODEGREE2;
            //} else if (calcPrefs.trigo_mode == radian)
                respopup_choices[count++] = TORADIAN;
            respopup_choices[count++] = TOGRAD;
        }
        if (resultPrefs.ansType == complex) {
            respopup_choices[count++] = TOGONIO;
            respopup_choices[count++] = TOCIS;
        }
    }

    // Display the popup contents to get action
    skin->resultActionsPopup(respopup_choices, count, hWnd);
}

/***********************************************************************
 *
 * FUNCTION:     result_action
 *
 * DESCRIPTION:  Execute action from the result popup dialog.
 *
 * PARAMETERS:   hWnd_p     OS specific handle to the dialog window
 *               hWnd_calc  OS specific handle to the calculator window
 *
 * RETURN:       None
 *
 ***********************************************************************/
void result_action(Skin *skin, void *hWnd_p, void *hWnd_calc, int selection) {
    // Execute the action
    switch (respopup_choices[selection]) {
    case COPYRESULT:
        result_copy(skin);
        break;
    case VARSAVEAS:
        FrmPopupForm(varEntryForm, hWnd_p);
        break;
    case DEFMGR:
        FrmPopupForm(defForm, hWnd_p);
        break;
    case TODEGREE:
        ans_redisplay(skin, hWnd_calc, _T("todeg(ans)"));
        break;
    case TODEGREE2:
        ans_redisplay(skin, hWnd_calc, _T("todeg2(ans)"));
        break;
    case GUESSIT:
        ans_guess(skin, hWnd_calc);
        break;
    case TOGONIO:
        ans_redisplay(skin, hWnd_calc, _T("togonio(ans)"));
        break;
    case TOCIS:
        ans_redisplay(skin, hWnd_calc, _T("tocis(ans)"));
        break;
    case TORADIAN:
        ans_redisplay(skin, hWnd_calc, _T("torad(ans)"));
        break;
    case TOGRAD:
        ans_redisplay(skin, hWnd_calc, _T("tograd(ans)"));
        break;
    case ENGDISPLAY:
        {
        Tdisp_mode oldmode;
        oldmode = fp_set_dispmode(disp_eng);
        ans_redisplay(skin, hWnd_calc, _T("ans"));
        fp_set_dispmode(oldmode);
        }
        break;
    case SCIDISPLAY:
        {
        Tdisp_mode oldmode;
        oldmode = fp_set_dispmode(disp_sci);
        ans_redisplay(skin, hWnd_calc, _T("ans"));
        fp_set_dispmode(oldmode);
        }
        break;
    case NORMDISPLAY:
        {
        Tdisp_mode oldmode;
        oldmode = fp_set_dispmode(disp_normal);
        ans_redisplay(skin, hWnd_calc, _T("ans"));
        fp_set_dispmode(oldmode);
        }
        break;
    }
}


/***********************************************************************
 *
 * FUNCTION:    result_copy
 *
 * DESCRIPTION: Copy contents of Result field to clipboard
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 ***********************************************************************/
void result_copy(Skin *skin) {
    skin->clipCopy_result();
}


/***********************************************************************
 *
 * FUNCTION:     result_print
 *
 * DESCRIPTION:  Prints a given text on display
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *
 ***********************************************************************/
static void result_print(Skin *skin, void *hWnd_p, TCHAR *text) {
    skin->print_result(hWnd_p, text);
}


/***********************************************************************
 *
 * FUNCTION:     result_print_pow
 *
 * DESCRIPTION:  Formats a a^b in a nice way on the display.
 *               Note from Mapi = completely rewritten in Skin. It now sends
 *               pieces for output from right to left, or left to right
 *               depending on scroll, using recursivity.
 *
 * PARAMETERS:   text to display, should contain a '^' character. If not
 *               it will revert to normal print_result().
 *
 * RETURN:       None
 *
 ***********************************************************************/
static void result_print_pow(Skin *skin, void *hWnd_p, TCHAR *text) {
    skin->print_resultpow(hWnd_p, text);
}


/***********************************************************************
 *
 * FUNCTION:     result_print_num
 *
 * DESCRIPTION:  Formats a generic real/int number, adds
 *               spacing to display e.g. 1111 1111, or 100.222,333 444
 *
 * PARAMETERS:   text to display
 *
 * RETURN:       None
 *
 ***********************************************************************/
static void result_print_num(Skin *skin, void *hWnd_p, TCHAR *text) {
    TCHAR *result;
    Int16  spacing;
    Int16  i,j,k;
    Int16  endint;

    if (resultPrefs.dispBase == disp_octal ||
        (resultPrefs.dispBase != disp_hexa && _tcschr(text, _T('E'))) ||
        _tcschr(text, _T('r'))) {
        result_print(skin, hWnd_p, text);
        return;
    }
    if (resultPrefs.dispBase == disp_decimal)
        spacing = 3;
    else
        spacing = 4;

    result = (TCHAR *) MemPtrNew(_tcslen(text)*sizeof(TCHAR)*2); // Twice the number of bytes
    k = 0;

    /* Handle the negative sign */
    if (text[0] == _T('-')) {
        result[k++] = text[0];
        text++;
    }

    /* Find postion of decimal point */
    for (i=0;text[i];i++)
        if ((text[i] == _T('.')) || (text[i] == _T(',')))
            break;

    /* In engineer notation it may end with something other than
     * number */
    if ((text[i-1] > _T('9')) && ((text[i-1] < _T('A')) || (text[i-1] > _T('F'))))
        endint = i-1;
    else
        endint = i;
    /* Handle the integer part of text */
    for (j=0;j < endint;j++) {
        if (j>0 && ((endint-j) % spacing) == 0)
            result[k++] = flSpaceChar;
        result[k++] = text[j];
    }
    /* Move the engineering suffix, if exists */
    if (endint != i)
        result[k++] = text[endint];
    /* Move decimal point */
    result[k++] = text[i];

    /* Handle the after-dot part of number */
    if (text[i] != _T('\0')) { /* It was not an integer num */
        for (j=i+1;text[j];j++) {
            if ((j-i-1) != 0 && (j-i-1) % spacing == 0)
                result[k++] = ' ';
            result[k++] = text[j];
        }
        /* Handle the case, when there would be
         * a unit preceded by space and */
        if ((result[k-2] == _T(' ')) && (result[k-1] > _T('9'))) {
            result[k-2] = result[k-1];
            k--;
        }
        result[k++] = _T('\0');
    }
    result_print(skin, hWnd_p, result);
    MemPtrFree(result);
}


/***********************************************************************
 *
 * FUNCTION:     result_set_text
 *
 * DESCRIPTION:  set text result on display
 *
 * PARAMETERS:   text - text to display
 *               type - type of the displayed text
 *
 * RETURN:       None
 *
 ***********************************************************************/
void result_set_text(Skin *skin, void *hWnd_p, TCHAR *text, rpntype type) {
    if (type != notype)
        resultPrefs.ansType = type;
    resultPrefs.formatType = plaintext;

    result_print(skin, hWnd_p, text);
}


/***********************************************************************
 *
 * FUNCTION:     result_set_pow
 *
 * DESCRIPTION:  set formatted text on display,
 *               DOESN'T CHANGE ANSTYPE!
 *
 * PARAMETERS:   text - text to display
 *
 * RETURN:       None
 *
 ***********************************************************************/
void result_set_pow(Skin *skin, void *hWnd_p, TCHAR *text) {
    resultPrefs.formatType = powtext;
    result_print_pow(skin, hWnd_p, text);
}


/********************************************************************************
 * Display error text in the display area.                                      *
 ********************************************************************************/
void result_error(Skin *skin, void *hWnd_p, CError errcode) {
    TCHAR *text;

    text = print_error(errcode);
    result_set_text(skin, hWnd_p, text, notype);
    MemPtrFree(text);
}

/********************************************************************************
 * Print an application error to memory.                                        *
 ********************************************************************************/
TCHAR *print_error(CError err) {
    TCHAR *res;

    res = (TCHAR *) MemPtrNew(MAX_RSCLEN*sizeof(TCHAR));
    _tcsncpy (res, libLang->translate(strErrCodes[err]), MAX_RSCLEN-1);
    res[MAX_RSCLEN-1] = 0;
    return res;
}

/***********************************************************************
 *
 * FUNCTION:     result_set
 *
 * DESCRIPTION:  set new result to display, automatically convert
 *               given item
 *
 * PARAMETERS:   Trpn item - the object to display
 *
 * RETURN:       None
 *
 ***********************************************************************/
void result_set(Skin *skin, void *hWnd_p, Trpn item) {
    TCHAR  *text;
    CError  err;
    Boolean freeitem = false;

    /* Evaluate variables */
    if (item.type==variable) {
        err = rpn_eval_variable(&item, item);
        if (err) {
            result_error(skin, hWnd_p, err);
            return;
        }
        freeitem = true;
    }

    /* Do not display complete matrices */
    // Mapi: the type of value can change, especially with things like matrix[x:.]
    // which create a list.
    rpntype ansType;
    if ((((item.type == matrix) || (item.type == cmatrix))
         && (item.u.matrixval->cols * item.u.matrixval->rows > 9))
        || ((item.type == list) && (item.u.listval->size > 6)))
        text = display_default(item, false, &ansType);
    else
        text = display_default(item, true, &ansType);
//    resultPrefs.ansType = item.type;
    resultPrefs.ansType = ansType;
    resultPrefs.dispBase = dispPrefs.base;

    if ((item.type == integer) || (item.type == real)) {
        resultPrefs.formatType = fmtnumber;
        result_print_num(skin, hWnd_p, text);
    } else {
        resultPrefs.formatType = plaintext;
        result_print(skin, hWnd_p, text);
    }

    MemPtrFree(text);
    if (freeitem)
        rpn_delete(item);
}


/***********************************************************************
 *
 * FUNCTION:     input_exec
 *
 * DESCRIPTION:  Execute contents of input line and show the result
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       err - Possible error that occurred during computation
 *
 ***********************************************************************/
int input_exec (Skin *skin, void *hWnd_p) {
    TCHAR *inp;
    CError err;
    Trpn result;

    inp = skin->get_input_text();
    err = main_input_exec(inp, &result);
    if (!err) {
        result_set(skin, hWnd_p, result);
        err=set_ans_var(result);
        rpn_delete(result);
        skin->select_input_text(NULL);
    } else {
        result_error(skin, hWnd_p, err);
    }

    return (err);
}