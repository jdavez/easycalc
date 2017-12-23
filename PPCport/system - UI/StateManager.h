#pragma once

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H 1

#include "compat/DataManager.h"
#include "core/prefs.h"
#include "core/lstedit.h"
#include "core/grprefs.h"

// Filename, including path, max size.
#define FILENAMELEN 256

// Number of skins that one can load and access
#define NB_SKINS    4

typedef struct {
    TCHAR      skinName[NB_SKINS][FILENAMELEN];
    TCHAR      langName[FILENAMELEN];
    int        cur_skin_nb;
    bool       mouse_cont;
    tPrefs     calcPrefs;
    tlistPrefs listPrefs;
    TCHAR      matrixName[MAX_FUNCNAME+1];
    TgrPrefs   graphPrefs;
    // Always add new fields at end, for keeping version migration simple
} t_state;

class StateManager {
protected:
    TCHAR   *stateFilename;
    FILE    *stateFile;
    int      init_mode;

    void init_shell_state(int4 stVersion, void *hWnd_p);
    int4 shell_read_saved_state(void *buf, int4 bufsize);
    bool shell_write_saved_state(const void *buf, int4 nbytes);
    bool read_shell_state(void *hWnd_p);
    bool write_shell_state(void);
    void initCalcPrefs(void);

public:
    t_state     state;
    int4        state_version;
    DataManager historyDB;
    DataManager calcDB;

    StateManager(void);
    ~StateManager(void);

    t_state *getState(void);
    void init(TCHAR *fn, void *hWnd_p);
    void save(TCHAR *fn, void *hWnd_p);
};

#endif