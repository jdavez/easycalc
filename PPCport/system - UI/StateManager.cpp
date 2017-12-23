#include "StdAfx.h"

#include "system - UI/StateManager.h"
#include "core/mlib/calcDB.h"
#include "core/mlib/fp.h"
#include "core/mlib/history.h"
#include "core/mlib/mathem.h"
#include "EasyCalc.h"

StateManager::StateManager (void) {
    // Set interface and calculator options to their defaults
    stateFile = NULL;
    _tcscpy(state.skinName[0], _T("EasyCalc"));
    _tcscpy(state.skinName[1], _T("EasyCalc2"));
    _tcscpy(state.skinName[2], _T("EasyCalc3"));
    _tcscpy(state.skinName[3], _T("EasyCalcG"));
    state.cur_skin_nb = 0;
    _tcscpy(state.langName, _T("en")); // Language = english by default
    state.mouse_cont = true;
    state.listPrefs.list[0][0] = _T('\0');
    state.listPrefs.list[1][0] = _T('\0');
    state.listPrefs.list[2][0] = _T('\0');
    state.listPrefs.list[3][0] = _T('\0');
    state.matrixName[0] = _T('\0');

    historyDB.in_state = false;
}

StateManager::~StateManager (void) {
    if (stateMgrDB != NULL)
        delete stateMgrDB;
    if (stateMgrHistDB != NULL)
        delete stateMgrHistDB;
    if (stateMgrSolvDB != NULL)
        delete stateMgrSolvDB;
}

void StateManager::initCalcPrefs (void) {
    /* Coming from prefs.c */
    state.calcPrefs.version = PREF_VERSION;
    state.calcPrefs.input[0]=_T('\0');
    state.calcPrefs.skin = 0;
    state.calcPrefs.btnRow = 0;
    state.calcPrefs.insertPos = 0;
    state.calcPrefs.selPosStart=calcPrefs.selPosEnd = 0;
    state.calcPrefs.solverWorksheet = -1;
    state.calcPrefs.finBegin = false;
    state.calcPrefs.matchParenth = false;
    state.calcPrefs.trigo_mode = radian;

    state.calcPrefs.dispPrefs.forceInteger = false;
    state.calcPrefs.dispPrefs.decPoints = 9;
    state.calcPrefs.dispPrefs.stripZeros = true;
    state.calcPrefs.reducePrecision = false;
    state.calcPrefs.dispPrefs.base = disp_decimal;
    state.calcPrefs.dispPrefs.mode = disp_normal;
    state.calcPrefs.dispPrefs.cvtUnits = true;
    state.calcPrefs.dispPrefs.penMode = pen_centerwide;
    state.calcPrefs.insertHelp = true;
    state.calcPrefs.acceptOSPref = false;
    state.calcPrefs.dispScien = false;

    state.graphPrefs.xmin = -7.0;
    state.graphPrefs.xmax = 7.0;
    state.graphPrefs.ymin = -7.0;
    state.graphPrefs.ymax = 7.0;
    state.graphPrefs.xscale = 1.0;
    state.graphPrefs.yscale = 1.0;
    state.graphPrefs.fimin = 0.0;
    state.graphPrefs.fimax = M_PIl * 2.0;
    state.graphPrefs.fistep = M_PIl * 0.02;
    state.graphPrefs.tmin = 0.0;
    state.graphPrefs.tmax = 10.0;
    state.graphPrefs.tstep = 0.1;
    state.graphPrefs.functype = graph_func;
    for (int i=0 ; i<MAX_GRFUNCS ; i++) {
        graphPrefs.funcFunc[i][0]='\0';
        graphPrefs.funcPol[i][0]='\0';
        graphPrefs.funcPar[i][0][0]='\0';
        graphPrefs.funcPar[i][1][0]='\0';
    }
    state.graphPrefs.logx = false;
    state.graphPrefs.logy = false;
    state.graphPrefs.speed = 1;
    for (int i=0; i<9; i++){
        /* Note: grEnable[8] enables axis labels,
         *       colors[8] is the background color
         */
        // Re-note: this structure is never used for now in the Windows version,
        // since there is no palette to change colors specified in the .layout file
        // and no way to enable/disable axis or grid on the GUI.
        for (int ft=graph_func ; ft<nb_func_types ; ft++)
            state.graphPrefs.grEnable[ft][i] = true;
//       if (colorDisplay)
            state.graphPrefs.colors[i] = WinRGBToIndex(graphRGBColors+i);
//        if (grayDisplay)
//            graphPrefs.colors[i] = funcolors[i];
    }
    for (int i=0 ; i<6 ; i++) {
        state.graphPrefs.grType[i] = 1; // Use lines
    }

    // Publish calcPrefs, dispPrefs and graphPrefs for old core modules
    calcPrefs = state.calcPrefs;
    dispPrefs = state.calcPrefs.dispPrefs; /* db_recompile needs dispPrefs */
    graphPrefs = state.graphPrefs;
    db_recompile_all();
}

t_state *StateManager::getState (void) {
    return (&state);
}

void StateManager::init (TCHAR *fn, void *hWnd_p) {
    stateFilename = fn;
    stateFile = _tfopen(fn, _T("rb"));
    if (stateFile != NULL) {
        if (read_shell_state(hWnd_p)) {
            init_mode = 1;
        } else { // Could not read state file, create default bases and create state contents
            FrmPopupForm(altNoStateFile, hWnd_p);
            Err error = db_open();
            ErrFatalDisplayIf(error, _T("Can't open CalcDB"));
            error = history_open();
            ErrFatalDisplayIf(error, _T("Can't open History DB"));
            init_shell_state(-1, NULL);
            init_mode = 2;
        }

        fclose(stateFile);
        stateFile = NULL;
    } else { // No state file, create default bases and create state contents
        FrmPopupForm(altNoStateFile, hWnd_p);
        Err error = db_open();
        ErrFatalDisplayIf(error, _T("Can't open CalcDB"));
        error = history_open();
        ErrFatalDisplayIf(error, _T("Can't open History DB"));
        init_shell_state(-1, NULL);
        init_mode = 0;
    }

    /* Coming from prefs.c, after preferences are loaded or initialized */
    fp_set_prefs(calcPrefs.dispPrefs);
}

void StateManager::save(TCHAR *fn, void *hWnd_p) {
    state.calcPrefs = calcPrefs;
    state.graphPrefs = graphPrefs;

    stateFilename = fn;
    stateFile = _tfopen(fn, _T("wb"));
    if (stateFile != NULL) {
        if (!write_shell_state()) { // Error ...
            FrmPopupForm(altWriteStateFile, hWnd_p);
        }

        fclose(stateFile);
        stateFile = NULL;
    } else { // Error ...
        FrmPopupForm(altWriteStateFile, hWnd_p);
    }
}

/********************************************************************************
 *  FUNCTION: init_shell_state(int4)                                            *
 *  Initialize the state module structure                                       *
 ********************************************************************************/
/* private */
void StateManager::init_shell_state (int4 stVersion, void *hWnd_p) {
    if (stVersion == -1) {
        // Set interface and calculator options to their defaults
        initCalcPrefs();
    } else {
        switch (stVersion-STATE_VERSION_ORIG) {
            case 0: // Version 1 to 2
                state.matrixName[0] = _T('\0');
                // Fall through migration steps ...
            case 1:
            case 2: // Versions 2,3 to 4
                _tcscpy(state.skinName[1], _T("EasyCalc2"));
                _tcscpy(state.skinName[2], _T("EasyCalc3"));
            case 3: // Version 4 to 5
                // Insert the new skin name space
                memmove(state.langName, state.skinName+3,
                        sizeof(state) - (((size_t) &(state.langName))
                                         - ((size_t) &state)
                                        )
                       );
                _tcscpy(state.skinName[3], _T("EasyCalcG"));
                // Insert the new graph pen mode space
                memmove(&(state.calcPrefs.insertHelp),
                        &(state.calcPrefs.dispPrefs.penMode),
                        sizeof(state) - (((size_t) &(state.calcPrefs.insertHelp))
                                         - ((size_t) &state)
                                        )
                       );
                // Set the new graph preferences to defaults
                state.calcPrefs.dispPrefs.penMode = pen_centerwide;
                state.graphPrefs.xmin = -7.0;
                state.graphPrefs.xmax = 7.0;
                state.graphPrefs.ymin = -7.0;
                state.graphPrefs.ymax = 7.0;
                state.graphPrefs.xscale = 1.0;
                state.graphPrefs.yscale = 1.0;
                state.graphPrefs.fimin = 0.0;
                state.graphPrefs.fimax = M_PIl * 2.0;
                state.graphPrefs.fistep = M_PIl * 0.02;
                state.graphPrefs.tmin = 0.0;
                state.graphPrefs.tmax = 10.0;
                state.graphPrefs.tstep = 0.1;
                state.graphPrefs.functype = graph_func;
                for (int i=0 ; i<MAX_GRFUNCS ; i++) {
                    graphPrefs.funcFunc[i][0]='\0';
                    graphPrefs.funcPol[i][0]='\0';
                    graphPrefs.funcPar[i][0][0]='\0';
                    graphPrefs.funcPar[i][1][0]='\0';
                }
                state.graphPrefs.logx = false;
                state.graphPrefs.logy = false;
                state.graphPrefs.saved_reducePrecision = state.calcPrefs.reducePrecision;
                state.graphPrefs.speed = 1;
                for (int i=0 ; i<9 ; i++){
                    /* Note: grEnable[8] enables axis labels,
                     *       colors[8] is the background color
                     */
                    for (int ft=graph_func ; ft<nb_func_types ; ft++)
                        state.graphPrefs.grEnable[ft][i] = true;
//                    if (colorDisplay)
                        state.graphPrefs.colors[i] = WinRGBToIndex(graphRGBColors+i);
//                    if (grayDisplay)
//                        graphPrefs.colors[i] = funcolors[i];
                }
                for (int i=0 ; i<6 ; i++) {
                    state.graphPrefs.grType[i] = 1; // Use lines
                }
            default: // No migration
                ;
        }

        if (state.calcPrefs.version != PREF_VERSION) {
            // Incorrect version/contents, create the state contents
            FrmPopupForm(altNoStateFile, hWnd_p);
            initCalcPrefs();
        } else {
            // Publish calcPrefs, dispPrefs and graphPrefs for old core modules
            // as done by initCalcPrefs().
            calcPrefs = state.calcPrefs;
            dispPrefs = state.calcPrefs.dispPrefs;
            graphPrefs = state.graphPrefs;
        }
    }

    state_version = STATE_VERSION;
}

/********************************************************************************
 *  FUNCTION: shell_read_saved_state(*void, int4)                               *
 *  Read a piece from the state file.                                           *
 *  Returns the number of read bytes.                                           *
 ********************************************************************************/
/* private */
int4 StateManager::shell_read_saved_state (void *buf, int4 bufsize) {
    if (stateFile == NULL)
        return (-1);
    else {
        int4 n = fread(buf, 1, bufsize, stateFile);
        if (n != bufsize && ferror(stateFile)) {
            // On error, close the file
            fclose(stateFile);
            stateFile = NULL;
            return (-1);
        } else    return (n);
    }
}

/********************************************************************************
 *  FUNCTION: shell_write_saved_state(*void, int4)                              *
 *  Write a piece to the state file.                                            *
 ********************************************************************************/
/* private */
bool StateManager::shell_write_saved_state (const void *buf, int4 nbytes) {
    if (stateFile == NULL)
        return (false);
    else {
        int4 n = fwrite(buf, 1, nbytes, stateFile);
        if (n != nbytes) {
            // On error, close the file, and remove whatever we have written so far.
            fclose(stateFile);
            DeleteFile(stateFilename);
            stateFile = NULL;
            return (false);
        } else {
            return (true);
        }
    }
}

/********************************************************************************
 *  FUNCTION: read_shell_state()                                                *
 *  Read the state module structure from file                                   *
 ********************************************************************************/
/* private */
bool StateManager::read_shell_state (void *hWnd_p) {
    int4 magic;
    int4 version;
    int4 stVersion;
    int4 state_size;
    DataManager *dm, *dmHist, *dmSolv;
    Err error;

    if (shell_read_saved_state(&magic, sizeof(int4)) != sizeof(int4))
        return (false);
    if (magic != EASYCALC_MAGIC)
        return (false);

    if (shell_read_saved_state(&version, sizeof(int4)) != sizeof(int4))
        return (false);
    if (version < 0 || version > EASYCALC_VERSION)
        /* Unknown state file version */
        return (false);

    if (version > 0) {
        if (shell_read_saved_state(&state_size, sizeof(int4)) != sizeof(int4))
            return (false);
        if (shell_read_saved_state(&stVersion, sizeof(int4)) != sizeof(int4))
            return (false);
        if ((stVersion < STATE_VERSION_ORIG) || (stVersion > STATE_VERSION))
            /* Unknown shell state version */
            return (false);
        if (shell_read_saved_state(&state, state_size) != state_size)
            return (false);
        if (shell_read_saved_state(&dm, sizeof(DataManager *)) != sizeof(DataManager *))
            return (false);
        if (shell_read_saved_state(&dmHist, sizeof(DataManager *)) != sizeof(DataManager *))
            return (false);

        if (dm != NULL) {
            dm = new DataManager ();
            if (dm->deSerialize(stateFile)) {
                delete dm;
                return (false);
            }
            // Overwrite previously initialized object
            if (stateMgrDB != NULL)
                delete stateMgrDB;
            stateMgrDB = dm;
        }
        error = db_open();
        ErrFatalDisplayIf(error, _T("Can't open CalcDB"));

        if (dmHist != NULL) {
            dmHist = new DataManager ();
            if (dmHist->deSerialize(stateFile)) {
                delete dmHist;
                return (false);
            }
            // Overwrite previously initialized object
            if (stateMgrHistDB != NULL)
                delete stateMgrHistDB;
            stateMgrHistDB = dmHist;
        }
        error = history_open();
        ErrFatalDisplayIf(error, _T("Can't open History DB"));

        if (stVersion <= 2) { // Introduced with version 3
            dmSolv = NULL;
        } else {
            if (shell_read_saved_state(&dmSolv, sizeof(DataManager *)) != sizeof(DataManager *))
                return (false);

            if (dmSolv != NULL) {
                dmSolv = new DataManager ();
                if (dmSolv->deSerialize(stateFile)) {
                    delete dmSolv;
                    return (false);
                }
                // Overwrite previously initialized object
                if (stateMgrSolvDB != NULL)
                    delete stateMgrSolvDB;
                stateMgrSolvDB = dmSolv;
            }
        }

        // Initialize the parts of the shell state
        // that were NOT read from the state file
        init_shell_state(stVersion, hWnd_p);
    } else { // Version 0
        init_shell_state(-1, NULL);
    }

    return (true);
}

/********************************************************************************
 *  FUNCTION: write_shell_state()                                               *
 *  Saves the state module structure to file for the next run.                  *
 ********************************************************************************/
/* private */
bool StateManager::write_shell_state (void) {
    int4 magic = EASYCALC_MAGIC;
    int4 version = EASYCALC_VERSION;
    int4 state_size = sizeof(t_state);
    int4 state_version = STATE_VERSION;

    if (!shell_write_saved_state(&magic, sizeof(int4)))
        return (false);
    if (!shell_write_saved_state(&version, sizeof(int4)))
        return (false);
    if (!shell_write_saved_state(&state_size, sizeof(int4)))
        return (false);
    if (!shell_write_saved_state(&state_version, sizeof(int4)))
        return (false);
    if (!shell_write_saved_state(&state, sizeof(t_state)))
        return (false);
    if (!shell_write_saved_state(&stateMgrDB, sizeof(DataManager *)))
        return (false);
    if (!shell_write_saved_state(&stateMgrHistDB, sizeof(DataManager *)))
        return (false);
    if ((stateMgrDB != NULL) && stateMgrDB->serialize(stateFile))
        return (false);
    if ((stateMgrHistDB != NULL) && stateMgrHistDB->serialize(stateFile))
        return (false);
    // Introduced with state version 3
    if (!shell_write_saved_state(&stateMgrSolvDB, sizeof(DataManager *)))
        return (false);
    if ((stateMgrSolvDB != NULL) && stateMgrSolvDB->serialize(stateFile))
        return (false);

    return (true);
}