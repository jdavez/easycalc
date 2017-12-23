/*
 * $Id: calc.h,v 1.12 2006/11/10 04:12:56 tvoverbe Exp $
 * 
 * Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000 Ondrej Palkovsky
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

#ifndef _CALC_H_
#define _CALC_H_

void *GetObjectPtr(UInt16 objectID);
void GotHardKey(void);
Int16 chooseForm(Int16 button);
void alertErrorMessage(CError err);
void ChangeForm(Int16 formID);
void wait_draw(void);
void wait_erase(void);
void calc_nil_timeout(Int32 timeout);
Int16 calc_input_exec(void);
void gadget_bounds(FormType *frm, Int16 gadget,
                   RectangleType *natbounds, RectangleType *stdbounds);

extern Boolean palmOS3;
extern Boolean palmOS35;
extern Boolean handera;
extern Boolean colorDisplay;
extern Boolean grayDisplay;

#endif
