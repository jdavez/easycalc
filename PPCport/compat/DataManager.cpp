#include "StdAfx.h"
#include "compat/PalmOS.h"
#define _DATAMANAGER_C_
#include "DataManager.h"
#include "defuns.h"

#define RECD_MAGIC    0x52454344

DataManager::DataManager(void) {
    in_state   = false;
    attr       = 0;
    type       = 0;
    creator    = 0;
    version    = 0;
    numRecords = 0;
    head       = NULL;
    sortArray  = NULL;
    name       = NULL;
}

static void freeDR (DataRecord *dr) {
    if (dr != NULL) {
        freeDR(dr->next);
        delete dr;
    }
}

DataManager::~DataManager(void) {
    // Free all records in the DB
    freeDR(head);
    if (sortArray != NULL)   mfree(sortArray);
    if (name != NULL)   mfree(name);
}

/***********************************************************************
 * Store the DataManager object to a file, in binary mode.
 ***********************************************************************/
int DataManager::serialize(FILE *f) {
    // Save previous file mode for restoring later.
    int prev_mode = _setmode(f, _O_BINARY);

    // Save object itself
    if (fwrite(&in_state, sizeof(in_state), 1, f) != 1)
        return (-1);
    if (fwrite(&attr, sizeof(attr), 1, f) != 1)
        return (-1);
    if (fwrite(&type, sizeof(type), 1, f) != 1)
        return (-1);
    if (fwrite(&creator, sizeof(creator), 1, f) != 1)
        return (-1);
    if (fwrite(&version, sizeof(version), 1, f) != 1)
        return (-1);
    if (fwrite(&numRecords, sizeof(numRecords), 1, f) != 1)
        return (-1);
    if (fwrite(&head, sizeof(head), 1, f) != 1)
        return (-1);
    if (fwrite(&sortArray, sizeof(sortArray), 1, f) != 1)
        return (-1);
    if (fwrite(&name, sizeof(name), 1, f) != 1)
        return (-1);

    // Save elements pointed at by the object.
    // First, the name
    if (name != NULL) {
        size_t siz = _mmsize(name);
        if (fwrite(&siz, sizeof(siz), 1, f) != 1)
            return (-1);
        if (fwrite(name, siz, 1, f) != 1)
            return (-1);
    }
    // Then, the records, numbering them
    int num = 0;
    if (head != NULL) {
        DataRecord *temp = head;
        while (temp != NULL) {
            temp->num = num++;
            if (temp->serialize(f) != 0)
                return (-1);
            temp = temp->next;
        }
    }
    // And last, the sort array. Save record numbers instead of addresses,
    // for rebuild at read.
    if (sortArray != NULL) {
//        size_t siz = _msize(sortArray);
//        if (fwrite(&siz, sizeof(siz), 1, f) != 1)
//            return (-1);
//        if (fwrite(sortArray, siz, 1, f) != 1)
//            return (-1);
        size_t siz = num * sizeof(int);
        if (fwrite(&siz, sizeof(siz), 1, f) != 1)
            return (-1);
        for (int i=0 ; i<num ; i++) {
            if (fwrite(&(sortArray[i]->num), sizeof(int), 1, f) != 1)
                return (-1);
        }
    }

    // Restore previous file mode.
    _setmode(f, prev_mode);

    return (0);
}

/***********************************************************************
 * Retrieve the DataManager object from a file, in binary mode.
 ***********************************************************************/
int DataManager::deSerialize(FILE *f) {
    // Save previous file mode for restoring later.
    int prev_mode = _setmode(f, _O_BINARY);

    // Load object itself
    if (fread(&in_state, sizeof(in_state), 1, f) != 1)
        return (-1);
    if (fread(&attr, sizeof(attr), 1, f) != 1)
        return (-1);
    if (fread(&type, sizeof(type), 1, f) != 1)
        return (-1);
    if (fread(&creator, sizeof(creator), 1, f) != 1)
        return (-1);
    if (fread(&version, sizeof(version), 1, f) != 1)
        return (-1);
    if (fread(&numRecords, sizeof(numRecords), 1, f) != 1)
        return (-1);
    void *phead;
    if (fread(&phead, sizeof(head), 1, f) != 1)
        return (-1);
    void *psortArray;
    if (fread(&psortArray, sizeof(sortArray), 1, f) != 1)
        return (-1);
    void *pname;
    if (fread(&pname, sizeof(name), 1, f) != 1)
        return (-1);

    // Load elements pointed at by the object.
    // First, the name
    if (pname != NULL) {
        size_t siz;
        if (fread(&siz, sizeof(siz), 1, f) != 1)
            return (-1);
        name = (TCHAR *) mmalloc(siz);
        if (fread(name, siz, 1, f) != 1)
            return (-1);
    }
    // Then, numbered records, storing their allocated address
    DataRecord **listArray = (DataRecord **) mmalloc(numRecords * sizeof(DataRecord *));
    if (phead != NULL) {
        DataRecord *temp1 = NULL, *temp2;
        for (int i=0 ; i<numRecords ; i++) {
            temp2 = new DataRecord;
            if (temp2->deSerialize(f) != 0) {
                delete temp2;
                return (-1);
            }
            // Remember its address by index
            listArray[temp2->num] = temp2;
            if (temp1 == NULL) {
                head = temp1 = temp2;
            } else {
                temp1->next = temp2;
                temp1 = temp2;
            }
        }
    }
    // And last, the sort array. Record numbers have been saved instead of addresses.
    if (psortArray != NULL) {
        size_t siz;
        if (fread(&siz, sizeof(siz), 1, f) != 1)
            return (-1);
//        sortArray = (DataRecord **) malloc(siz);
//        if (fread(sortArray, siz, 1, f) != 1)
//            return (-1);
        sortArray = (DataRecord **) mmalloc(numRecords * sizeof(DataRecord *));
        int index;
        for (int i=0 ; i<numRecords ; i++) {
            if (fread(&index, sizeof(int), 1, f) != 1)
                return (-1);
            // We got the record number, now convert it to its allocated
            // address in memory.
            sortArray[i] = listArray[index];
        }
    }
    mfree(listArray);

    // Restore previous file mode.
    _setmode(f, prev_mode);

    return (0);
}

DataManager *stateMgrDB = NULL;     // This one goes with CalcDB.cpp
DataManager *stateMgrHistDB = NULL; // This one goes with EasyCalc.cpp
DataManager *stateMgrSolvDB = NULL; // This one goes with solver.cpp

void registerDmDatabase (UINT16 cardNo, const TCHAR *nameP, DataManager *dbId) {
    if (_tcscmp(nameP, HISTORYDBNAME) == 0) {
        stateMgrHistDB = (DataManager *) dbId;
    } else if (_tcscmp(nameP, DBNAME) == 0) {
        stateMgrDB = (DataManager *) dbId;
    } else if (_tcscmp(nameP, SOLVERDBNAME) == 0) {
        stateMgrSolvDB = (DataManager *) dbId;
    }
}

LocalID DmFindDatabase (UINT16 cardNo, const TCHAR *nameP) {
    LocalID dbId = NULL;
    if (_tcscmp(nameP, HISTORYDBNAME) == 0) {
        dbId = (LocalID) stateMgrHistDB;
    } else if (_tcscmp(nameP, DBNAME) == 0) {
        dbId = (LocalID) stateMgrDB;
    } else if (_tcscmp(nameP, SOLVERDBNAME) == 0) {
        dbId = (LocalID) stateMgrSolvDB;
    }
    return (dbId);
}

int DmDatabaseInfo (UINT16 cardNo, LocalID dbID, TCHAR *nameP, UINT16 *attributesP,
                    UINT16 *versionP, UINT32 *crDateP, UINT32 *modDateP, UINT32 *bckUpDateP,
                    UINT32 *modNumP, LocalID *appInfoIDP, LocalID *sortInfoIDP,
                    UINT32 *typeP, UINT32 *creatorP) {
    int rc = -1;
    DataManager *db = (DataManager *) dbID;
    if ((db == stateMgrHistDB) || (db == stateMgrDB) || (db == stateMgrSolvDB)) {
        *attributesP = db->attr;
        *versionP    = (UINT16) (db->version);
        *typeP       = db->type;
        *creatorP    = db->creator;
        rc = 0;
    }
    return (rc);
}

int DmSetDatabaseInfo (UINT16 cardNo, LocalID dbID, const TCHAR *nameP, UINT16 *attributesP,
                       UINT16 *versionP, UINT32 *crDateP, UINT32 *modDateP, UINT32 *bckUpDateP,
                       UINT32 *modNumP, LocalID *appInfoIDP, LocalID *sortInfoIDP,
                       UINT32 *typeP, UINT32 *creatorP) {
    if (dbID == (LocalID) stateMgrHistDB) {
        stateMgrHistDB->attr    = *attributesP;
        stateMgrHistDB->version = *versionP;
    } else if (dbID == (LocalID) stateMgrDB) {
        stateMgrDB->attr    = *attributesP;
        stateMgrDB->version = *versionP;
    } else if (dbID == (LocalID) stateMgrSolvDB) {
        stateMgrSolvDB->attr    = *attributesP;
        stateMgrSolvDB->version = *versionP;
    }
    return (0);
}

DmOpenRef DmOpenDatabase (UINT16 cardNo, LocalID dbID, UINT16 mode) {
    if (dbID == (LocalID) stateMgrHistDB) {
        return (stateMgrHistDB);
    } else if (dbID == (LocalID) stateMgrDB) {
        return (stateMgrDB);
    } else if (dbID == (LocalID) stateMgrSolvDB) {
        return (stateMgrSolvDB);
    }
    return (NULL);
}

int DmGetLastErr (void) {
    return (0);
}

int DmCreateDatabase (UINT16 cardNo, const TCHAR *nameP, UINT32 creator,
                      UINT32 type, bool resDB) {
    int rc = 0;
    if (_tcscmp(nameP, HISTORYDBNAME) == 0) {
        if (stateMgrHistDB == NULL) {
            stateMgrHistDB = new DataManager;
            stateMgrHistDB->creator = creator;
            stateMgrHistDB->type    = type;
            int l = _tcslen(nameP);
            stateMgrHistDB->name = (TCHAR *) mmalloc(2*(l+1));
            _tcscpy(stateMgrHistDB->name, nameP);
        }
        rc = 0;
    } else if (_tcscmp(nameP, DBNAME) == 0) {
        if (stateMgrDB == NULL) {
            stateMgrDB = new DataManager;
            stateMgrDB->creator = creator;
            stateMgrDB->type    = type;
            int l = _tcslen(nameP);
            stateMgrDB->name = (TCHAR *) mmalloc(2*(l+1));
            _tcscpy(stateMgrDB->name, nameP);
        }
        rc = 0;
    } else if (_tcscmp(nameP, SOLVERDBNAME) == 0) {
        if (stateMgrSolvDB == NULL) {
            stateMgrSolvDB = new DataManager;
            stateMgrSolvDB->creator = creator;
            stateMgrSolvDB->type    = type;
            int l = _tcslen(nameP);
            stateMgrSolvDB->name = (TCHAR *) mmalloc(2*(l+1));
            _tcscpy(stateMgrSolvDB->name, nameP);
        }
        rc = 0;
    }
    return (rc);
}

int DmCloseDatabase (DmOpenRef dbP) {
    return (0);
}

int DmDeleteDatabase (UINT16 cardNo, LocalID dbID) {
    DataManager *db = (DataManager *) dbID;
    if ((db == stateMgrHistDB) || (db == stateMgrDB) || (db == stateMgrSolvDB)) {
        if (db == stateMgrHistDB)   stateMgrHistDB = NULL;
        else if (db == stateMgrSolvDB)   stateMgrSolvDB = NULL;
        else   stateMgrDB = NULL;
        delete db;
    }
    return (0);
}

UINT16 DmNumRecords (DmOpenRef dbP) {
    return (dbP->numRecords);
}

MemHandle DmNewRecord (DmOpenRef dbP, UInt16 *atP, UInt32 size) {
    MemHandle p = NULL;
    if ((dbP == stateMgrHistDB) || (dbP == stateMgrDB) || (dbP == stateMgrSolvDB)) {
        if ((p = MemHandleNew(size)) != NULL) {
            DataRecord *prec = NULL;
            DataRecord *temp = dbP->head;
            if (*atP > dbP->numRecords)
                *atP = dbP->numRecords;
            dbP->numRecords++;
            // Locate the records around the one to insert
            if (dbP->sortArray == NULL) { // No sort array to maintain, and no direct access
                for (int i=0 ; (i<*atP)&&(temp!=NULL) ; i++)
                    temp = (prec = temp)->next;
            } else { // Maintain the sort array in sync
                DataRecord **destArray = (DataRecord **) mmalloc(dbP->numRecords * sizeof(DataRecord **));
                if (*atP > 0) {
                    memcpy (destArray,
                            dbP->sortArray,
                            *atP * sizeof(DataRecord **));
                    prec = dbP->sortArray[*atP-1];
                }
                if (1 < dbP->numRecords - *atP) {
                    memcpy (destArray+*atP+1,
                            dbP->sortArray+*atP,
                            (dbP->numRecords-*atP-1) * sizeof(DataRecord **));
                    temp = dbP->sortArray[*atP];
                } else
                    temp = NULL; // Insert at end.
                // Switch sortArray to the new allocated array
                mfree(dbP->sortArray);
                dbP->sortArray = destArray;
            }
            if (prec == NULL) { // Insert at head
                temp = dbP->head = new DataRecord(p, dbP->head);
            } else {
                temp = prec->next = new DataRecord(p, temp);
            }
            // Sign the mem handle
            p->setOwner((void *) RECD_MAGIC);
            // And update the new sortArray entry with new DataRecord address, if needed
            if (dbP->sortArray != NULL) {
                dbP->sortArray[*atP] = temp;
            }
        }
    }
    return (p);
}

MemHandle DmGetRecord (DmOpenRef dbP, UINT16 index) {
    return (DmQueryRecord(dbP, index));
}

int DmRemoveRecord (DmOpenRef dbP, UINT16 index) {
    if ((dbP == stateMgrHistDB) || (dbP == stateMgrDB) || (dbP == stateMgrSolvDB)) {
        DataRecord *prec = NULL;
        DataRecord *temp = dbP->head;
        if (index >= dbP->numRecords)
            return (-1);
        dbP->numRecords--;
        // Locate record to delete
        if (dbP->sortArray == NULL) { // No sort array to maintain, and no direct access
            for (int i=0 ; (i<index)&&(temp!=NULL) ; i++)
                temp = (prec = temp)->next;
        } else { // Maintain the sort array in sync
            DataRecord **destArray = NULL;
            if (dbP->numRecords > 0) { // Do not allocate a new array if size is 0
                destArray = (DataRecord **) mmalloc(dbP->numRecords * sizeof(DataRecord **));
                if (index > 0) {
                    memcpy (destArray,
                            dbP->sortArray,
                            index * sizeof(DataRecord **));
                    prec = dbP->sortArray[index-1];
                }
                if (index < dbP->numRecords) {
                    memcpy (destArray+index,
                            dbP->sortArray+index+1,
                            (dbP->numRecords-index) * sizeof(DataRecord **));
                }
                temp = dbP->sortArray[index];
            }
            // Switch sortArray to the new allocated array
            mfree(dbP->sortArray);
            dbP->sortArray = destArray;
        }
        if (prec == NULL) { // Delete at head
            dbP->head = dbP->head->next;
        } else {
            prec->next = temp->next;
        }
        MemHandleFree(temp->h);
        delete temp;
        return (0);
    }
    return (-1);
}

int DmWrite (void *recordP, UINT32 offset, const void *srcP, UINT32 bytes) {
   // Find record to which this data belongs
   MemHandle p = c_MemHandle::GetMemHandle(recordP);
   if (p == NULL)   return (-1);
   if (((UInt32) (p->getOwner())) != RECD_MAGIC)   return (-1);
   if (offset + bytes > p->getSize())   return (-2);
   memcpy (((byte *) (recordP))+offset, srcP, bytes);
   return (0);
}

MemHandle DmQueryRecord (DmOpenRef dbP, UINT16 index) {
    MemHandle p = NULL;
    if ((dbP == stateMgrHistDB) || (dbP == stateMgrDB) || (dbP == stateMgrSolvDB)) {
        DataRecord *temp = dbP->head;
        // Find record
        if (dbP->sortArray == NULL) { // No sort array, no direct access
            if (index < dbP->numRecords) {
                for (int i=0 ; i<index ; i++)
                    temp = temp->next;
                p = temp->h;
            }
        } else { // Direct access
            p = dbP->sortArray[(int)index]->h;
        }
    }
    return (p);
}

/***********************************************************
 * Binary search on a non empty sorted array.              *
 ***********************************************************/
int binSearch (DataRecord **sortArray, int numRecords,
               void *newRecord, SortRecordInfoPtr newRecordInfo,
               DmComparF *compar, Int16 other) {
    int i, imin, imax;
    imax = numRecords - 1;
    imin = 0;
    int c;

    // First, initiate conditions on boundaries of the recurring algorithm
    if (compar(newRecord,
               sortArray[imax]->h->getMemAddr(),
               other,
               NULL,                    // newRecordInfo, not used, so not implemented
               NULL,                    // sortArray[imax]->, not used, so not implemented
               NULL                     // Not used, so not implemented
              ) >= 0) {
        // After last one
        i = numRecords;
    } else if ((numRecords == 1) // First == last ! No need to redo the comparison.
               || (c = compar(newRecord,
                              sortArray[0]->h->getMemAddr(),
                              other,
                              NULL,     // newRecordInfo, not used, so not implemented
                              NULL,     // sortArray[0]->, not used, so not implemented
                              NULL      // Not used, so not implemented
                             )
                  ) < 0) {
        // Before first one
        i = 0;
    } else if ((c == 0)    // Matches with first one
               || (numRecords == 2)) { // Optimization .. We know it's neither 0 or 1,
                                       // but between them ... means insert at 1.
        i = 1;
        // If several with the same key, then insert after all of them
        while ((i < imax) // We already compared with imax, and if we reach this point,
                          // we already know that they are not equal.
                          // And note: when numRecords == 2, imax is 1.
               && (compar(newRecord,
                          sortArray[i]->h->getMemAddr(),
                          other,
                          NULL,         // newRecordInfo, not used, so not implemented
                          NULL,         // sortArray[i]->, not used, so not implemented
                          NULL          // Not used, so not implemented
                         ) == 0)
              ) {
            i++;
        }
    } else { // Regular condition as of now: somewhere between imin and imax,
             // neither of them, and imax - imin > 1.
             // This will be the recursive and propagated property.
        bool found = false;
        while (!found) {
            // Get to the middle point of current scope
            i = (imax + imin) >> 1;
            c = compar(newRecord,
                       sortArray[i]->h->getMemAddr(),
                       other,
                       NULL,            // newRecordInfo, not used, so not implemented
                       NULL,            // sortArray[i]->, not used, so not implemented
                       NULL             // Not used, so not implemented
                      );
            if (c == 0) {
                // If several with the same key, then insert after all of them
                while ((i < numRecords)
                       && (compar(newRecord,
                                  sortArray[i]->h->getMemAddr(),
                                  other,
                                  NULL, // newRecordInfo, not used, so not implemented
                                  NULL, // sortArray[i]->, not used, so not implemented
                                  NULL  // Not used, so not implemented
                                 ) == 0)
                      ) {
                    i++;
                }
                found = true;
            } else if (c < 0) { // Between imin and i, and neither of them.
                // We didn't find it yet, but if scope is reduced to 0,
                // we have found the place to insert.
                if (i - imin <= 1) {
                    // Insert at i (i.e. it will shift i by 1)
                    found = true;
                } else {
                    imax = i;
                }
            } else { // Between i and imax, and neither of them.
                // We didn't find it yet, but if scope is reduced to 0,
                // we have found the place to insert.
                if (imax - i <= 1) {
                    // Insert at imax (i.e., just after i)
                    i = imax;
                    found = true;
                } else {
                    imin = i;
                }
            }
        }
    }

    return (i);
}

UInt16 DmFindSortPosition (DmOpenRef dbP, void *newRecord, SortRecordInfoPtr newRecordInfo,
                           DmComparF *compar, INT16 other) {
    int i = -1;
    if ((dbP == stateMgrHistDB) || (dbP == stateMgrDB) || (dbP == stateMgrSolvDB)) {
        // If DB is empty .. do nothing !
        if (dbP->numRecords == 0)
            i = 0;
        else { // There something in the DB to compare to
            // If not yet sorted, then sort the DB
            if (dbP->sortArray == NULL) {
                // Allocate the sort array
                dbP->sortArray = (DataRecord **) mmalloc(dbP->numRecords * sizeof(DataRecord **));
                // Then fill it by adding elements to it in a sorted manner.
                // Add the first element from the list.
                dbP->sortArray[0] = dbP->head;
                // Add next elements of the linked list
                int num = 1;
                DataRecord *temp = dbP->head->next;
                while (temp != NULL) {
                    // Find position
                    i = binSearch(dbP->sortArray, num,
                                  newRecord, NULL, // newRecord Info is not used
                                  compar, other);
                    // Insert in the array
                    if (i+1 < num) {
                        memmove (dbP->sortArray+i+1,
                                 dbP->sortArray+i,
                                 (num-i-1) * sizeof(DataRecord **));
                    }
                    dbP->sortArray[i] = temp;

                    // Next element
                    num++;
                    temp = temp->next;
                }
            }
            // Find the record by a binary search.
            i = binSearch(dbP->sortArray, dbP->numRecords,
                          newRecord, newRecordInfo,
                          compar, other);
        }
    }
    return (i);
}

//int DmReleaseRecord (DmOpenRef dbP, UInt16 index, Boolean dirty) {
//    return (0);
//}

DataRecord::DataRecord(void) {
    magic = RECD_MAGIC;
//    size  = 0;
    h     = NULL;
    next  = NULL;
}

DataRecord::DataRecord(/*UINT32 s, */MemHandle p, DataRecord *n) {
    magic = RECD_MAGIC;
//    size = s;
    h    = p;
    next = n;
}

DataRecord::~DataRecord(void) {
    if (h != NULL)
        MemHandleFree(h);
}

/***********************************************************************
 * Store the DataRecord object to a file, in binary mode.
 ***********************************************************************/
int DataRecord::serialize(FILE *f) {
    if (fwrite(&magic, sizeof(magic), 1, f) != 1)
        return (-1);
//    if (fwrite(&size, sizeof(size), 1, f) != 1)
//        return (-1);
    if (fwrite(&num, sizeof(num), 1, f) != 1)
        return (-1);
    if (fwrite(&h, sizeof(h), 1, f) != 1)
        return (-1);
    if ((h != NULL) && (h->serialize(f) != 0))
        return (-1);

    return (0);
}

/***********************************************************************
 * Retrieve the DataRecord object from a file, in binary mode.
 ***********************************************************************/
int DataRecord::deSerialize(FILE *f) {
    if (fread(&magic, sizeof(magic), 1, f) != 1)
        return (-1);
//    if (fread(&size, sizeof(size), 1, f) != 1)
//        return (-1);
    if (fread(&num, sizeof(num), 1, f) != 1)
        return (-1);
    void *ph;
    if (fread(&ph, sizeof(h), 1, f) != 1)
        return (-1);
    if (ph != NULL) {
        h = new c_MemHandle ();
        if (h->deSerialize(f) != 0) {
            return (-1);
        }
    }

    return (0);
}
