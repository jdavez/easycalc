#include "StdAfx.h"

#define _MEMORYMANAGER_C_
#include "compat/PalmOS.h"
#include "MemoryManager.h"

#define MEMH_MAGIC 0x4D454D48

c_MemHandle::c_MemHandle(void) {
    magic   = MEMH_MAGIC;
    owner   = NULL;
    size    = 0;
    memPtr  = NULL;
    lockCnt = 0;
}

c_MemHandle::~c_MemHandle(void) {
    if (memPtr != NULL)
        mfree(memPtr);
}

/***********************************************************************
 * Store the c_MemHandle object to a file, in binary mode.
 ***********************************************************************/
int c_MemHandle::serialize(FILE *f) {
    if (fwrite(&magic, sizeof(magic), 1, f) != 1)
        return (-1);
    if (fwrite(&owner, sizeof(owner), 1, f) != 1)
        return (-1);
    if (fwrite(&size, sizeof(size), 1, f) != 1)
        return (-1);
    if (fwrite(&memPtr, sizeof(memPtr), 1, f) != 1)
        return (-1);
    if (fwrite(&lockCnt, sizeof(lockCnt), 1, f) != 1)
        return (-1);
    if (memPtr != NULL) {
        size_t siz = _mmsize(memPtr);
        if (fwrite(&siz, sizeof(siz), 1, f) != 1)
            return (-1);
        if (fwrite(memPtr, siz, 1, f) != 1)
            return (-1);
    }

    return (0);
}

/***********************************************************************
 * Retrieve the c_MemHandle object from a file, in binary mode.
 ***********************************************************************/
int c_MemHandle::deSerialize(FILE *f) {
    if (fread(&magic, sizeof(magic), 1, f) != 1)
        return (-1);
    if (fread(&owner, sizeof(owner), 1, f) != 1)
        return (-1);
    if (fread(&size, sizeof(size), 1, f) != 1)
        return (-1);
    void *pmemPtr;
    if (fread(&pmemPtr, sizeof(memPtr), 1, f) != 1)
        return (-1);
    if (fread(&lockCnt, sizeof(lockCnt), 1, f) != 1)
        return (-1);
    if (pmemPtr != NULL) {
        size_t siz;
        if (fread(&siz, sizeof(siz), 1, f) != 1)
            return (-1);
        memPtr = mmalloc(siz);
        if (fread(memPtr, siz, 1, f) != 1)
            return (-1);
        // Re-establish proper link to this object at beginning of memPtr
        *((UInt32 *) memPtr) = (UInt32) this;
    }

    return (0);
}

/* Static method */
MemHandle c_MemHandle::MemHandleNew (UInt32 size) {
    MemHandle h = new c_MemHandle;
    if (h != NULL) {
        // Add 4 bytes, round up to next 4 bytes limit
        if ((h->memPtr = mmalloc((size+7) & 0xFFFFFFFC)) != NULL) {
            // Put metadata MemHandle address in allocated memory
            *((UInt32 *) (h->memPtr)) = (UInt32) h;
            h->size = size;
        } else {
            delete h;
            h = NULL;
        }        
    }
    return (h);
}

/* Static method */
Err c_MemHandle::MemHandleFree (MemHandle h) {
    delete h;
    return (0);
}

void c_MemHandle::setOwner (void *owner) {
    this->owner = owner;
}

void *c_MemHandle::getOwner (void) {
    return (this->owner);
}

MemPtr c_MemHandle::lock (void) {
    this->lockCnt++;
    // Skip metadata pointer
    return ((MemPtr) (((UInt32 *) (this->memPtr)) + 1));
}

MemPtr c_MemHandle::getMemAddr (void) {
    return ((MemPtr) (((UInt32 *) (this->memPtr)) + 1));
}

/* Static method */
MemHandle c_MemHandle::GetMemHandle (MemPtr p) {
    // Retrieve metadata pointer
    MemHandle h = (MemHandle) (*(((UInt32 *) p) - 1));
    if (h->magic != MEMH_MAGIC) {
        h = NULL;
    }
    return (h);
}

Err c_MemHandle::unlock (void) {
    if (this->lockCnt > 0)   this->lockCnt--;
    return (0);
}

UInt32 c_MemHandle::getSize (void) {
    return (this->size);
}

Err c_MemHandle::resize (UInt32 newSize) {
    if (this->lockCnt > 0)
        return (-1);
    if (this->size == newSize) // No resize to do
        return (0);
    if ((newSize == 0) && (this->memPtr)) { // Free existing allocated memory
        mfree(this->memPtr);
        this->memPtr = NULL;
        this->size = 0;
    } else {
        // Add 4 bytes, round up to next 4 bytes limit
        void *p = mrealloc(this->memPtr, (newSize+7) & 0xFFFFFFFC);
        if (p != NULL) {
            this->memPtr = p;
            // Put metadata MemHandle address in allocated memory
            *((UInt32 *) this->memPtr) = (UInt32) this;
            this->size = newSize;
        } else {
            return (-2);
        }
    }
    return (0);
}

#ifdef _DEBUG
#define MEM_MAGIC_ALLOC 0xACACACAC
#define MEM_MAGIC_FREED 0xFCFCFCFC

struct s_debugMalloc {
    int                   size;
    struct s_debugMalloc *prec;
    struct s_debugMalloc *next;
    int                   magic;
};

#define DBG_ADDSIZE (sizeof(struct s_debugMalloc)+4)

#include <stdio.h>
static FILE *trcfile = NULL;
void init_debug_malloc (TCHAR *trcpath) {
    TCHAR fn[128];
    _tcscpy(fn, trcpath);
    _tcscat(fn, _T("\\malloctrc.txt"));
    trcfile = _tfopen(fn, _T("w"));
}

struct s_debugMalloc *free_chain = NULL;
struct s_debugMalloc *free_tail = NULL;
struct s_debugMalloc *alloc_chain = NULL;
struct s_debugMalloc *alloc_tail = NULL;

void end_debug_malloc (void) {
    debug_check_allocated(true);
    fclose(trcfile);
    debug_check_free(true);
}

char *debug__strdup (TCHAR *sfile, int sline, const char *strSource) {
//    char *s = _strdup(strSource);
    int size = strlen(strSource) + 1;
    int wrounded_size = ((size+3) & 0xFFFFFFFC);
    struct s_debugMalloc *p = (struct s_debugMalloc *) malloc(wrounded_size+DBG_ADDSIZE);
    int alloc_size = _msize(p);
    p->size = size;
    p->prec = alloc_tail;
    if (alloc_chain == NULL)
        alloc_tail = alloc_chain = p;
    else
        alloc_tail = alloc_tail->next = p;
    p->next = NULL;
    p->magic = MEM_MAGIC_ALLOC;
    _ftprintf(trcfile, _T("malloc(_strdup)\t0x%08X\t%d\t%s\t%d\n"), (int) (p+1), alloc_size, sfile, sline);
    memcpy(p+1, strSource, size);
    *((int *)(((byte *)p)+alloc_size-4)) = MEM_MAGIC_ALLOC;
    return((char *) (p+1));
//    return (s);
}

TCHAR *debug__tcsdup (TCHAR *sfile, int sline, const TCHAR *strSource) {
//    TCHAR *s = _tcsdup(strSource);
    int size = (_tcslen(strSource) + 1) * sizeof(TCHAR);
    int wrounded_size = ((size+3) & 0xFFFFFFFC);
    struct s_debugMalloc *p = (struct s_debugMalloc *) malloc(wrounded_size+DBG_ADDSIZE);
    int alloc_size = _msize(p);
    p->size = size;
    p->prec = alloc_tail;
    if (alloc_chain == NULL)
        alloc_tail = alloc_chain = p;
    else
        alloc_tail = alloc_tail->next = p;
    p->next = NULL;
    p->magic = MEM_MAGIC_ALLOC;
    _ftprintf(trcfile, _T("malloc(_tcsdup)\t0x%08X\t%d\t%s\t%d\n"), (int) (p+1), alloc_size, sfile, sline);
    memcpy(p+1, strSource, size);
    *((int *)(((byte *)p)+alloc_size-4)) = MEM_MAGIC_ALLOC;
    return((TCHAR *) (p+1));
//    return (s);
}

void *debug_malloc (TCHAR *sfile, int sline, size_t size) {
    int wrounded_size = ((size+3) & 0xFFFFFFFC);
    struct s_debugMalloc *p = (struct s_debugMalloc *) malloc(wrounded_size+DBG_ADDSIZE);
    int alloc_size = _msize(p);
    p->size = size;
    p->prec = alloc_tail;
    if (alloc_chain == NULL)
        alloc_tail = alloc_chain = p;
    else
        alloc_tail = alloc_tail->next = p;
    p->next = NULL;
    p->magic = MEM_MAGIC_ALLOC;
    _ftprintf(trcfile, _T("malloc\t0x%08X\t%d\t%d\t%s\t%d\n"), (int) (p+1), alloc_size, size, sfile, sline);
    memset(p+1, 0, alloc_size-DBG_ADDSIZE);
    *((int *)(((byte *)p)+alloc_size-4)) = MEM_MAGIC_ALLOC;
    return((void *) (p+1));
}

size_t debug__msize(void *memblock) {
    struct s_debugMalloc *p = ((struct s_debugMalloc *) memblock)-1;
    return (_msize(p) - DBG_ADDSIZE);
}

void *debug_realloc (TCHAR *sfile, int sline, void *memblock, size_t size) {
    size_t old_size;
    struct s_debugMalloc *op;
    if (memblock == NULL) {
        old_size = 0;
        op = NULL;
    } else {
        old_size = debug__msize(memblock);
        op = ((struct s_debugMalloc *) memblock)-1;

        // Verify old buffer
        int *trig_error = NULL;
        if (op->magic != MEM_MAGIC_ALLOC) {
            op->magic = *trig_error;
        }
        if (*((int *)(((byte *)(op+1))+old_size)) != MEM_MAGIC_ALLOC) {
            op->magic = *trig_error;
        }
    }

    int wrounded_size = ((size+3) & 0xFFFFFFFC);
    int alloc_size = 0;
    //    struct s_debugMalloc *p = (struct s_debugMalloc *) realloc(op, wrounded_size+DBG_ADDSIZE);
    struct s_debugMalloc *p = (struct s_debugMalloc *) malloc(wrounded_size+DBG_ADDSIZE);
    if (p != op) { // Get rid of op and prepare p
        if (op != NULL) {
            if (free_chain == NULL) {
                free_tail = free_chain = op;
            } else {
                free_tail = free_tail->next = op;
            }
            // Unchain buffer from alloc list and put it in the free list.
            if (op->prec != NULL)
                op->prec->next = op->next;
            else {
                alloc_chain = op->next;
            }
            if (op->next == NULL) {
                alloc_tail = op->prec;
            } else {
                op->next->prec = op->prec;
                op->next = NULL;
            }
            // "Free" buffer
            op->magic = MEM_MAGIC_FREED;
            memset(op+1, 0, old_size);
            *((int *)(((byte *)(op+1))+old_size)) = MEM_MAGIC_FREED;
        }
        if (p != NULL) {
            alloc_size = _msize(p);
            p->size = size;
            p->prec = alloc_tail;
            if (alloc_chain == NULL)
                alloc_tail = alloc_chain = p;
            else {
                alloc_tail = alloc_tail->next = p;
            }
            p->next = NULL;
            p->magic = MEM_MAGIC_ALLOC;
            *((int *)(((byte *)p)+alloc_size-4)) = MEM_MAGIC_ALLOC;
        }
    } else { // p and op are both NULL, or p == op
        if (p != NULL) {
            alloc_size = _msize(p);
            p->size = size;
            *((int *)(((byte *)p)+alloc_size-4)) = MEM_MAGIC_ALLOC;
        }
    }

    _ftprintf(trcfile, _T("realloc\t0x%08X\t%d\t%d\t0x%08X\t%d\t%s\t%d\n"), (int) (p+1), alloc_size, size, (int) memblock, old_size+DBG_ADDSIZE, sfile, sline);

    if (p == NULL)
        return (NULL);
    return((void *) (p+1));
}

void debug_free (TCHAR *sfile, int sline, void *memblock) {
    int *trig_error = NULL;
    struct s_debugMalloc *p = ((struct s_debugMalloc *) memblock)-1;
    if (p->magic != MEM_MAGIC_ALLOC) {
        p->magic = *trig_error;
    }
    p->magic = MEM_MAGIC_FREED;
    size_t size = debug__msize(memblock);
    if (*((int *)(((byte *)(p+1))+size)) != MEM_MAGIC_ALLOC) {
        p->magic = *trig_error;
    }
    memset(p+1, 0, size);
    *((int *)(((byte *)(p+1))+size)) = MEM_MAGIC_FREED;
    _ftprintf(trcfile, _T("free\t0x%08X\t%d\t%d\t%s\t%d\n"), (int) memblock, size+DBG_ADDSIZE, size, sfile, sline);
//    free(p);
    if (free_chain == NULL) {
        free_tail = free_chain = p;
    } else {
        free_tail = free_tail->next = p;
    }
    // Unchain buffer from alloc list and put it in the free list.
    if (p->prec != NULL)
        p->prec->next = p->next;
    else {
        alloc_chain = p->next;
    }
    if (p->next == NULL) {
        alloc_tail = p->prec;
    } else {
        p->next->prec = p->prec;
        p->next = NULL;
    }
}

bool debug_check_allocated (bool final) {
    bool chain_ok = true;
    int  size;
    struct s_debugMalloc *p = alloc_chain;

    while (p != NULL) {
        size = debug__msize(p+1);
        if (final)
            _ftprintf(trcfile, _T("still allocated:\t0x%08X\t%d\t%d\n"), (int) (p+1), size+DBG_ADDSIZE, size);
        // Verify allocated buffer.
        if (p->magic != MEM_MAGIC_ALLOC) {
            chain_ok = false;
            if (final)
                _fputts(_T("\tInvalid start magic !!"), trcfile);
            else   break;
        }
        if (*((int *)(((byte *)(p+1))+size)) != MEM_MAGIC_ALLOC) {
            chain_ok = false;
            if (final)
                _fputts(_T("\tInvalid end magic !!"), trcfile);
            else   break;
        }

        // All is ok, go to next one.
        p = p->next;
    }

    return (chain_ok);
}

bool debug_check_free (bool final) {
    bool chain_ok = true;
    int *trig_error = NULL;
    int *tmp, size;
    struct s_debugMalloc *p = free_chain, *p1;

    while (p != NULL) {
        // Verify "freed" buffer.
        if (p->magic != MEM_MAGIC_FREED) {
            chain_ok = false;
            if (final)
                p->magic = *trig_error;
            else   break;
        }
        size = debug__msize(p+1);
        if (*((int *)(((byte *)(p+1))+size)) != MEM_MAGIC_FREED) {
            chain_ok = false;
            if (final)
                p->magic = *trig_error;
            else   break;
        }
        tmp = (int *) (p + 1);
        while (size > 0) {
            if (*tmp != 0) {
                chain_ok = false;
                if (final)
                    p->magic = *trig_error;
                else   break;
            }
            tmp++;
            size -= sizeof(int);
        }

        // All is ok, go to next one, after really freeing it if we are in the
        // final debug check.
        p1 = p->next;
        if (final)
            free(p);
        p = p1;
    }

    return (chain_ok);
}

#endif