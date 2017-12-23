/*   
 *   $Id: calcDB.h,v 1.7 2006/09/12 19:40:55 tvoverbe Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999 Ondrej Palkovsky
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

#ifndef _CALCDB_H_
#define _CALCDB_H_

#include <PalmOS.h>
#include "konvert.h"

typedef struct {
	Int16 size;
	rpntype *type;
	char *list[0];
	/* Do not add anything here, it gets overwritten */
}dbList;

typedef struct {
	Int16 count;
	Int16 orig_offset;
	char param_name[MAX_FUNCNAME+1];
}dbStackHeader;

typedef struct {
	Trpn rpn;
	UInt16 datasize; /* size of additional packed data */
	char data[0];
}dbStackItem;

typedef struct {
	dbStackHeader header;
	char data[0];
}dbPackedStack;

typedef struct {
	rpntype type;  /* This can be ONLY variable/function, nothing else can be
					* stored in databases */
	char name[MAX_FUNCNAME+1];
	union {
		dbStackItem data[1];  /* Variable */
		dbPackedStack stack;		
	}u;	
}dbPackedRecord;

CError db_write_function(const char *name,CodeStack *stack,char *origtext) MLIB;
CodeStack * db_read_function(const char *name,CError *err) MLIB;
CError db_write_variable(const char *name,Trpn contents) MLIB;
Trpn db_read_variable(const char *name,CError *err) MLIB;
CError  db_func_description(const char *funcname,char **dest,char *param) MLIB;
dbPackedRecord * db_read_record(const char *name) MLIB;
Int16  db_delete_record(const char *name) MLIB;
void db_recompile_all(void) MLIB;
Int16 db_open() MLIB;
Int16 db_close() MLIB;
Boolean db_record_exists(const char *name) MLIB;

void db_delete_list(dbList *list) MLIB;
dbList * db_get_list(rpntype type) MLIB;
void  db_save_real(const char *name, double number) MLIB;

#endif
