/*
 *   $Id: slist.h,v 1.22 2006/09/25 20:49:05 cluny Exp $
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
*/

#ifndef _LIST_H_
#define _LIST_H_

#define LIST_INPUT      1000
#define LIST_MIN        1001
#define LIST_MAX        1002
#define LIST_MEAN       1003
#define LIST_SUM        1004
#define LIST_MEDIAN     1005
#define LIST_SORTA      1006
#define LIST_SORTD      1007
#define LIST_GMEAN      1008
#define LIST_PROD       1009
#define LIST_VARIANCE   1010
#define LIST_STDDEV     1011
#define LIST_LINREG     1012
#define LIST_DIM        1013
#define LIST_CUMSUM     1014
#define LIST_MODE       1015
#define LIST_MOMENT     1016
#define LIST_SKEWNESS   1017
#define LIST_KURTOSIS   1018
#define LIST_VARCOEF    1019
#define LIST_RAND       1020
#define LIST_RNORM      1021
#define LIST_RANGE      1022
#define LIST_RTOP       1023
#define LIST_PTOR       1024
#define LIST_CONV       1025
#define LIST_CONCAT     1026
#define LIST_KRON       1027
#define LIST_SAMPLE     1028
#define LIST_FFT	      1029
#define LIST_IFFT	      1030
#define LIST_DFT	      1031
#define LIST_IDFT       1032
#define LIST_DFTSHIFT   1033
#define LIST_FIND       1034

List * list_new(Int16 size) NEWFUNC;
List * list_dup(List *oldlist) NEWFUNC;
CError list_input(Functype *func,CodeStack *stack) NEWFUNC;
void list_delete(List *lst) NEWFUNC;
CError list_func(Functype *func,CodeStack *stack) NEWFUNC;
CError list_func2(Functype *func,CodeStack *stack) NEWFUNC;
CError list_func3(Functype *func,CodeStack *stack) NEWFUNC;
CError list_concat(Functype *func,CodeStack *stack) NEWFUNC;
CError list_stat(Functype *func,CodeStack *stack) NEWFUNC;
CError list_regr(Functype *func,CodeStack *stack) NEWFUNC;
CError list_rand(Functype *func,CodeStack *stack) NEWFUNC;
CError list_item(Functype *func, CodeStack *stack) NEWFUNC;
CError list_range(Functype *func, CodeStack *stack) NEWFUNC;
CError list_mathem(Functype *func, CodeStack *stack) NEWFUNC;
CError list_filter(Functype *func,CodeStack *stack) NEWFUNC;
CError list_factor(Functype *func,CodeStack *stack) NEWFUNC;
CError list_gcdex(Functype *func,CodeStack *stack) NEWFUNC;
CError list_sift(Functype *func,CodeStack *stack) NEWFUNC;
CError list_map(Functype *func,CodeStack *stack) NEWFUNC;
CError list_zip(Functype *func,CodeStack *stack) NEWFUNC;
CError list_repeat(Functype *func,CodeStack *stack) NEWFUNC;
CError list_fourier(Functype *func,CodeStack *stack) NEWFUNC;

#endif
