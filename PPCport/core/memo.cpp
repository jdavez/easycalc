/*
 *   $Id: memo.cpp,v 1.1 2011/02/28 22:07:18 mapibid Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 2000 Ondrej Palkovsky
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

#include "stdafx.h"
#include "compat/PalmOS.h"
#include <string.h>

#include "konvert.h"
#include "calcDB.h"
#include "display.h"
#include "memo.h"
#include "stack.h"
//#include "calcrsc.h"
#include "calc.h"
#include "prefs.h"
#include "funcs.h"
//#include "result.h"
#include "solver.h"

#include <stdlib.h>
#include "core_display.h"
#include "system - UI/EasyCalc.h"

#define EQ_SEPARATOR _T("--------------------")

/***********************************************************************
 *
 * FUNCTION:     strslashcat
 *
 * DESCRIPTION:  Make the string 'quote-safe'
 *
 * PARAMETERS:   str1 - destination string
 *               str2 - source string
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
static void strslashcat(TCHAR *str1, TCHAR *str2) {
    str1+=StrLen(str1);
    while ((*str1++ = *str2++))
        if ((*str2 == _T('\"')) || (*str2 == _T('\\')))
            *(str1++) = _T('\\');
}

static TCHAR *memo_dump_item (dbList *list, Int16 i) {
    TCHAR *dump = NULL;
    TCHAR *text;
    CError err;
    Trpn   item;
    TCHAR  parameter[MAX_FUNCNAME+1];

    if (list->type[i] == function) {
        err = db_func_description(list->list[i], &text, parameter);
        if (err)
            return NULL;
        dump = (TCHAR *) MemPtrNew((StrLen(text)+StrLen(list->list[i])+30) * sizeof(TCHAR));
        dump[0] = _T('\0');
        /* Define the parameter */
        StrCat(dump, _T("defparamn(\""));
        StrCat(dump, parameter);
//#ifdef _WINDOWS
//        StrCat(dump, _T("\")\r\n"));
//#else
        StrCat(dump, _T("\")\n"));
//#endif
        StrCat(dump, list->list[i]);
        StrCat(dump, _T("()=\""));
        strslashcat(dump, text);
//#ifdef _WINDOWS
//        StrCat(dump, _T("\"\r\n"));
//#else
        StrCat(dump, _T("\"\n"));
//#endif
        MemPtrFree(text);
    } /* Do not export matrices for now */
    else if (list->type[i]==variable) {
        item = db_read_variable(list->list[i], &err);
        if (err)
            return NULL;
        /* Do not work with strings now */
        if (item.type == string) {
            rpn_delete(item);
            return NULL;
        }
        // Mapi: let's not export function parameter ...
        if (StrCompare(list->list[i], parameter_name) == 0) {
            /* Special - parameter of a function */
            rpn_delete(item);
            return NULL;
        }

        text = display_default(item, true, NULL);
        dump = (TCHAR *) MemPtrNew((StrLen(text)+StrLen(list->list[i])+5) * sizeof(TCHAR));
        StrCopy(dump, list->list[i]);
        StrCat(dump, _T("="));
        StrCat(dump, text);
//#ifdef _WINDOWS
//        StrCat(dump, _T("\r\n"));
//#else
        StrCat(dump, _T("\n"));
//#endif
        rpn_delete(item);
        MemPtrFree(text);
    }
    return dump;
}

static void memo_new_record (FILE *f, TCHAR *text) {
//#ifdef _WINDOWS
//    TCHAR *header = _T("EasyCalc data structures\r\n");
//#else
    TCHAR *header = _T("EasyCalc data structures\n");
//#endif

    if (text == NULL) {
        _fputts(header, f);
    } else {
        _fputts(text, f);
    }
}

/***********************************************************************
 *
 * FUNCTION:     memo_dump_db
 *
 * DESCRIPTION:  Create a string dump of an EasyCalc's database
 *               containing definitions of variables and functions
 *
 * PARAMETERS:   type - function,variable or all
 *
 * RETURN:       pointer to a newly allocated memo-dump
 *
 ***********************************************************************/
static void memo_dump_db (FILE *f, rpntype type) {
    TCHAR  *text;
    dbList *list;
    Int16   i;
    TCHAR  *dump;

    dump = (TCHAR *) MemPtrNew((MAX_DUMP+20)*sizeof(TCHAR));
    StrCopy(dump, _T(""));

    list = db_get_list(type);

    // Start recording
    memo_new_record(f, NULL);
    /* Dump the list */
    for (i=0 ; i<list->size ; i++) {
        text = memo_dump_item(list, i);
        if (!text)
            continue;
        if (StrLen(text) > MAX_DUMP) {
            MemPtrFree(text);
            continue;
        }
        if (StrLen(dump)+StrLen(text) > MAX_DUMP) {
            if (type != variable)
//#ifdef _WINDOWS
//                StrCat(dump, _T("defparamn(\"x\")\r\n"));
//#else
                StrCat(dump, _T("defparamn(\"x\")\n"));
//#endif
            memo_new_record(f, dump);
            StrCopy(dump, _T(""));
        }
        StrCat(dump, text);
        MemPtrFree(text);
    }
    if (type != variable)
//#ifdef _WINDOWS
//        StrCat(dump, _T("defparamn(\"x\")\r\n"));
//#else
        StrCat(dump, _T("defparamn(\"x\")\n"));
//#endif
    memo_new_record(f, dump);

    db_delete_list(list);
    MemPtrFree(dump);
}

/***********************************************************************
 *
 * FUNCTION:     memo_dump
 *
 * DESCRIPTION:  Dump an EasyCalc db to file
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void memo_dump (rpntype type, const TCHAR *filename, Boolean unicode) {
    FILE *f;

    if (unicode)
        f = _tfopen(filename, _T("wb")); // Write unicode as is
    else
        f = _tfopen(filename, _T("w")); // Write unicode as MBCS
    if (f == NULL)
        return;

    if (type == EQUATIONS) {
        slv_export_memo(f, EQ_SEPARATOR);
    } else {
        memo_dump_db(f, type);
    }

    fclose(f);
}

static Boolean memo_execute(Skin *skin, void *hWnd_p, TCHAR *input) {
    int i, j;
    CodeStack *stack;
    CError err;
    Trpn result;

    i = 0;
    while (input[i]) {
        for (j=i ; input[j]!=_T('\n') && input[j] ; j++)
            ;
        input[j] = _T('\0');
        if (input[j-1] == _T('\r'))
            input[j-1] = _T('\0');

        if (StrLen(input+i) == 0)
            break;
        stack = text_to_stack(input+i, &err);
        if (!err) {
            err = stack_compute(stack);
            if (!err) {
                result = stack_pop(stack);
                set_ans_var(result);
                result_set(skin, hWnd_p, result);
                rpn_delete(result);
            }
            stack_delete(stack);
        }
        if (err)
            return false;
        i = j+1;
    }
    return true;
}

/***********************************************************************
 *
 * FUNCTION:     memo_import_memo
 *
 * DESCRIPTION:  Executes a selected memo into easycalc
 *
 * PARAMETERS:   filename - the file selected by user
 *
 * RETURN:       true - operation ok
 *               false - error during execution
 *
 ***********************************************************************/
Boolean memo_import_memo (Skin *skin, void *hWnd_p, const TCHAR *filename, Boolean unicode) {
    TCHAR *input;
    char  *mbcs_input = NULL;
    int    memolen, i=0;
    Boolean start = true;
    Boolean res;
#ifdef SPECFUN_ENABLED
    TCHAR  *next;
    Boolean is_equation = false;
#endif


    FILE *f = _tfopen(filename, _T("rb")); // Read unicode and MBCS as is
    if (f == NULL) {
        return (false);
    }

    input = (TCHAR *) MemPtrNew((MAX_DUMP+1)*sizeof(TCHAR));
    if (!unicode)
        mbcs_input = (char *) MemPtrNew((MAX_DUMP+1));
    do {
        // Read a bunch of chars when previous bunch is exhausted
        if (unicode) {
            memolen = fread(input+i, sizeof(TCHAR), MAX_DUMP-i, f);
            input[memolen] = _T('\0');
        } else {
            memolen = fread(mbcs_input+i, 1, MAX_DUMP-i, f);
            mbcs_input[memolen] = '\0';
            TCHAR const *text = libLang->convert(mbcs_input+i);
            StrCopy(input+i, text);
        }

        if (start) {
            /* Skip the first line */
            for (i=0 ; input[i]!=_T('\n') && i<memolen ; i++)
                ;
            i++;
#ifdef SPECFUN_ENABLED
            if (input[i] == _T(':'))
                is_equation = true;
#endif
            start = false;
        }

#ifdef SPECFUN_ENABLED
        if (is_equation) {
            TCHAR *temp = input;
            while ((temp != NULL) && (*temp)) {
                // Find next "--------------------"
                next = _tcsstr(temp, EQ_SEPARATOR);
                if (next != NULL) {
                    *next = _T('\0');
                    // Go to the end of EQ_SEPARATOR + '\n'
                    do {
                        next++;
                    } while (*next && (*next != _T('\n')));
                    if (*next == _T('\n'))
                        next++; // Skip '\n' also
                }
                if ((next != NULL) || feof(f)) { // If not at end of file, maybe the
                                                 // equation separator is in the next
                                                 // bunch of chars, so let's use this
                                                 // only when we have a separator or
                                                 // we are at end of file.
                    res = slv_memo_import(temp);
                    temp = next;
                } else { // No separator, and not at end of file,
                         // let's not consume that piece.
                    next = temp; 
                    temp = NULL;
                }
            }

        } else {
#endif
            // Find last '\n'
            next = _tcsrchr(input+i, _T('\n'));
            if (next != NULL) {
                *next = _T('\0');
                if (*(next-1) == _T('\r'))
                    *(next-1) = _T('\0');
                next++;
            }
            // Execute sequence till then
            res = memo_execute(skin, hWnd_p, input+i);
#ifdef SPECFUN_ENABLED
        }
#endif
        // Move unread sequence at begining of buffer, and read more chars if there are
        if (next) {
            i = memolen - (next-input);
            if (i > 0)
                memcpy(input, next, i*sizeof(TCHAR));
        } else {
            i = 0; // All consumed
        }
    } while (res && !feof(f));

    MemPtrFree(input);
    if (mbcs_input)
        MemPtrFree(mbcs_input);
    return res;
}
