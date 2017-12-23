#pragma once

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H 1

#include "PalmOS.h"
#include <stdio.h>

#define dmModeReadWrite 1
#define dmHdrAttrBackup 2

class DataRecord {
public:
    UInt32      magic;
//    UInt32      size;
    int         num;
    MemHandle   h;
    DataRecord *next;

    DataRecord(void);
    DataRecord(/*UINT32 s, */MemHandle p, DataRecord *n);
    ~DataRecord(void);
    int serialize(FILE *f);
    int deSerialize(FILE *f);
};

typedef struct {
    UInt8 attributes;
    UInt8 uniqueID[3];
} SortRecordInfoType;

typedef SortRecordInfoType *SortRecordInfoPtr;

typedef Int16 DmComparF (void *rec1, void *rec2, Int16 other,
                         SortRecordInfoPtr rec1SortInfo,
                         SortRecordInfoPtr rec2SortInfo,
                         MemHandle appInfoH);

class DataManager {
public:
    bool        in_state;
    UInt16      attr;
    UInt32      type;
    UInt32      creator;
    UInt16      version;
    UInt16      numRecords;
    DataRecord *head;
    DataRecord **sortArray; // To support binary searches in DmFindSortPosition
    TCHAR      *name;

    DataManager(void);
    ~DataManager(void);
    int serialize(FILE *f);
    int deSerialize(FILE *f);
};

typedef DataManager *DmOpenRef;
typedef void *LocalID;

void registerDmDatabase (UINT16 cardNo, const TCHAR *nameP, DataManager *dbId);

LocalID DmFindDatabase (UINT16 cardNo, const TCHAR *nameP);
int DmDatabaseInfo (UINT16 cardNo, LocalID dbID, TCHAR *nameP, UINT16 *attributesP,
                    UINT16 *versionP, UINT32 *crDateP, UINT32 *modDateP, UINT32 *bckUpDateP,
                    UINT32 *modNumP, LocalID *appInfoIDP, LocalID *sortInfoIDP,
                    UINT32 *typeP, UINT32 *creatorP);
int DmSetDatabaseInfo (UINT16 cardNo, LocalID dbID, const Char *nameP,
                       UINT16 *attributesP, UINT16 *versionP, UINT32 *crDateP,
                       UINT32 *modDateP, UINT32 *bckUpDateP, UINT32 *modNumP,
                       LocalID *appInfoIDP, LocalID *sortInfoIDP, UINT32 *typeP,
                       UINT32 *creatorP);
DmOpenRef DmOpenDatabase (UINT16 cardNo, LocalID dbID, UINT16 mode);
int DmGetLastErr (void);
int DmCreateDatabase (UINT16 cardNo, const Char *nameP, UINT32 creator, UINT32 type, Boolean resDB);
int DmCloseDatabase (DmOpenRef dbP);
int DmDeleteDatabase (UINT16 cardNo, LocalID dbID);
UInt16 DmNumRecords (DmOpenRef dbP);
MemHandle DmNewRecord (DmOpenRef dbP, UINT16 *atP, UINT32 size);
MemHandle DmGetRecord (DmOpenRef dbP, UINT16 index);
int DmWrite (void *recordP, UINT32 offset, const void *srcP, UINT32 bytes);
int DmRemoveRecord (DmOpenRef dbP, UINT16 index);
MemHandle DmQueryRecord (DmOpenRef dbP, UInt16 index);
UINT16 DmFindSortPosition (DmOpenRef dbP, void *newRecord, SortRecordInfoPtr newRecordInfo,
                           DmComparF *compar, Int16 other);


//Err DmReleaseRecord (DmOpenRef dbP, UInt16 index, Boolean dirty);
#define DmReleaseRecord(a,b,c)

#ifndef _DATAMANAGER_C_
extern DataManager *stateMgrDB;
extern DataManager *stateMgrHistDB;
extern DataManager *stateMgrSolvDB;
#endif

#endif