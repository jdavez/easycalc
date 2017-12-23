/*
 *   $Id: calcDB.cpp,v 1.7 2011/02/28 22:07:47 mapibid Exp $
 *
 *   Scientific Calculator for Palms.
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
#include "stdafx.h"

#include "compat/PalmOS.h"
#include <string.h>

#include "defuns.h"
#include "konvert.h"
#include "calcDB.h"
#include "stack.h"
#include "compat/dbutil.h"

static DmOpenRef gDB;

static Int16 CompareRecordFunc(dbPackedRecord *rec1,dbPackedRecord *rec2,
        Int16 unesedInt16,SortRecordInfoPtr unused1,SortRecordInfoPtr unused2,
        MemHandle appInfoH) MLIB;
static Int16 CompareRecordFunc(dbPackedRecord *rec1,dbPackedRecord *rec2,
        Int16 unesedInt16,SortRecordInfoPtr unused1,SortRecordInfoPtr unused2,
        MemHandle appInfoH)
{
    Int16 tmp;

    /* This will sort it somewhat intelligently, regardless of capitalized
     * letters */
    tmp=StrCaselessCompare(rec1->name,rec2->name);
    if (tmp)
      return tmp;
    return StrCompare(rec1->name,rec2->name);
}

/***********************************************************************
 *
 * FUNCTION:     db_create_record
 *
 * DESCRIPTION:  Create a record in a database, possibly overwriting
 *               an existing one
 *
 * PARAMETERS:   newRecord - pointer to a new record
 *               size - size of a new record
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
static CError db_create_record(dbPackedRecord *newRecord,UInt16 size) MLIB;
static CError
db_create_record(dbPackedRecord *newRecord,UInt16 size)
{
    UInt16 recordNumber;
    MemHandle myRecordMemHandle;
    dbPackedRecord *newRecordPtr;
    CError err = c_noerror;

    recordNumber = DmFindSortPosition(gDB,newRecord,0,
                     (DmComparF *)CompareRecordFunc,0);
    if (recordNumber > 0) {  /* We might modify the record */
        dbPackedRecord *record;
        MemHandle recordMemHandle;
        Int16 foundIt;

        recordMemHandle = DmQueryRecord(gDB,recordNumber - 1);
        record = (dbPackedRecord *) MemHandleLock(recordMemHandle);
        foundIt = StrCompare(newRecord->name,record->name) == 0;
        MemHandleUnlock(recordMemHandle);
        if (foundIt) {
            /* OK, we have to modify existing record */
            MemHandle modRecord = DmGetRecord(gDB,recordNumber-1);

            /* resize the handle */
            if (!MemHandleResize(modRecord,size)) {
                record = (dbPackedRecord *) MemHandleLock(modRecord);
                DmWrite(record,0,newRecord,size);
                MemHandleUnlock(modRecord);
            }
            else
                err = c_memory;
            DmReleaseRecord(gDB,recordNumber-1,true);
            return (err);
        }
    }
    myRecordMemHandle = DmNewRecord(gDB,&recordNumber,size);
    if (myRecordMemHandle ) {
        newRecordPtr = (dbPackedRecord *) MemHandleLock(myRecordMemHandle);
        DmWrite(newRecordPtr,0,newRecord,size);
        MemHandleUnlock(myRecordMemHandle);
        DmReleaseRecord(gDB,recordNumber,true); /* Now the recordNumber contains real index */
    } else
        err = c_memory;

    return (err);
}


/***********************************************************************
 *
 * FUNCTION:     db_recompile_all
 *
 * DESCRIPTION:  Recompiles functions stored in database and removes
 *               functions where a recompile fails. It should be called
 *               whenever a function is inserted into konvert.c
 *
 * PARAMETERS:   None
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void
db_recompile_all(void)
{
    dbList *list;
    Int16 i;
    TCHAR *origtext;
    CError err;
    CodeStack *stack;
    TCHAR oldparam[MAX_FUNCNAME+1];

    /* Save the default parameter name */
    StrCopy(oldparam,parameter_name);

    list = db_get_list(function);
    for (i=0;i<list->size;i++) {
        err = db_func_description(list->list[i],&origtext,parameter_name);
        if (err) {
            db_delete_record(list->list[i]);
            continue;
        }
        stack = text_to_stack(origtext,&err);
        if (err) {
            MemPtrFree(origtext);
            db_delete_record(list->list[i]);
            continue;
        }
        db_write_function(list->list[i],stack,origtext);
        stack_delete(stack);
        MemPtrFree(origtext);
    }
    db_delete_list(list);
    StrCopy(parameter_name,oldparam);
}

/***********************************************************************
 *
 * FUNCTION:     db_write_function
 *
 * DESCRIPTION:  Write a defined piece of code into database
 *
 * PARAMETERS:   name - name of function
 *               stack - compiled code
 *               origtext - original uncompiled code
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError db_write_function(const TCHAR *name,CodeStack *stack,TCHAR *origtext) MLIB;
CError
db_write_function(const TCHAR *name,CodeStack *stack,TCHAR *origtext)
{
    dbPackedRecord *record;
    dbPackedStack *pstack;
    Int16 psize,rsize;
    byte *ptr;
    CError err;

    pstack = stack_pack(stack,&psize);
    rsize = sizeof(*record) + psize + (StrLen(origtext)+1) * sizeof(TCHAR);
    record = (dbPackedRecord *) MemPtrNew(rsize);
    if (!record)
        return c_memory;

    record->type = function;
    StrCopy(record->name,name);
    memcpy(&record->u.stack,pstack,psize);

    StrCopy(record->u.stack.header.param_name,parameter_name);

// Mapi: Fix ??
// Or Microsoft compiler isn't handling that one the same way than Palm app compilers
//    ptr = (byte *) &(record->u.stack.data);
    ptr = (byte *) (record->u.stack.data);
    ptr += record->u.stack.header.orig_offset;
    StrCopy((TCHAR *) ptr, origtext);
    rsize -= sizeof(dbStackItem)>sizeof(dbPackedStack)?sizeof(dbStackItem):sizeof(dbPackedStack);
    err = db_create_record(record,rsize);
    MemPtrFree(record);
    MemPtrFree(pstack);
    return err;
}

/***********************************************************************
 *
 * FUNCTION:     db_read_function
 *
 * DESCRIPTION:  Read a compiled function from database
 *
 * PARAMETERS:   name - name of the function
 *
 * RETURN:       CodeStack - compiled code
 *               err - error code or 0
 *
 ***********************************************************************/
CodeStack * db_read_function(const TCHAR *name,CError *err) MLIB;
CodeStack *
db_read_function(const TCHAR *name,CError *err)
{
    dbPackedRecord *record;
    CodeStack *result;

    *err = c_noerror;

    record = db_read_record(name);
    if (!record || record->type!=function) {
        if (record)
          MemPtrFree(record);
        *err = c_notvar;
        return NULL;
    }
    result = stack_unpack(&record->u.stack);
    MemPtrFree(record);

    return result;
}

/***********************************************************************
 *
 * FUNCTION:     db_write_variable
 *
 * DESCRIPTION:  Write variable to database
 *
 * PARAMETERS:   name - name of the variable
 *               contents - the value of the variable
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
CError db_write_variable(const TCHAR *name,Trpn contents) MLIB;
CError
db_write_variable(const TCHAR *name,Trpn contents)
{
    dbPackedRecord *record;
    dbStackItem *packitem;
    UInt16 adatalen; /* Additional data size */
    CError err;

    packitem = rpn_pack_record(contents);
    if(!packitem)
        return c_memory;

    adatalen = packitem->datasize;

    record = (dbPackedRecord *) MemPtrNew(sizeof(*record) + adatalen);
    if (!record) {
        MemPtrFree(packitem);
        return c_memory;
    }
    record->type = variable;
    StrCopy(record->name,name);
    memcpy(record->u.data,packitem,sizeof(*packitem)+adatalen);

    MemPtrFree(packitem);
    err = db_create_record(record,sizeof(*record)+adatalen);

    MemPtrFree(record);

    return err;
}
/***********************************************************************
 *
 * FUNCTION:     db_read_variable
 *
 * DESCRIPTION:  Read a variable value from database
 *
 * PARAMETERS:   name - name of the requested variable
 *
 * RETURN:       Trpn - value of the variable
 *               err - error code or zero
 *
 ***********************************************************************/
Trpn db_read_variable(const TCHAR *name,CError *err) MLIB;
Trpn
db_read_variable(const TCHAR *name,CError *err)
{
    Trpn result;
    dbPackedRecord *record;

    *err = c_noerror;

    record = db_read_record(name);
    if (!record || record->type!=variable) {
        if (record)
          MemPtrFree(record);
        *err = c_notvar;
        memset(&result, 0, sizeof(result)); // Mapi: added this to return an initialized value
                                            // in all cases.
        return result;
    }
    result = rpn_unpack_record(record->u.data);
    MemPtrFree(record);

    return result;
}

/***********************************************************************
 *
 * FUNCTION:     db_delete_list
 *
 * DESCRIPTION:  Free memory allocated by list
 *
 * PARAMETERS:   list - list to free
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void
db_delete_list(dbList *list)
{
    Int16 i;

    for (i=0;i<list->size;i++)
      MemPtrFree(list->list[i]);
    if (list->type)
      MemPtrFree(list->type);
    MemPtrFree(list);
}

/***********************************************************************
 *
 * FUNCTION:     db_get_list
 *
 * DESCRIPTION:  Creates a dynamic list of all requested records
 *
 * PARAMETERS:   type - type of records to build a list
 *                      or 0 for all types
 *
 * RETURN:       dbList - array of all records of given type
 *
 ***********************************************************************/
dbList *
db_get_list(rpntype type)
{
    UInt16 totalItems = DmNumRecords(gDB);
    UInt16 count,i;
    MemHandle recordHandle;
    dbPackedRecord *record;
    dbList *result;

    result = (dbList *) MemPtrNew(sizeof(dbList)+totalItems*sizeof(result->list[0]));
    result->type = (rpntype *) MemPtrNew(sizeof(result->type[0])*totalItems);

    for (i=0,count=0;i<totalItems;i++) {
        recordHandle = DmQueryRecord(gDB,i);
        record = (dbPackedRecord *) MemHandleLock(recordHandle);
        if (!type || record->type == type ||
            (record->type == variable
             && record->u.data[0].rpn.type == type)) {
            result->type[count] = record->type;
            result->list[count] = (TCHAR *) MemPtrNew((StrLen(record->name)+1)*sizeof(TCHAR));
            StrCopy(result->list[count],record->name);
            count++;
        }
        MemHandleUnlock(recordHandle);
    }
    result->size = count;
    return result;
}

/***********************************************************************
 *
 * FUNCTION:     db_func_description
 *
 * DESCRIPTION:  Return function pre-compilation text
 *
 * PARAMETERS:   funcname - name of the requested function
 *
 * RETURN:       CError - c_noerror on success
 *               dest - pointer to a dynamically allocated chunk of memory
 *               param - parameter name ('x'), that was used for compialtion
 *
 ***********************************************************************/
CError db_func_description(const TCHAR *funcname,TCHAR **dest,TCHAR *param) MLIB;
CError db_func_description(const TCHAR *funcname,TCHAR **dest,TCHAR *param) {
    dbPackedRecord *tmp;

    tmp = db_read_record(funcname);
    if (!tmp)
      return c_notvar;
    if (tmp->type != function) {
        MemPtrFree(tmp);
        return c_badarg;
    }
    *dest = (TCHAR *) MemPtrNew((StrLen((TCHAR *) (tmp->u.stack.data + tmp->u.stack.header.orig_offset))
                                 +1
                                )*sizeof(TCHAR)
                               );
    StrCopy(*dest, (TCHAR *) (tmp->u.stack.data + tmp->u.stack.header.orig_offset));
    if (param)
        StrCopy(param, tmp->u.stack.header.param_name);

    MemPtrFree(tmp);
    return c_noerror;
}

void
db_save_real(const TCHAR *name, double number)
{
    CodeStack *stack;
    Trpn item;

    stack = stack_new(1);
    stack_add_val(stack,&number,real);
    item = stack_pop(stack);
    stack_delete(stack);
    db_write_variable(name,item);
    rpn_delete(item);
}

/***********************************************************************
 *
 * FUNCTION:     db_read_record
 *
 * DESCRIPTION:  Reads a record from a database
 *
 * PARAMETERS:   name - name of the record (variable or function)
 *
 * RETURN:       pointer to a dynamical record or NULL on error
 *
 ***********************************************************************/
dbPackedRecord * db_read_record(const TCHAR *name) MLIB;
dbPackedRecord *
db_read_record(const TCHAR *name)
{
    UInt16 recordNumber;
    Int16 foundIt=0;
    dbPackedRecord tmp;

    StrCopy(tmp.name,name);
    recordNumber=DmFindSortPosition(gDB,&tmp,0,
                    (DmComparF *)CompareRecordFunc,0);
    if (recordNumber >0) {
        MemHandle theRecordMemHandle;
        dbPackedRecord *record;

        theRecordMemHandle = DmQueryRecord(gDB,recordNumber - 1);

        record = (dbPackedRecord *) MemHandleLock(theRecordMemHandle);
        foundIt = StrCompare(name,record->name)==0;
        if (foundIt) {
            dbPackedRecord *result;

            result = (dbPackedRecord *) MemPtrNew(MemHandleSize(theRecordMemHandle));
            if (result)
                memcpy(result,record,MemHandleSize(theRecordMemHandle));
            MemHandleUnlock(theRecordMemHandle);

            return result;
        }
        MemHandleUnlock(theRecordMemHandle);
    }
    return NULL;
}

/***********************************************************************
 *
 * FUNCTION:     db_record_exists
 *
 * DESCRIPTION:  Check, if there is a corresponding record in the
 *               database
 *
 * PARAMETERS:   name - name of the record
 *
 * RETURN:       true - record exists, otherwise false
 *
 ***********************************************************************/
Boolean db_record_exists(const TCHAR *name) MLIB;
Boolean
db_record_exists(const TCHAR *name)
{
    UInt16 recordNumber;
    bool foundIt=false;
    dbPackedRecord tmprec;

    StrCopy(tmprec.name,name);

    recordNumber=DmFindSortPosition(gDB,&tmprec,0,
                    (DmComparF *)CompareRecordFunc,0);

    if (recordNumber >0) {
        MemHandle theRecordMemHandle;
        dbPackedRecord *record;

        theRecordMemHandle = DmQueryRecord(gDB,recordNumber - 1);

        record = (dbPackedRecord *) MemHandleLock(theRecordMemHandle);
        foundIt = (StrCompare(tmprec.name,record->name)==0);
        MemHandleUnlock(theRecordMemHandle);
    }
    return foundIt;
}

/***********************************************************************
 *
 * FUNCTION:     db_delete_record
 *
 * DESCRIPTION:  Deletes a record of a given name from the database
 *
 * PARAMETERS:   name - name of the record
 *
 * RETURN:       true - record existed in database
 *               false - there was no such record
 *
 ***********************************************************************/
Int16 db_delete_record(const TCHAR *name) MLIB;
Int16
db_delete_record(const TCHAR *name)
{
    UInt16 recordNumber;
    Int16 foundIt=0;
    dbPackedRecord tmprec;

    StrCopy(tmprec.name,name);

    recordNumber=DmFindSortPosition(gDB,&tmprec,0,
                    (DmComparF *)CompareRecordFunc,0);

    if (recordNumber >0) {
        MemHandle theRecordMemHandle;
        dbPackedRecord *record;

        theRecordMemHandle = DmQueryRecord(gDB,recordNumber - 1);

        record = (dbPackedRecord *) MemHandleLock(theRecordMemHandle);
        foundIt = StrCompare(tmprec.name,record->name)==0;
        MemHandleUnlock(theRecordMemHandle);
        if (foundIt)
          DmRemoveRecord(gDB,recordNumber-1);
    }
    return foundIt;
}

/***********************************************************************
 *
 * FUNCTION:     db_open
 *
 * DESCRIPTION:  Open a main database for EasyCalc, creates a new one,
 *               if there isn't such a database or if the old database
 *               is bad version
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       0 - on success
 *               other - on error
 *
 ***********************************************************************/
Int16 db_open() MLIB;
Int16
db_open()
{
    return open_db(DBNAME, DBVERSION, LIB_ID, DBTYPE, &gDB);
}

Int16 db_close() MLIB;
Int16
db_close()
{
    return DmCloseDatabase(gDB);
}
