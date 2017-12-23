/*
 *   $Id: ansops.c,v 1.4 2006/09/12 19:40:55 tvoverbe Exp $
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

#include <PalmOS.h>
#include "konvert.h"
#include "stack.h"
#include "result.h"
#include "defuns.h"
#include "ansops.h"

#include "calcrsc.h"

/***********************************************************************
 *
 * FUNCTION:     ans_redisplay
 * 
 * DESCRIPTION:  Executes a code and display its result on display
 *               If the result is string, display it as formatted
 *
 * PARAMETERS:   text - code to execute
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void ans_redisplay(char *text)
{
	CError err;
	CodeStack *stack;
	Trpn vysledek;
	
	stack = text_to_stack(text,&err);
	if (!err) {
		err = stack_compute(stack);
		if (!err) {
			vysledek = stack_pop(stack);
			if (vysledek.type == string)
			  result_set_pow(vysledek.u.stringval);
			else
			  result_set(vysledek);
			rpn_delete(vysledek);
		}
		stack_delete(stack);
	}
	if (err) 
	  result_error(err);
	
	result_draw();
}

/***********************************************************************
 *
 * FUNCTION:     ans_guess
 * 
 * DESCRIPTION:  Guesses last entered value and displays the result on display
 *
 * PARAMETERS:   None
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void 
ans_guess()
{
	char *result;
	CError err;
	Trpn ans;
	
	ans = db_read_variable("ans",&err);
	if (err) {
		FrmAlert(altAnsProblem);
		return;
	}
	result = guess(ans);
	if (!result) {
		FrmAlert(altGuessNotFound);
		rpn_delete(ans);
		return;
	}

	result_set_pow(result);
	rpn_delete(ans);
	MemPtrFree(result);
	result_draw();
}
