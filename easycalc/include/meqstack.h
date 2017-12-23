/*
 *   $Id: meqstack.h,v 1.3 2006/09/12 19:40:55 tvoverbe Exp $
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
*/

#ifndef _MEQSTACK_H_
#define _MEQSTACK_H_

#include "konvert.h"

typedef struct {
	Int16 allocated;
	Int16 count;
	Tmeq *array;
}Tdynmeq;

Int16 meq_push(Tdynmeq *array,Tmeq item);
void meq_free(Tdynmeq *array);
void meq_update_t_pcount(Tdynmeq *array,Int16 position,Int16 pcount);
void meq_update_f_pcount(Tdynmeq *array,Int16 position,Int16 pcount);
Tmeq meq_pop(Tdynmeq *array);
Tmeq meq_last(Tdynmeq *array);
Tmeq meq_fetch(Tdynmeq *array);
Int16 meq_count(Tdynmeq *array);
Tdynmeq *meq_new(void);


#endif
