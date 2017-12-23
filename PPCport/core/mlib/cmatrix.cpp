/*
 *   $Id: cmatrix.cpp,v 1.1 2009/10/17 13:48:34 mapibid Exp $
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
 *		 Added routines QRQ and QRR for QR Factorizaton of MXN Matrix
*/
#include "stdafx.h"

#include "compat/PalmOS.h"

#include "compat/MathLib.h"
#include "compat/segment.h"
#include "konvert.h"
#include "matrix.h"
#include "cmatrix.h"
#include "complex.h"
#include "stack.h"
#include "funcs.h"
#include "mathem.h"
#include "slist.h"

/***********************************************************************
 *
 * FUNCTION:     cmatrix_to_matrix
 * 
 * DESCRIPTION:  Convert complex matrix to normal matrix, if possible
 *
 * PARAMETERS:   cmatrix
 *
 * RETURN:       NULL or pointer to new matrix
 *      
 ***********************************************************************/
Matrix *
cmatrix_to_matrix(CMatrix *cm)
{
	Matrix *m;
	Int16 i,j;

	m = matrix_new(cm->rows,cm->cols);
	for (i=0;i < cm->rows;i++)
		for (j=0;j < cm->cols;j++) {
			if (MATRIX(cm,i,j).imag != 0.0) {
				matrix_delete(m);
				return NULL;
			}
			MATRIX(m,i,j) = MATRIX(cm,i,j).real;
		}
	return m;
}

CMatrix *
matrix_to_cmatrix(Matrix *m)
{
	CMatrix *cm;
	Int16 i,j;

	cm = cmatrix_new(m->rows,m->cols);
	for (i=0;i < m->rows;i++)
		for (j=0;j < m->cols;j++) {
			MATRIX(cm,i,j).real = MATRIX(m,i,j);
		}
	return cm;
}



/***********************************************************************
 *
 * FUNCTION:     cmatrix_new
 * 
 * DESCRIPTION:  Create a new matrix set to 0.0
 *
 * PARAMETERS:   rows, cols
 *
 * RETURN:       cmatrix(rows,cols)
 *      
 ***********************************************************************/
CMatrix *
cmatrix_new(Int16 rows,Int16 cols)
{
	CMatrix *m;
	Int16 i,j;

	m = (CMatrix *) MemPtrNew(sizeof(*m) + rows*cols*sizeof(m->item[0]));
	if (!m)
	    return NULL;
	m->rows = rows;
	m->cols = cols;

	for (i=0;i < rows; i++)
		for (j=0; j< cols; j++) {
			MATRIX(m,i,j).real = 0.0;
			MATRIX(m,i,j).imag = 0.0;
		}

	return m;
}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_dup
 * 
 * DESCRIPTION:  Duplicate a complex matrix
 *
 * PARAMETERS:   m - matrix
 *
 * RETURN:       new matrix same as m
 *      
 ***********************************************************************/
CMatrix *
cmatrix_dup(CMatrix *m)
{
	CMatrix *newm;
	Int16 i,j;

	newm = cmatrix_new(m->rows,m->cols);
	if (!newm)
	    return NULL;
	for (i=0;i < m->rows;i++)
		for (j=0;j<m->cols;j++)
			MATRIX(newm,i,j) = MATRIX(m,i,j);
	return newm;
}

void
cmatrix_delete(CMatrix *m)
{
	MemPtrFree(m);
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
static CMatrix * cmatrix_transpose(CMatrix *m) MATFUNC;
static CMatrix * 
cmatrix_transpose(CMatrix *m)
{
	CMatrix *r;
	Int16 i,j;

	r = cmatrix_new(m->cols,m->rows);
	for (i=0;i<m->rows;i++)
		for (j=0;j<m->cols;j++){
			MATRIX(m,i,j).imag = -1.0 * MATRIX(m,i,j).imag;	
			MATRIX(r,j,i) = MATRIX(m,i,j);
		}
	return r;
}


/***********************************************************************
 *
 * FUNCTION:     cmatrix_input
 * 
 * DESCRIPTION:  Creates a complex matrix form its parameters - 
 *               matrix(r:c:1:2:..)
 *
 * PARAMETERS:   row, column, items
 *
 * RETURN:       cmatrix
 *      
 ***********************************************************************/
CError
cmatrix_input(Functype *func,CodeStack *stack)
{
	CMatrix *m;
	Int16 i;
	CError err;
	UInt32 rows,cols;

	if (func->paramcount < 3)
		return c_badargcount;

	m = cmatrix_new(1,func->paramcount - 2);
	for (i=func->paramcount-3;i>=0;i--) {
		err = stack_get_val(stack,&MATRIX(m,0,i),complex);
		if (err) {
			cmatrix_delete(m);
			return err;
		}
	}
	err = stack_get_val(stack,&cols,integer);
	if (!err)
		err = stack_get_val(stack,&rows,integer);
	if (err) {
		cmatrix_delete(m);
		return err;
	}

	if (rows == 0 || cols==0
	    || rows>MATRIX_MAX || cols>MATRIX_MAX) {
		cmatrix_delete(m);
		return c_badarg;
	}
	if (rows * cols != func->paramcount-2) {
		cmatrix_delete(m);
		return c_badargcount;
	}
	m->rows = rows;
	m->cols = cols;

	err = stack_add_val(stack,&m,cmatrix);
	cmatrix_delete(m);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_inverse
 * 
 * DESCRIPTION:  Create an inverse complex matrix
 *
 * PARAMETERS:   A - complex matrix
 *
 * RETURN:       A^-1
 *               NULL - cmatrix is singular, inverse doesn't exist
 *      
 ***********************************************************************/
static CMatrix * cmatrix_inverse(CMatrix *A) MATFUNC;
static CMatrix *
cmatrix_inverse(CMatrix *A)
{
	Int16 d;
	Int16 col, i, j, k, l, u=0;
	Complex p, e, y,cone;
	CMatrix *W,*AI;

	cone.real = 1.0;cone.imag = 0.0; /* 1 in complex nums */
	p.real = p.imag = 0.0;
	d = A->cols;
	W = cmatrix_new(d,2*d);

	col = 2 * d;

	for (i = 0 ; i < d ; i++) {
		for (j = 0 ; j < d ; j++)
			MATRIX(W, i, j) = MATRIX(A,i,j);
	}

	for (i = 0 ; i < d ; i++) 
		MATRIX(W, i, d + i) = cone;

	for (k = 0 ; k < d ; k++) {
		for (i = k ; i < d ; i++) {
			if (MATRIX(W, i, k).real == 0 
			    && MATRIX(W, i, k).imag == 0.0) {
				p.real = p.imag = 0.0;
			} else {
				p = MATRIX(W, i, k);
				u = i;
				break;
			}
		}
		if (p.real == 0.0 && p.imag == 0.0) {
			cmatrix_delete(W);
			return NULL;
		} else {
			for (i = 0 ; i < col ; i++) {
				y = MATRIX(W, k, i);
				MATRIX(W, k, i) = MATRIX(W, u, i);
				MATRIX(W, u, i) = y;
			}
		}
		if (cplx_abs(p) != 1) {
			for (i = k ; i < col ; i++)
				MATRIX(W, k, i) = cplx_div(MATRIX(W, k, i),p);
		}
		for (l = k + 1 ; l < d ; l++) {
			e = MATRIX(W, l, k);
			for (i = k ; i < col ; i++)
				MATRIX(W, l, i) = cplx_sub(MATRIX(W, l, i),
					   cplx_multiply(e,MATRIX(W, k, i)));
		}
	}

	for (k = d - 1 ; k >= 1; k--) {
		for (i = k - 1 ; i >= 0 ; i--) {
			e = MATRIX(W, i, k);
			for (l = k ; l < col ; l++)
				MATRIX(W, i, l) = cplx_sub(MATRIX(W, i, l),
					    cplx_multiply(e,MATRIX(W, k, l)));
		}
	}

	AI = cmatrix_new(A->cols,A->cols);
	for (i = 0 ; i < d ; i++) {
		for (j = 0 ; j < d ; j++)
			MATRIX(AI,i,j) = MATRIX(W, i, j + d);
	}

	cmatrix_delete(W);
	return AI;
}


/***********************************************************************
 *
 * FUNCTION:     cmatrix_deter
 * 
 * DESCRIPTION:  Compute a complex determinant of a complex matrix
 *
 * PARAMETERS:   A - complex matrix
 *
 * RETURN:       det(A)
 *      
 ***********************************************************************/
static Complex cmatrix_deter(CMatrix *A) MATFUNC;
static Complex
cmatrix_deter(CMatrix *A)
{
	Int16 i, j, k, u, v;
	Complex e, y;
	Complex det;
	double m;
	Int16 col;
	Int32 w = 0;
	Complex czero;

	czero.real = czero.imag = 0.0;

	col = A->cols;
	for (j = 0 ; j < col - 1 ; j++) {
		m = cplx_abs(MATRIX(A, j, j));
		k = j;
		for (i = j + 1 ; i < col ; i++) {
			if (cplx_abs(MATRIX(A, i, j))  > m) {
				m = cplx_abs(MATRIX(A, i, j));
				k = i;
			}
		}
		if (m == 0) 
			return czero;

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
				MATRIX(A, u, v) = cplx_sub(MATRIX(A, u, v),\
	       cplx_multiply(e,cplx_div(MATRIX(A, j, v),MATRIX(A, j, j))));
		}
	}
	if (MATRIX(A, (col - 1), (col - 1)).real == 0 && \
	    MATRIX(A, (col - 1), (col - 1)).imag == 0) 
		return czero;

	det = MATRIX(A, 0, 0);
	for (i = 1; i < col ; i++)
		det = cplx_multiply(det,MATRIX(A, i, i));

	if (w % 2) {
		det.real = -det.real;
		det.imag = -det.imag;
	}

	return det;
}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_qrq
 * 
 * DESCRIPTION:  Q of QR Decomposition of a matrix
 *
 * PARAMETERS:   m - cmatrix
 *
 * RETURN:       m'
 *      
 ***********************************************************************/
static CMatrix * cmatrix_qrq(CMatrix *A) MATFUNC;
static CMatrix * 
cmatrix_qrq(CMatrix *A)
{
	CMatrix *I,*qt,*e,*a,*v,*H,*C,*Q,*vt;
	Int16 i,ii,j,jj,k,kk,n,nn,m,mx,flg=0;
	double tmp;
	Complex cone,czero,ctwo,nm,nm2,s,sgn;
	
	m = A->rows;
	n = nn = A->cols;

	I   = cmatrix_new(m,m);
	e   = cmatrix_new(m,1);
	a   = cmatrix_new(m,1);
	v   = cmatrix_new(m,1);
	H   = cmatrix_new(m,m);
	vt  = cmatrix_new(m,1);

	if (n>m) mx=n; else mx=m;
	C   = cmatrix_new(mx,mx);
	qt  = cmatrix_new(mx,mx);

	if(n>m)
	{
		n  = m;
		flg = 1;
	}
	cone.real = 1.0;   cone.imag = 0.0; /* 1 in complex nums */
	czero.real = 0.0;  czero.imag =0.0; /* 0 in complex nums */
	ctwo.real = 2.0;   ctwo.imag = 0.0;
	
	for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(I,i,j)  = i==j ? cone : czero;
	for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(qt,i,j) = i==j ? cone : czero;

	for (i=0;i<n;i++)
	{
		for(j=0;j<m;j++) MATRIX(e,j,0) = MATRIX(I,j,i);
		for(j=0;j<m;j++) MATRIX(a,j,0) = MATRIX(A,j,i);

		for(j=0;j<m;j++) if(i>0) if(j<i) MATRIX(a,j,0) = czero;

		tmp = sqrt(MATRIX(a,i,0).real*MATRIX(a,i,0).real + MATRIX(a,i,0).imag * MATRIX(a,i,0).imag);
		sgn.real = MATRIX(a,i,0).real/tmp;
		sgn.imag = MATRIX(a,i,0).imag/tmp;
		nm = czero;
		for(j=0;j<m;j++) {
			nm.real += MATRIX(a,j,0).real * MATRIX(a,j,0).real;
			nm.imag += MATRIX(a,j,0).imag * MATRIX(a,j,0).imag;
		}
		nm.real = sqrt(nm.real+nm.imag);
		nm.imag = 0.0;

		for(j=0;j<m;j++)
			MATRIX(v,j,0) = cplx_add(MATRIX(a,j,0),cplx_multiply(cplx_multiply(sgn,nm),MATRIX(e,j,0)));
		
		nm2 = czero;
		for(j=0;j<m;j++)
			nm2.real += MATRIX(v,j,0).real * MATRIX(v,j,0).real + MATRIX(v,j,0).imag * MATRIX(v,j,0).imag;
		if(nm2.real == 0.0)
		{
			cmatrix_delete(I); 
			cmatrix_delete(e); 
			cmatrix_delete(a); 
			cmatrix_delete(v); 
			cmatrix_delete(H);
			cmatrix_delete(vt);
			cmatrix_delete(C);
			cmatrix_delete(qt);
			return NULL;
		}
		
		for(j=0;j<m;j++){
			MATRIX(vt,j,0).real = MATRIX(v,j,0).real; 
			MATRIX(vt,j,0).imag = -1.0 * MATRIX(v,j,0).imag;
		}

		for(j=0;j<m;j++) for(k=0;k<m;k++)
			MATRIX(H,k,j) = cplx_sub(MATRIX(I,k,j),cplx_div(cplx_multiply(ctwo,cplx_multiply(MATRIX(v,k,0),MATRIX(vt,j,0))),nm2));
	
		for(ii=0;ii<m;ii++)
			for(jj=0;jj<(flg==1 ? nn : n);jj++)
			{
				s.real = s.imag = 0;
				for(kk=0;kk<m;kk++)
					s = cplx_add(s,cplx_multiply(MATRIX(H,ii,kk),MATRIX(A,kk,jj)));
				MATRIX(C,ii,jj)=s;
			}

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<(flg==1 ? nn : n);jj++)
				MATRIX(A,ii,jj)=MATRIX(C,ii,jj);

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
			{
				s.real = s.imag =0;
				for(kk=0;kk<m;kk++)
					s = cplx_add(s,cplx_multiply(MATRIX(H,ii,kk),MATRIX(qt,kk,jj)));
				MATRIX(C,ii,jj)=s;
			}

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
				MATRIX(qt,ii,jj)=MATRIX(C,ii,jj);
	}

	cmatrix_delete(I);			
	cmatrix_delete(e);
	cmatrix_delete(a);
	cmatrix_delete(v);
	cmatrix_delete(H);
	cmatrix_delete(C);
	cmatrix_delete(vt);

	Q = cmatrix_new(A->rows,A->rows);
	for(ii=0;ii<A->rows;ii++) 
		for(jj=0;jj<A->rows;jj++) {
			MATRIX(Q,ii,jj).real = MATRIX(qt,jj,ii).real;
			MATRIX(Q,ii,jj).imag = -1.0 * MATRIX(qt,jj,ii).imag;
		}

	cmatrix_delete(qt);
	return Q;

}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_qrr
 * 
 * DESCRIPTION:  R of QR Decomposition of a matrix
 *
 * PARAMETERS:   m - cmatrix
 *
 * RETURN:       m'
 *      
 ***********************************************************************/
static CMatrix * cmatrix_qrr(CMatrix *A) MATFUNC;
static CMatrix * 
cmatrix_qrr(CMatrix *A)
{
	CMatrix *I,*qt,*e,*a,*v,*H,*R,*C,*vt;
	Int16 i,ii,j,jj,k,kk,n,nn,m,mx,flg=0;
	double tmp,z;
	Complex cone,czero,ctwo,nm,nm2,s,sgn;
	
	z = 1e-15;
	m = A->rows;
	n = nn = A->cols;

	I   = cmatrix_new(m,m);
	e   = cmatrix_new(m,1);
	a   = cmatrix_new(m,1);
	v   = cmatrix_new(m,1);
	H   = cmatrix_new(m,m);
	vt  = cmatrix_new(m,1);

	if (n>m) mx=n; else mx=m;
	C   = cmatrix_new(mx,mx);
	qt  = cmatrix_new(mx,mx);

	if(n>m)
	{
		n  = m;
		flg = 1;
	}
	cone.real = 1.0;   cone.imag = 0.0; /* 1 in complex nums */
	czero.real = 0.0;  czero.imag =0.0; /* 0 in complex nums */
	ctwo.real = 2.0;   ctwo.imag = 0.0;
	
	for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(I,i,j)  = i==j ? cone : czero;
	for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(qt,i,j) = i==j ? cone : czero;

	for (i=0;i<n;i++)
	{
		for(j=0;j<m;j++) MATRIX(e,j,0) = MATRIX(I,j,i);
		for(j=0;j<m;j++) MATRIX(a,j,0) = MATRIX(A,j,i);

		for(j=0;j<m;j++) if(i>0) if(j<i) MATRIX(a,j,0) = czero;

		tmp = sqrt(MATRIX(a,i,0).real*MATRIX(a,i,0).real + MATRIX(a,i,0).imag * MATRIX(a,i,0).imag);
		sgn.real = MATRIX(a,i,0).real/tmp;
		sgn.imag = MATRIX(a,i,0).imag/tmp;
		nm = czero;
		for(j=0;j<m;j++) {
			nm.real += MATRIX(a,j,0).real * MATRIX(a,j,0).real;
			nm.imag += MATRIX(a,j,0).imag * MATRIX(a,j,0).imag;
		}
		nm.real = sqrt(nm.real+nm.imag);
		nm.imag = 0.0;

		for(j=0;j<m;j++)
			MATRIX(v,j,0) = cplx_add(MATRIX(a,j,0),cplx_multiply(cplx_multiply(sgn,nm),MATRIX(e,j,0)));

		nm2 = czero;
		for(j=0;j<m;j++)
			nm2.real += MATRIX(v,j,0).real * MATRIX(v,j,0).real + MATRIX(v,j,0).imag * MATRIX(v,j,0).imag;
		if(nm2.real == 0.0)
		{
			cmatrix_delete(I); 
			cmatrix_delete(e); 
			cmatrix_delete(a); 
			cmatrix_delete(v); 
			cmatrix_delete(H);
			cmatrix_delete(vt);
			cmatrix_delete(C);
			cmatrix_delete(qt);
			return NULL;
		}

		for(j=0;j<m;j++){
			MATRIX(vt,j,0).real = MATRIX(v,j,0).real; 
			MATRIX(vt,j,0).imag = -1.0 * MATRIX(v,j,0).imag;
		}

		for(j=0;j<m;j++) for(k=0;k<m;k++)
			MATRIX(H,k,j) = cplx_sub(MATRIX(I,k,j),cplx_div(cplx_multiply(ctwo,cplx_multiply(MATRIX(v,k,0),MATRIX(vt,j,0))),nm2));


		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
			{
				s.real = s.imag = 0.0;
				for(kk=0;kk<m;kk++)
					s = cplx_add(s,cplx_multiply(MATRIX(H,ii,kk),MATRIX(qt,kk,jj)));
				MATRIX(C,ii,jj) = s;
			}

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
				MATRIX(qt,ii,jj)=MATRIX(C,ii,jj);

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<(flg==1 ? nn : n);jj++)
			{
				s.real = s.imag = 0.0;
				for(kk=0;kk<m;kk++)
					s = cplx_add(s,cplx_multiply(MATRIX(H,ii,kk),MATRIX(A,kk,jj)));
				MATRIX(C,ii,jj) = s;
			}

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<(flg==1 ? nn :n);jj++)
				MATRIX(A,ii,jj)=MATRIX(C,ii,jj);

	}
	cmatrix_delete(I);			
	cmatrix_delete(qt);
	cmatrix_delete(e);
	cmatrix_delete(a);
	cmatrix_delete(v);
	cmatrix_delete(H);
	cmatrix_delete(vt);

	R = cmatrix_new(A->rows,A->cols);

	for(ii=0;ii<A->rows;ii++) 
		for(jj=0;jj<A->cols;jj++){
			MATRIX(R,ii,jj).real = fabs(MATRIX(C,ii,jj).real) >= z ? MATRIX(C,ii,jj).real : 0.0;
			MATRIX(R,ii,jj).imag = fabs(MATRIX(C,ii,jj).imag) >= z ? MATRIX(C,ii,jj).imag : 0.0;
		}

	cmatrix_delete(C);
	return R;
}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_qrs
 * 
 * DESCRIPTION:  Polynomial Solution of Ax=b using QR Decomposition
 * of a square cmatrix appended on entry with col b.
 *
 * PARAMETERS:   A - rectangular cmatrix composed of square matrix A
 * appended with column b.  [A|b]
 *
 * RETURN:       x - replace b in original cmatrix as [A|x]
 *      
 ***********************************************************************/
static CMatrix * cmatrix_qrs(CMatrix *A) MATFUNC;
static CMatrix * 
cmatrix_qrs(CMatrix *A)
{
	CMatrix *I,*qt,*e,*a,*v,*H,*C,*vt,*AA;
	Int16 i,ii,j,jj,k,kk,m;
	double tmp;
	Complex cone,czero,ctwo,nm,nm2,s,sgn;

	AA = cmatrix_dup(A);  //save copy of original cmatrix
	
	m = A->rows;

	I   = cmatrix_new(m,m);
	e   = cmatrix_new(m,1);
	a   = cmatrix_new(m,1);
	v   = cmatrix_new(m,1);
	H   = cmatrix_new(m,m);
	vt  = cmatrix_new(m,1);
	C   = cmatrix_new(m,m);
	qt  = cmatrix_new(m,m);

	cone.real = 1.0;   cone.imag = 0.0; /* 1 in complex nums */
	czero.real = 0.0;  czero.imag =0.0; /* 0 in complex nums */
	ctwo.real = 2.0;   ctwo.imag = 0.0;
	
	for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(I,i,j)  = i==j ? cone : czero;
	for(i=0;i<m;i++) for(j=0;j<m;j++) MATRIX(qt,i,j) = i==j ? cone : czero;

	for (i=0;i<m;i++)
	{
		for(j=0;j<m;j++) MATRIX(e,j,0) = MATRIX(I,j,i);
		for(j=0;j<m;j++) MATRIX(a,j,0) = MATRIX(A,j,i);

		for(j=0;j<m;j++) if(i>0) if(j<i) MATRIX(a,j,0) = czero;

		tmp = sqrt(MATRIX(a,i,0).real*MATRIX(a,i,0).real + MATRIX(a,i,0).imag * MATRIX(a,i,0).imag);
		sgn.real = MATRIX(a,i,0).real/tmp;
		sgn.imag = MATRIX(a,i,0).imag/tmp;
		nm = czero;
		for(j=0;j<m;j++) {
			nm.real += MATRIX(a,j,0).real * MATRIX(a,j,0).real;
			nm.imag += MATRIX(a,j,0).imag * MATRIX(a,j,0).imag;
		}
		nm.real = sqrt(nm.real+nm.imag);
		nm.imag = 0.0;

		for(j=0;j<m;j++)
			MATRIX(v,j,0) = cplx_add(MATRIX(a,j,0),cplx_multiply(cplx_multiply(sgn,nm),MATRIX(e,j,0)));
		
		nm2 = czero;
		for(j=0;j<m;j++)
			nm2.real += MATRIX(v,j,0).real * MATRIX(v,j,0).real + MATRIX(v,j,0).imag * MATRIX(v,j,0).imag;
		if(nm2.real == 0.0)
		{
			cmatrix_delete(I); 
			cmatrix_delete(e); 
			cmatrix_delete(a); 
			cmatrix_delete(v); 
			cmatrix_delete(H);
			cmatrix_delete(vt);
			cmatrix_delete(C);
			cmatrix_delete(qt);
			return NULL;
		}
		
		for(j=0;j<m;j++){
			MATRIX(vt,j,0).real = MATRIX(v,j,0).real; 
			MATRIX(vt,j,0).imag = -1.0 * MATRIX(v,j,0).imag;
		}

		for(j=0;j<m;j++) for(k=0;k<m;k++)
			MATRIX(H,k,j) = cplx_sub(MATRIX(I,k,j),cplx_div(cplx_multiply(ctwo,cplx_multiply(MATRIX(v,k,0),MATRIX(vt,j,0))),nm2));
	
		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
			{
				s.real = s.imag = 0;
				for(kk=0;kk<m;kk++)
					s = cplx_add(s,cplx_multiply(MATRIX(H,ii,kk),MATRIX(A,kk,jj)));
				MATRIX(C,ii,jj)=s;
			}

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
				MATRIX(A,ii,jj)=MATRIX(C,ii,jj);            //This is R !

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
			{
				s.real = s.imag =0;
				for(kk=0;kk<m;kk++)
					s = cplx_add(s,cplx_multiply(MATRIX(H,ii,kk),MATRIX(qt,kk,jj)));
				MATRIX(C,ii,jj)=s;
			}

		for(ii=0;ii<m;ii++)
			for(jj=0;jj<m;jj++)
				MATRIX(qt,ii,jj)=MATRIX(C,ii,jj);           //This is Q' !
	}

	cmatrix_delete(I);			
	cmatrix_delete(a);
	cmatrix_delete(H);
	cmatrix_delete(C);
	cmatrix_delete(vt);

	for(ii=0;ii<m;ii++){
		MATRIX(v,ii,0) = czero;
		for(jj=0;jj<m;jj++)
			MATRIX(v,ii,0) = cplx_add(MATRIX(v,ii,0),cplx_multiply(MATRIX(qt,ii,jj),MATRIX(A,jj,m)));  // v = Q'* b (b is in AA)
	}

	MATRIX(e,m-1,0) = cplx_div(MATRIX(v,m-1,0),MATRIX(A,m-1,m-1));                  // x = v / R

	for(ii=m-1;ii>=0;ii--){                                               // continue x = v / R
		s = MATRIX(v,ii,0);
		for(jj=ii+1;jj<m;jj++)
			    s = cplx_sub(s,cplx_multiply(MATRIX(A,ii,jj),MATRIX(e,jj,0)));

		    MATRIX(e,ii,0) = cplx_div(s,MATRIX(A,ii,ii));
		    MATRIX(AA,ii,m) = MATRIX(e,ii,0);                        //create solved matrix [A|x]
	}

	cmatrix_delete(e);
	cmatrix_delete(v);
	cmatrix_delete(qt);
	return AA;

}


/***********************************************************************
 *
 * FUNCTION:     cmatrix_echelon
 * 
 * DESCRIPTION:  Return triangular complex matrix with 1's only on diagonal
 *
 * PARAMETERS:   m - matrix
 *
 * RETURN:       triangular complex matrix
 *      
 ***********************************************************************/
static CMatrix * cmatrix_echelon(CMatrix *m) MATFUNC;
static CMatrix *
cmatrix_echelon(CMatrix *m)
{
    CMatrix *B;
    Int16 row,col;
    int i, j, k, u = 0, n = 0;
    Complex p, e, y;
    
    p.imag = p.real = 0.0;

    row = m->rows;
    col = m->cols;

    B = cmatrix_dup(m);

    for (k = 0 ; k < col ; k++) {
	for (i = n ; i < row ; i++) {
	    if (MATRIX(B, i, k).real == 0.0 &&
		MATRIX(B, i, k).imag == 0.0)
		p = MATRIX(B, i, k);
	    else {
		p = MATRIX(B, i, k);
		u = i;
		break;
	    }
	}
	if (p.real != 0.0 || p.imag != 0.0)  {
	    if (n != u) {
		for (i = 0 ; i < col ; i++)  {
		    y = MATRIX(B, n, i);
		    MATRIX(B, n, i) = MATRIX(B, u, i);
		    MATRIX(B, u, i) = y;
		}
	    }
	    if (p.real != 1.0 || p.imag != 0.0) {
		for (i = k ; i < col ; i++)
		    MATRIX(B, n, i) = cplx_div(MATRIX(B, n, i),p);
	    }
	    for (j = n + 1 ; j < row ; j++) {
		e = MATRIX(B, j, k);
		for (i = k ; i < col ; i++)
		    MATRIX(B, j, i) = cplx_sub(MATRIX(B, j, i),
					       cplx_multiply(e,MATRIX(B, n, i)));
	    }
	    for (j = n - 1 ; j >= 0 ; j--) {
		e = MATRIX(B, j, k);
		for (i = k ; i < col ; i++)
		    MATRIX(B, j, i) = cplx_sub(MATRIX(B, j, i),
					       cplx_multiply(e,MATRIX(B, n, i)));
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
 * FUNCTION:     cmatrix_prod
 * 
 * DESCRIPTION:  multiply 2 complex matrices
 *
 * PARAMETERS:   a,b - complex matrices
 *
 * RETURN:       a*b
 *      
 ***********************************************************************/
static CMatrix * cmatrix_prod(CMatrix *a, CMatrix *b) MATFUNC;
static CMatrix *
cmatrix_prod(CMatrix *a, CMatrix *b)
{
	CMatrix *m;
	Int16 i,j,k;
	
	m = cmatrix_new(a->rows,b->cols);
	for (i=0;i < a->rows;i++)
		for (j=0;j<b->cols;j++) 
			for (k=0;k<b->rows;k++)
				MATRIX(m,i,j) = cplx_add(MATRIX(m,i,j),
							 cplx_multiply(MATRIX(a,i,k),
								       MATRIX(b,k,j)));
	return m;
}


/***********************************************************************
 *
 * FUNCTION:     cmatrix_oper
 * 
 * DESCRIPTION:  Wrapper function for operations with 2 complex matrices
 *
 * PARAMETERS:   2 complex matrices
 *
 * RETURN:       complex matrix
 *      
 ***********************************************************************/
CError
cmatrix_oper(Functype *func,CodeStack *stack)
{
	CError err;
	CMatrix *a,*b,*m,*q;
	Int16 i,j;

	err = stack_get_val(stack,&b,cmatrix);
	if (err)
		return err;
	err = stack_get_val(stack,&a,cmatrix);
	if (err) {
		cmatrix_delete(b);
		return err;
	}
	
	switch (func->num) {
	case FUNC_PLUS:
	case FUNC_MINUS:
		if (a->cols != b->cols || a->rows != b->rows) {
			err = c_baddim;
			break;
		}
		m = cmatrix_new(a->rows,a->cols);
		for (i=0;i<a->rows;i++)
			for (j=0;j<a->cols;j++)
				if (func->num == FUNC_PLUS) {
					MATRIX(m,i,j)=cplx_add(MATRIX(a,i,j),
							       MATRIX(b,i,j));
				} else {
					MATRIX(m,i,j)=cplx_sub(MATRIX(a,i,j),
							       MATRIX(b,i,j));
				}
		break;
	case FUNC_MULTIPLY:
		if (a->cols != b->rows) {
			err = c_baddim;
			break;
		}
		m = cmatrix_prod(a,b);
		break;
	case FUNC_DIVIDE:
		/* We can do inverse only on cols=rows */
		if (a->cols != a->rows || a->cols != b->cols ||
		    a->cols != b->rows) {
			err = c_baddim;
			break;
		}
		q = cmatrix_inverse(b);
		if (q == NULL) {
			err = c_singular;
			break;
		}
		m = cmatrix_prod(a,q);
		cmatrix_delete(q);
		break;
	default:
		err = c_internal;
		break;
	}
	cmatrix_delete(a);
	cmatrix_delete(b);
	if (err)
		return err;
	
	err = stack_add_val(stack,&m,cmatrix);
	cmatrix_delete(m);

	return err;
}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_oper_cplx
 * 
 * DESCRIPTION:  Wrapper function for the (r+i)*M and M*(r+i) and
 *               M^-1 and M^r operations
 *
 * PARAMETERS:   Complex Matrix and complex number
 *
 * RETURN:       Complex
 *      
 ***********************************************************************/
CError
cmatrix_oper_cplx(Functype *func,CodeStack *stack)
{
	CError err;
	CMatrix *m,*tmp,*tmp2;
	rpntype type2;
	Complex c;
	Int16 j;
	
	err = stack_item_type(stack,&type2,0);
	if (err)
		return err;
	
	if (type2 == cmatrix) {
		err = stack_get_val(stack,&m,cmatrix);
		if (err)
			return err;
		err = stack_get_val(stack,&c,complex);
		if (err) {
			cmatrix_delete(m);
			return err;
		}
	} else {
		err = stack_get_val(stack,&c,complex);
		if (err)
			return err;
		err = stack_get_val(stack,&m,cmatrix);
		if (err)
			return err;
	}
	
	switch (func->num) {
	case FUNC_POWER:
		if ((type2 != cmatrix && type2 != matrix) && 
		    IS_ZERO(c.real-round(c.real) && c.imag == 0.0) 
		    && c.real > 0.5 && c.real < 50) {
			if (m->cols != m->rows) {
				cmatrix_delete(m);
				return c_baddim;
			}
			tmp = cmatrix_dup(m);
			j = (Int16) round(c.real);
			while (--j) {
				tmp2 = cmatrix_prod(tmp,m);
				cmatrix_delete(tmp);
				tmp = tmp2;
			}
			cmatrix_delete(m);
			m = tmp;
		} else if ((type2 != cmatrix && type2 != matrix) 
			   && IS_ZERO(c.real+1.0) && c.imag == 0.0) {
			if (m->cols != m->rows) {
				cmatrix_delete(m);
				return c_baddim;
			}
			tmp = cmatrix_inverse(m);
			cmatrix_delete(m);
			if (!tmp) 
				return c_singular;
			m = tmp;
		} else {
			cmatrix_delete(m);
			return c_badarg;
		}
		break;
	default:
		cmatrix_delete(m);
		return c_badarg;
	}
		
	err = stack_add_val(stack,&m,cmatrix);
	cmatrix_delete(m);

	return err;
}

CError
cmatrix_func2(Functype *func,CodeStack *stack)
{
	CMatrix *m,*r;
	CError err;

	err = stack_get_val(stack,&m,cmatrix);
	if (err)
		return err;

	switch (func->num) {
  	case MATRIX_ECHELON:
  		r = cmatrix_echelon(m);
  		break;
	case FUNC_CONJ: /* Transpose */
		r = cmatrix_transpose(m);
		break;
	case MATRIX_QRQ:
		r = cmatrix_qrq(m);
		if (r == NULL)
			err = c_singular;
		break;
	case MATRIX_QRR:
		r = cmatrix_qrr(m);
		if (r == NULL)
			err = c_singular;
		break;
	case MATRIX_QRS:
		if(m->rows != m->cols - 1){
			err = c_baddim;
			break;
		}
		r = cmatrix_qrs(m);
		if (r == NULL)
			err = c_singular;
		break;	      
	default:
		err = c_internal;
		break;
	}

	cmatrix_delete(m);

	if (!err){
		err = stack_add_val(stack,&r,cmatrix);
		cmatrix_delete(r);
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
cmatrix_func(Functype *func,CodeStack *stack)
{
	CMatrix *m;
	CError err;
	Complex result;

	err = stack_get_val(stack,&m,cmatrix);
	if (err)
		return err;

	switch (func->num) {
	case MATRIX_DET:
		if (m->cols != m->rows) {
			err = c_baddim;
			break;
		}
		result = cmatrix_deter(m);
		break;
	default:
		err = c_internal;
		break;
	}
	cmatrix_delete(m);
	if (!err)
		err = stack_add_val(stack,&result,complex);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_dim
 * 
 * DESCRIPTION:  Return dimension of complex matrix
 *
 * PARAMETERS:   matrix on stack
 *
 * RETURN:       list(rows:columns)
 *      
 ***********************************************************************/
CError 
cmatrix_dim(Functype *func, CodeStack *stack)
{
	CError err;
	CMatrix *m;
	List *result;
	
	err = stack_get_val(stack,&m,cmatrix);
	if (err)
		return err;
	
	result = list_new(2);
	result->item[0].imag = result->item[1].imag = 0.0;
	result->item[0].real = m->rows;
	result->item[1].real = m->cols;

	err = stack_add_val(stack,&result,list);
	
	list_delete(result);
	cmatrix_delete(m);
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     cmatrix_item
 * 
 * DESCRIPTION:  Function for the M[1:2] type of operation
 *
 * PARAMETERS:   matrix, row [, column]
 *
 * RETURN:       complex number or list
 *      
 ***********************************************************************/
CError
cmatrix_item(Functype *func, CodeStack *stack)
{
	CError err;
	CMatrix *m;
	Int32 row,col;

	if (func->paramcount != 3)
		return c_badargcount;

	err = stack_get_val(stack, &col, integer);
	if (err) 
		return err;

	err = stack_get_val(stack, &row, integer);
	if (err) 
		return err;

	err = stack_get_val(stack, &m, cmatrix);
	if (err) 
		return err;

	if ( row > m->rows || col > m->cols || (col == 0 && row==0)){
		cmatrix_delete(m);
		return c_baddim;
	}

	if(col && row)
		err = stack_add_val(stack,&MATRIX(m,row-1,col-1),complex);
	else{
		Int16 i;
		List *lst;
		if(row==0){	
			lst=list_new(m->rows);
			if(!lst){
				cmatrix_delete(m);
				return c_memory;
			}
			for(i=0;i<m->rows;i++)
				lst->item[i]=MATRIX(m,i,col-1);
		}else{
			lst=list_new(m->cols);
			if(!lst){
				cmatrix_delete(m);
				return c_memory;
			}
			for(i=0;i<m->cols;i++)
				lst->item[i]=MATRIX(m,row-1,i);
		}
		err = stack_add_val(stack,&lst,list);
		list_delete(lst);
	}
		
	cmatrix_delete(m);
	return err;
}
