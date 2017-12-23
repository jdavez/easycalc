/*
 *   $Id: stack.h,v 1.8 2007/05/29 15:32:31 cluny Exp $
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

#ifndef _STACK_H_
#define _STACK_H_

#include "calcDB.h"
#include "segment.h"

dbPackedStack * stack_pack(CodeStack *stack,Int16 *size) MLIB;
CodeStack * stack_unpack(dbPackedStack *packed) MLIB;
dbStackItem * rpn_pack_record(Trpn item) MLIB;
Trpn rpn_unpack_record(dbStackItem *record) MLIB;
void rpn_delete(Trpn item) MLIB;
void rpn_duplicate(Trpn *dest,Trpn source) MLIB;
CError stack_get_val(CodeStack *stack,void *arg1,rpntype reqtype) MLIB;
CError stack_get_val2(CodeStack *stack,void *arg1,void *arg2,rpntype reqtype) MLIB;
CError stack_add_val(CodeStack *stack,void *arg,rpntype reqtype) MLIB;
CError stack_item_type(CodeStack *stack,rpntype *type,Int16 itnum) MLIB;
CError stack_compute(CodeStack *instack) MLIB;
Trpn stack_pop(CodeStack *stack) MLIB;
void stack_push(CodeStack *stack,Trpn rpn) MLIB;
CodeStack * stack_new(Int16 count) MLIB;
void stack_delete(CodeStack *stack) MLIB;
void stack_copy(CodeStack *dest,CodeStack *orig) MLIB;
CError  stack_item_type_nr(CodeStack *stack,rpntype *type,Int16 itnum) MLIB;
void stack_reverse(CodeStack *stack) MLIB;
double reduce_precision(double value) MLIB;
void stack_fix_variables(CodeStack *stack) MLIB;

#endif
