/*
 *   $Id: mtxedit.cpp,v 1.2 2009/12/15 21:36:54 mapibid Exp $
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
 *   This editor works automatically with complex matrices and upon
 *   save tries to convert them to the real ones.
*/

#include "stdafx.h"

#include "compat/PalmOS.h"
//#include <LstGlue.h>
//#include <WinGlue.h>

//#include "calcrsc.h"
#include "konvert.h"
#include "calc.h"
#include "defuns.h"
#include "compat/segment.h"
#include "mtxedit.h"
#include "matrix.h"
#include "cmatrix.h"
#include "calcDB.h"
#include "stack.h"
#include "display.h"
#include "varmgr.h"
#include "matrix.h"
#include "system - UI/StateManager.h"
#include "system - UI/EasyCalc.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

#define _MTXEDIT_C_
TCHAR mtxedit_matrixName[MAX_FUNCNAME+1];

static CMatrix *matrixValue = NULL;
static TCHAR matrixDim[MATRIX_MAX][4];
static Int16 rowPosition, colPosition;

/***********************************************************************
 *
 * FUNCTION:     mtxedit_tbl_load
 *
 * DESCRIPTION:  Function for drawing matrix table on screen
 *
 * PARAMETERS:   
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void mtxedit_tbl_load (void) IFACE;
void mtxedit_tbl_load (void) {
    if (matrixValue == NULL)
        return;

    int rows = matrixValue->rows;
    int cols = matrixValue->cols;
    TCHAR *text;
    for (int i=0 ; i<rows ; i++) {
        MtxEditSetRowLabel(i, i+1);
        for (int j=0 ; j<cols ; j++) {
            text = display_complex(MATRIX(matrixValue, i, j));
            MtxEditSetRowValue(i, j+1, text);
            MemPtrFree(text);
        }
    }
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_get_item
 *
 * DESCRIPTION:  (Mapi) Get item value from a matrix.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError mtxedit_get_item (Complex *cplx, int row, int col) IFACE;
CError mtxedit_get_item (Complex *cplx, int row, int col) {
    CError err = c_noerror;

    if ((row > matrixValue->rows)
        || (col > matrixValue->cols))
        return (c_range); // Security ..
    *cplx = MATRIX(matrixValue, row, col);

    return (err);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_replace_item
 *
 * DESCRIPTION:  (Mapi) Replace an item value in a matrix.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError mtxedit_replace_item (Complex *cplx, int row, int col) IFACE;
CError mtxedit_replace_item (Complex *cplx, int row, int col) {
    CError err = c_noerror;

    if ((row > matrixValue->rows)
        || (col > matrixValue->cols))
        return (c_range); // Security ..
    MATRIX(matrixValue, row, col) = *cplx;

    mtxedit_save_matrix();

    return (err);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_item_length
 *
 * DESCRIPTION:  (Mapi) Retrieve display length of an item.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
int mtxedit_item_length (int row, int col) IFACE;
int mtxedit_item_length (int row, int col) {
    TCHAR *text = display_complex(MATRIX(matrixValue, row, col));
    int len = _tcslen(text);
    MemPtrFree(text);
    return (len);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_save_matrix
 *
 * DESCRIPTION:  Save the loaded matrix to database
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void mtxedit_save_matrix (void) IFACE;
void mtxedit_save_matrix (void) {
    Trpn item;
    CodeStack *stack;
    Matrix *m;

    stack = stack_new(1);
    if (StrLen(mtxedit_matrixName)) {
        /* Try to scale down to normal matrix */
        m = cmatrix_to_matrix(matrixValue);
        if (m) {
            stack_add_val(stack, &m, matrix);
            matrix_delete(m);
        } else
            stack_add_val(stack, &matrixValue, cmatrix);
        item = stack_pop(stack);
        db_write_variable(mtxedit_matrixName, item);
        rpn_delete(item);
    }
    stack_delete(stack);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_load_matrix
 *
 * DESCRIPTION:  Load the matrix specified in mtxedit_matrixName to
 *               variable matrixValue. If it doesn't exist, set
 *               mtxedit_matrixName to ""
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void mtxedit_load_matrix (int *nb_rows, int *nb_cols) IFACE;
void mtxedit_load_matrix (int *nb_rows, int *nb_cols) {
    Trpn item;
    CError err;

    if (matrixValue != NULL) {
        cmatrix_delete(matrixValue);
        matrixValue = NULL;
    }

    if (StrLen(mtxedit_matrixName) == 0) {
        *nb_cols = *nb_rows = 0;
        return;
    }

    item = db_read_variable(mtxedit_matrixName, &err);
    if (err) {
        StrCopy(mtxedit_matrixName, _T(""));
        *nb_cols = *nb_rows = 0;
        return;
    }

    if (item.type == matrix)
        matrixValue = matrix_to_cmatrix(item.u.matrixval);
    else if (item.type == cmatrix)
        matrixValue = cmatrix_dup(item.u.cmatrixval);
    else {
        StrCopy(mtxedit_matrixName, _T(""));
        rpn_delete(item);
        *nb_cols = *nb_rows = 0;
        return;
    }
    rpn_delete(item);
    LstEditSetLabel(MATRIX_ID, mtxedit_matrixName);

    *nb_cols = matrixValue->cols;
    *nb_rows = matrixValue->rows;
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_init
 *
 * DESCRIPTION:  Initialize the matrix editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void mtxedit_init (Boolean showAns) IFACE;
void mtxedit_init (Boolean showAns) {
    if (showAns) {
        StrCopy(mtxedit_matrixName, _T("ans"));
    } else {
        if (stateMgr->state.matrixName[0] != _T('\0'))
            StrCopy(mtxedit_matrixName, stateMgr->state.matrixName);
        else
            StrCopy(mtxedit_matrixName, _T(""));
    }
    // Initialize the matrix name combo
    mtxedit_select();
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_new
 *
 * DESCRIPTION:  Create a new matrix in the matrix editor
 *
 * PARAMETERS:   size of the matrix
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void mtxedit_new (int rows, int cols) IFACE;
void mtxedit_new (int rows, int cols) {
    if (matrixValue != NULL)
        cmatrix_delete(matrixValue);
    matrixValue = cmatrix_new(1,1);
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_destroy
 *
 * DESCRIPTION:  Destructor for matrix editor
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void mtxedit_destroy (void) IFACE;
void mtxedit_destroy (void) {
    StrCopy(stateMgr->state.matrixName, mtxedit_matrixName);
    if (matrixValue != NULL) {
        cmatrix_delete(matrixValue);
        matrixValue = NULL;
    }
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_redim
 *
 * DESCRIPTION:  Handle when user selects a new size of matrix
 *               (reallocate data etc.)
 *
 * PARAMETERS:   
 *
 * RETURN:       
 *
 ***********************************************************************/
void mtxedit_redim (int rows, int cols) IFACE;
void mtxedit_redim (int rows, int cols) {
    if (matrixValue == NULL)
        return;

    int oldrows = matrixValue->rows;
    int oldcols = matrixValue->cols;

    CMatrix *nm = cmatrix_new(rows, cols);
    if (!nm)
        return;

    for (int i=0 ; (i<oldrows)&&(i<rows) ; i++)
        for (int j=0 ; (j<oldcols)&&(j<cols) ; j++)
            MATRIX(nm, i, j) = MATRIX(matrixValue, i, j);
    cmatrix_delete(matrixValue);
    matrixValue = nm;
}

/***********************************************************************
 *
 * FUNCTION:     mtxedit_select
 *
 * DESCRIPTION:  Handle the user tap on the matrix selector
 *               allows creating new matrix, deleting etc.
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       true - new matrix loaded
 *               false - no change
 *
 ***********************************************************************/
void mtxedit_select (void) IFACE;
void mtxedit_select (void) {
    dbList *matrices, *cmatrices;
    TCHAR **values;
    Int16   i, j, k, l, compsize;

    matrices = db_get_list(matrix);
    cmatrices = db_get_list(cmatrix);

    compsize = matrices->size + cmatrices->size;
    if (compsize)
        values = (TCHAR **) MemPtrNew(sizeof(*values) * compsize);
    else
        values = NULL;

    for (j=0 ; j<matrices->size ; j++)
        values[j] = matrices->list[j];
    /* Insert sort cmatrices */
    i = j;
    for (j=0 ; j<cmatrices->size ; j++,i++) {
        for (k=0 ; (k<i) && (StrCompare(cmatrices->list[j],values[k])>0) ; k++)
            ;
        for (l=i ; l>k ; l--)
            values[l] = values[l-1];
        values[k] = cmatrices->list[j];
    }

    LstEditSetListChoices(MATRIX_ID, values, compsize);
    LstEditSetLabel(MATRIX_ID, mtxedit_matrixName);

    /* Set incremental search */
    // No equivalent in Windows (in fact, this is done natively, but I don't know
    // how many chars .. (up to 5 in the Glue API).
//    LstGlueSetIncrementalSearch(slst, true);

    if (values)
        MemPtrFree(values);
    db_delete_list(matrices);
    db_delete_list(cmatrices);
}
