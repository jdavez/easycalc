/*
 *   $Id: history.cpp,v 1.6 2011/02/28 22:07:47 mapibid Exp $
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

#include "StdAfx.h"

#include "compat/PalmOS.h"
#include "Skin.h"

#include <string.h>

#include "calcDB.h"
#include "stack.h"
#include "defuns.h"
#include "history.h"
#include "display.h"
#include "compat/dbutil.h"

static DmOpenRef dbRef;

UInt16
history_total(void) MLIB;
UInt16
history_total(void)
{
	return DmNumRecords(dbRef);
}

void
history_add_line(TCHAR *line)
{
	tHistory *item,*newitem;
	UInt16 size;
	MemHandle newHandle;
	UInt16 recordNumber;
	
	size = sizeof(*item) + (StrLen(line) + 1)*sizeof(TCHAR);
	item = (tHistory *) MemPtrNew(size);
	item->isrpn = false;
	StrCopy(item->u.text,line);
	
	recordNumber = 0;
	newHandle = DmNewRecord(dbRef,&recordNumber,size);	
	newitem = (tHistory *) MemHandleLock(newHandle);
	DmWrite(newitem,0,item,size);
	MemHandleUnlock(newHandle);
	DmReleaseRecord(dbRef,recordNumber,true);
	
	MemPtrFree(item);

    // Limit history size to 100 items
    while ((size = DmNumRecords(dbRef)) > 100) {
        // Mapi: corrected behavior to keep last 100 records in a rolling window
//        DmRemoveRecord(dbRef, 0);
        DmRemoveRecord(dbRef, size-1);
    }
}

/***********************************************************************
 *
 * FUNCTION:     history_add
 * 
 * DESCRIPTION:  Add a record in a records database
 *
 * PARAMETERS:   item - the record that should be added
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void
history_add_item(Trpn item)
{
	dbStackItem *packitem;
	UInt16 size,psize;
	MemHandle newHandle;
	UInt16 recordNumber;
	tHistory *histitem,*newitem;
	
	packitem = rpn_pack_record(item);
	if (!packitem)
	    return;
	size = sizeof(*histitem) + packitem->datasize;
	psize = sizeof(*packitem) + packitem->datasize;
	histitem = (tHistory *) MemPtrNew(size);
	if (!histitem) {
	    MemPtrFree(packitem);
	    return;
	}
	histitem->isrpn = true;
	memcpy((void *)&histitem->u.item,(void *)packitem,psize);
	
	recordNumber = 0;
	newHandle = DmNewRecord(dbRef,&recordNumber,size);
	if (!newHandle) {
	    MemPtrFree(packitem);
	    MemPtrFree(histitem);
	    return;
	}
	newitem = (tHistory *) MemHandleLock(newHandle);
	DmWrite(newitem,0,histitem,size);
	MemHandleUnlock(newHandle);
	DmReleaseRecord(dbRef,recordNumber,true);
	
	MemPtrFree(packitem);
	MemPtrFree(histitem);
}

/***********************************************************************
 *
 * FUNCTION:     history_shrink
 * 
 * DESCRIPTION:  Shrink the history detabase to a defined
 *               number of records
 *
 * PARAMETERS:   How many records leave in database
 *
 * RETURN:       None
 *      
 ***********************************************************************/
void 
history_shrink(Int16 count)
{
	Int16 i;

	if (DmNumRecords(dbRef)==0)
	  return;
	i = DmNumRecords(dbRef)-1;
	while (i>=count) {
		DmRemoveRecord(dbRef,i--);
	}
}


/***********************************************************************
 *
 * FUNCTION:     history_get
 * 
 * DESCRIPTION:  Return a history record
 *
 * PARAMETERS:   num - how old the item is
 *
 * RETURN:       item
 *      
 ***********************************************************************/
Trpn 
history_get_item(UInt16 num)
{
	MemHandle recordHandle;
	tHistory *historyitem;
	Trpn result;
	
	recordHandle = DmQueryRecord(dbRef,num);
	historyitem = (tHistory *) MemHandleLock(recordHandle);
	result = rpn_unpack_record(&historyitem->u.item);
	MemHandleUnlock(recordHandle);
	
	return result;
}

TCHAR *
history_get_line(UInt16 num)
{
	MemHandle recordHandle;
	tHistory *historyitem;
	TCHAR *result;
	
	recordHandle = DmQueryRecord(dbRef,num);
	historyitem = (tHistory *) MemHandleLock(recordHandle);
	result = (TCHAR *) MemPtrNew((StrLen(historyitem->u.text) + 1)*sizeof(TCHAR));
	StrCopy(result,historyitem->u.text);
	MemHandleUnlock(recordHandle);
	
	return result;
}

Boolean
history_isrpn(UInt16 num)
{
	MemHandle recordHandle;
	tHistory *historyitem;
	Boolean result;
	
	recordHandle = DmQueryRecord(dbRef,num);
	historyitem = (tHistory *) MemHandleLock(recordHandle);
	result = historyitem->isrpn;
	MemHandleUnlock(recordHandle);
	
	return result;
}


/***********************************************************************
 *
 * FUNCTION:     history_command
 * 
 * DESCRIPTION:  This function is evaluated directly from a normal
 *               mathematical equation
 *
 * PARAMETERS:   On stack - 1 number convertible to integer
 *
 * RETURN:       On stack - value from history database
 *      
 ***********************************************************************/
CError
history_command(Functype *func,CodeStack *stack)
{
	CError err;
	UInt32 arg;
	
	if ((err=stack_get_val(stack,&arg,integer)))
	  return err;	
	if (arg > DmNumRecords(dbRef))
	  return c_badarg;

	if (!history_isrpn(arg))
		return c_badarg;
	stack_push(stack,history_get_item(arg));
	
	return c_noerror;
}

/***********************************************************************
 *
 * FUNCTION:     history_open
 * 
 * DESCRIPTION:  Open a history database
 *
 * PARAMETERS:   None
 *
 * RETURN:       0 on success
 *      
 ***********************************************************************/
Int16 
history_open(void)
{
	return open_db(HISTORYDBNAME, HIST_DB_VERSION, LIB_ID, DBTYPE, &dbRef);
}

/***********************************************************************
 *
 * FUNCTION:     history_close
 *
 * DESCRIPTION:  Close a history database
 *
 * PARAMETERS:   None
 *
 * RETURN:       0 on success
 *      
 ***********************************************************************/
Int16 
history_close(void)
{
	history_shrink(HISTORY_RECORDS);
	return DmCloseDatabase(dbRef);
}
