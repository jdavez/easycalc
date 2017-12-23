/*
 *   $Id: meqstack.cpp,v 1.1 2009/10/17 13:48:34 mapibid Exp $
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
#include "StdAfx.h"

#include "compat/PalmOS.h"
#include <string.h>
#include "stack.h"
#include "meqstack.h"

#define COUNT_INIT  15
#define COUNT_GROW  2

/***********************************************************************
 *
 * FUNCTION:     meq_new
 * 
 * DESCRIPTION:  Return initial of dynamically growing stack
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Dynamic stack, allocates COUNT_INIT on the beginning
 *      
 ***********************************************************************/
Tdynmeq *
meq_new(void)
{
	Tdynmeq *result;

	result = (Tdynmeq *) MemPtrNew(sizeof(*result));

	result->count = 0;
	result->allocated = COUNT_INIT;
	result->array = (Tmeq *) MemPtrNew(sizeof(result->array[0])*COUNT_INIT);

	return result;
}

/***********************************************************************
 *
 * FUNCTION:     meq_push
 * 
 * DESCRIPTION:  Push a new item on stack, grow allocated space if needed
 *
 * PARAMETERS:   array - stack
 *               item - new item
 *
 * RETURN:       Position of new item on stack
 *      
 ***********************************************************************/
Int16
meq_push(Tdynmeq *array,Tmeq item)
{
	Tmeq *tmp;

	if (array->count == array->allocated) {
		if (array->array == NULL) {
			tmp = (Tmeq *) MemPtrNew(sizeof(*tmp)*COUNT_INIT);
			array->allocated = COUNT_INIT;
		} else {
			tmp = (Tmeq *) MemPtrNew(sizeof(*tmp)*array->allocated*COUNT_GROW);
			memcpy(tmp,array->array,sizeof(*tmp)*array->count);
			MemPtrFree(array->array);
			array->allocated *= COUNT_GROW;
		}
		array->array = tmp;
	}
	array->array[array->count] = item;
	return array->count++;
}

/***********************************************************************
 *
 * FUNCTION:     meq_last
 * 
 * DESCRIPTION:  Return the top item on stack but do not remove it from
 *               stack
 *
 * PARAMETERS:   array - stack
 *
 * RETURN:       top item
 *      
 ***********************************************************************/
Tmeq
meq_last(Tdynmeq *array)
{
	return array->array[array->count - 1];
}

/***********************************************************************
 *
 * FUNCTION:     meq_pop
 * 
 * DESCRIPTION:  Remove top item from stack and return it
 *
 * PARAMETERS:   array - stack
 *
 * RETURN:       top item
 *      
 ***********************************************************************/
Tmeq
meq_pop(Tdynmeq *array)
{
	ErrFatalDisplayIf(array->count==0, _T("Meq underflowed"));
	return array->array[--array->count];
}

/***********************************************************************
 *
 * FUNCTION:     meq_fetch
 * 
 * DESCRIPTION:  Remove first item from stack and return it
 *
 * PARAMETERS:   array - stack
 *
 * RETURN:       first item
 *      
 ***********************************************************************/
Tmeq
meq_fetch(Tdynmeq *array)
{
    Tmeq result;

    ErrFatalDisplayIf(array->count==0,_T("Meq underflowed"));
    result = array->array[0];
 
    array->count--;
    memmove(&array->array[0],&array->array[1],sizeof(array->array[0])*array->count);
    
    return result;
}


/***********************************************************************
 *
 * FUNCTION:     meq_free
 * 
 * DESCRIPTION:  Free all memory associated with the stack
 *
 * PARAMETERS:   array - stack
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
meq_free(Tdynmeq *array)
{
	Int16 i;

	for (i=array->count-1;i>=0;i--)
		rpn_delete(array->array[i].rpn);

	if (array->array) 
		MemPtrFree(array->array);
	array->array = NULL;
	array->allocated = 0;
	array->count = 0;

	MemPtrFree(array);
}

/***********************************************************************
 *
 * FUNCTION:     meq_count
 * 
 * DESCRIPTION:  Returns number of items on the stack
 *
 * PARAMETERS:   array - stack
 *
 * RETURN:       number of items on stack
 *      
 ***********************************************************************/
Int16
meq_count(Tdynmeq *array)
{
	return array->count;
}

/***********************************************************************
 *
 * FUNCTION:     meq_update_f_pcount
 * 
 * DESCRIPTION:  Update function parameter count
 *
 * PARAMETERS:   array - stack
 *               position - position of corresponding item on stack
 *               pcount - new parameter count
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
meq_update_f_pcount(Tdynmeq *array,Int16 position,Int16 pcount)
{
	array->array[position].rpn.u.funcval.paramcount = pcount;
}

/***********************************************************************
 *
 * FUNCTION:     meq_update_t_pcount
 * 
 * DESCRIPTION:  Update text function parameter count
 *
 * PARAMETERS:   array - stack
 *               position - position of corresponding item on stack
 *               pcount - new parameter count
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
meq_update_t_pcount(Tdynmeq *array,Int16 position,Int16 pcount)
{
	array->array[position].rpn.u.textfunc.paramcount = pcount;
}
