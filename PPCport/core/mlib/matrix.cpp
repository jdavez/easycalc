/*
 *   $Id: matrix.cpp,v 1.2 2009/11/02 17:24:39 mapibid Exp $
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
 *  2002-07-28 - John Hodapp <bigshot@email.msn.com>
 *       Added routines QRQ and QRR for QR Factorizaton of MXN Matrix
 *  2002-08-11 - John Hodapp <bigshot@email.msn.com>
 *       Added routines QRS to solve Polynomial Ax=b for vector x
 *               using QR routine.  Requires room for x (can be set to zeros)
 *               and vector b to be appended to original matrix.
 *               x is returned embedded with original input - Format is [A|x|b].
*/

#include "stdafx.h"

#include "compat/PalmOS.h"

#include "compat/MathLib.h"
#include "compat/segment.h"
#include "konvert.h"
#include "matrix.h"
#include "stack.h"
#include "funcs.h"
#include "mathem.h"
#include "slist.h"
#include "cmatrix.h"

/***********************************************************************
 *
 * FUNCTION:     matrix_new
 *
 * DESCRIPTION:  Create a new matrix set to 0.0
 *
 * PARAMETERS:   rows, cols
 *
 * RETURN:       matrix(rows,cols)
 *
 ***********************************************************************/
Matrix *
matrix_new(Int16 rows,Int16 cols)
{
    Matrix *m;
    Int16 i,j;

    m = (Matrix *) MemPtrNew(sizeof(*m) + rows*cols*sizeof(m->item[0]));
    if (!m)
        return NULL;

    m->rows = rows;
    m->cols = cols;

    for (i=0;i < rows; i++)
        for (j=0; j< cols; j++)
            MATRIX(m,i,j) = 0.0;

    return m;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_dup
 *
 * DESCRIPTION:  Duplicate a matrix
 *
 * PARAMETERS:   m - matrix
 *
 * RETURN:       new matrix same as m
 *
 ***********************************************************************/
Matrix *
matrix_dup(Matrix *m)
{
    Matrix *newm;
    Int16 i,j;

    newm = matrix_new(m->rows,m->cols);
    if (!newm)
        return NULL;
    for (i=0;i < m->rows;i++)
        for (j=0;j<m->cols;j++)
            MATRIX(newm,i,j) = MATRIX(m,i,j);
    return newm;
}

void
matrix_delete(Matrix *m)
{
    MemPtrFree(m);
}

/***********************************************************************
 *
 * FUNCTION:     matrix_deter
 *
 * DESCRIPTION:  Compute a determinant of a matrix
 *
 * PARAMETERS:   A - matrix
 *
 * RETURN:       det(A)
 *
 ***********************************************************************/
static double matrix_deter(Matrix *A) MATFUNC;
static double
matrix_deter(Matrix *A)
{
    Int16 i, j, k, u, v;
    double e, m, y;
    double det;
    Int16 col;
    Int32 w = 0;

    col = A->cols;
    for (j = 0 ; j < col - 1 ; j++) {
        m = fabs(MATRIX(A, j, j));
        k = j;
        for (i = j + 1 ; i < col ; i++) {
            if (fabs(MATRIX(A, i, j))  > m) {
                m = fabs(MATRIX(A, i, j));
                k = i;
            }
        }
        if (m == 0)
            return 0;

        if (k != j) {
            for (i = j ; i < col ; i++) {
                y = MATRIX(A, j, i);
                MATRIX(A, j, i) = MATRIX(A, k, i);
                MATRIX(A, k, i) = y;
            }
            w = w + 1;
        }
        for (u = j + 1 ; u < col ; u++) {
            e = MATRIX(A, u, j);
            for (v = j ; v < col ; v++)
                MATRIX(A, u, v) = MATRIX(A, u, v) - e * (MATRIX(A, j, v) / MATRIX(A, j, j));
        }
    }
    if (MATRIX(A, (col - 1), (col - 1)) == 0)
        return 0;

    det = MATRIX(A, 0, 0);
    for (i = 1; i < col ; i++)
        det = det * MATRIX(A, i, i);

    if (w % 2)
        det = -det;

    return det;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_inverse
 *
 * DESCRIPTION:  Create an inverse matrix
 *
 * PARAMETERS:   A - matrix
 *
 * RETURN:       A^-1
 *               NULL - matrix is singular, inverse doesn't exist
 *
 ***********************************************************************/
static Matrix * matrix_inverse(Matrix *A) MATFUNC;
static Matrix *
matrix_inverse(Matrix *A)
{
    Int16 d;
    Int16 col, i, j, k, l, u=0;
    double p=0, e, y;
    Matrix *W,*AI;

    d = A->cols;
    W = matrix_new(d,2*d);

    col = 2 * d;

    for (i = 0 ; i < d ; i++) {
        for (j = 0 ; j < d ; j++)
            MATRIX(W, i, j) = MATRIX(A,i,j);
    }

    for (i = 0 ; i < d ; i++)
        MATRIX(W, i, d + i) = 1;

    for (k = 0 ; k < d ; k++) {
        for (i = k ; i < d ; i++) {
            if (MATRIX(W, i, k) == 0)
                p = 0;
            else {
                p = MATRIX(W, i, k);
                u = i;
                break;
            }
        }
        if (p == 0) {
            matrix_delete(W);
            return NULL;
        } else {
            for (i = 0 ; i < col ; i++) {
                y = MATRIX(W, k, i);
                MATRIX(W, k, i) = MATRIX(W, u, i);
                MATRIX(W, u, i) = y;
            }
        }
        if (p != 1) {
            for (i = k ; i < col ; i++)
                MATRIX(W, k, i) = MATRIX(W, k, i) / p;
        }
        for (l = k + 1 ; l < d ; l++) {
            e = MATRIX(W, l, k);
            for (i = k ; i < col ; i++)
                MATRIX(W, l, i) = MATRIX(W, l, i) - e * MATRIX(W, k, i);
        }
    }

    for (k = d - 1 ; k >= 1; k--) {
        for (i = k - 1 ; i >= 0 ; i--) {
            e = MATRIX(W, i, k);
            for (l = k ; l < col ; l++)
                MATRIX(W, i, l) = MATRIX(W, i, l) - e * MATRIX(W, k, l);
        }
    }

    AI = matrix_new(A->cols,A->cols);
    for (i = 0 ; i < d ; i++) {
        for (j = 0 ; j < d ; j++)
            MATRIX(AI,i,j) = MATRIX(W, i, j + d);
    }

    matrix_delete(W);
    return AI;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_prod
 *
 * DESCRIPTION:  multiply 2 matrices
 *
 * PARAMETERS:   a,b - matrices
 *
 * RETURN:       a*b
 *
 ***********************************************************************/
static Matrix * matrix_prod(Matrix *a, Matrix *b) MATFUNC;
static Matrix *
matrix_prod(Matrix *a, Matrix *b)
{
    Matrix *m;
    Int16 i,j,k;

    m = matrix_new(a->rows,b->cols);
    for (i=0;i < a->rows;i++)
        for (j=0;j<b->cols;j++) {
            for (k=0;k<b->rows;k++)
                MATRIX(m,i,j) += MATRIX(a,i,k) * MATRIX(b,k,j);
        }
    return m;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_transpose
 *
 * DESCRIPTION:  Transpose a matrix
 *
 * PARAMETERS:   m - matrix
 *
 * RETURN:       m'
 *
 ***********************************************************************/
static Matrix * matrix_transpose(Matrix *m) MATFUNC;
static Matrix *
matrix_transpose(Matrix *m)
{
    Matrix *r;
    Int16 i,j;

    r = matrix_new(m->cols,m->rows);
    for (i=0;i<m->rows;i++)
        for (j=0;j<m->cols;j++)
            MATRIX(r,j,i) = MATRIX(m,i,j);
    return r;
}
/***********************************************************************
 *
 * FUNCTION:     matrix_qrq
 *
 * DESCRIPTION:  Q of QR Decomposition of a matrix
 *
 * PARAMETERS:   A - matrix
 *
 * RETURN:       Q
 *
 ***********************************************************************/
static Matrix * matrix_qrq(Matrix *A) MATFUNC;
static Matrix *
matrix_qrq(Matrix *A)
{
    Matrix *I,*qt,*e,*a,*v,*H,*C,*Q;
    Int16 i,ii,j,jj,k,kk,n,nn,m,mx,sgn,flg=0;
    double nm,s;

    m = A->rows;
    n = nn = A->cols;

    I   = matrix_new(m,m);
    e   = matrix_new(m,1);
    a   = matrix_new(m,1);
    v   = matrix_new(m,1);
    H   = matrix_new(m,m);

    if (n>m) mx=n; else mx=m;
    C   = matrix_new(mx,mx);
    qt  = matrix_new(mx,mx);

    if(n>m)
    {
        n  = m;
        flg = 1;
    }

    for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(I,i,j)  = i==j ? 1 : 0;
    for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(qt,i,j) = i==j ? 1 : 0;

    for (i=0;i<n;i++)
    {
        for(j=0;j<m;j++) MATRIX(e,j,0) = MATRIX(I,j,i);
        for(j=0;j<m;j++) MATRIX(a,j,0) = MATRIX(A,j,i);

        for(j=0;j<m;j++) if(i>0) if(j<i) MATRIX(a,j,0) = 0;

        sgn = MATRIX(a,i,0)== 0 ? 0 : MATRIX(a,i,0)<0 ? -1 : 1;
        nm = 0.0;
        for(j=0;j<m;j++) nm += MATRIX(a,j,0) * MATRIX(a,j,0);
        nm = sqrt(nm);

        for(j=0;j<m;j++) MATRIX(v,j,0) = MATRIX(a,j,0) + sgn * nm * MATRIX(e,j,0);

        nm = 0.0;
        for(j=0;j<m;j++) nm+=MATRIX(v,j,0) * MATRIX(v,j,0);
        if(nm == 0.0)
        {
            matrix_delete(I);
            matrix_delete(e);
            matrix_delete(a);
            matrix_delete(v);
            matrix_delete(H);
            matrix_delete(C);
            matrix_delete(qt);
            return NULL;
        }

        for(j=0;j<m;j++) for(k=0;k<m;k++)
            MATRIX(H,k,j) = MATRIX(I,k,j) - 2 * (MATRIX(v,k,0) * MATRIX(v,j,0)) / nm;

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<(flg==1 ? nn : n);jj++)
            {
                s=0;
                for(kk=0;kk<m;kk++)
                    s += MATRIX(H,ii,kk)*MATRIX(A,kk,jj);
                MATRIX(C,ii,jj)=s;
            }

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<(flg==1 ? nn : n);jj++)
                MATRIX(A,ii,jj)=MATRIX(C,ii,jj);

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
            {
                s=0;
                for(kk=0;kk<m;kk++)
                    s += MATRIX(H,ii,kk)*MATRIX(qt,kk,jj);
                MATRIX(C,ii,jj)=s;
            }

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
                MATRIX(qt,ii,jj)=MATRIX(C,ii,jj);
    }

    matrix_delete(I);
    matrix_delete(e);
    matrix_delete(a);
    matrix_delete(v);
    matrix_delete(H);
    matrix_delete(C);

    Q = matrix_new(A->rows,A->rows);
    for(ii=0;ii<A->rows;ii++)
        for(jj=0;jj<A->rows;jj++)
            MATRIX(Q,ii,jj)=MATRIX(qt,jj,ii);

    matrix_delete(qt);
    return Q;

}

/***********************************************************************
 *
 * FUNCTION:     matrix_qrr
 *
 * DESCRIPTION:  R of QR Decomposition of a matrix - Upper Triangular
 *
 * PARAMETERS:   A - matrix
 *
 * RETURN:       R
 *
 ***********************************************************************/
static Matrix * matrix_qrr(Matrix *A) MATFUNC;
static Matrix *
matrix_qrr(Matrix *A)
{
    Matrix *I,*qt,*e,*a,*v,*H,*C,*R;
    Int16 i,ii,j,jj,k,kk,n,nn,m,mx,sgn,flg=0;
    double nm,s,z;

    z = 1e-15;
    m = A->rows;
    n = nn = A->cols;

    I   = matrix_new(m,m);
    e   = matrix_new(m,1);
    a   = matrix_new(m,1);
    v   = matrix_new(m,1);
    H   = matrix_new(m,m);

    if (n>m) mx=n; else mx=m;
    C   = matrix_new(mx,mx);
    qt  = matrix_new(mx,mx);

    if(n>m)
    {
        n  = m;
        flg = 1;
    }

    for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(I,i,j)  = i==j ? 1 : 0;
    for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(qt,i,j) = i==j ? 1 : 0;

    for (i=0;i<n;i++)
    {
        for(j=0;j<m;j++) MATRIX(e,j,0) = MATRIX(I,j,i);
        for(j=0;j<m;j++) MATRIX(a,j,0) = MATRIX(A,j,i);

        for(j=0;j<m;j++) if(i>0) if(j<i) MATRIX(a,j,0) = 0;

        sgn = MATRIX(a,i,0)== 0 ? 0 : MATRIX(a,i,0)<0 ? -1 : 1;
        nm = 0.0;
        for(j=0;j<m;j++) nm += MATRIX(a,j,0) * MATRIX(a,j,0);
        nm = sqrt(nm);

        for(j=0;j<m;j++) MATRIX(v,j,0) = MATRIX(a,j,0) + sgn * nm * MATRIX(e,j,0);

        nm = 0.0;
        for(j=0;j<m;j++) nm+=MATRIX(v,j,0) * MATRIX(v,j,0);
        if(nm == 0.0)
        {
            matrix_delete(I);
            matrix_delete(e);
            matrix_delete(a);
            matrix_delete(v);
            matrix_delete(H);
            matrix_delete(C);
            matrix_delete(qt);
            return NULL;
        }

        for(j=0;j<m;j++) for(k=0;k<m;k++)
            MATRIX(H,k,j) = MATRIX(I,k,j) - 2 * (MATRIX(v,k,0) * MATRIX(v,j,0)) / nm;

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
            {
                s=0;
                for(kk=0;kk<m;kk++)
                    s += MATRIX(H,ii,kk)*MATRIX(qt,kk,jj);
                MATRIX(C,ii,jj)=s;
            }

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
                MATRIX(qt,ii,jj)=MATRIX(C,ii,jj);

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<(flg==1 ? nn : n);jj++)
            {
                s=0;
                for(kk=0;kk<m;kk++)
                    s += MATRIX(H,ii,kk)*MATRIX(A,kk,jj);
                MATRIX(C,ii,jj)=s;
            }

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<(flg==1 ? nn : n);jj++)
                MATRIX(A,ii,jj)=MATRIX(C,ii,jj);
    }
    matrix_delete(I);
    matrix_delete(qt);
    matrix_delete(e);
    matrix_delete(a);
    matrix_delete(v);
    matrix_delete(H);

    R = matrix_new(A->rows,A->cols);
    for(ii=0;ii<A->rows;ii++)
        for(jj=0;jj<A->cols;jj++)
            MATRIX(R,ii,jj)=(fabs(MATRIX(C,ii,jj)) >= z ? MATRIX(C,ii,jj) : 0.0);

    matrix_delete(C);
    return R;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_qrs
 *
 * DESCRIPTION:  Polynomial Solution of Ax=b using QR Decomposition
 * of a square matrix appended on entry with col b.
 *
 * PARAMETERS:   A - rectangular matrix composed of square matrix A
 * appended with column b.  [A|b]
 *
 * RETURN:       x - replace b in original matrix as [A|x]
 *
 ***********************************************************************/
static Matrix * matrix_qrs(Matrix *A) MATFUNC;
static Matrix *
matrix_qrs(Matrix *A)
{
    Matrix *I,*qt,*e,*a,*v,*H,*C,*AA;
    Int16 i,ii,j,jj,k,kk,m,sgn;
    double nm,s;

    AA = matrix_dup(A);  //save copy of original matrix

    m = A->rows;  // the size of square matrix A and length of vectors x and b
    // on entry last columns is b

    I   = matrix_new(m,m);
    e   = matrix_new(m,1);
    a   = matrix_new(m,1);
    v   = matrix_new(m,1);
    H   = matrix_new(m,m);
    C   = matrix_new(m,m);
    qt  = matrix_new(m,m);


    for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(I,i,j)  = i==j ? 1 : 0;
    for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(qt,i,j) = i==j ? 1 : 0;

    for (i=0;i<m;i++)
    {
        for(j=0;j<m;j++) MATRIX(e,j,0) = MATRIX(I,j,i);
        for(j=0;j<m;j++) MATRIX(a,j,0) = MATRIX(A,j,i);

        for(j=0;j<m;j++) if(i>0) if(j<i) MATRIX(a,j,0) = 0;

        sgn = MATRIX(a,i,0)== 0 ? 0 : MATRIX(a,i,0)<0 ? -1 : 1;
        nm = 0.0;
        for(j=0;j<m;j++) nm += MATRIX(a,j,0) * MATRIX(a,j,0);
        nm = sqrt(nm);

        for(j=0;j<m;j++) MATRIX(v,j,0) = MATRIX(a,j,0) + sgn * nm * MATRIX(e,j,0);

        nm = 0.0;
        for(j=0;j<m;j++) nm+=MATRIX(v,j,0) * MATRIX(v,j,0);
        if(nm == 0.0)
        {
            matrix_delete(I);
            matrix_delete(e);
            matrix_delete(a);
            matrix_delete(v);
            matrix_delete(H);
            matrix_delete(C);
            matrix_delete(qt);
            matrix_delete(AA);
            return NULL;
        }

        for(j=0;j<m;j++) for(k=0;k<m;k++)
            MATRIX(H,k,j) = MATRIX(I,k,j) - 2 * (MATRIX(v,k,0) * MATRIX(v,j,0)) / nm;

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
            {
                s=0;
                for(kk=0;kk<m;kk++)
                    s += MATRIX(H,ii,kk)*MATRIX(A,kk,jj);
                MATRIX(C,ii,jj)=s;
            }

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
                MATRIX(A,ii,jj)=MATRIX(C,ii,jj);              //This is R !

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
            {
                s=0;
                for(kk=0;kk<m;kk++)
                    s += MATRIX(H,ii,kk)*MATRIX(qt,kk,jj);
                MATRIX(C,ii,jj)=s;
            }

        for(ii=0;ii<m;ii++)
            for(jj=0;jj<m;jj++)
                MATRIX(qt,ii,jj)=MATRIX(C,ii,jj);             //This is Q' !
    }

    matrix_delete(I);
    matrix_delete(a);
    matrix_delete(H);
    matrix_delete(C);

    for(ii=0;ii<m;ii++){
        MATRIX(v,ii,0) = 0.0;
        for(jj=0;jj<m;jj++)
            MATRIX(v,ii,0) += MATRIX(qt,ii,jj)*MATRIX(A,jj,m);  // v = Q'* b (b is in AA)
    }

    MATRIX(e,m-1,0) = MATRIX(v,m-1,0)/MATRIX(A,m-1,m-1);                  // x = v / R

    for(ii=m-1;ii>=0;ii--){                                               // continue x = v / R
        s = MATRIX(v,ii,0);
        for(jj=ii+1;jj<m;jj++)
                s-=MATRIX(A,ii,jj)*MATRIX(e,jj,0);

            MATRIX(e,ii,0) = s/MATRIX(A,ii,ii);
            MATRIX(AA,ii,m) = MATRIX(e,ii,0);                        //create solved matrix [A|x]
    }

    matrix_delete(e);
    matrix_delete(v);
    matrix_delete(qt);
    return AA;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_echelon
 *
 * DESCRIPTION:  Return triangular matrix with 1's only on diagonal
 *
 * PARAMETERS:   m - matrix
 *
 * RETURN:       triangular matrix
 *
 ***********************************************************************/
#define ALLOWED_ERROR    1E-12
static Matrix * matrix_echelon(Matrix *m) MATFUNC;
static Matrix *
matrix_echelon(Matrix *m)
{
    Matrix *B;
    Int16 row,col;
    int i, j, k, u = 0, n = 0;
    double p = 0.0, e, y;

    row = m->rows;
    col = m->cols;

    B = matrix_dup(m);

    for (k = 0 ; k < col ; k++) {
        for (i = n ; i < row ; i++) {
//      if (MATRIX(B, i, k) == 0)
            if (fabs(MATRIX(B, i, k)) < ALLOWED_ERROR)
                p = 0;
            else {
                p = MATRIX(B, i, k);
                u = i;
                break;
            }
        }
        if (fabs(p) > ALLOWED_ERROR)  {
//  if (p != 0)  {
            if (n != u) {
                for (i = 0 ; i < col ; i++)  {
                    y = MATRIX(B, n, i);
                    MATRIX(B, n, i) = MATRIX(B, u, i);
                    MATRIX(B, u, i) = y;
                }
            }
            if (p != 1) {
                for (i = k ; i < col ; i++)
                    MATRIX(B, n, i) = MATRIX(B, n, i) / p;
            }
            for (j = n + 1 ; j < row ; j++) {
                e = MATRIX(B, j, k);
                for (i = k ; i < col ; i++)
                    MATRIX(B, j, i) = MATRIX(B, j, i) - e * MATRIX(B, n, i);
            }
            for (j = n - 1 ; j >= 0 ; j--) {
                e = MATRIX(B, j, k);
                for (i = k ; i < col ; i++)
                    MATRIX(B, j, i) = MATRIX(B, j, i) - e * MATRIX(B, n, i);
            }
            n = n + 1;
            if (n == row)
                return B;
        }
    }
    return B;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_func3
 *
 * DESCRIPTION:  Wrapper for functions that receive int and return matrix
 *
 * PARAMETERS:   integer number
 *
 * RETURN:       matrix
 *
 ***********************************************************************/
CError
matrix_func3(Functype *func,CodeStack *stack)
{
    Matrix *r;
    UInt32 arg;
    CError err;
    UInt16 i;

    err = stack_get_val(stack,&arg,integer);
    if (err)
        return err;
    if (arg == 0 || arg > MATRIX_MAX)
        return c_baddim;

    r = matrix_new(arg,arg);
    for (i=0 ; i<arg ; i++)
        MATRIX(r,i,i) = 1.0;

    err = stack_add_val(stack,&r,matrix);
    matrix_delete(r);

    return err;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_func2
 *
 * DESCRIPTION:  Wrapper for functions on matrices that return matrix
 *
 * PARAMETERS:   matrix
 *
 * RETURN:       matrix
 *
 ***********************************************************************/
CError
matrix_func2(Functype *func,CodeStack *stack)
{
    Matrix *m,*r;
    CError err;
    rpntype type;

    err = stack_item_type(stack,&type,0);
    if (err)
        return err;
    if (type == cmatrix)
        return cmatrix_func2(func,stack);

    err = stack_get_val(stack,&m,matrix);
    if (err)
        return err;

    switch (func->num) {
    case MATRIX_ECHELON:
        r = matrix_echelon(m);
        break;
    case FUNC_CONJ: /* Transpose */
        r = matrix_transpose(m);
        break;
    case MATRIX_QRQ:
        r = matrix_qrq(m);
        if (r == NULL)
            err = c_singular;
        break;
    case MATRIX_QRR:
        r = matrix_qrr(m);
        if (r == NULL)
            err = c_singular;
        break;
    case MATRIX_QRS:
        if(m->rows != m->cols - 1){
            err = c_baddim;
            break;
        }
        r = matrix_qrs(m);
        if (r == NULL)
            err = c_singular;
        break;
    default:
        err = c_internal;
        break;
    }

    matrix_delete(m);

    if (!err){
        err = stack_add_val(stack,&r,matrix);
        matrix_delete(r);
    }
    return err;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_func
 *
 * DESCRIPTION:  Wrapper for functions on matrices that return real
 *
 * PARAMETERS:   matrix
 *
 * RETURN:       real number
 *
 ***********************************************************************/
CError
matrix_func(Functype *func,CodeStack *stack)
{
    Matrix *m;
    CError err;
    double result;
    rpntype type;

    err = stack_item_type(stack,&type,0);
    if (err)
        return err;
    if (type == cmatrix)
        return cmatrix_func(func,stack);

    err = stack_get_val(stack,&m,matrix);
    if (err)
        return err;

    switch (func->num) {
    case MATRIX_DET:
        if (m->cols != m->rows) {
            err = c_baddim;
            break;
        }
        result = matrix_deter(m);
        break;
    default:
        err = c_internal;
        break;
    }
    matrix_delete(m);
    if (!err)
        err = stack_add_val(stack,&result,real);
    return err;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_item
 *
 * DESCRIPTION:  Function for the M[1:2] type of operation
 *
 * PARAMETERS:   matrix, row [, column]
 *
 * RETURN:       real number or list
 *
 ***********************************************************************/
CError
matrix_item(Functype *func, CodeStack *stack)
{
    CError err;
    Matrix *m;
    Int32 row,col;

    if (func->paramcount != 3)
        return c_badargcount;

    err = stack_get_val(stack, &col, integer);
    if (err)
        return err;

    err = stack_get_val(stack, &row, integer);
    if (err)
        return err;

    err = stack_get_val(stack, &m, matrix);
    if (err)
        return err;

    if ( row > m->rows || col > m->cols || (col == 0 && row==0)){
        matrix_delete(m);
        return c_baddim;
    }

    if(col && row)
        err = stack_add_val(stack,&MATRIX(m,row-1,col-1),real);
    else{
        Int16 i;
        List *lst;
        if(row==0){
            lst=list_new(m->rows);
            if(!lst){
                matrix_delete(m);
                return c_memory;
            }
            for(i=0;i<m->rows;i++){
                lst->item[i].real=MATRIX(m,i,col-1);
                lst->item[i].imag=0.0;
            }
        }else{
            lst=list_new(m->cols);
            if(!lst){
                matrix_delete(m);
                return c_memory;
            }
            for(i=0;i<m->cols;i++){
                lst->item[i].real=MATRIX(m,row-1,i);
                lst->item[i].imag=0.0;
            }
        }
        err = stack_add_val(stack,&lst,list);
        list_delete(lst);
    }

    matrix_delete(m);
    return err;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_input
 *
 * DESCRIPTION:  Creates a matrix form its parameters - matrix(r:c:1:2:..)
 *
 * PARAMETERS:   row, column, items
 *
 * RETURN:       matrix
 *
 ***********************************************************************/
CError
matrix_input(Functype *func,CodeStack *stack)
{
    Matrix *m;
    Int16 i;
    CError err;
    UInt32 rows,cols;

    if (func->paramcount < 3)
        return c_badargcount;

    m = matrix_new(1,func->paramcount - 2);
    for (i=func->paramcount-3;i>=0;i--) {
        err = stack_get_val(stack,&MATRIX(m,0,i),real);
        if (err) {
            matrix_delete(m);
            return err;
        }
    }
    err = stack_get_val(stack,&cols,integer);
    if (!err)
        err = stack_get_val(stack,&rows,integer);
    if (err) {
        matrix_delete(m);
        return err;
    }

    if (rows == 0 || cols==0
        || rows>MATRIX_MAX || cols>MATRIX_MAX) {
        matrix_delete(m);
        return c_badarg;
    }
    if (rows * cols != func->paramcount-2) {
        matrix_delete(m);
        return c_badargcount;
    }
    m->rows = rows;
    m->cols = cols;

    err = stack_add_val(stack,&m,matrix);
    matrix_delete(m);
    return err;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_oper
 *
 * DESCRIPTION:  Wrapper function for operations with 2 matrices
 *
 * PARAMETERS:   2 matrices
 *
 * RETURN:       matrix
 *
 ***********************************************************************/
CError
matrix_oper(Functype *func,CodeStack *stack)
{
    CError err;
    Matrix *a,*b,*m,*q;
    Int16 i,j;

    err = stack_get_val(stack,&b,matrix);
    if (err)
        return err;
    err = stack_get_val(stack,&a,matrix);
    if (err) {
        matrix_delete(b);
        return err;
    }

    switch (func->num) {
    case FUNC_PLUS:
    case FUNC_MINUS:
        if (a->cols != b->cols || a->rows != b->rows) {
            err = c_baddim;
            break;
        }
        m = matrix_new(a->rows,a->cols);
        for (i=0;i<a->rows;i++)
            for (j=0;j<a->cols;j++)
                if (func->num == FUNC_PLUS) {
                    MATRIX(m,i,j)=MATRIX(a,i,j)+MATRIX(b,i,j);
                } else {
                    MATRIX(m,i,j)=MATRIX(a,i,j)-MATRIX(b,i,j);
                }
        break;
    case FUNC_MULTIPLY:
        if (a->cols != b->rows) {
            err = c_baddim;
            break;
        }
        m = matrix_prod(a,b);
        break;
    case FUNC_DIVIDE:
        /* We can do inverse only on cols=rows */
        if (a->cols != a->rows || a->cols != b->cols ||
            a->cols != b->rows) {
            err = c_baddim;
            break;
        }
        q = matrix_inverse(b);
        if (q == NULL) {
            err = c_singular;
            break;
        }
        m = matrix_prod(a,q);
        matrix_delete(q);
        break;
    default:
        err = c_internal;
        break;
    }
    matrix_delete(a);
    matrix_delete(b);
    if (err)
        return err;

    err = stack_add_val(stack,&m,matrix);
    matrix_delete(m);

    return err;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_oper_real
 *
 * DESCRIPTION:  Wrapper function for the r*M and M*r and
 *               M^-1 and M^r operations
 *
 * PARAMETERS:   Matrix and real number
 *
 * RETURN:       Matrix
 *
 ***********************************************************************/
CError
matrix_oper_real(Functype *func,CodeStack *stack)
{
    CError err;
    Matrix *m,*tmp,*tmp2;
    rpntype type2;
    double rnum;
    Int16 j;

    err = stack_item_type(stack,&type2,0);
    if (err)
        return err;

    if (type2 == matrix) {
        err = stack_get_val(stack,&m,matrix);
        if (err)
            return err;
        err = stack_get_val(stack,&rnum,real);
        if (err) {
            matrix_delete(m);
            return err;
        }
    } else {
        err = stack_get_val(stack,&rnum,real);
        if (err)
            return err;
        err = stack_get_val(stack,&m,matrix);
        if (err)
            return err;
    }

    switch (func->num) {
    case FUNC_POWER:
        if (type2 != matrix && IS_ZERO(rnum-round(rnum))
            && rnum > 0.5 && rnum < 50) {
            if (m->cols != m->rows) {
                matrix_delete(m);
                return c_baddim;
            }
            tmp = matrix_dup(m);
            j = (Int16) round(rnum);
            while (--j) {
                tmp2 = matrix_prod(tmp,m);
                matrix_delete(tmp);
                tmp = tmp2;
            }
            matrix_delete(m);
            m = tmp;
        } else if (type2 != matrix && IS_ZERO(rnum+1.0)) {
            if (m->cols != m->rows) {
                matrix_delete(m);
                return c_baddim;
            }
            tmp = matrix_inverse(m);
            matrix_delete(m);
            if (!tmp)
                return c_singular;
            m = tmp;
        } else {
            matrix_delete(m);
            return c_badarg;
        }
        break;
    default:
        matrix_delete(m);
        return c_badarg;
    }

    err = stack_add_val(stack,&m,matrix);
    matrix_delete(m);

    return err;
}

/***********************************************************************
 *
 * FUNCTION:     matrix_dim
 *
 * DESCRIPTION:  Return dimension of matrix
 *
 * PARAMETERS:   matrix on stack
 *
 * RETURN:       list(rows:columns)
 *
 ***********************************************************************/
CError
matrix_dim(Functype *func, CodeStack *stack)
{
    CError err;
    rpntype type;
    Matrix *m;
    List *result;

    err = stack_item_type(stack,&type,0);
    if (err)
        return err;
    if (type == list)
        return list_func(func,stack);

    if (type == cmatrix)
        return cmatrix_dim(func,stack);

    err = stack_get_val(stack,&m,matrix);
    if (err)
        return err;

    result = list_new(2);
    result->item[0].imag = result->item[1].imag = 0.0;
    result->item[0].real = m->rows;
    result->item[1].real = m->cols;

    err = stack_add_val(stack,&result,list);

    list_delete(result);
    matrix_delete(m);
    return err;
}
