#pragma once

#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H 1

#include "PalmOS.h"

typedef void *MemPtr;

class c_MemHandle {
protected:
    UInt32 magic;
    void  *owner;
    UInt32 size;
    void  *memPtr;
    int    lockCnt;

public:
    c_MemHandle(void);
    ~c_MemHandle(void);
    int serialize(FILE *f);
    int deSerialize(FILE *f);

    static c_MemHandle *MemHandleNew (UInt32 size);
    static Err MemHandleFree (c_MemHandle *h);
    void setOwner (void *owner);
    void *getOwner (void);
    MemPtr lock (void);
    MemPtr getMemAddr (void);
    static c_MemHandle *GetMemHandle (MemPtr p);
    Err unlock (void);
    UInt32 getSize (void);
    Err resize (UInt32 newSize);
};

typedef c_MemHandle *MemHandle;

#ifndef _MEMORYMANAGER_C_
#define MemHandleNew c_MemHandle::MemHandleNew
#ifdef _DEBUG
#define MemHandleFree(h) {c_MemHandle::MemHandleFree(h);h=NULL;}
#else
#define MemHandleFree c_MemHandle::MemHandleFree
#endif
#define MemHandleLock(h) ((h)->lock())
#define MemHandleUnlock(h) ((h)->unlock())
#define MemHandleSize(h) ((h)->getSize())
#define MemHandleResize(h,ns) ((h)->resize(ns))
#endif

#ifdef _DEBUG
#define __WFILE__ _T(__FILE__)
void init_debug_malloc(TCHAR *trcpath);
void end_debug_malloc(void);
bool debug_check_allocated(bool final);
bool debug_check_free(bool final);
char *debug__strdup(TCHAR *sfile, int sline, const char *strSource);
TCHAR *debug__tcsdup(TCHAR *sfile, int sline, const TCHAR *strSource);
void *debug_malloc(TCHAR *sfile, int sline, size_t size);
size_t debug__msize(void *memblock);
void *debug_realloc(TCHAR *sfile, int sline, void *memblock, size_t size);
void debug_free(TCHAR *sfile, int sline, void *memblock);
#define MemPtrNew(s)  debug_malloc(__WFILE__, __LINE__, (s))
#define MemPtrFree(m) {debug_free(__WFILE__, __LINE__, (void *)(m));(m)=NULL;}
#define _mstrdup(s)  debug__strdup(__WFILE__, __LINE__, (s))
#define _mtcsdup(s)  debug__tcsdup(__WFILE__, __LINE__, (s))
#define mmalloc(s)  debug_malloc(__WFILE__, __LINE__, (s))
#define _mmsize  debug__msize
#define mrealloc(m,s)  debug_realloc(__WFILE__, __LINE__, (void *)(m), (s))
#define mfree(m) {debug_free(__WFILE__, __LINE__, (void *)(m));(m)=NULL;}
#else
#define MemPtrNew  malloc
#define MemPtrFree free
#define _mstrdup _strdup
#define _mtcsdup _tcsdup
#define mmalloc  malloc
#define _mmsize  _msize
#define mrealloc  realloc
#define mfree(m) free((void *)(m))
#endif

#endif