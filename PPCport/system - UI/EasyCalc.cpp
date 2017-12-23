/*****************************************************************************
 * EasyCalc -- a scientific calculator
 * Copyright (C) 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * The name and many features come from
 * - EasyCalc on Palm:
 *
 * It also is reusing elements from
 * - Free42:  Thomas Okken
 * for its adaptation to the PocketPC world.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/
/* EasyCalc.cpp : Defines the entry point for the application.
 *****************************************************************************/

#include "stdafx.h"
// This one is needed to do a true C++ subclass of CEdit, but this is generating
// too big a code (1+ MB !!).
// #include "afxwin.h"

#include "compat/PalmOS.h"
#define _EASYCALC_C_
#include "EasyCalc.h"
#include "StateManager.h"
#include "system - UI/Skin.h"
#include "system - UI/SkinList.h"
#include "compat/Lang.h"
#include "core/core_main.h"
#include "core/core_display.h"
#include "core/mlib/defuns.h"
#include "core/Main.h"
#include "core/varmgr.h"
#include "core/defmgr.h"
#include "core/calc.h"
#include "core/mlib/fp.h"
#include "core/ansops.h"
#include "core/mlib/calcDB.h"
#include "core/mlib/history.h"
#include "core/lstedit.h"
#include "core/mlib/funcs.h"
#include "core/mlib/stack.h"
#include "core/mtxedit.h"
#include "core/mlib/matrix.h"
#include "core/solver.h"
#include "core/finance.h"
#include "core/memo.h"
#include "core/grprefs.h"
#include "core/grsetup.h"
#include "core/graph.h"
#include "core/mlib/mathem.h"
#include "compat/MathLib.h"
#include "core/mlib/integ.h"

// Avoid clash on Strxxx functions
#define NO_SHLWAPI_STRFCNS
#include <afxdlgs.h>


/*-------------------------------------------------------------------------------
 - Constants.                                                                   -
 -------------------------------------------------------------------------------*/
// Title bar text max size
#define MAX_LOADSTRING 100

#define EASYCALC_APPVERSION _T("1.25g-1")

#define WM_APP_ENDVMENU      (WM_APP+1)
#define WM_APP_ENDFMENU      (WM_APP+2)
#define WM_APP_ENDFCMENU     (WM_APP+3)
#define WM_APP_EXEBUTTON     (WM_APP+4)
#define WM_APP_MATCHBRKT     (WM_APP+5)
#define WM_APP_GRPHDRAWASYNC (WM_APP+6)
#define WM_APP_ENDGTMENU     (WM_APP+7)
#define WM_APP_ENDGCMENU     (WM_APP+8)


/*-------------------------------------------------------------------------------
 - Type declarations.                                                           -
 -------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------
 - Global variables.                                                            -
 -------------------------------------------------------------------------------*/
HINSTANCE     g_hInst = NULL;         // current instance
HWND          g_hWnd = NULL;          // current main window handle
HWND          g_hwndE = NULL;         // Handle to the edit control input area.
void         *g_cedit2_obj = NULL;    // Input area subclassing object.
HWND          g_hWndMenuBar = NULL;   // menu bar handle
int           g_systUserLangId = 0;   // Detected language of the system user
StateManager *stateMgr = NULL;
LibLang      *libLang = NULL;
int           ann_state[NB_ANNUN];    // Annunciators state


/*-------------------------------------------------------------------------------
 - Module variables.                                                            -
 -------------------------------------------------------------------------------*/
static HWND hMainWnd = NULL;
static int  MainShowCmd;
static TCHAR  easyCalcDirname[FILENAMELEN];
static TCHAR *easyCalcStateFileName = _T("\\state.bin");
static TCHAR fileName[FILENAMELEN];
static t_state *state;

// For handling changes to the main window and application after option changes
static bool change_screen = false;
static bool reload_skins = false;
static TCHAR *new_lang = NULL;

static int ckey = KEY_NONE;         // Current key
static int skey;
static bool mouse_key;
static int active_keycode = 0;
static bool ctrl_down = false;
static bool alt_down = false;
static bool shift_down = false;
static bool just_pressed_shift = false;
static unsigned char *macro;

static int keymap_length = 0;
static keymap_entry *keymap = NULL;

static UINT timer = 0;
static UINT BrH_timer = 0;
static int BrH = 0;
static int running = 0;
static int enqueued = 0;

// Skins
static Skin *cur_skin = NULL;
static Skin *loaded_skins[NB_SKINS] = {NULL, NULL, NULL, NULL};
static bool no_skin_in_mem;
static bool out_of_memory_skin[NB_SKINS];
static SkinList skin_list;

// Interface
// Structure for talking with the SaveAsVar() panel.
typedef struct {
    const TCHAR *title;
    bool         saveAns;
    bool         verify;
} t_saveAsVar_param;

// Structure for talking with the GetComplex() panel.
typedef struct {
    const TCHAR *title;
    const TCHAR *label; // Can be NULL, and then "$$VALUE" will be used
    const TCHAR *param; // Used only for functions
    Complex     *value;
} t_getcplx_param;

typedef struct {
    HWND parent;
    int  rownb;
} t_getGraphFct_param;

static PROPSHEETPAGE* propPages = NULL;
static TCHAR *varPopup_name;
static TCHAR *fctPopup_name;
static TCHAR *builtinFctPopup_name;
static TCHAR *varStringPopup_name;
static Complex cplxPopup_value;
static int defSelectedItem;
static TCHAR *GrphTrkDescr[MAX_GRFUNCS];
static int    GrphTrkNums[MAX_GRFUNCS];
static int    grphTrkCount;
static int    grphTrkCurve_nb;
static t_grFuncType grFuncType;
static const TCHAR *calcResult_name;
static bool graphCalcIntersect = false;

/*-------------------------------------------------------------------------------
 - Forward declarations.                                                        -
 -------------------------------------------------------------------------------*/
static ATOM MyRegisterClass(HINSTANCE, LPTSTR);
static BOOL InitScreen (bool load, HDC hdc);
static BOOL InitInstance(HINSTANCE, int);
// WindowProcs
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
// PropSheetProcs
int CALLBACK WndPropSheetCallback(HWND hwndDlg, UINT message, LPARAM lParam);
// DialogProcs
BOOL CALLBACK WndAbout (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK WndOptionsGen (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndOptionsSkins (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndOptionsCalcPrefs (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndResMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndHstMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndSaveAsVar (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndDataMgr (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndNewVMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndNewFMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndModVMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndModFMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndVMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndFMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndfMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndAskTxtPopup (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndListEditor (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndMatrixEditor (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndSolver (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndSolverOptions (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndSolverNotes (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndFinCalc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndGraphPrefs (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndGraphFctMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndGraphConf (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndGraphTrackMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndGraphCalcMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndGetVarString (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WndGetComplex (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

static void init_shell_state(int4 version);
static bool read_shell_state(int4 *version);

static VOID CALLBACK repeater(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
static VOID CALLBACK timeout1(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
static VOID CALLBACK timeout2(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
static VOID CALLBACK matchBracket_timeout(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);


/*-------------------------------------------------------------------------------
 - Procedures.                                                                  -
 -------------------------------------------------------------------------------*/
/********************************************************************************
 * Class for wrapping the standard CEdit behavior.                              *
 * The intent is to catch the mouse left button up events which do not          *
 * correspond to a left mouse down event in the EDIT window, and send them      *
 * back to the parent window.                                                   *
 * Reason: when we do not have the dynamic mouse, pressing on a key in the main *
 * screen, or an annunciator, and moving the mouse in the EDIT window to        *
 * unpress the button would lose the event, and keep the key or the annunciator *
 * visibly activated until the next click.                                      *
 ********************************************************************************/
static WNDPROC CEdit_fnWndProc = NULL;  // Contains the CEdit standard window proc.
// Do no subclass in the C++ sense of term, this is bringing in about 1.3 MBytes ...!
//class CEdit2 : public CEdit {
class CEdit2 {
private:
    bool MouseLDownFlag;
    HWND parentWnd;
    WNDPROC fnWndProc;

public:
    CEdit2();
    void setParentWnd (HWND hwndParent);
    void setPrevFn (WNDPROC prevFn);
    WNDPROC getPrevFn (void);

    static LRESULT CALLBACK WndProc2 (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
        //Retrieve the 'this' pointer stored in User data.
        //If not there, and this is the creation time, then allocate the object
        //and store the value in the User data for later retrieval.
        // This happens only when subclassing at class level. When subclassing
        // at instance level, the subclass action must set properly user data,
        // parent and local pointer at chain window function.
        // At class level, the register class is taking care of the CEdit_fnWndProc
        // static.
        CEdit2 *pCEdit2 = (CEdit2 *) GetWindowLong(hWnd, GWL_USERDATA);
        if (!pCEdit2) {
            ASSERT(Msg == WM_CREATE);
            if (Msg == WM_CREATE) {
                LPCREATESTRUCT cStr = (LPCREATESTRUCT) lParam;
                pCEdit2 = (CEdit2 *) (cStr->lpCreateParams);
                pCEdit2 = new CEdit2 ();
                pCEdit2->setParentWnd(cStr->hwndParent);
//                cStr->lpCreateParams = (LPVOID) pCEdit2;
                SetWindowLong(hWnd, GWL_USERDATA, (LONG) pCEdit2);
            }
        }

        switch (Msg) {
            case WM_LBUTTONDOWN:
                pCEdit2->MouseLDownFlag = true;
                break;

            case WM_LBUTTONUP:
                if (!pCEdit2->MouseLDownFlag) { // LButton down was not in this window,
                                                // pass to parent window.
                    return (SendMessage(pCEdit2->parentWnd, Msg, wParam, lParam));
                }
                pCEdit2->MouseLDownFlag = false;
                break;

            case WM_CHAR:
                if (BrH_timer) // Remove any highlighting, and restore cursor position if so
                    matchBracket_timeout(NULL, 0, 0, 0);
                if (wParam == 13) { // Enter pressed on keyboard, or ok button on PDA / Phone
                    PostMessage(pCEdit2->parentWnd, WM_APP_EXEBUTTON, (WPARAM) 0, (LPARAM) 0);
                    return (0);
                }
                break;
        }

        LRESULT res;
        if (pCEdit2->fnWndProc != NULL)
            res = CallWindowProc(pCEdit2->fnWndProc, hWnd, Msg, wParam, lParam);
        else
            res = CallWindowProc(CEdit_fnWndProc, hWnd, Msg, wParam, lParam);
        // Highlight corresponding bracket if any.
        if ((Msg == WM_CHAR) && (_tcschr(_T("()[]"), wParam) != NULL)) {
            BrH = 2;
            matchBracket_timeout(NULL, 0, 0, 0);
        }
        return (res);
    }
};

CEdit2::CEdit2() {
    MouseLDownFlag = false;
    parentWnd = NULL;
    fnWndProc = NULL;
}

void CEdit2::setParentWnd (HWND hwndParent) {
    parentWnd = hwndParent;
}

void CEdit2::setPrevFn (WNDPROC prevFn) {
    fnWndProc = prevFn;
}

WNDPROC CEdit2::getPrevFn (void) {
    return (fnWndProc);
}


/********************************************************************************
 * Instance subclassing function.                                               *
 ********************************************************************************/
void *subclassEdit (HWND hWnd_p, HWND hWndE) {
    // Subclass the standard EDIT Control with CEdit2
    CEdit2 *pCEdit2 = new CEdit2 ();
    SetWindowLong(hWndE, GWL_USERDATA, (LONG) pCEdit2);
    pCEdit2->setParentWnd(hWnd_p);
    LONG prevFn = GetWindowLong(hWndE, GWL_WNDPROC);
    ASSERT(prevFn != 0);
    pCEdit2->setPrevFn((WNDPROC) prevFn);
    LONG res = SetWindowLong(hWndE, GWL_WNDPROC, (LONG) (CEdit2::WndProc2));
    ASSERT(res == prevFn);
    if (res != prevFn) { // Error !
        delete pCEdit2;
        pCEdit2 = NULL;
    }
    return ((void *) pCEdit2);
}

void unsubclassEdit (HWND hWndE, void *cedit2_obj) {
    CEdit2 *pCEdit2 = (CEdit2 *) cedit2_obj;
    // Note that this can fail with Invalid Window handle when at end
    // of execution, since the main window and all its children  have
    // been destroyed already.
    LONG res = SetWindowLong(hWndE, GWL_WNDPROC, (LONG) (pCEdit2->getPrevFn()));
    delete pCEdit2;
}

/********************************************************************************
 * Externalize displaying wait cursor ...                                       *
 ********************************************************************************/
void wait_draw (void) {
    SetCursor(LoadCursor(NULL, IDC_WAIT));
}

void wait_erase(void) {
    SetCursor(NULL);
}

/********************************************************************************
 * Mouse move integration to filter out spurious signals ...                    *
 ********************************************************************************/
#define MOUSE_INTEGSIZE 10
#define SENS_X 40
#define SENS_Y 30
int integ_index, integ_firstIndex, integ_size;
//int integ_total;
bool looped;
LPARAM integ_value_sum, integ_deltax_sum, integ_deltay_sum;
LPARAM integ_value_array[MOUSE_INTEGSIZE],
       integ_deltax_array[MOUSE_INTEGSIZE],
       integ_deltay_array[MOUSE_INTEGSIZE];
void beginIntegrate (LPARAM lParam) {
    // Store first value, initiate move integration
    looped = false;
    integ_index = integ_firstIndex = 0;
//    integ_total =
    integ_size = 1;
    integ_deltax_array[0] = integ_deltay_array[0]
                          = integ_deltax_sum
                          = integ_deltay_sum
                          = 0;
    integ_value_array[0] = integ_value_sum = lParam;
}

void moveIntegrate (MSG *msg) {
    // Calculate instant speed (always positive)
    LPARAM prev;
    LPARAM deltax = abs(((int)LOWORD(msg->lParam))
                        - ((int)LOWORD(prev = integ_value_array[integ_index++])));
    LPARAM deltay = abs(((int)HIWORD(msg->lParam)) - ((int)HIWORD(prev)));
    // If already looping, free first place in array, we are going to
    // reuse it. This also ensures that the xxx_sum variables will
    // never overflow.
    if (looped) {
        integ_deltax_sum -= integ_deltax_array[integ_firstIndex];
        integ_deltay_sum -= integ_deltay_array[integ_firstIndex];
        integ_value_sum -= integ_value_array[integ_firstIndex++];
        if (integ_firstIndex >= MOUSE_INTEGSIZE)
            integ_firstIndex = 0;
    }
    // Get new place
    if (integ_index >= MOUSE_INTEGSIZE) {
        if (!looped) {
            // Starting to loop, remove first value, we are going to
            // reuse its room.
            // Note: we can use 0 instead of integ_firstIndex, this
            //       is its value when looping first time.
            integ_deltax_sum -= integ_deltax_array[0];
            integ_deltay_sum -= integ_deltay_array[0];
            integ_value_sum -= integ_value_array[integ_firstIndex++];
            looped = true;
        }
        integ_index = 0;
    }
    // Store new values and integrate.
    integ_deltax_sum += (integ_deltax_array[integ_index] = deltax);
    integ_deltay_sum += (integ_deltay_array[integ_index] = deltay);
    integ_value_sum += (integ_value_array[integ_index] = msg->lParam);
    // Keep track of size in array
    if (integ_size < MOUSE_INTEGSIZE)
        integ_size++;
//    integ_total++;
}

void endIntegrate (MSG *msg) {
    // If integration array has 1 or less record, then use the latest
    // value, don't try to be intelligent.
//    if (integ_size <= 1) {
//        if (msg->lParam != integ_value_sum) {
//            // Worst case, only start and end, no move,
//            // and they are different ! Keep last one.
//            ;
//        }
//    } else {
    if (integ_size > 1) { // This means there is at least one valid
                          // calculated instant speed in array.
        // Calculate last instant speed
        LPARAM prev;
        LPARAM deltax = abs(((int)LOWORD(msg->lParam))
                            - ((int)LOWORD(prev = integ_value_array[integ_index])));
        LPARAM deltay = abs(((int)HIWORD(msg->lParam)) - ((int)HIWORD(prev)));
        // Calculate average speed
        LPARAM integ_deltax = integ_deltax_sum / integ_size;
        LPARAM integ_deltay = integ_deltay_sum / integ_size;
        // Average position - not really needed
//        LPARAM integ_valuex = ((LPARAM)LOWORD(integ_value_sum)) / integ_size;
//        LPARAM integ_valuey = ((LPARAM)HIWORD(integ_value_sum)) / integ_size;
        // If spurious signal at end, detected by a peak speed at end,
        // find assumed last good one based on the most recent valid speed.
        if ((deltax > integ_deltax) || (deltay > integ_deltay)) {
            // Come back in history to where speed is within average move,
            // and retain that stylus position.
            int index_back = integ_index;
            while ((integ_deltax_array[index_back] > integ_deltax)
                   || (integ_deltay_array[index_back] > integ_deltay)) {
                if (--index_back < 0)
                    index_back = MOUSE_INTEGSIZE-1;
                if (index_back == integ_index)
                    break; // We didn't find a proper value .. keep last but one.
                           // Should not occur, this is a safeguard against infinite loop.
            }
            msg->lParam = integ_value_array[index_back];
        }
    }
}

/********************************************************************************
 * Main function, application entry point.                                      *
 ********************************************************************************/
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR    lpCmdLine,
                    int       nCmdShow) {
    MSG msg;

    MainShowCmd = nCmdShow; // Make it available to other parts of code
    BOOL res = AfxWinInit(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    ASSERT(res);

    // Check if the application is running. If it's running then focus on the window.
    if (hPrevInstance) {
        return(FALSE);
    }

    TCHAR language[100];
    int rc = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTLANGUAGE, language, sizeof(language));
    ASSERT(rc != 0);
    rc = _stscanf(language, _T("%x"), &g_systUserLangId);
    if (rc != 1) // English by default if we can't recognize user language
        g_systUserLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    rc = InitInstance(hInstance, nCmdShow);
    if (!rc) {
        return (FALSE);
    }

    HACCEL hAccelTable;
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EASYCALC_ACCEL));
    ASSERT(hAccelTable != NULL);

    // Main message loop:
    while ((rc = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (rc == -1) // Error case ..
            break;
        // Handle spurious stylus signals, especially on A639
        if (msg.message == WM_LBUTTONDOWN) {
            beginIntegrate(msg.lParam);
        } else if (msg.message == WM_MOUSEMOVE) {
            moveIntegrate(&msg);
        } else if (msg.message == WM_LBUTTONUP) {
            endIntegrate(&msg);
        }
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            if (msg.message == WM_APP_GRPHDRAWASYNC) {
                // Give a chance to other external messages while drawing graphes by bits
                MSG msg_tmp;
                rc = PeekMessage(&msg_tmp, NULL, 0, 0, PM_REMOVE);
                if (rc != 0) {
                    TranslateMessage(&msg_tmp);
                    // Filter out additional drawing messages which can happen while
                    // doing menu or other things when graphing
                    if (msg_tmp.message != WM_APP_GRPHDRAWASYNC)
                        DispatchMessage(&msg_tmp);
                } else
                    rc += 0;
            }
            DispatchMessage(&msg);
        }
    }

    // Perform application close actions
    StopApplication();

    if (g_cedit2_obj != NULL) {
        unsubclassEdit(g_hwndE, g_cedit2_obj);
        g_cedit2_obj = NULL; // Object has been deleted in unsubclassEdit
    }

    graph_clean_cache();
    for (int i=0 ; i<NB_SKINS ; i++) {
        if (loaded_skins[i] != NULL) {
            delete loaded_skins[i];
            loaded_skins[i] = NULL;
        }
    }
    delete stateMgr;
    stateMgr = NULL;
    delete libLang;
    libLang = NULL;
#ifdef _DEBUG
    end_debug_malloc();
#endif

    return ((int) msg.wParam);
}

/********************************************************************************
 *  FUNCTION: MyRegisterClass()                                                 *
 *  Registers the window class.                                                 *
 ********************************************************************************/
static ATOM MyRegisterClass (HINSTANCE hInstance, LPTSTR szWindowClass) {
    ATOM a;
    WNDCLASS wc;

    // Register the current class
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EASYCALC));
    wc.hCursor       = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = 0;
    wc.lpszClassName = szWindowClass;
    a = RegisterClass(&wc);

    // Subclass the standard EDIT Control with CEdit2
//    BOOL res = GetClassInfo(NULL, _T("Edit"), &wc);
//    CEdit_fnWndProc = wc.lpfnWndProc;
//    wc.lpfnWndProc = CEdit2::WndProc2;
//    wc.hInstance = hInstance;
//    wc.lpszClassName = _T("EDIT2");
//    a |= RegisterClass(&wc);

    return (a);
}

/********************************************************************************
 *  FUNCTION: InitScreen(HINSTANCE)                                             *
 *  (Re-)creates main window and menus.                                         *
 ********************************************************************************/
static BOOL InitScreen (bool load, HDC hdc) {
    // Prepare options panels
    #define DEF_NUM_OF_PAGES    3
    propPages = new PROPSHEETPAGE[DEF_NUM_OF_PAGES];
    memset(propPages, 0, sizeof(PROPSHEETPAGE) * DEF_NUM_OF_PAGES);

    propPages[0].dwSize = sizeof(PROPSHEETPAGE);
    propPages[0].dwFlags = PSP_DEFAULT | PSP_USETITLE;
    propPages[0].pszTemplate = MAKEINTRESOURCE(IDD_OPT_GENERAL);
    propPages[0].pfnDlgProc = (DLGPROC) WndOptionsGen;
    propPages[0].hInstance = g_hInst;
    propPages[0].pszIcon = NULL;
    propPages[0].pszTitle = libLang->translate(_T("$$GENERAL"));
    propPages[0].lParam = (LPARAM) NULL;

    propPages[1].dwSize = sizeof(PROPSHEETPAGE);
    propPages[1].dwFlags = PSP_DEFAULT | PSP_USETITLE;
    propPages[1].pszTemplate = MAKEINTRESOURCE(IDD_OPT_SKINS);
    propPages[1].pfnDlgProc = (DLGPROC) WndOptionsSkins;
    propPages[1].hInstance = g_hInst;
    propPages[1].pszIcon = NULL;
    propPages[1].pszTitle = libLang->translate(_T("$$SKINS"));
    propPages[1].lParam = (LPARAM) NULL;

    propPages[2].dwSize = sizeof(PROPSHEETPAGE);
    propPages[2].dwFlags = PSP_DEFAULT | PSP_USETITLE;
    propPages[2].pszTemplate = MAKEINTRESOURCE(IDD_OPT_PREFS);
    propPages[2].pfnDlgProc = (DLGPROC) WndOptionsCalcPrefs;
    propPages[2].hInstance = g_hInst;
    propPages[2].pszIcon = NULL;
    propPages[2].pszTitle = libLang->translate(_T("$$PREFERENCES TITLE"));
    propPages[2].lParam = (LPARAM) NULL;

    // Get the screen dimensions and load skins on it (up to 3 for now, plus Graph)
    RECT r;
    int rc = GetClientRect(g_hWnd, &r);
    ASSERT(rc != 0);

    // Load/reload skins if needed
    if (load) {
        no_skin_in_mem = true;
        skin_list.clear();
        for (int i=0 ; i<NB_SKINS ; i++) {
            // If no skin specified, skip the slot, deleting object if there is one.
            if (state->skinName[i][0] == 0) {
                if (loaded_skins[i] != NULL) {
                    delete loaded_skins[i];
                    loaded_skins[i] = NULL;
                }
                continue;
            }
            // Allocate skin object if not present
            if (loaded_skins[i] == NULL)
                loaded_skins[i] = new Skin ();
            // Load skin to it
            if ((rc = loaded_skins[i]->load(state->skinName[i],
                                       easyCalcDirname,
                                       r.right,
                                       r.bottom,
                                       hdc))) { // If error, remove skin object
                switch (rc) {
                    case -1: // No layout file
                        FrmCustomAlert(altNoLayoutFile, state->skinName[i], NULL, NULL, g_hWnd);
                        break;
                    case -2: // No GIF file
                        FrmCustomAlert(altErrorGifFile, state->skinName[i], NULL, NULL, g_hWnd);
                        break;
                    case -3: // Error loading GIF file
                        FrmCustomAlert(altBadGifFile, state->skinName[i], NULL, NULL, g_hWnd);
                        break;
                    default: //Memory allocation errors
                        out_of_memory_skin[i] = true; // Remember we got an out of memory on this one
                }
                delete loaded_skins[i];
                loaded_skins[i] = NULL;
            } else { // Add skin to list of loaded skins
                no_skin_in_mem = out_of_memory_skin[i] = false;
                skin_list.add(i);
            }
        }
    }

    // At this point, all skins are in memory, except when error or out of memory.
    // Set destination skin
    Skin *dest_skin;
    if ((state->cur_skin_nb < 0) || (state->cur_skin_nb >= NB_SKINS))
        dest_skin = NULL;
    else
        dest_skin = loaded_skins[state->cur_skin_nb];
    // If skin is not present, and there is another one, try loading again, and if still
    // memory problem, free least used one to try again,
    // or default to first one active if not possible (note: this is checked at change, so such
    // a situation can only occur when:
    // - loading previous state, that is at start => no cur_skin
    // - loading a new set of skins => no guarantee cur_skin is still a valid one.
    if (dest_skin == NULL) {
        int j = state->cur_skin_nb;
        if (out_of_memory_skin[j]) { // Try reload or freeing another skin
                                     // and take its room.
            loaded_skins[j] = new Skin ();
            if ((rc = loaded_skins[j]->load(state->skinName[j],
                                       easyCalcDirname,
                                       r.right,
                                       r.bottom,
                                       hdc))) {
                switch (rc) {
                    case -1: // No layout file
                        FrmCustomAlert(altNoLayoutFile, state->skinName[j], NULL, NULL, g_hWnd);
                        break;
                    case -2: // No GIF file
                        FrmCustomAlert(altErrorGifFile, state->skinName[j], NULL, NULL, g_hWnd);
                        break;
                    case -3: // Error loading GIF file
                        FrmCustomAlert(altBadGifFile, state->skinName[j], NULL, NULL, g_hWnd);
                        break;
                    default:
                        if (!no_skin_in_mem) { // Try again, after freeing space
                            // Find least used skin. Note: the list remembers last one used, and in case
                            // of egality, keeps last one used over the other one, favouring the return move.
                            int i = skin_list.least_used();
                            delete loaded_skins[i];
                            loaded_skins[i] = NULL;
                            out_of_memory_skin[i] = true; // Now this one is out of memory ...
                            if ((rc = loaded_skins[j]->load(state->skinName[j],
                                                       easyCalcDirname,
                                                       r.right,
                                                       r.bottom,
                                                       hdc))) {
                                // Still memory allocation errors
                                no_skin_in_mem = true;
                                for (int k=0 ; k<NB_SKINS ; k++) {
                                    if (!out_of_memory_skin[k])
                                        no_skin_in_mem = false;
                                }
                                FrmCustomAlert(altMemAllocError, state->skinName[j], (TCHAR *) (j+1), (TCHAR *) rc, g_hWnd);
                            } else {
                                dest_skin = loaded_skins[j];
                                no_skin_in_mem = out_of_memory_skin[j] = false;
                                skin_list.add(j);
                            }
                        } else {
                            FrmCustomAlert(altMemAllocError, state->skinName[j], (TCHAR *) (j+1), (TCHAR *) rc, g_hWnd);
                        }
                }
                if (rc != 0) {
                    delete loaded_skins[j];
                    loaded_skins[j] = NULL;
                }
            } else {
                dest_skin = loaded_skins[j];
                no_skin_in_mem = out_of_memory_skin[j] = false;
                skin_list.add(j);
            }
        }
        if ((dest_skin == NULL) && !no_skin_in_mem) { // Skin isn't there, and there's at least one other
                                                      // Take first one active
            for (int i=0 ; i<NB_SKINS ; i++) {
                if (loaded_skins[i] != NULL) {
                    state->cur_skin_nb = i;
                    dest_skin = loaded_skins[i];
                    break;
                }
            }
        }
        // If none found, error !
        if (dest_skin == NULL) {
            state->cur_skin_nb = -1;
        }
    }
    // Switch to new skin (or reset existing ...)
    if ((cur_skin != NULL) && (dest_skin != NULL)) {
        if (cur_skin->graph && !dest_skin->graph) { // Current skin is changing from a graph mode one
                                                    // to a non graph mode one, restore reduce precision.
            cur_skin->stop_graph();
            calcPrefs.reducePrecision = graphPrefs.saved_reducePrecision;
            graph_clean_cache();
        } else if (!cur_skin->graph && dest_skin->graph) { // Current skin is changing from non graph mode one
                                                           // to a graph mode one, save reduce precision.
            dest_skin->recalc = true;
            graphPrefs.saved_reducePrecision = calcPrefs.reducePrecision;
            calcPrefs.reducePrecision = false;
            graph_init_cache();
        }
    }
    cur_skin = dest_skin;
    // Remember the use of this skin, to find least used one in case of memory shortage
    if (state->cur_skin_nb != -1) {
        skin_list.add_use(state->cur_skin_nb);
    }

    // Initialize menus with proper translated strings
    TBBUTTONINFO tbbi;
    memset(&tbbi,0,sizeof(tbbi));
    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = (TCHAR *) libLang->translate(_T("$$FILE"));
    rc = SendMessage(g_hWndMenuBar, TB_SETBUTTONINFO, IDM_FILE, (LPARAM) &tbbi);
    tbbi.pszText = (TCHAR *) libLang->translate(_T("$$EDIT"));
    rc = SendMessage(g_hWndMenuBar, TB_SETBUTTONINFO, IDM_EDIT, (LPARAM) &tbbi);
    tbbi.pszText = (TCHAR *) libLang->translate(_T("$$SPECIAL"));
    rc = SendMessage(g_hWndMenuBar, TB_SETBUTTONINFO, IDM_SPECIAL, (LPARAM) &tbbi);
    tbbi.pszText = (TCHAR *) libLang->translate(_T("$$HELP"));
    rc = SendMessage(g_hWndMenuBar, TB_SETBUTTONINFO, IDM_HELP, (LPARAM) &tbbi);
    // Doesn't seem to do anything ...
    // rc = DrawMenuBar(g_hWndMenuBar);

    // Initialize sub-menus
    HMENU hmmb = (HMENU)SendMessage(g_hWndMenuBar, SHCMBM_GETMENU, (WPARAM) 0, (LPARAM) 0);
    MENUITEMINFO mii;
    memset(&mii,0,sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_STRING;
    // For menu items whose state depends on having a skin displayed.
    UINT fstate = ((cur_skin == NULL) ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND;
    HMENU hmmb_sub = (HMENU) SendMessage(g_hWndMenuBar, SHCMBM_GETSUBMENU, (WPARAM) 0,
                                        (LPARAM) IDM_FILE);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$OPTIONS"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_FILE_OPTIONS, FALSE, &mii);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$IMPORT"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_FILE_IMPORT, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_FILE_IMPORT, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$EXPORT"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, 3, TRUE, &mii);  // No identifier for popup menu items, only position
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$VARIABLES ONLY"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EXPORT_VARIABLES, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EXPORT_VARIABLES, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$FUNCTIONS ONLY"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EXPORT_FUNCTIONS, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EXPORT_FUNCTIONS, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$EQUATIONS ONLY"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EXPORT_EQUATIONS, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EXPORT_EQUATIONS, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$EXIT"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_FILE_EXIT, FALSE, &mii);

    hmmb_sub = (HMENU) SendMessage(g_hWndMenuBar, SHCMBM_GETSUBMENU, (WPARAM) 0,
                                   (LPARAM) IDM_EDIT);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$COPY"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EDIT_CLIPCOPY, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EDIT_CLIPCOPY, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$CUT"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EDIT_CLIPCUT, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EDIT_CLIPCUT, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$PASTE"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EDIT_CLIPPASTE, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EDIT_CLIPPASTE, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$CLEAR HISTORY"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EDIT_CLEARHISTORY, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EDIT_CLEARHISTORY, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$DEFINITION TITLE"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EDIT_DATAMANAGER, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EDIT_DATAMANAGER, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$LIST EDITOR"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EDIT_LISTEDITOR, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EDIT_LISTEDITOR, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$MATRIX EDITOR"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_EDIT_MATRIXEDITOR, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_EDIT_MATRIXEDITOR, fstate);

    hmmb_sub = (HMENU) SendMessage(g_hWndMenuBar, SHCMBM_GETSUBMENU, (WPARAM) 0,
                                   (LPARAM) IDM_SPECIAL);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$SOLVER"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_SPECIAL_SOLVER, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_SPECIAL_SOLVER, fstate);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$FINANCIAL TITLE"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_SPECIAL_FINANCIALCALCULATOR, FALSE, &mii);
    rc = EnableMenuItem(hmmb_sub, IDM_SPECIAL_FINANCIALCALCULATOR, fstate);

    hmmb_sub = (HMENU) SendMessage(g_hWndMenuBar, SHCMBM_GETSUBMENU, (WPARAM) 0,
                                   (LPARAM) IDM_HELP);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$ABOUT..."));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_HELP_ABOUT, FALSE, &mii);
    mii.dwTypeData = (TCHAR *) libLang->translate(_T("$$HELP"));  // Force "const" off
    rc = SetMenuItemInfo(hmmb_sub, IDM_HELP_HELP, FALSE, &mii);
    // Doesn't seem to do anything ...
    // rc = DrawMenuBar(g_hWndMenuBar);

    // Do not get any further if no skin.
    if (cur_skin == NULL)
        return (FALSE);

    // Set the annunciators state
    for (int i=0 ; i<NB_ANNUN ; i++)   ann_state[i] = false;
    ann_state[ANN_DEG + calcPrefs.trigo_mode - degree] = true;
    switch (dispPrefs.base) {
        case disp_decimal:
            ann_state[ANN_DEC] = true;
            break;
        case disp_octal:
            ann_state[ANN_OCT] = true;
            break;
        case disp_binary:
            ann_state[ANN_BIN] = true;
            break;
        case disp_hexa:
            ann_state[ANN_HEX] = true;
            break;
    }
    ann_state[ANN_SELZONE + dispPrefs.penMode - pen_selzone] = true;
    ann_state[ANN_S1 + state->cur_skin_nb] = true;

    // Prepare the painting brush for the input area Edit control
    if (cur_skin->editBgBrush == NULL)
        cur_skin->editBgBrush = CreateSolidBrush(cur_skin->getDisplayBgColor());
    cur_skin->create_input_area(g_hWnd, g_hInst);

    // Force redisplay of full zone.
    InvalidateRect(g_hWnd, NULL, TRUE);

    return(TRUE);
}

/********************************************************************************
 *  FUNCTION: InitInstance(HINSTANCE, int)                                      *
 *  Saves instance handle, creates main window, initialize overall application  *
 *  variables.                                                                  *
 ********************************************************************************/
static BOOL InitInstance (HINSTANCE hInstance, int nCmdShow) {
    TCHAR szTitle[MAX_LOADSTRING];       // title bar text
    TCHAR szWindowClass[MAX_LOADSTRING]; // main window class name

    g_hInst = hInstance; // Store instance handle in our global variable

#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
    // SHInitExtraControls should be called once during your application's initialization
    // to initialize any of the device specific controls such as CAPEDIT and SIPPREF.
    BOOL res = SHInitExtraControls();
    ASSERT(res);
#endif // WIN32_PLATFORM_PSPC || WIN32_PLATFORM_WFSP

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_EASYCALC, szWindowClass, MAX_LOADSTRING);

#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
    //If it is already running, then focus on the window, and exit
    hMainWnd = FindWindow(szWindowClass, szTitle);
    if (hMainWnd) {
        // set focus to foremost child window
        // The "| 0x00000001" is used to bring any owned windows to the foreground and
        // activate them.
        SetForegroundWindow((HWND)((ULONG) hMainWnd | 0x00000001));
        return (FALSE);
    }
#endif // WIN32_PLATFORM_PSPC || WIN32_PLATFORM_WFSP

    // Not yet running, create the main window
    if (!MyRegisterClass(hInstance, szWindowClass)) {
        return (FALSE);
    }

    hMainWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    // The call above immediately passes control to WndProc with a WM_CREATE
    // message. So g_hWnd and g_hWndMenuBar should be set by now.
    if (!hMainWnd || (hMainWnd != g_hWnd)) {
        return (FALSE);
    }

    // Register our own APP message to be sent by PostThreadMessage and dispatched
    // by DispatchMessage.
    // Indeed, normal APP messages are filtered out by DispatchMessage, and
    // using PostMessage to g_hWnd bypasses the WinMain loop, giving no chance
    // to user taps / actions to insert between them !! (Windows UI API is really not well designed !)
//    WM_APP_GRPHDRAWASYNC_REG = RegisterWindowMessage(WM_APP_GRPHDRAWASYNC_STR);
//    if (WM_APP_GRPHDRAWASYNC_REG == 0) { // We can't function without that !
//        return (FALSE);
//    }

#ifdef WIN32_PLATFORM_PSPC
    // When the main window is created using CW_USEDEFAULT the height of the menubar (if one
    // is created) is not taken into account. So we resize the window after creating it
    // if a menubar is present
    if (g_hWndMenuBar) {
        RECT rc;
        RECT rcMenuBar;

        GetWindowRect(g_hWnd, &rc);
        GetWindowRect(g_hWndMenuBar, &rcMenuBar);
        rc.bottom -= (rcMenuBar.bottom - rcMenuBar.top);

        MoveWindow(g_hWnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, FALSE);
    }
#endif // WIN32_PLATFORM_PSPC

    // Get the path of the executable, without the last "\\".
    GetModuleFileName(0, easyCalcDirname, FILENAMELEN - 1);
    TCHAR *lastbackslash = _tcsrchr(easyCalcDirname, _T('\\'));
    if (lastbackslash != NULL) {
        *lastbackslash = 0;
    } else { // No path
        easyCalcDirname[0] = 0;
    }
#ifdef _DEBUG
    init_debug_malloc(easyCalcDirname);
#endif

    // Perform application initialization.
    // Show the hourglass during that initialization since it will take some time.
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Build the lang file name, open it, and read it
    _tcscpy(fileName, easyCalcDirname);
    _tcscat(fileName, _T("\\lang.rcp"));
    FILE *f = _tfopen(fileName, _T("r"));
    if (f == NULL) {
        LPCTSTR title, txt;

        ShowWindow(g_hWnd, nCmdShow);
        UpdateWindow(g_hWnd);

        title = _T("Error");
        txt   = _T("Could not find language file lang.rcp");
        // At this stage, WndProc has alreadybeen called, and g_hWnd is set, we can use it
        MessageBox(g_hWnd, txt, title, MB_OK | MB_ICONERROR | MB_APPLMODAL);
        return (FALSE);
    }
    libLang = new LibLang (f);

    // Create the StateMgr object with proper default language.
    stateMgr = new StateManager ();
    state = stateMgr->getState(); // This will simplify accessing state part of the object
    const TCHAR *defLangName;
    if ((defLangName = libLang->getLang(g_systUserLangId)) != NULL) {
        // If NULL, then "en" has already been set by default by the constructor.
        // Handle other case.
        _tcscpy(state->langName, defLangName);
    }

    SetCursor(NULL); // Set hourglass off

    // Initialize application level objects
    Err err=0;
    if ((err = StartApplication()) != 0) {
        LPCTSTR title, txt;

        ShowWindow(g_hWnd, nCmdShow);
        UpdateWindow(g_hWnd);

        title = _T("Error");
        txt   = _T("Error when initializing EasyCalc");
        MessageBox(g_hWnd, txt, title, MB_OK | MB_ICONERROR | MB_APPLMODAL);
        return (FALSE);
    }

    // From there, proper language has been set, either from initialization
    // or from loading state.bin.
    // Set corresponding current language in libLang.
    libLang->setLang(state->langName);
    _tcscpy(state->langName, libLang->getLang()); // Copy in state the final selected language.

    // Create main window contents
    skin_init();
    HDC hdc = GetDC(g_hWnd);
    HDC memdc = CreateCompatibleDC(hdc);
    InitScreen(true, memdc);
    DeleteDC(memdc);
    ReleaseDC(g_hWnd, hdc);

    // Show window contents
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return (TRUE);
}

/********************************************************************************
 *  FUNCTION: read_state()                                                      *
 *  Read stateMgr contents from state file.                                     *
 ********************************************************************************/
void read_state (void) {
    // Build the state file name, open it, read it and register it in the compatibility layer
    _tcscpy(fileName, easyCalcDirname);
    _tcscat(fileName, easyCalcStateFileName);
    stateMgr->init(fileName, g_hWnd);
}

/********************************************************************************
 *  FUNCTION: save_state()                                                      *
 *  Save stateMgr contents to state file.                                       *
 ********************************************************************************/
void save_state (void) {
    // Build the state file name, open it, and write it
    _tcscpy(fileName, easyCalcDirname);
    _tcscat(fileName, easyCalcStateFileName);
    stateMgr->save(fileName, g_hWnd);
}

/********************************************************************************
 *  FUNCTION: shell_keydown()                                                   *
 *  Handle the key down on screen.                                              *
 ********************************************************************************/
static void shell_keydown() {
    if (skey == RESULT_AREA) { /* Tap on result screen */
        return; // Do nothing at this time.
    }
    if (ckey != KEY_NONE) { // Draw annunciator in its selected state
        HDC hdc = GetDC(g_hWnd);
        HDC memdc = CreateCompatibleDC(hdc);
        if ((skey >= ANNUNBASE-NB_ANNUN+1) && (skey <= ANNUNBASE)) { /* Annunciator */
            cur_skin->repaint_annunciator(hdc, memdc, -(skey-ANNUNBASE), 2);
        } else {
            if (skey == NOSKEY)
                skey = cur_skin->find_skey(ckey);
            cur_skin->repaint_key(hdc, memdc, skey, 1);
        }
        DeleteDC(memdc);
        ReleaseDC(g_hWnd, hdc);
    }
    if (timer != 0) {
        KillTimer(NULL, timer);
        timer = 0;
    }
    int repeat;
    if (macro != NULL) {
        if (*macro == 0) {
            squeak();
            return;
        }
        bool one_key_macro = macro[1] == 0 || (macro[2] == 0 && macro[0] == 28);
        if (!one_key_macro)
            cur_skin->display_set_enabled(false);
        while (*macro != 0) {
            running = core_keydown(*macro++, &enqueued, &repeat);
            if (*macro != 0 && !enqueued)
                core_keyup(cur_skin, g_hWnd);
        }
        if (!one_key_macro) {
            cur_skin->display_set_enabled(true);
            HDC hdc = GetDC(g_hWnd);
            cur_skin->repaint_result(hdc);
            HDC memdc = CreateCompatibleDC(hdc);
            for (int i=0 ; i<NB_ANNUN ; i++)
                cur_skin->repaint_annunciator(hdc, memdc, i, ann_state[i]);
            DeleteDC(memdc);
            ReleaseDC(g_hWnd, hdc);
            repeat = 0;
        }
    } else
        running = core_keydown(ckey, &enqueued, &repeat);

    if (!running) {
        if (repeat != 0)
            timer = SetTimer(NULL, 0, repeat == 1 ? 1000 : 500, repeater);
        else if (!enqueued)
            timer = SetTimer(NULL, 0, 250, timeout1);
    }
}

/********************************************************************************
 *  FUNCTION: shell_keyup()                                                     *
 *  Handle the key up on screen.                                                *
 ********************************************************************************/
static void shell_keyup(bool action, HWND hWnd) {
#ifdef SPECFUN_ENABLED
    if (action && (skey == RESULT_AREA)) { /* Tap on result screen */
        /* Popup list editor if the displayed thing is a list, showing it */
        if (resultPrefs.ansType == list) {
            DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_LISTEDIT), (HWND) g_hWnd,
                           WndListEditor, (LPARAM) TRUE);
        } else if (resultPrefs.ansType == matrix ||
                   resultPrefs.ansType == cmatrix) {
            DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_MATRIXEDIT), (HWND) g_hWnd,
                           WndMatrixEditor, (LPARAM) TRUE);
        }
        return; // We're done
    }
#endif

    // Draw annunciator back in previous state
    HDC hdc1 = GetDC(g_hWnd);
    HDC memdc = CreateCompatibleDC(hdc1);
    if ((skey >= ANNUNBASE-NB_ANNUN+1) && (skey <= ANNUNBASE)) { /* Annunciator */
        cur_skin->repaint_annunciator(hdc1, memdc, -(skey-ANNUNBASE), ann_state[-(skey-ANNUNBASE)]);
    } else
        cur_skin->repaint_key(hdc1, memdc, skey, 0);
    DeleteDC(memdc);
    ReleaseDC(g_hWnd, hdc1);

    if (timer != 0) {
        KillTimer(NULL, timer);
        timer = 0;
    }

    // If no action, stop there
    if (!action) {
        ckey = KEY_NONE;
        skey = NOSKEY;
        return;
    }

    // Process annunciators, only if the state is up or if it is always selectable
    skey = -(skey-ANNUNBASE);
    if ((skey >= 0) && (skey <= NB_ANNUN-1) &&
        ((ann_state[skey] == 1) || ((skey >= ANN_S1) && (skey <= ANN_CTRWIDE)))) {
        switch (skey) {
            case ANN_DEG: // Go to RAD
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_RAD, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_RAD: // Go to GRAD
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_GRAD, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_GRAD: // Go to DEG
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_DEG, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_DEC: // Go to OCT
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_OCT, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_OCT: // Go to BIN
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_BIN, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_BIN: // Go to HEX
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_HEX, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_SCRL:
                cur_skin->scroll_result_left(g_hWnd);
                break;
            case ANN_SCRR:
                cur_skin->scroll_result_right(g_hWnd);
                break;
            case ANN_HEX:
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_DEC, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_S1: // Switch to skin if not already there
                shell_annunciators(ANNVAL_S1, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_S2: // Switch to skin if not already there
                shell_annunciators(ANNVAL_S2, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_S3: // Switch to skin if not already there
                shell_annunciators(ANNVAL_S3, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_SG: // Switch to skin if not already there
                shell_annunciators(ANNVAL_SG, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                break;
            case ANN_VAR: // Display the variable popup
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hWnd, WndVMenu,
                                  (LPARAM) hWnd);
                break;
            case ANN_UFCT: // Display the user function popup
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hWnd, WndFMenu,
                                  (LPARAM) hWnd);
                break;
            case ANN_CFCT: // Display the calc function popup
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hWnd, WndfMenu,
                                  (LPARAM) hWnd);
                break;
            case ANN_RESMENU: // Display the result menu
                CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_RES_MENU), hWnd, WndResMenu);
                break;
            case ANN_HSTMENU: // Display the history menu
                CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_RES_MENU), hWnd, WndHstMenu);
                break;
            case ANN_SELZONE: // Go to zone selection mode
                {
                // Remove any existing selection
                HDC hdc = GetDC(g_hWnd);
                cur_skin->switchSelGraph(hdc); // Remove any active selection or cross
                cur_skin->resetGraphSel(); // Ensure no selection / cross
                ReleaseDC(g_hWnd, hdc);
                }
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_PEN_SELZONE, ANNVAL_UNCH);
                break;
            case ANN_MOVGRPH: // Go to zone move mode
                if ((dispPrefs.penMode == pen_trackpt) || (dispPrefs.penMode == pen_selzone)) {
                    // If leaving curve track or selpen mode, remove any existing selection
                    HDC hdc = GetDC(g_hWnd);
                    cur_skin->switchSelGraph(hdc); // Remove any active selection or cross
                    cur_skin->resetGraphSel(); // Ensure no selection / cross
                    ReleaseDC(g_hWnd, hdc);
                }
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_PEN_MOVZONE, ANNVAL_UNCH);
                break;
            case ANN_TRACKPT: // Go to curve track (X,Y) mode
                {
                HDC hdc = GetDC(g_hWnd);
                cur_skin->switchSelGraph(hdc); // Remove any active selection or cross
                cur_skin->resetGraphSel(); // Ensure no selection / cross
                ReleaseDC(g_hWnd, hdc);
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_PEN_TRACKPT, ANNVAL_UNCH);
                // Get list of curves, and display menu only if 1 or more available
                grphTrkCount = grsetup_fn_descr_arr(GrphTrkDescr, GrphTrkNums);
                if (grphTrkCount != 0) {
                    CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hWnd,
                                      WndGraphTrackMenu, (LPARAM) hWnd);
                }
                }
                break;
            case ANN_CTRWIDE: // Go to wide area centered on zone mode
                {
                HDC hdc = GetDC(g_hWnd);
                if ((dispPrefs.penMode == pen_trackpt) || (dispPrefs.penMode == pen_selzone)) {
                    // If leaving curve track or selpen mode, remove any existing selection
                    cur_skin->switchSelGraph(hdc); // Remove any active selection or cross
                    cur_skin->resetGraphSel(); // Ensure no selection / cross
                }
                shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_PEN_CTRWIDE, ANNVAL_UNCH);
                // Redraw wide view, if necessary.
                cur_skin->refreshViewAndTrigRecalc(hdc);
                ReleaseDC(g_hWnd, hdc);
                }
                break;
        }
        action = false; // Do not do core_keyup() after that !
    }
    ckey = KEY_NONE;
    skey = NOSKEY;

    if (action && !enqueued) {
        running = core_keyup(cur_skin, g_hWnd);
    }
}

/********************************************************************************
 *  FUNCTION: shell_graphAction()                                               *
 *  Handle graph key presses.                                                   *
 ********************************************************************************/
void shell_graphAction (int key) {
    switch (key) {
        case KEY_CTRZONE:
            {
            HDC hdc = GetDC(g_hWnd);
            cur_skin->moveGraph(hdc, ZONE_AREA, 0.0, 0.0);
            ReleaseDC(g_hWnd, hdc);
            }
            break;
        case KEY_CTRWIDE:
            {
            HDC hdc = GetDC(g_hWnd);
            cur_skin->moveGraph(hdc, WVIEW_AREA, 0.0, 0.0);
            ReleaseDC(g_hWnd, hdc);
            }
            break;
        case KEY_GPREFS:
            {
            int rc = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_GRAPH_PREFS), g_hWnd, WndGraphPrefs);
            if ((rc == 1) && (cur_skin->graph)) {
                HDC hdc = GetDC(g_hWnd);
                cur_skin->refreshView(hdc);
                ReleaseDC(g_hWnd, hdc);
                cur_skin->trigRecalcGraph();
            }
            }
            break;
        case KEY_GCONF:
            if (graphPrefs.functype == graph_param) {
                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_GRAPH_CONFIGXY), g_hWnd, WndGraphConf);
            } else {
                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_GRAPH_CONFIG), g_hWnd, WndGraphConf);
            }
            cur_skin->trigRecalcGraph();
            break;
        case KEY_GCALC:
            if (dispPrefs.penMode != pen_trackpt) {
                FrmPopupForm(altCalcNeedTrackPt, NULL);
            } else if (cur_skin->curve_nb < 0) {
                FrmPopupForm(altCalcNeedCurveSel, NULL);
            } else if (!cur_skin->is_cross && !cur_skin->is_selecting) {
                FrmPopupForm(altCalcNeedSelect, NULL);
            } else {
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), g_hWnd,
                                  WndGraphCalcMenu, (LPARAM) g_hWnd);
            }
            break;
        case KEY_GSETX:
            {
            double realx, realy;
            Complex cplx;
            cplx.real = cplx.imag = 0.0;
            t_getcplx_param param;
            if ((dispPrefs.penMode != pen_trackpt) || (graphPrefs.functype == graph_func)) {
                param.label = param.title = _T("X =");
            } else if (graphPrefs.functype == graph_polar) {
                param.label = param.title = _T("th =");
            } else {
                param.label = param.title = _T("t =");
            }
            param.value = &cplx;
            if (DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), g_hWnd,
                                WndGetComplex,
                                (LPARAM) &param
                               )
               ) {
                if ((dispPrefs.penMode == pen_trackpt) && (cur_skin->curve_nb >= 0)) {
                    CodeStack *stack = graphCurves[cur_skin->curve_nb].stack1;
                    if (stack != NULL)
                        if (graphPrefs.functype == graph_func) {
                            realx = cplxPopup_value.real;
                            func_get_value(stack, realx, &realy, NULL);
                            if (finite(realy)) {
                                HDC hdc = GetDC(g_hWnd);
                                cur_skin->moveGraph(hdc, ZONE_AREA, realx, realy);
                                ReleaseDC(g_hWnd, hdc);
                            }
                        } else if (graphPrefs.functype == graph_polar) {
                            double radius; // cplxPopup_value contains an angle
                            func_get_value(stack, cplxPopup_value.real, &radius, NULL);
                            realx = radius * cos(cplxPopup_value.real);
                            realy = radius * sin(cplxPopup_value.real);
                            if (finite(realx) && finite(realy)) {
                                HDC hdc = GetDC(g_hWnd);
                                cur_skin->moveGraph(hdc, ZONE_AREA, realx, realy);
                                ReleaseDC(g_hWnd, hdc);
                            }
                        } else {
                            CodeStack *stack2 = graphCurves[cur_skin->curve_nb].stack2;
                            if (stack2 != NULL) { // cplxPopup_value is the parameter t
                                func_get_value(stack, cplxPopup_value.real, &realy, NULL);
                                func_get_value(stack2, cplxPopup_value.real, &realx, NULL);
                                if (finite(realx) && finite(realy)) {
                                    HDC hdc = GetDC(g_hWnd);
                                    cur_skin->moveGraph(hdc, ZONE_AREA, realx, realy);
                                    ReleaseDC(g_hWnd, hdc);
                                }
                            }
                        }
                } else {
                    realx = cplxPopup_value.real;
                    param.label = param.title = _T("Y =");
                    if (DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), g_hWnd,
                                        WndGetComplex,
                                        (LPARAM) &param
                                       )
                                       ) {
                        realy = cplxPopup_value.real;
                        HDC hdc = GetDC(g_hWnd);
                        cur_skin->moveGraph(hdc, ZONE_AREA, realx, realy);
                        ReleaseDC(g_hWnd, hdc);
                    }
                }
            }
            }
            break;
        case KEY_ZONEIN:
            {
            HDC hdc = GetDC(g_hWnd);
            cur_skin->scaleGraph(hdc, ZONE_AREA, 0.5);
            ReleaseDC(g_hWnd, hdc);
            }
            break;
        case KEY_ZONEOUT:
            {
            HDC hdc = GetDC(g_hWnd);
            cur_skin->scaleGraph(hdc, ZONE_AREA, 2.0);
            ReleaseDC(g_hWnd, hdc);
            }
            break;
        case KEY_WIDEIN:
            {
            HDC hdc = GetDC(g_hWnd);
            cur_skin->scaleGraph(hdc, WVIEW_AREA, 0.5);
            ReleaseDC(g_hWnd, hdc);
            }
            break;
        case KEY_WIDEOUT:
            {
            HDC hdc = GetDC(g_hWnd);
            cur_skin->scaleGraph(hdc, WVIEW_AREA, 2.0);
            ReleaseDC(g_hWnd, hdc);
            }
            break;
        case KEY_NORMZON:
            {
            HDC hdc = GetDC(g_hWnd);
            cur_skin->normZone(hdc);
            ReleaseDC(g_hWnd, hdc);
            }
            break;
    }
}

/********************************************************************************
 *  FUNCTION: shell_clBracket()                                                 *
 *  Handle highlighting matching (, ), [ or ] on screen.                        *
 ********************************************************************************/
void shell_clBracket (void) {
    BrH = 2;
    matchBracket_timeout(NULL, 0, 0, 0);
}

/********************************************************************************
 *  FUNCTION: shell_beeper(int, int)                                            *
 *  Sound on the OS.                                                            *
 ********************************************************************************/
void shell_beeper(int frequency, int duration) {
    HWAVEOUT     hSoundDevice;
    WAVEFORMATEX wf;
    WAVEHDR      wh;
    HANDLE       hEventSound;
    DWORD        i;

    hEventSound = CreateEvent(NULL,FALSE,FALSE,NULL);

    wf.wFormatTag      = WAVE_FORMAT_PCM;
    wf.nChannels       = 1;
    wf.nSamplesPerSec  = 11025;
    wf.nAvgBytesPerSec = 11025;
    wf.nBlockAlign     = 1;
    wf.wBitsPerSample  = 8;
    wf.cbSize          = 0;

    if (waveOutOpen(&hSoundDevice,WAVE_MAPPER,&wf,(DWORD)hEventSound,0,CALLBACK_EVENT) != 0) {
        CloseHandle(hEventSound);                       // no sound available
        MessageBeep(MB_ICONASTERISK);
        return;
    }

    // (samp/sec) * msecs * (secs/msec) = samps
    wh.dwBufferLength = (DWORD) ((__int64) 11025 * duration / 1000);
    wh.lpData = (LPSTR) LocalAlloc(LMEM_FIXED,wh.dwBufferLength);
    if (wh.lpData != NULL) {
        wh.dwBytesRecorded = 0;
        wh.dwUser = 0;
        wh.dwFlags = 0;
        wh.dwLoops = 0;

        for (i = 0; i < wh.dwBufferLength; ++i) // generate square wave
             wh.lpData[i] = (BYTE) (((__int64) 2 * frequency * i / 11025) & 1) * 64;

        if (waveOutPrepareHeader(hSoundDevice,&wh,sizeof(wh)) == MMSYSERR_NOERROR) {
            ResetEvent(hEventSound);  // prepare event for finishing
            if (waveOutWrite(hSoundDevice,&wh,sizeof(wh)) == MMSYSERR_NOERROR)
                WaitForSingleObject(hEventSound,INFINITE); // wait for finishing

            waveOutUnprepareHeader(hSoundDevice,&wh,sizeof(wh));
            waveOutClose(hSoundDevice);
        }

        LocalFree(wh.lpData);
    }
    CloseHandle(hEventSound);
    return;
}

/********************************************************************************
 *  FUNCTION: shell_annunciators()                                              *
 * Callback invoked by the emulator core to change the state of the display     *
 * annunciators (skin mode, shift, angle_mode, int_mode, run).                  *
 * Every parameter can have a value (like false, true, ANNVAL_DEG, ..) or       *
 * ANNVAL_UNCH for unchanged.                                                   *
 ********************************************************************************/
void shell_annunciators(int skin, int shf, int angle_mode, int int_mode, int pen_mode, int run) {
    HDC hdc = GetDC(g_hWnd);
    HDC memdc = CreateCompatibleDC(hdc);

    if (skin != ANNVAL_UNCH) {
        if (!ann_state[ANN_S1+skin]) {
            // Verify we can load destination skin
            if ((loaded_skins[skin] != NULL) || out_of_memory_skin[skin]) {
                for (int i=ANNVAL_S1; i<=ANNVAL_SG ; i++) {
                    cur_skin->repaint_annunciator(hdc, memdc, ANN_S1+i,
                                                  (ann_state[ANN_S1+i] = (i == skin)));
                }
                state->cur_skin_nb = skin;
                change_screen = TRUE;
            } else {
                squeak();
            }
        }
    }

    if ((shf != ANNVAL_UNCH) && (ann_state[ANN_SHIFT] != shf)) {
        ann_state[ANN_SHIFT] = shf;
        cur_skin->repaint_annunciator(hdc, memdc, ANN_SHIFT, shf);
    }

    if (angle_mode != ANNVAL_UNCH) {
        if (!ann_state[ANN_DEG+angle_mode]) {
            for (int i=ANNVAL_DEG; i<=ANNVAL_GRAD ; i++) {
                cur_skin->repaint_annunciator(hdc, memdc, ANN_DEG+i,
                                              (ann_state[ANN_DEG+i] = (i == angle_mode)));
            }
            calcPrefs.trigo_mode = (Ttrigo_mode) (degree + angle_mode);
            if (cur_skin->graph)
                cur_skin->trigRecalcGraph();
        }
    }

    Tbase base = (Tbase) 0;
    if (int_mode != ANNVAL_UNCH) {
        if (!ann_state[ANN_DEC+int_mode]) {
            for (int i=ANNVAL_DEC; i<=ANNVAL_HEX ; i++) {
                cur_skin->repaint_annunciator(hdc, memdc, ANN_DEC+i,
                                              (ann_state[ANN_DEC+i] = (i == int_mode)));
            }
            switch (int_mode) {
                case ANNVAL_DEC:
                    base = disp_decimal;
                    break;
                case ANNVAL_OCT:
                    base = disp_octal;
                    break;
                case ANNVAL_BIN:
                    base = disp_binary;
                    break;
                case ANNVAL_HEX:
                    base = disp_hexa;
                    break;
            }
        }
    }
    if (base != 0) {
        dispPrefs.base = base;
        fp_set_base(base);
        ans_redisplay(cur_skin, g_hWnd, _T("ans"));
    }

    if (pen_mode != ANNVAL_UNCH) {
        if (!ann_state[ANN_SELZONE+pen_mode]) {
            for (int i=ANNVAL_PEN_SELZONE; i<=ANNVAL_PEN_CTRWIDE ; i++) {
                cur_skin->repaint_annunciator(hdc, memdc, ANN_SELZONE+i,
                                              (ann_state[ANN_SELZONE+i] = (i == pen_mode)));
            }
            dispPrefs.penMode = (Tpen_mode) (pen_selzone + pen_mode);
        }
    }

    DeleteDC(memdc);
    ReleaseDC(g_hWnd, hdc);
}

/********************************************************************************
 *  FUNCTION: shell_scroll_annunciators()                                       *
 * Callback invoked by the skin object to change the state of the result scroll *
 * annunciators (none, left, right).                                            *
 ********************************************************************************/
void shell_scroll_annunciators(int scroll) {
    HDC hdc = GetDC(g_hWnd);
    HDC memdc = CreateCompatibleDC(hdc);

//    if (scroll == ANNVAL_SCR_NONE) {
//        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRL, (ann_state[ANN_SCRL] = false));
//        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRR, (ann_state[ANN_SCRR] = false));
//    } else if (scroll == ANNVAL_SCR_LEFT) {
//        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRL, (ann_state[ANN_SCRL] = true));
//        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRR, (ann_state[ANN_SCRR] = false));
//    } else {
//        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRL, (ann_state[ANN_SCRL] = false));
//        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRR, (ann_state[ANN_SCRR] = true));
//    }
    if ((scroll & ANNVAL_SCR_LEFT) != 0) {
        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRL, (ann_state[ANN_SCRL] = true));
    } else {
        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRL, (ann_state[ANN_SCRL] = false));
    }
    if ((scroll & ANNVAL_SCR_RIGHT) != 0) {
        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRR, (ann_state[ANN_SCRR] = true));
    } else {
        cur_skin->repaint_annunciator(hdc, memdc, ANN_SCRR, (ann_state[ANN_SCRR] = false));
    }

    DeleteDC(memdc);
    ReleaseDC(g_hWnd, hdc);
}

/********************************************************************************
 *  FUNCTION: shell_powerdown()                                                 *
 *  Execute command to end the calculator.                                      *
 ********************************************************************************/
void shell_powerdown() {
    PostQuitMessage(0);
}

/********************************************************************************
 * Print a system error on screen.                                              *
 * (coming from calc.c)                                                         *
 ********************************************************************************/
void alertErrorMessage(CError err) {
    TCHAR *txt;
    const TCHAR *title;

    txt = print_error(err);
    title = libLang->translate(_T("$$ERROR"));
    MessageBox(g_hWnd, txt, title, MB_OK | MB_ICONERROR | MB_APPLMODAL);
    MemPtrFree(txt);
}

/********************************************************************************
 * Print a text error on screen.                                                *
 ********************************************************************************/
void ErrFatalDisplayIf(int cond, TCHAR *msg) {
    if (cond) {
        const TCHAR *title = libLang->translate(_T("$$ERROR"));
        MessageBox(g_hWnd, msg, title, MB_OK | MB_ICONERROR | MB_APPLMODAL);
    }
}

/********************************************************************************
 * Display one of the forms/alert messages on screen.                           *
 ********************************************************************************/
int FrmPopupForm(int formNb, void *hWnd_p) {
    const TCHAR *title, *txt;

    int ret = -1;
    if (hWnd_p == NULL) // Use main window
        hWnd_p = (void *) g_hWnd;

    switch (formNb) {
        case varEntryForm:
            {
            t_saveAsVar_param param;
            param.title = libLang->translate(_T("$$SAVE AS TITLE"));
            param.saveAns = true;
            param.verify  = true;
            // Use g_hWnd instead of hWnd_p, because this is coming from a modeless dialog
            DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VARENTRY), g_hWnd,
                           WndSaveAsVar, (LPARAM) &param);
            }
            break;
        case defForm:
            // Use g_hWnd instead of hWnd_p, because this is coming from a modeless dialog
            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DATAMGR), g_hWnd, WndDataMgr);
            break;
        case altAnsProblem:
            txt   = libLang->translate(_T("$$ANS PROBLEM"));
l_alertError:
            title = libLang->translate(_T("$$ERROR"));
            MessageBox(g_hWnd, txt, title, MB_OK | MB_ICONERROR | MB_APPLMODAL);
            break;
        case altGuessNotFound:
            txt   = libLang->translate(_T("$$NUMBER NOT RECOG"));
            goto l_alertError;
            break;
        case altBadVariableName:
            txt   = libLang->translate(_T("$$BADNAME MSG"));
            goto l_alertError;
            break;
        case altCompute:
            txt   = libLang->translate(_T("$$COMPUTE MSG"));
l_alertErrorQuestion:
            title = libLang->translate(_T("$$ERROR"));
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_APPLMODAL);
            if (ret == IDOK)
                ret = 0;
            else   ret = 1;
            break;
        case altBadParameter:
            txt   = libLang->translate(_T("$$BADPARAM MSG"));
            goto l_alertErrorQuestion;
            break;
        case altNoStateFile:
            // When this happens at start of application, need to
            // update full screen before displaying message.
            ShowWindow(g_hWnd, MainShowCmd);
            UpdateWindow(g_hWnd);
            txt   = libLang->translate(_T("$$NO STATE FILE"));
            goto l_alertError;
            break;
        case altWriteStateFile:
            txt   = libLang->translate(_T("$$ERROR SAVING STATE"));
            goto l_alertError;
            break;
        case altGraphBadVal:
            txt   = libLang->translate(_T("$$BAD VALUES"));
            goto l_alertError;
            break;
        case altMemoImportError:
            txt   = libLang->translate(_T("$$ERROR DURING IMPORT"));
            goto l_alertError;
            break;
        case altCalcNeedTrackPt:
            txt   = libLang->translate(_T("$$CALC NEED TRACK MODE"));
            goto l_alertError;
            break;
        case altCalcNeedCurveSel:
            txt   = libLang->translate(_T("$$CALC NEED CURVE"));
            goto l_alertError;
            break;
        case altCalcNeedSelect:
            txt   = libLang->translate(_T("$$CALC NEED SELECT"));
            goto l_alertError;
            break;
        case altGrcFuncErr:
            txt   = libLang->translate(_T("$$GRCFUNCERR"));
            goto l_alertError;
            break;
    }

    return (ret);
}

/********************************************************************************
 * Display one of the custom alert messages on screen and return answer.        *
 ********************************************************************************/
static TCHAR *substituteTxt (const TCHAR *str, const TCHAR *searchStr, const TCHAR *replStr) {
    TCHAR *text;

    // Calculate final length of substituted string
    const TCHAR *tmp;
    int ilen = (_tcslen(str) + 1)*sizeof(TCHAR); // Number of bytes + 1
    int flen = ilen; // final length of string
    int wlen_s = _tcslen(searchStr);
    int wlen_r = 0;
    if ((replStr != NULL) && ((wlen_r = _tcslen(replStr)) > 0)) {
        tmp = str;
        while ((tmp = _tcsstr(tmp, searchStr)) != NULL) {
            flen += wlen_r*sizeof(TCHAR);
            tmp += wlen_s;
        }
    }

    // Allocate memory, and do the substitution
    text = (TCHAR *) mmalloc(flen);
    if ((replStr != NULL) && (wlen_r > 0)) {
        byte *tmp1, *tmp2;
        size_t n;

        tmp1 = (byte *) str;
        tmp2 = (byte *) text;
        while ((tmp = _tcsstr((TCHAR *) tmp1, searchStr)) != NULL) {
            // Found a piece to substitute
            n = (((long) tmp) - ((long)tmp1));
            if (n > 0) // Copy the good portion
                memcpy(tmp2, tmp1, n);
            // Skip substituted portion, and replace by the new one
            tmp1 = ((byte *) (tmp + wlen_s));
            tmp2 += n;
            memcpy(tmp2, replStr, wlen_r*sizeof(TCHAR));
            tmp2 += wlen_r*sizeof(TCHAR);
        }
        // Copy the end of string (which can be limited to the end '\0')
        memcpy(tmp2, tmp1, ilen-(int)(((long)tmp1)-((long)str)));
    } else
        _tcscpy(text, str);

    return(text);
}

int FrmCustomAlert(int formNb, const TCHAR *s1, const TCHAR *s2, const TCHAR *s3, void *hWnd_p) {
    int ret = -1;

    switch (formNb) {
        case altNoLayoutFile:
            {
            const TCHAR *title, *txt;
            title = libLang->translate(_T("$$ERROR"));
            txt   = substituteTxt(libLang->translate(_T("$$NO LAYOUT FILE")), _T("^1"), s1);
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OK | MB_ICONERROR | MB_APPLMODAL);
            mfree(txt);
            }
            ret = 0;
            break;
        case altErrorGifFile:
            {
            const TCHAR *title, *txt;
            title = libLang->translate(_T("$$ERROR"));
            txt   = substituteTxt(libLang->translate(_T("$$ERROR LOADING GIF")), _T("^1"), s1);
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OK | MB_ICONERROR | MB_APPLMODAL);
            mfree(txt);
            }
            ret = 0;
            break;
        case altConfirmDelete:
            {
            const TCHAR *title;
            TCHAR *txt, *txt1;
            title = libLang->translate(_T("$$DELETE TITLE"));
            txt   = substituteTxt(libLang->translate(_T("$$DELETE MSG")), _T("^1"), s1);
            if (s2) {
                txt1 = substituteTxt(txt, _T("^2"), s2);
                mfree(txt);
                txt = txt1;
            }
            if (s3) {
                txt1 = substituteTxt(txt1, _T("^3"), s3);
                mfree(txt);
                txt = txt1;
            }
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_APPLMODAL);
            mfree(txt);
            }
            if (ret == IDOK)
                ret = 0;
            else   ret = 1;
            break;
        case altConfirmOverwrite:
            {
            const TCHAR *title;
            TCHAR *txt;
            title = libLang->translate(_T("$$OVERWRITE TITLE"));
            txt   = substituteTxt(libLang->translate(_T("$$OVERWRITE MSG")), _T("^1"), s1);
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_APPLMODAL);
            mfree(txt);
            }
            if (ret == IDOK)
                ret = 0;
            else   ret = 1;
            break;
        case altSolverResult:
            {
            const TCHAR *title;
            TCHAR *txt;
            title = libLang->translate(_T("$$RESULT"));
            txt   = substituteTxt(libLang->translate(_T("$$RESULT MSG")), _T("^1"), s1);
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
            mfree(txt);
            }
            ret = 0;
            break;
        case altBadGifFile:
            {
            const TCHAR *title;
            TCHAR *txt;
            title = libLang->translate(_T("$$ERROR"));
            txt   = substituteTxt(libLang->translate(_T("$$BAD GIF")), _T("^1"), s1);
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
            mfree(txt);
            }
            ret = 0;
            break;
        case altMemAllocError:
            {
            const TCHAR *title;
            TCHAR *txt, *txt1;
            title = libLang->translate(_T("$$ERROR"));
            txt   = substituteTxt(libLang->translate(_T("$$MEMALLOC FAIL")), _T("^1"), s1);
            TCHAR text[10];
            _ltot((long) s2, text, 10);
            txt1 = substituteTxt(txt, _T("^2"), text);
            mfree(txt);
            txt = txt1;
            _ltot((long) s3, text, 10);
            txt1 = substituteTxt(txt, _T("^3"), text);
            mfree(txt);
            txt = txt1;
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
            mfree(txt);
            }
            ret = 0;
            break;
        case altComputeResult:
            {
            const TCHAR *title;
            TCHAR *txt, *txt1;
            title = libLang->translate(_T("$$GRAPH COMPUTING RESULT"));
            txt   = substituteTxt(libLang->translate(_T("$$COMPRESULT")), _T("^1"), s1);
            txt1 = substituteTxt(txt, _T("^2"), s2);
            mfree(txt);
            txt = txt1;
            ret = MessageBox(g_hWnd, txt, title,
                             MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
            mfree(txt);
            }
            ret = 0;
            break;
    }

    return (ret);
}

/********************************************************************************
 * Display the modal ask() panel, and get result.                               *
 ********************************************************************************/
Boolean popupAskTxt (t_asktxt_param *param) {
    return (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_ASKTXT), (HWND) g_hWnd,
                           WndAskTxtPopup, (LPARAM) param)
            != 0);
}

/********************************************************************************
 *  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)                               *
 *  Processes messages for the main window.                                     *
 *  WM_COMMAND  - process the application menu                                  *
 *  WM_PAINT    - Paint the main window                                         *
 *  WM_DESTROY  - post a quit message and return                                *
 *  ex ApplicationHandleEvent                                                   *
 ********************************************************************************/
LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static SHACTIVATEINFO s_sai;
    static bool mouseMoved;
    static int  skey0;
    bool processed = false;
    long res = 0;
    int wmId, wmEvent;
    rpntype export_type;

    //if (g_hWnd == NULL)
        // Make window handle available to other modules or procedures.
        g_hWnd = hWnd;
    switch (message) {
        case WM_COMMAND:
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId) {
                case IDM_FILE_OPTIONS:
                    PROPSHEETHEADER psh;
                    psh.dwSize = sizeof(PROPSHEETHEADER);
                    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_MAXIMIZE;
                    psh.pfnCallback = WndPropSheetCallback;
                    psh.nPages = DEF_NUM_OF_PAGES;
                    psh.nStartPage = 0;
                    psh.pszIcon = NULL;
                    psh.pszCaption = NULL;
                    psh.hwndParent = g_hWnd;
                    psh.hInstance = g_hInst;
                    psh.ppsp = propPages;
                    PropertySheet(&psh);
                    // Re-create main window contents if necessary
                    if (new_lang) { // Set new language and remember it
                        _tcscpy(state->langName, new_lang);
                        new_lang = NULL;
                        libLang->setLang(state->langName);
                    }
                    if (change_screen) {
                        change_screen = false;
                        if (reload_skins) {
                            reload_skins = false; // Avoid multiple calls if a paint
                                                  // occurs in the middle.
                            HDC hdc = GetDC(g_hWnd);
                            HDC memdc = CreateCompatibleDC(hdc);
                            InitScreen(true, memdc);
                            DeleteDC(memdc);
                            ReleaseDC(g_hWnd, hdc);
                        } else
                            InitScreen(false, NULL); // Do not reload skins, just change
                    }
                    processed = true;
                    break;
                case IDM_FILE_IMPORT:
                    {
                    // Need AfxWinInit() called at beginning (not done if not an MFC type project)
                    TCHAR szFilters[]= _T("Unicode txt Files (*.utxt)|*.utxt|Txt Files (*.txt)|*.txt|All Files (*.*)|*.*||");
                    CFileDialog dlgFile(TRUE, _T("utxt"), NULL,
                                        OFN_EXPLORER,
                                        szFilters
                                       );
                    if (dlgFile.DoModal() == IDOK) {
                        CString pathName = dlgFile.GetPathName();
                        // Open and read file. This can be long ...
                        SetCursor(LoadCursor(NULL, IDC_WAIT));
                        bool unicode = (pathName.Find(_T(".utxt")) != -1);
                        bool res = memo_import_memo(cur_skin, g_hWnd, pathName.GetString(), unicode);
                        SetCursor(NULL);
                        if (!res) {
                            FrmAlert(altMemoImportError, g_hWnd);
                        }
                    }
                    }
                    break;
                case IDM_EXPORT_VARIABLES:
                    export_type = variable;
                    goto export_objects;
                case IDM_EXPORT_FUNCTIONS:
                    export_type = function;
                    goto export_objects;
                case IDM_EXPORT_EQUATIONS:
                    export_type = (rpntype) EQUATIONS;
                export_objects:
                    {
                    // Need AfxWinInit() called at beginning (not done if not an MFC type project)
                    TCHAR szFilters[]= _T("Unicode txt Files (*.utxt)|*.utxt|Txt Files (*.txt)|*.txt|All Files (*.*)|*.*||");
                    CFileDialog dlgFile(FALSE, _T("utxt"), NULL,
                                        OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                                        szFilters
                                       );
                    if (dlgFile.DoModal() == IDOK) {
                        CString pathName = dlgFile.GetPathName();
                        // Delete file first if it exists, to have the correct creation date
                        DeleteFile(pathName.GetString());
                        // Open and write to file. This can be long ...
                        SetCursor(LoadCursor(NULL, IDC_WAIT));
                        bool unicode = (pathName.Find(_T(".utxt")) != -1);
                        memo_dump(export_type, pathName.GetString(), unicode);
                        SetCursor(NULL);
                    }
                    }
                    break;
#ifdef WIN32_PLATFORM_PSPC
                case IDM_FILE_EXIT:
                    SendMessage (g_hWnd, WM_CLOSE, 0, 0);
                    processed = true;
                    break;
#endif // WIN32_PLATFORM_PSPC
                case IDM_EDIT_CLIPCOPY:
                    cur_skin->clipboardAction(NULL, WM_COPY);
                    processed = true;
                    break;
                case IDM_EDIT_CLIPCUT:
                    cur_skin->clipboardAction(NULL, WM_CUT);
                    processed = true;
                    break;
                case IDM_EDIT_CLIPPASTE:
                    cur_skin->clipboardAction(NULL, WM_PASTE);
                    processed = true;
                    break;
                case IDM_EDIT_CLEARHISTORY:
                    history_shrink(0);
                    processed = true;
                    break;
                case IDM_EDIT_DATAMANAGER:
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DATAMGR), g_hWnd, WndDataMgr);
                    processed = true;
                    break;
                case IDM_EDIT_LISTEDITOR:
                    DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_LISTEDIT), g_hWnd,
                                   WndListEditor, (LPARAM) FALSE);
                    processed = true;
                    break;
                case IDM_EDIT_MATRIXEDITOR:
                    DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_MATRIXEDIT), g_hWnd,
                                   WndMatrixEditor, (LPARAM) FALSE);
                    processed = true;
                    break;
                case IDM_SPECIAL_SOLVER:
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SOLVER), g_hWnd, WndSolver);
                    processed = true;
                    break;
                case IDM_SPECIAL_FINANCIALCALCULATOR:
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FINANCIALS), g_hWnd, WndFinCalc);
                    processed = true;
                    break;
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), g_hWnd, WndAbout);
                    processed = true;
                    break;
                case IDM_HELP_HELP:
                    {
                    PROCESS_INFORMATION pi;
                    TCHAR args[MAX_PATH];
                    _tcscpy(args, easyCalcDirname);
                    _tcscat(args, _T("\\docs\\manual.html"));
                    CreateProcess(_T("iexplore.exe"),
                                  args,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  CREATE_NEW_CONSOLE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &pi
                                  );
                    }
                    processed = true;
                    break;
            }
            break;

        case WM_CREATE:
#ifdef SHELL_AYGSHELL
            // Initialize the shell activate info structure
            memset(&s_sai, 0, sizeof (s_sai));
            s_sai.cbSize = sizeof (s_sai);

            SHMENUBARINFO mbi;
            memset(&mbi, 0, sizeof(SHMENUBARINFO));
            mbi.cbSize     = sizeof(SHMENUBARINFO);
            mbi.hwndParent = g_hWnd;
            mbi.nToolBarId = IDR_MENU;
            mbi.hInstRes   = g_hInst;
            if (!SHCreateMenuBar(&mbi)) {
                g_hWndMenuBar = NULL;
            } else {
                g_hWndMenuBar = mbi.hwndMB;
            }

            processed = true;
#endif // SHELL_AYGSHELL
            break;

        case WM_PAINT:
            {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(g_hWnd, &ps);

            if (cur_skin != NULL) { // We can be called before the skin was initialized,
                                    // for example in error cases.
                HDC memdc = CreateCompatibleDC(hdc);
                cur_skin->repaint(hdc, memdc);
                if (cur_skin->graph) {
                    if (cur_skin->draw_graph(hdc) == 0) { // More drawing needed ?
                        PostMessage(g_hWnd, WM_APP_GRPHDRAWASYNC, (WPARAM) 1, (LPARAM) 1);
                        if (!cur_skin->continueOnView)
                            cur_skin->draw_zoneOnView(hdc);
                    } else
                        cur_skin->draw_zoneOnView(hdc);
                } else {
                    cur_skin->repaint_result(hdc);
                }
                for (int i=0 ; i<NB_ANNUN ; i++)
                    if (skey != (ANNUNBASE-i))
                        cur_skin->repaint_annunciator(hdc, memdc, i, ann_state[i]);
                if (ckey != KEY_NONE) {
                    if ((skey >= ANNUNBASE-NB_ANNUN+1) && (skey <= ANNUNBASE))  /* Annunciator */
                        cur_skin->repaint_annunciator(hdc, memdc, -(skey-ANNUNBASE), 2);
                    else   cur_skin->repaint_key(hdc, memdc, skey, 1);
                }
                if (cur_skin->graph
                    && (((dispPrefs.penMode == pen_trackpt) || (dispPrefs.penMode == pen_selzone))))
                    // Redraw selection if there is one
                    cur_skin->switchSelGraph(hdc);
                DeleteDC(memdc);
            }

            EndPaint(g_hWnd, &ps);
            }
            processed = true;
            break;

        case WM_LBUTTONDOWN:
            if ((ckey == KEY_NONE) && (cur_skin != NULL)) {  // No current key
                int x = LOWORD(lParam);  // horizontal position of cursor
                int y = HIWORD(lParam);  // vertical position of cursor
                cur_skin->find_key(x, y, (ann_state[ANN_SHIFT] != 0), &skey, &ckey);
                // Handle drag movements
                skey0 = skey;
                if (skey == RESULT_AREA) {
                    HDC hdc = GetDC(g_hWnd);
                    cur_skin->tapOnResult(hdc, x, y, track_set);
                    ReleaseDC(g_hWnd, hdc);
                    mouseMoved = false;
                } else if ((skey == ZONE_AREA) || (skey == WVIEW_AREA)) {
                    HDC hdc = GetDC(g_hWnd);
                    cur_skin->tapOnGraph(hdc, skey, x, y, track_set);
                    ReleaseDC(g_hWnd, hdc);
                } else {
                    // If we are on an annunciator which is not enabled nor always selectable,
                    // and this is not a skin annunciator, or result / history, cancel.
                    if ((skey <= ANNUNBASE) && (skey >= ANNUNBASE-NB_ANNUN+1) && (ann_state[-(skey-ANNUNBASE)] == 0)
                        && ((skey > -(ANN_S1-ANNUNBASE)) || (skey < -(ANN_CTRWIDE-ANNUNBASE)))) {
                        skey = NOSKEY;
                        ckey = KEY_NONE;
                    }
                    if (ckey != KEY_NONE) {
                        macro = cur_skin->find_macro(ckey);
                        shell_keydown();
                        mouse_key = true;
                    }
                }
            }
            processed = true;
            break;

        case WM_MOUSEMOVE:
            if ((cur_skin != NULL)
                && (state->mouse_cont ||(skey == RESULT_AREA) || (skey == ZONE_AREA)
                    || (skey == WVIEW_AREA))) {
                int x = LOWORD(lParam);  // horizontal position of cursor
                int y = HIWORD(lParam);  // vertical position of cursor
                int new_ckey, new_skey;
                cur_skin->find_key(x, y, (ann_state[ANN_SHIFT] != 0), &new_skey, &new_ckey);

                if (skey == RESULT_AREA) {
                    HDC hdc = GetDC(g_hWnd);
                    if (new_skey == RESULT_AREA) {
                        mouseMoved |= cur_skin->tapOnResult(hdc, x, y, track_move);
                    } else {
                        mouseMoved |= cur_skin->tapOnResult(hdc, x, y, track_off);
                    }
                    ReleaseDC(g_hWnd, hdc);
                } else if ((skey == ZONE_AREA) || (skey == WVIEW_AREA)) {
                    HDC hdc = GetDC(g_hWnd);
                    if ((new_skey == ZONE_AREA) || (new_skey == WVIEW_AREA)){
                        cur_skin->tapOnGraph(hdc, new_skey, x, y, track_move);
                    } else {
                        cur_skin->tapOnGraph(hdc, skey, x, y, track_off);
                    }
                    ReleaseDC(g_hWnd, hdc);
                }
                if (state->mouse_cont) {
                    // Do this only if continuous/dynamic mouse is enabled
                    // The primary reason for continuous mouse is for
                    // spurious signals on some touch screens, like
                    // mine ... (an ASUS MyPal A639). This is to ignore
                    // first erratical (x,y) and to get stable one(s) coming
                    // after.
                    // If we are on an annunciator which is not enabled nor always selectable,
                    // and this is not a skin annunciator, or result / history, cancel.
                    if ((new_skey <= ANNUNBASE) && (new_skey >= ANNUNBASE-NB_ANNUN+1) && (ann_state[-(new_skey+2)] == 0)
                        && ((new_skey > -(ANN_S1-ANNUNBASE)) || (new_skey < -(ANN_CTRWIDE-ANNUNBASE)))) {
                        new_skey = NOSKEY;
                        new_ckey = KEY_NONE;
                    }
                    if ((new_skey != skey) || (new_ckey != ckey)) { // We are not on the same key
                                                                    // anymore.
                        // Get the previous key up & no further action
                        shell_keyup(false, g_hWnd);
                        if (new_ckey != KEY_NONE) {
                            ckey = new_ckey;
                            skey = new_skey;
                            macro = cur_skin->find_macro(ckey);
                            shell_keydown();
                            mouse_key = true;
                        } else if (new_skey != NOSKEY) {
                            ckey = new_ckey;
                            skey = new_skey;
                            shell_keydown();
                            mouse_key = true;
                            // If entering a drag area and we did not start there,
                            // initiate moving it.
                            if ((new_skey == RESULT_AREA) && (skey0 != RESULT_AREA)) {
                                HDC hdc = GetDC(g_hWnd);
                                cur_skin->tapOnResult(hdc, x, y, track_set);
                                ReleaseDC(g_hWnd, hdc);
                                mouseMoved = false;
                            } else if (((new_skey == ZONE_AREA) || (new_skey == WVIEW_AREA))
                                       && (new_skey != skey0)) {
                                HDC hdc = GetDC(g_hWnd);
                                cur_skin->tapOnGraph(hdc, new_skey, x, y, track_set);
                                ReleaseDC(g_hWnd, hdc);
                            }
                        }
                    }
                }
            }
            processed = true;
            break;

        case WM_LBUTTONUP:
            if ((ckey != KEY_NONE) && mouse_key && (cur_skin != NULL)) { // We have a current key
                if (BrH_timer) // Remove any highlighting, and restore cursor position if so
                    matchBracket_timeout(NULL, 0, 0, 0);
                shell_keyup(true, g_hWnd);
                if (change_screen) {
                    change_screen = false;
                    InitScreen(false, NULL); // Draw, but do not reload skins,
                                             // here we can only have a change of skin.
                }
            } else if (skey == RESULT_AREA) {
                HDC hdc = GetDC(g_hWnd);
                cur_skin->tapOnResult(hdc, -1, -1, track_off);
                ReleaseDC(g_hWnd, hdc);
                if (!mouseMoved) // If only a tap, could be a tap to edit a list or matrix
                    shell_keyup(true, g_hWnd);
            } else if ((skey == ZONE_AREA) || (skey == WVIEW_AREA)){
                HDC hdc = GetDC(g_hWnd);
                cur_skin->tapOnGraph(hdc, skey, -1, -1, track_off);
                ReleaseDC(g_hWnd, hdc);
            }

            processed = true;
            break;

        case WM_KEYUP:
            if (cur_skin->graph) {
                double x, y;
                int nVirtKey = (int) wParam;
                int lKeydata = (int) lParam;

                switch (nVirtKey) {
                    case 37: // Arrow left
                        x = graphPrefs.xmin;
                        y = (graphPrefs.ymin + graphPrefs.ymax) / 2.0;
                        processed = true;
                        break;
                    case 38: // Arrow up
                        x = (graphPrefs.xmin + graphPrefs.xmax) / 2.0;
                        y = graphPrefs.ymax;
                        processed = true;
                        break;
                    case 39: // Arrow right
                        x = graphPrefs.xmax;
                        y = (graphPrefs.ymin + graphPrefs.ymax) / 2.0;
                        processed = true;
                        break;
                    case 40: // Arrow down
                        x = (graphPrefs.xmin + graphPrefs.xmax) / 2.0;
                        y = graphPrefs.ymin;
                        processed = true;
                        break;
                }
                if (processed) {
                    HDC hdc = GetDC(g_hWnd);
                    cur_skin->moveGraph(hdc, ZONE_AREA, x, y);
                    ReleaseDC(g_hWnd, hdc);
                }
            }
            break;

        case WM_APP_ENDVMENU:
            if (BrH_timer) // Remove any highlighting, and restore cursor position if so
                matchBracket_timeout(NULL, 0, 0, 0);
            cur_skin->insert_input_text(NULL, (TCHAR *) lParam);
            mfree(lParam);
            processed = true;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            if (BrH_timer) // Remove any highlighting, and restore cursor position if so
                matchBracket_timeout(NULL, 0, 0, 0);
            // NULL as hWnd parameter means the input Edit area
            main_insert(cur_skin, NULL, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            processed = true;
            break;

        case WM_APP_GRPHDRAWASYNC: // Need to continue graph drawing
            if (cur_skin->graph) { // We may have switched skin in between
                int rc = cur_skin->drawgraph_async();
                if ((rc == 0) && !cur_skin->graphComplete) // More drawing needed ?
                    PostMessage(g_hWnd, WM_APP_GRPHDRAWASYNC, (WPARAM) 0, (LPARAM) 0);
            }
            break;

        case WM_APP_ENDGTMENU: // Redisplay the tracked curve annunciator
            if (cur_skin->graph) { // We may have switched skin in between
                HDC hdc = GetDC(g_hWnd);
                cur_skin->repaint_grphVal(hdc, ANN_CURVENB);
                ReleaseDC(g_hWnd, hdc);
            }
            break;

        case WM_APP_ENDGCMENU: // Execute graphic calculation
            {
            double inputmin, inputmax, result;
            CodeStack *fcstack,*fcstack2;

            inputmin = cur_skin->param0;
            inputmax = cur_skin->param;
            if ((inputmin != NaN) && (inputmin > inputmax)) {
                result = inputmin;
                inputmax = inputmin;
                inputmin = result;
            }

            fcstack = graphCurves[cur_skin->curve_nb].stack1;
            fcstack2 = graphCurves[cur_skin->curve_nb].stack2;
            switch (grFuncType) {
                case cp_zero:        // Find a zero between min & max
                    result = integ_zero(inputmin, inputmax, 0.0, fcstack,
                                        DEFAULT_ERROR, MATH_FZERO, NULL);
                    break;
                case cp_value:       // Find a point at which fct returns value between min & max
                    {
                    Complex cplx;
                    cplx.real = cplx.imag = 0.0;
                    t_getcplx_param param;
                    if (graphPrefs.functype == graph_func) {
                        param.label = param.title = _T("Y =");
                    } else if (graphPrefs.functype == graph_polar) {
                        param.label = param.title = _T("r =");
                    }
                    param.value = &cplx;
                    if (DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), g_hWnd,
                                        WndGetComplex,
                                        (LPARAM) &param
                                       )
                       ) {
                        result = integ_zero(inputmin, inputmax, cplxPopup_value.real, fcstack,
                                            DEFAULT_ERROR, MATH_FVALUE, NULL);
                    } else {
                        result = NaN;
                    }
                    }
                    break;
                case cp_min:         // Find minimum output over [min, max]
                    result = integ_zero(inputmin, inputmax, 0.0, fcstack,
                                        DEFAULT_ERROR, MATH_FMIN, NULL);
                    break;
                case cp_max:         // Find maximum output over [min, max]
                    result = integ_zero(inputmin, inputmax, 0.0, fcstack,
                                        DEFAULT_ERROR, MATH_FMAX, NULL);
                    break;
                case cp_dydx1:       // Derived value at selected input x value
                case cp_pdxdt:       // Value of dx/dt in parametric mode at a given t
                case cp_odrdfi:      // Value of dr/dth in polar mode at a given th
                    result = integ_derive1(inputmax, fcstack, DEFAULT_ERROR, NULL);
                    break;
                case cp_dydx2:       // Derived of derived at selected input x
                    result = integ_derive2(inputmax, fcstack, DEFAULT_ERROR, NULL);
                    break;
                case cp_integ:       // Integration of fct between min & max
                    result = integ_romberg(inputmin, inputmax, fcstack,
                                           DEFAULT_ROMBERG, NULL);
//                    result = integ_simps(inputmin, inputmax, fcstack,
//                                         DEFAULT_ERROR);
                    break;
                case cp_intersect:   // Intersection point between 2 curves over min & max
                    if (!graphCalcIntersect) {
                        graphCalcIntersect = true;
                        // Get list of curves, and display menu only if 1 or more available
                        grphTrkCount = grsetup_fn_descr_arr(GrphTrkDescr, GrphTrkNums);
                        CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hWnd,
                                          WndGraphTrackMenu, (LPARAM) hWnd);
                        result = NaN; // No result yet => no action
                    } else {
                        graphCalcIntersect = false;
                        if (grphTrkCurve_nb == -1) {
                            result = NaN;
                            break;
                        }
                        if (cur_skin->curve_nb == grphTrkCurve_nb) {
                            FrmAlert(altGrcFuncErr, NULL);
                            result = NaN;
                            break;
                        }
                        fcstack2 = graphCurves[grphTrkCurve_nb].stack1;
                        result = integ_intersect(inputmin, inputmax, fcstack,
                                                 fcstack2, DEFAULT_ERROR, NULL);
                    }
                    break;
                case cp_pdydx:       // value of dy/dx at point t in parametric mode at a given t
                    result = integ_derive1(inputmax, fcstack2, DEFAULT_ERROR, NULL);
                    result /= integ_derive1(inputmax, fcstack, DEFAULT_ERROR, NULL);
                    break;
                case cp_pdydt:       // Value of dy/dt in parametric mode at a given t
                    result = integ_derive1(inputmax, fcstack2, DEFAULT_ERROR, NULL);
                    break;
            }

            // Display result, if there is one
            if (finite(result)) {
                /* The default error is lower, but this may
                   display better values */
                /* Save the result as 'graphres' variable */
                db_save_real(_T("graphres"), result);

                /* Round the result with respect to computing error */
                result /= 1E-5;
                result = round(result) * 1E-5;

                /* Set the cross to result where applicable */
                switch (grFuncType) {
                case cp_zero:
                case cp_value:
                case cp_min:
                case cp_max:
                case cp_intersect:
                    {
                    HDC hdc = GetDC(g_hWnd);
                    cur_skin->showOnGraph(hdc, result);
                    ReleaseDC(g_hWnd, hdc);
                    }
                    break;
                }

                TCHAR *text = display_real(result);
                FrmCustomAlert(altComputeResult, calcResult_name, text, NULL, NULL);
                MemPtrFree(text);
            } else if (!graphCalcIntersect) {
                FrmAlert(altCompute, NULL);
            }
            }
            break;

        case WM_APP_EXEBUTTON: // User pressed the enter key or the OK button PDA / Phone
            {
            if (BrH_timer) // Remove any highlighting, and restore cursor position if so
                matchBracket_timeout(NULL, 0, 0, 0);
            ckey = KEY_EXE;
            int repeat;
            running = core_keydown(ckey, &enqueued, &repeat);
            running = core_keyup(cur_skin, g_hWnd);
            }
            processed = true;
            break;

        case WM_APP_MATCHBRKT: // Bracket typed in the input area
            {
            BrH = 2;
            matchBracket_timeout(NULL, 0, 0, 0);
            }
            processed = true;
            break;

        case WM_DESTROY:
#ifdef SHELL_AYGSHELL
            CommandBar_Destroy(g_hWndMenuBar);
#endif // SHELL_AYGSHELL
            PostQuitMessage(0); // Send WM_QUIT to the main loop
            processed = true;
            break;

        case WM_ACTIVATE:
            {
            // When gaining or losing activation, force immediate close down of SIP.
            // This is to fight ActiveSync which, once it got focus with SIP on,
            // seems to introduce a weird behavior on the SIP each time the main
            // window gains or loses activation state.
            // Note: might be due to MagicButton ...
            BOOL rc = SHSipPreference(g_hWnd, SIP_FORCEDOWN);
            ASSERT(rc);
            // Notify shell of our activate message
            rc = SHHandleWMActivate(g_hWnd, wParam, lParam, &s_sai, FALSE);
            ASSERT(rc);
            }
            processed = true;
            break;

        case WM_SETTINGCHANGE:
            {
            // No resize due to SIP ..
            // Screen orientation changes also get to here
//            if (wParam != SPI_SETSIPINFO) {
                BOOL rc = SHHandleWMSettingChange(g_hWnd, wParam, lParam, &s_sai);
                ASSERT(rc);
//            }
            processed = true;
            }
            break;

        case WM_CTLCOLOREDIT: {// Notification sent by an Edit control
                               // when about to be drawn = the input area.
                               // Use that to modify drawing colors.
            // Note that this message is sent each time the control is repainted,
            // not just the first time, and all this needs to be done each time !!

            // This sets the background mode of the TEXT to transparent.
            // It doesn't impact the background of the control itself.
            int i = SetBkMode((HDC)wParam, TRANSPARENT);
            i = SetTextColor((HDC)wParam, cur_skin->getDisplayFgColor());

            // This one affects the background of the control.
            res = (long) (cur_skin->editBgBrush);
            }
            processed = true;
            break;
    }

    if (!processed)
        res = DefWindowProc(g_hWnd, message, wParam, lParam);
    return (res);
}

/********************************************************************************
 *  FUNCTION: WndAbout(HWND, UINT, WPARAM, LPARAM)                              *
 *  Message handler for the About box.                                          *
 ********************************************************************************/
BOOL CALLBACK WndAbout (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
#ifdef SHELL_AYGSHELL
            // Create a Done button and size it.
//            SHINITDLGINFO shidi;
//            shidi.dwMask = SHIDIM_FLAGS;
//            shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN | SHIDIF_EMPTYMENU;
//            shidi.hDlg = hDlg;
//            SHInitDialog(&shidi);
#endif // SHELL_AYGSHELL
            SetWindowText(hDlg, libLang->translate(_T("$$ABOUT...")));
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_STATIC_2);
            TCHAR text[64];
            _tcscpy(text, _T("EasyCalc Version "));
            _tcscat(text, EASYCALC_APPVERSION);
            _tcscat(text, _T(" PPC"));
            SetWindowText(hWnd, text);
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
#ifdef SHELL_AYGSHELL
            if ((LOWORD(wParam) == IDOK)
                && (HIWORD(wParam) == BN_CLICKED))
#endif
            {
                EndDialog(hDlg, LOWORD(wParam));
                processed = TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndPropSheetCallback(HWND, UINT, LPARAM)                          *
 *  Message handler for the Options property sheet.                             *
 ********************************************************************************/
int CALLBACK WndPropSheetCallback(HWND hwndDlg, UINT message, LPARAM lParam) {
    int res = 0;

    switch(message) {
        case PSCB_INITIALIZED:
            {
            HWND hwndChild = GetWindow(hwndDlg, GW_CHILD);
            while (hwndChild) {
                TCHAR szTemp[32];
                GetClassName(hwndChild, szTemp, 32);
                if (_tcscmp(szTemp, _T("SysTabControl32")) == 0)
                    break;
                hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
            }
            if (hwndChild) {
                // Set tabs at bottom for the property sheet
                DWORD dwStyle = GetWindowLong(hwndChild, GWL_STYLE) | TCS_BOTTOM;
                SetWindowLong(hwndChild, GWL_STYLE, dwStyle);
            }
            }
            break;

        case PSCB_GETVERSION:
            res = COMCTL32_VERSION;
            break;
    }
    return (res);
}

/********************************************************************************
 *  FUNCTION: DefDlgCtlColorStaticProc(HWND, WPARAM, LPARAM)                    *
 *  Some formatting for the title of option property pages.                     *
 ********************************************************************************/
BOOL DefDlgCtlColorStaticProc(HWND hDlg, WPARAM wParam, LPARAM lParam, int idc_title) {
    BOOL res = FALSE;

    HDC hDC = (HDC) wParam;
    if (GetDlgCtrlID((HWND)lParam) == idc_title) {
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        res = (BOOL) GetStockObject(WHITE_BRUSH);
    }
    return (res);
}

/********************************************************************************
 *  FUNCTION: WndOptionsGen(HWND, UINT, WPARAM, LPARAM)                         *
 *  Message handler for the General page of Options.                            *
 ********************************************************************************/
BOOL CALLBACK WndOptionsGen (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static LPPROPSHEETPAGE propSheet;
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            {
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            propSheet = (LPPROPSHEETPAGE) lParam;
            const TCHAR *text = libLang->translate(_T("$$GENERAL"));
            SetWindowText(hDlg, text);
            HWND hWnd = GetDlgItem(hDlg, IDC_TITLE);
            SetWindowText(hWnd, text);
            hWnd = GetDlgItem(hDlg, IDC_CHECK_MOUSE);
            SetWindowText(hWnd, libLang->translate(_T("$$DYNAMIC MOUSE")));
            if (state->mouse_cont)
                SendMessage(hWnd, BM_SETCHECK, BST_CHECKED, 0);
            hWnd = GetDlgItem(hDlg, IDC_LANGUAGE);
            SetWindowText(hWnd, libLang->translate(_T("$$LANGUAGE")));
            hWnd = GetDlgItem(hDlg, IDC_LANGUAGE_CB);
            int nbLangs = libLang->getNbLang();
            int numSel = 0;
            const TCHAR *selLang = libLang->getLang();
            const TCHAR *lang;
            for (int i=0 ; i<nbLangs ; i++) {
                lang = libLang->getLangByIndex(i);
                SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) lang);
                if (selLang == lang)
                    numSel = i;
            }
            SendMessage(hWnd, CB_SETCURSEL, (WPARAM) numSel, (LPARAM) 0);
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->code == PSN_APPLY) {
                state->mouse_cont = (SendMessage(GetDlgItem(hDlg, IDC_CHECK_MOUSE), BM_GETCHECK, 0, 0) == 1);
                res = PSNRET_NOERROR;
                processed = TRUE;
            }
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            switch (notify_src) {
                case IDC_LANGUAGE_CB:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int cur_sel = (int) SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            // Set chosen language, and tell WndProc to modify menus
                            // and propSheets accordingly.
                            if (cur_sel != CB_ERR) {
                                new_lang = (TCHAR *) libLang->getLangByIndex(cur_sel); // Force "const" off
                                change_screen = true;
                            }
                            processed = TRUE;
                            break;
                    }
                    break;
            }
            }
            break;

        case WM_CTLCOLORSTATIC:
            // Direct return value
            processed = DefDlgCtlColorStaticProc(hDlg, wParam, lParam, IDC_TITLE);
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  Utility functions for WndOptionsSkins.                                      *
 ********************************************************************************/
// Set values in athe skin combo box for the Skin options property sheet.
TCHAR **getListSkins(TCHAR *dirname, int *list_size) {
    WIN32_FIND_DATA wfd;
    HANDLE          hf;
    TCHAR           find_pattern[MAX_PATH];
    TCHAR         **list = NULL;

    _tcscpy(find_pattern, dirname);
    _tcscat(find_pattern, _T("\\*.layout"));
    // Do a first round to count the number of matching files.
    int max_len = 0;
    *list_size = 0;
    hf = FindFirstFile(find_pattern, &wfd);
    if (hf != INVALID_HANDLE_VALUE) {
        do {
            (*list_size)++;
        } while (FindNextFile(hf, &wfd));
        FindClose(hf);
    } else {
        if (GetLastError() == ERROR_NO_MORE_FILES) {
            // Nothing is matching ...
            FindClose(hf);
        }
    }

    // Now, really get the data
    if ((*list_size) > 0) {
        list = (TCHAR **) mmalloc((*list_size) * sizeof(TCHAR *));
        hf = FindFirstFile(find_pattern, &wfd);
        if (hf != INVALID_HANDLE_VALUE) {
            int i = 0;
            TCHAR *tmp;
            do {
                list[i] = (TCHAR *) mmalloc(MAX_PATH * sizeof(TCHAR));
                tmp = _tcsstr(wfd.cFileName, _T(".layout"));
                if (tmp != NULL)
                    *tmp = _T('\0');
                _tcscpy(list[i], wfd.cFileName);
                i++;
            } while (FindNextFile(hf, &wfd));
            FindClose(hf);
        }
    }

    return (list);
}

// Set values in athe skin combo box for the Skin options property sheet.
void setSkinCombo (HWND hWnd, TCHAR **values, int size, TCHAR *selValue) {
    SendMessage(hWnd, CB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);

    for (int i=0 ; i<size ; i++) {
        SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) values[i]);
    }

    int cur_sel = -1;
    if (selValue[0] != _T('\0')) {
        int res = SendMessage(hWnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) selValue);
        if (res == CB_ERR)
            res = SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) selValue);
        cur_sel = res;
    }
    SendMessage(hWnd, CB_SETCURSEL, (WPARAM) cur_sel, (LPARAM) 0);
}

/********************************************************************************
 *  FUNCTION: WndOptionsSkins(HWND, UINT, WPARAM, LPARAM)                       *
 *  Message handler for the Skins page of Options.                              *
 ********************************************************************************/
BOOL CALLBACK WndOptionsSkins (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static LPPROPSHEETPAGE propSheet;
    static TCHAR **skin_list;
    static int     skinList_size;
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            {
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            propSheet = (LPPROPSHEETPAGE) lParam;
            const TCHAR *text = libLang->translate(_T("$$SKINS"));
            SetWindowText(hDlg, text);
            HWND hWnd = GetDlgItem(hDlg, IDC_TITLE);
            SetWindowText(hWnd, text);

            text = libLang->translate(_T("$$SKIN"));
            TCHAR textTmp[20];
            hWnd = GetDlgItem(hDlg, IDC_SKIN1);
            _tcscpy(textTmp, text);
            SetWindowText(hWnd, _tcscat(textTmp, _T(" 1")));
            hWnd = GetDlgItem(hDlg, IDC_SKIN2);
            _tcscpy(textTmp, text);
            SetWindowText(hWnd, _tcscat(textTmp, _T(" 2")));
            hWnd = GetDlgItem(hDlg, IDC_SKIN3);
            _tcscpy(textTmp, text);
            SetWindowText(hWnd, _tcscat(textTmp, _T(" 3")));
            hWnd = GetDlgItem(hDlg, IDC_SKING);
            _tcscpy(textTmp, text);
            SetWindowText(hWnd, _tcscat(textTmp, _T(" G")));

            // Get list of available skins in EasyCalc dir
            skin_list = getListSkins(easyCalcDirname, &skinList_size);
            hWnd = GetDlgItem(hDlg, IDC_COMBO_SKIN1);
            setSkinCombo(hWnd, skin_list, skinList_size, state->skinName[0]);
            hWnd = GetDlgItem(hDlg, IDC_COMBO_SKIN2);
            setSkinCombo(hWnd, skin_list, skinList_size, state->skinName[1]);
            hWnd = GetDlgItem(hDlg, IDC_COMBO_SKIN3);
            setSkinCombo(hWnd, skin_list, skinList_size, state->skinName[2]);
            hWnd = GetDlgItem(hDlg, IDC_COMBO_SKING);
            setSkinCombo(hWnd, skin_list, skinList_size, state->skinName[3]);

            SetWindowText(GetDlgItem(hDlg, IDC_SKIN), libLang->translate(_T("$$CURRENT SKIN")));
            hWnd = GetDlgItem(hDlg, IDC_SKINVAL);
            int numSel = -1;
            for (int i=0 ; i<NB_SKINS ; i++) {
                if (state->skinName[i][0] != 0) {
                    TCHAR numText[2];
                    if (i < NB_SKINS-1)
                        numText[0] = _T('1'+i);
                    else   numText[0] = _T('G');
                    numText[1] = 0;
                    SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) numText);
                    if (i == state->cur_skin_nb)
                        numSel = i;
                }
            }
            if (numSel != -1) {
                SendMessage(hWnd, CB_SETCURSEL, (WPARAM) numSel, (LPARAM) 0);
            }
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->code == PSN_APPLY) {
                if (skin_list != NULL) {
                    for (int i=0 ; i<skinList_size ; i++)
                        mfree(skin_list[i]);
                    mfree(skin_list);
                    skin_list = NULL;
                }
                res = PSNRET_NOERROR;
                processed = TRUE;
            }
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            switch (notify_src) {
                case IDC_SKINVAL:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int numSel = SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            for (int i=0 ; i<NB_SKINS ; i++) {
                                if (state->skinName[i][0] != 0) {
                                    if (numSel-- == 0) {
                                        // Done in shell_annunciators
                                        // state->cur_skin_nb = i;
                                        shell_annunciators(ANNVAL_S1+i, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                                        break; // Stop loop, we have found it
                                    }
                                }
                            }
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_COMBO_SKIN1:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int numSel = SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            if (numSel != CB_ERR) {
                                SendMessage(hWnd, CB_GETLBTEXT, (WPARAM) numSel, (LPARAM) state->skinName[0]);
                                reload_skins = change_screen = true;
                            }
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_COMBO_SKIN2:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int numSel = SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            if (numSel != CB_ERR) {
                                SendMessage(hWnd, CB_GETLBTEXT, (WPARAM) numSel, (LPARAM) state->skinName[1]);
                                reload_skins = change_screen = true;
                            }
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_COMBO_SKIN3:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int numSel = SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            if (numSel != CB_ERR) {
                                SendMessage(hWnd, CB_GETLBTEXT, (WPARAM) numSel, (LPARAM) state->skinName[2]);
                                reload_skins = change_screen = true;
                            }
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_COMBO_SKING:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int numSel = SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            if (numSel != CB_ERR) {
                                SendMessage(hWnd, CB_GETLBTEXT, (WPARAM) numSel, (LPARAM) state->skinName[3]);
                                reload_skins = change_screen = true;
                            }
                            processed = TRUE;
                            break;
                    }
                    break;
            }
            }
            break;

        case WM_CTLCOLORSTATIC:
            // Direct return value
            processed = DefDlgCtlColorStaticProc(hDlg, wParam, lParam, IDC_TITLE);
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 * Function
 ********************************************************************************/
void addPrecValues(HWND hWnd, int maxPrec) {
    TCHAR text[10];

    for (int i=0 ; i<=maxPrec ; i++) {
        _stprintf(text, _T("%d"), i);
        SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) text);
    }
}

/********************************************************************************
 *  FUNCTION: WndOptionsCalcPrefs(HWND, UINT, WPARAM, LPARAM)                   *
 *  Message handler for the Calculation Preferences page of Options.            *
 *  Ex PreferencesHandleEvent()                                                 *
 ********************************************************************************/
BOOL CALLBACK WndOptionsCalcPrefs (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static LPPROPSHEETPAGE propSheet;
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            {
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            propSheet = (LPPROPSHEETPAGE) lParam;
            const TCHAR *text = libLang->translate(_T("$$PREFERENCES TITLE"));
            SetWindowText(hDlg, text);
            HWND hWnd = GetDlgItem(hDlg, IDC_TITLE);
            SetWindowText(hWnd, text);
            hWnd = GetDlgItem(hDlg, IDC_DEGREE);
            SetWindowText(hWnd, libLang->translate(_T("$$DEGREE")));
            if (calcPrefs.trigo_mode == degree) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDC_RADIAN);
            SetWindowText(hWnd, libLang->translate(_T("$$RADIAN")));
            if (calcPrefs.trigo_mode == radian) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDC_GRAD);
            SetWindowText(hWnd, libLang->translate(_T("$$GRAD")));
            if (calcPrefs.trigo_mode == grad) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }

            hWnd = GetDlgItem(hDlg, IDC_DECIMAL);
            SetWindowText(hWnd, libLang->translate(_T("$$DECIMAL")));
            if (dispPrefs.base == disp_decimal) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDC_BINARY);
            SetWindowText(hWnd, libLang->translate(_T("$$BINARY")));
            if (dispPrefs.base == disp_binary) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDC_OCTAL);
            SetWindowText(hWnd, libLang->translate(_T("$$OCTAL")));
            if (dispPrefs.base == disp_octal) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDC_HEXADECIMAL);
            SetWindowText(hWnd, libLang->translate(_T("$$HEXADEC")));
            if (dispPrefs.base == disp_hexa) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }

            SetWindowText(GetDlgItem(hDlg, IDC_CALCGROUP), libLang->translate(_T("$$CALCULATIONS")));
            hWnd = GetDlgItem(hDlg, IDC_TRUNCPREC);
            TCHAR text1[40], text2[10];
            _tcscpy(text1, libLang->translate(_T("$$REDUCE PRECISION")));
            _stprintf(text2, _T(" (%d)"), ROUND_OFFSET);
            _tcscat(text1, text2);
            SetWindowText(hWnd, text1);
            bool reducePrec;
            if (cur_skin->graph)   reducePrec = graphPrefs.saved_reducePrecision;
            else   reducePrec = calcPrefs.reducePrecision;
            if (reducePrec) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            // Enable the option if not in force integer, else disable it
            EnableWindow(hWnd, !dispPrefs.forceInteger);
            hWnd = GetDlgItem(hDlg, IDC_INTEGERS);
            SetWindowText(hWnd, libLang->translate(_T("$$FORCE INTEGER")));
            if (dispPrefs.forceInteger) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }

            hWnd = GetDlgItem(hDlg, IDC_FORMATNUM);
            SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) libLang->translate(_T("$$NORMAL")));
            SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) libLang->translate(_T("$$MNSCIENTIFIC")));
            SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) libLang->translate(_T("$$ENGINEER")));
            int numSel;
            if (dispPrefs.mode == disp_normal) {
                numSel = 0;
            } else if (dispPrefs.mode == disp_sci) {
                numSel = 1;
            } else if (dispPrefs.mode == disp_eng) {
                numSel = 2;
            }
            SendMessage(hWnd, CB_SETCURSEL, (WPARAM) numSel, (LPARAM) 0);
            SetWindowText(GetDlgItem(hDlg, IDC_PRECISION), libLang->translate(_T("$$PRECISION")));
            hWnd = GetDlgItem(hDlg, IDC_PRECVAL);
            addPrecValues(hWnd, (reducePrec ? ROUND_OFFSET : 15));
            if (reducePrec && (dispPrefs.decPoints > ROUND_OFFSET)) {
                dispPrefs.decPoints = ROUND_OFFSET;
            }
            SendMessage(hWnd, CB_SETCURSEL, (WPARAM) dispPrefs.decPoints, (LPARAM) 0);
            // Enable the option if not in force integer, else disable it
            EnableWindow (hWnd, !dispPrefs.forceInteger);
            SetWindowText(GetDlgItem(hDlg, IDC_FORMATGROUP), libLang->translate(_T("$$FORMAT NUMBERS")));
            hWnd = GetDlgItem(hDlg, IDC_STRIP_0);
            SetWindowText(hWnd, libLang->translate(_T("$$STRIP ZEROS")));
            if (dispPrefs.stripZeros) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            // Enable the option if not in force integer, else disable it
            EnableWindow (hWnd, !dispPrefs.forceInteger);
            hWnd = GetDlgItem(hDlg, IDC_SHOW_UNITS);
            SetWindowText(hWnd, libLang->translate(_T("$$SHOW UNITS")));
            if (dispPrefs.cvtUnits) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            // Enable the option if in Engineer mode, else disable it
            EnableWindow (hWnd, (dispPrefs.mode == disp_eng));
            hWnd = GetDlgItem(hDlg, IDC_OS_NUMPREFS);
            SetWindowText(hWnd, libLang->translate(_T("$$OS NUM PREFS")));
            if (calcPrefs.acceptOSPref) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }

            SetWindowText(GetDlgItem(hDlg, IDC_ASSISTGROUP), libLang->translate(_T("$$EDIT ASSISTANCE")));
            hWnd = GetDlgItem(hDlg, IDC_MATCH_BKT);
            SetWindowText(hWnd, libLang->translate(_T("$$MATCH BRACKETS")));
            if (calcPrefs.matchParenth) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDC_INS_HELP);
            SetWindowText(hWnd, libLang->translate(_T("$$INSERT HELP")));
            if (calcPrefs.insertHelp) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->code == PSN_APPLY) {
                fp_set_prefs(dispPrefs);
                res = PSNRET_NOERROR;
                processed = TRUE;
            }
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            switch (notify_src) {
                case IDC_DEGREE:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            // Already done in shell_annunciators
                            // calcPrefs.trigo_mode = degree;
                            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_DEG, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_RADIAN:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            // Already done in shell_annunciators
                            // calcPrefs.trigo_mode = radian;
                            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_RAD, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_GRAD:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            // Already done in shell_annunciators
                            // calcPrefs.trigo_mode = grad;
                            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_GRAD, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_DECIMAL:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            // Already done in shell_annunciators
                            dispPrefs.base = disp_decimal;
                            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_DEC, ANNVAL_UNCH, ANNVAL_UNCH);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_BINARY:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            // Already done in shell_annunciators
                            // dispPrefs.base = disp_binary;
                            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_BIN, ANNVAL_UNCH, ANNVAL_UNCH);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_OCTAL:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            // Already done in shell_annunciators
                            // dispPrefs.base = disp_octal;
                            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_OCT, ANNVAL_UNCH, ANNVAL_UNCH);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_HEXADECIMAL:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            // Already done in shell_annunciators
                            // dispPrefs.base = disp_hexa;
                            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_HEX, ANNVAL_UNCH, ANNVAL_UNCH);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_TRUNCPREC:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                            bool reducePrec = (sel == BST_CHECKED);
                            if (cur_skin->graph)   graphPrefs.saved_reducePrecision = reducePrec;
                            else   calcPrefs.reducePrecision = reducePrec;
                            // Modify contents of PRECVAL according to this option
                            HWND hWnd_tmp = GetDlgItem(hDlg, IDC_PRECVAL);
                            SendMessage(hWnd_tmp, CB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);
                            addPrecValues(hWnd_tmp, (reducePrec ? ROUND_OFFSET : 15));
                            if (reducePrec && (dispPrefs.decPoints > ROUND_OFFSET)) {
                                dispPrefs.decPoints = ROUND_OFFSET;
                            }
                            SendMessage(hWnd_tmp, CB_SETCURSEL, (WPARAM) dispPrefs.decPoints, (LPARAM) 0);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_INTEGERS:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                            dispPrefs.forceInteger = (sel == BST_CHECKED);
                            // Enable / disable precision options depending on the value
                            HWND hWnd_tmp = GetDlgItem(hDlg, IDC_TRUNCPREC);
                            EnableWindow(hWnd_tmp, !dispPrefs.forceInteger);
                            hWnd_tmp = GetDlgItem(hDlg, IDC_PRECVAL);
                            EnableWindow(hWnd_tmp, !dispPrefs.forceInteger);
                            hWnd_tmp = GetDlgItem(hDlg, IDC_STRIP_0);
                            EnableWindow(hWnd_tmp, !dispPrefs.forceInteger);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_FORMATNUM:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int cur_sel = (int) SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            dispPrefs.mode = ((cur_sel == 0)
                                              ? disp_normal
                                              : ((cur_sel == 1)
                                                 ? disp_sci
                                                 : disp_eng
                                                )
                                             );
                            // Enable the Show units option in disp_eng mode, else disable it
                            HWND hWnd_tmp = GetDlgItem(hDlg, IDC_SHOW_UNITS);
                            EnableWindow(hWnd_tmp, (dispPrefs.mode == disp_eng));
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_PRECVAL:
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int cur_sel = (int) SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            dispPrefs.decPoints = cur_sel;
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_STRIP_0:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                            dispPrefs.stripZeros = (sel == BST_CHECKED);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_SHOW_UNITS:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                            dispPrefs.cvtUnits = (sel == BST_CHECKED);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_OS_NUMPREFS:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                            calcPrefs.acceptOSPref = (sel == BST_CHECKED);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_MATCH_BKT:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                            calcPrefs.matchParenth = (sel == BST_CHECKED);
                            processed = TRUE;
                            break;
                    }
                    break;
                case IDC_INS_HELP:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                            calcPrefs.insertHelp = (sel == BST_CHECKED);
                            processed = TRUE;
                            break;
                    }
                    break;
            }
            }
            break;

        case WM_CTLCOLORSTATIC:
            // Direct return value
            processed = DefDlgCtlColorStaticProc(hDlg, wParam, lParam, IDC_TITLE);
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndResMenu(HWND, UINT, WPARAM, LPARAM)                            *
 *  Message handler for the Result Menu.                                        *
 ********************************************************************************/
BOOL CALLBACK WndResMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static HWND hLst;

    switch (message) {
        case WM_INITDIALOG:
            hLst = GetDlgItem(hDlg, IDC_LIST_RESM);
            result_popup(cur_skin, hLst);
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_RESM) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hLst, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        // Then call the action
                        destroying = true;
                        DestroyWindow(hDlg);
                        result_action(cur_skin, hDlg, g_hWnd, cur_sel);
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // This window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    destroying = true;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndHstMenu(HWND, UINT, WPARAM, LPARAM)                            *
 *  Message handler for the History Menu.                                        *
 ********************************************************************************/
BOOL CALLBACK WndHstMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static HWND hLst;

    switch (message) {
        case WM_INITDIALOG:
            hLst = GetDlgItem(hDlg, IDC_LIST_RESM);
            history_popup(cur_skin, hLst);
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_RESM) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        TCHAR *text;
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hLst, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        // Then call the action
                        destroying = true;
                        DestroyWindow(hDlg);
                        text = history_action(cur_skin, hDlg, g_hWnd, cur_sel);
                        if (text) {
                            cur_skin->insert_input_text(NULL, text);
                            MemPtrFree(text);
                        }
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // The window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    destroying = true;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndSaveAsVar(HWND, UINT, WPARAM, LPARAM)                          *
 *  Message handler for the SaveVarAs IDD_VARENTRY dialog.                      *
 *  Ex VarmgrEntryHandleEvent in varmgr.c                                       *
 ********************************************************************************/
BOOL CALLBACK WndSaveAsVar (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
    static t_saveAsVar_param *param;
    static HWND hLst;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            param = (t_saveAsVar_param *) lParam;
            SetWindowText(hDlg, param->title);
            HWND hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            SetWindowText(hWnd, libLang->translate(_T("$$NAME:")));
            hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            hLst = GetDlgItem(hDlg, IDC_VARCOMBO);
            if (param->verify) {
                // Add existing variable names to the combo
                varmgr_listVar(cur_skin, hLst);
            }
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            //SetFocus(hLst);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hLst, (LPARAM) TRUE) ;
            // Disable OK by default
            hWnd = GetDlgItem(hDlg, IDOK);
            EnableWindow (hWnd, FALSE);
            varPopup_name = NULL;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            HWND hWnd;
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDC_VARCOMBO) { // Message from the List Box
                switch (notify_msg) {
                    case CBN_CLOSEUP:
                        {
                        // Handle state of OK button
                        int cur_sel = (int) SendMessage(hLst, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        int siz = (int) SendMessage(hLst, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        hWnd = GetDlgItem(hDlg, IDOK);
                        EnableWindow (hWnd, (cur_sel != CB_ERR) || (siz > 0));
                        }
                        processed = TRUE;
                        break;
                    case CBN_EDITCHANGE:
                        {
                        // Handle state of OK button
                        int siz = (int) SendMessage(hLst, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        hWnd = GetDlgItem(hDlg, IDOK);
                        EnableWindow (hWnd, (siz > 0));
                        }
                        processed = TRUE;
                        break;
                    // Double click is only recognized for Simple ComboBoxes,
                    // not DropDown, nor DropDownList.
                }
            } else if (notify_src == IDCANCEL) {
                unsigned int notify_msg = wParam >> 16;
                if (notify_msg == BN_CLICKED) {
                    if (param->verify) {
                        varmgr_listVar_action(NULL, NULL, false, false, false);
                    }
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDOK) {
                unsigned int notify_msg = wParam >> 16;
                if (notify_msg == BN_CLICKED) {
                    // When input is made, retrieve it
                    int cur_sel = (int) SendMessage(hLst, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    int siz = (int) SendMessage(hLst, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                    TCHAR *text;
                    text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                    SendMessage(hLst, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                    if (param->verify) {
                        // Then call the action: verify, and save ans in there is required
                        if (varmgr_listVar_action(text, g_hWnd, true, (cur_sel != CB_ERR), param->saveAns)) {
                            if (!param->saveAns) { // Retrieve name and put it in varPopup_name
                                varPopup_name = text;
                            } else {
                                // Error, nobody is going to read text, free memory
                                MemPtrFree(text);
                            }
                            EndDialog(hDlg, 1);
                        } else {
                            // Nobody is going to read text, free memory
                            MemPtrFree(text);
                            // Recreate existing variable names, since we remain in the dialog
                            varmgr_listVar(cur_skin, hLst);
                        }
                    } else {
                        varPopup_name = text;
                        EndDialog(hDlg, 1);
                    }
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_CLOSE:
            if (param->verify) {
                varmgr_listVar_action(NULL, NULL, false, false, false);
            }
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndDataMgr(HWND, UINT, WPARAM, LPARAM)                            *
 *  Message handler for the Data manager dialog.                                *
 *  Ex DefmgrHandleEvent in defmgr.c                                            *
 ********************************************************************************/
BOOL CALLBACK WndDataMgr (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
    static bool wHide;
    static HWND hWnd_fvlist;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$DEFINITION TITLE")));
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDC_NEW);
            SetWindowText(hWnd, libLang->translate(_T("$$NEW")));
            // No initial selection, disable Modify and Delete
            hWnd = GetDlgItem(hDlg, IDC_MODIFY);
            SetWindowText(hWnd, libLang->translate(_T("$$MODIFY")));
            EnableWindow (hWnd, FALSE);
            hWnd = GetDlgItem(hDlg, IDC_DELETE);
            SetWindowText(hWnd, libLang->translate(_T("$$DELETE")));
            EnableWindow (hWnd, FALSE);
            // Set 2 columns in the list view
            hWnd = GetDlgItem(hDlg, IDC_DATALIST);
            LVCOLUMN lvc;
            lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 60 * cur_skin->magnification;
//            lvc.pszText = _T("Name");
            lvc.pszText = (TCHAR *) libLang->translate(_T("$$NAME:"));
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 0, (LPARAM) (&lvc));
            lvc.cx = 170 * cur_skin->magnification;
//            lvc.pszText = _T("Definition");
            lvc.pszText = (TCHAR *) libLang->translate(_T("$$VALUE:"));
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 1, (LPARAM) (&lvc));
            // Add existing variables and functions to the list view
            def_init_varlist(cur_skin, hWnd);
            // The FV list is not visible yet
            hWnd_fvlist = NULL;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->idFrom == IDC_DATALIST) {
                if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_SETFOCUS)
                    || (pnmh->code == NM_KILLFOCUS)) {
                    // Enable Modify and Delete only if there is a selection
                    HWND hWnd = pnmh->hwndFrom;
                    int enableBut = (SendMessage(hWnd, LVM_GETSELECTIONMARK, (WPARAM) 0, (LPARAM) 0)
                                     != -1);
                    hWnd = GetDlgItem(hDlg, IDC_MODIFY);
                    EnableWindow (hWnd, enableBut);
                    hWnd = GetDlgItem(hDlg, IDC_DELETE);
                    EnableWindow (hWnd, enableBut);
                    processed = TRUE;
                } else if (pnmh->code == NM_DBLCLK) {
                    HWND hWnd = pnmh->hwndFrom;
                    // Retrieve selection
                    defSelectedItem = SendMessage(hWnd, LVM_GETSELECTIONMARK, (WPARAM) 0, (LPARAM) 0);

                    if (defSelectedItem != -1) {
                        // Then call the action
                        if (def_type(defSelectedItem) == function) {
                            int rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDITFCT), hDlg, WndModFMenu, NULL);
                            if (rc) {
                                def_destroy_varlist();
                                hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                                def_init_varlist(cur_skin, hWnd);
                            }
                        } else {
                            int rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDITVAR), hDlg, WndModVMenu, NULL);
                            if (rc) {
                                def_destroy_varlist();
                                hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                                def_init_varlist(cur_skin, hWnd);
                            }
                        }
                    }
                    processed = TRUE;
                } else if (pnmh->code == NM_RECOGNIZEGESTURE) {
                    // This will ensure that double click will be recognized.
                    // Not all notification messages are processed, and it seems
                    // that answering TRUE to NM_RECOGNIZEGESTURE instead of FALSE
                    // allows to get the double click notification.
                    res = processed = TRUE;
                }
            }
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    def_destroy_varlist();
                    EndDialog(hDlg, 1);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_NEW) { // Message from the New button
                if (notify_msg == BN_CLICKED) {
                    // Disable the button
                    EnableWindow (hWnd, FALSE);
                    // Show the list box for chosing function or variable
                    hWnd_fvlist = GetDlgItem(hDlg, IDC_LIST_FV);
                    SendMessage(hWnd_fvlist, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);
                    SendMessage(hWnd_fvlist, LB_ADDSTRING, (WPARAM) 0,
                                (LPARAM) libLang->translate(_T("$$FUNCTION")));
                    SendMessage(hWnd_fvlist, LB_ADDSTRING, (WPARAM) 0,
                                (LPARAM) libLang->translate(_T("$$VARIABLE")));
                    wHide = false;
                    // Note: for this to work and show above the ListView,
                    // the control must be declared BEFORE the list view
                    // in EasyCalc.rc.
                    // I don't know why, I may be missing something, or go figure ...!
                    BOOL rc = ShowWindow (hWnd_fvlist, SW_SHOWNORMAL);
                    // This is to ensure a KILLFOCUS msg when clicking elsewhere
                    hWnd = SetFocus(hWnd_fvlist); // Not using WM_NEXTDLGCTL to not trap focus in a
                                                  // control disabled most of the time.
                    processed = TRUE;
                }
            } else if (notify_src == IDC_LIST_FV) {
                bool action = false;
                int cur_sel = 0;
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        // When selection is made, retrieve it (LB_GETCURSEL Message)
                        cur_sel = (int) SendMessage(hWnd, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        action = true;
                    case LBN_KILLFOCUS:
                    case LBN_SELCANCEL:
                        // Then hide the listbox, if not already done
                        if (!wHide) {
                            hWnd_fvlist = NULL; // Forget about this handle
                            wHide = TRUE;
                            ShowWindow (hWnd, SW_HIDE); // This will re-send a KILLFOCUS msg
                            // Re-enable the New button
                            hWnd = GetDlgItem(hDlg, IDC_NEW);
                            EnableWindow (hWnd, TRUE);
                        }
                        // Process action
                        if (action) {
                            if (cur_sel == 0) { // New function
                                int rc = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_EDITFCT), hDlg, WndNewFMenu);
                                if (rc) {
                                    def_destroy_varlist();
                                    hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                                    def_init_varlist(cur_skin, hWnd);
                                }
                            } else { // New variable
                                int rc = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_EDITVAR), hDlg, WndNewVMenu);
                                if (rc) {
                                    def_destroy_varlist();
                                    hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                                    def_init_varlist(cur_skin, hWnd);
                                }
                            }
                        }
                        processed = TRUE;
                        break;
                }
            }  else if (notify_src == IDC_MODIFY) {
                if (notify_msg == BN_CLICKED) {
                    // Retrieve selection
                    hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                    defSelectedItem = SendMessage(hWnd, LVM_GETSELECTIONMARK, (WPARAM) 0, (LPARAM) 0);

                    // Then call the action if there is a selection
                    if (defSelectedItem != -1) {
                        if (def_type(defSelectedItem) == function) {
                            int rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDITFCT), hDlg, WndModFMenu, NULL);
                            if (rc) {
                                def_destroy_varlist();
                                hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                                def_init_varlist(cur_skin, hWnd);
                            }
                        } else {
                            int rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDITVAR), hDlg, WndModVMenu, NULL);
                            if (rc) {
                                def_destroy_varlist();
                                hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                                def_init_varlist(cur_skin, hWnd);
                            }
                        }
                    }
                    processed = TRUE;
                }
            } else if (notify_src == IDC_DELETE) {
                if (notify_msg == BN_CLICKED) {
                    // Delete selection
                    hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                    defSelectedItem = SendMessage(hWnd, LVM_GETSELECTIONMARK, (WPARAM) 0, (LPARAM) 0);
                    if (defSelectedItem != -1) {
                        const TCHAR *tmptxt;
                        rpntype typ = def_type(defSelectedItem);
                        if (typ == variable)
                            tmptxt = libLang->translate(_T("$$STRVARIABLE"));
                        else
                            tmptxt = libLang->translate(_T("$$STRFUNCTION"));
                        if (!FrmCustomAlert(altConfirmDelete, tmptxt, def_name(defSelectedItem), NULL, NULL)) {
                            def_delete(defSelectedItem);
                            def_destroy_varlist();
                            hWnd = GetDlgItem(hDlg, IDC_DATALIST);
                            def_init_varlist(cur_skin, hWnd);

                            // Disable Modify and Delete since the record has disappeared
                            hWnd = GetDlgItem(hDlg, IDC_MODIFY);
                            EnableWindow (hWnd, FALSE);
                            hWnd = GetDlgItem(hDlg, IDC_DELETE);
                            EnableWindow (hWnd, FALSE);
                        }
                    }
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            // Got when clicking in an unhandled part of the window
            if (hWnd_fvlist != NULL) { // FV list is open, kill it
                ShowWindow (hWnd_fvlist, SW_HIDE); // This will send a KILLFOCUS msg
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            def_destroy_varlist();
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndNewVMenu(HWND, UINT, WPARAM, LPARAM)                           *
 *  Message handler for editing a new variable.                                 *
 *  Ex varmgr_edit + varmgr_edit_handler in varmgr.c                            *
 ********************************************************************************/
BOOL CALLBACK WndNewVMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$NEW VARIABLE")));
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            SetWindowText(hWnd, libLang->translate(_T("$$NAME:")));
            hWnd = GetDlgItem(hDlg, IDC_EDIT1);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            //SetFocus(hWnd);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = GetDlgItem(hDlg, IDC_EDIT1);
                    int len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *varName = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) varName);
                    hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *text = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) text);
                    if (varmgr_edit_save(varName, text, NULL, variable, true, hDlg)) {
                        EndDialog(hDlg, 1);
                    }
                    mfree(varName);
                    mfree(text);
                    processed = TRUE;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_VAR) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndVMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_USERF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndFMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }  else if (notify_src == IDC_CALCF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndfMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_APP_ENDVMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SendMessage(hWnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) lParam);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            main_insert(cur_skin, hWnd, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndNewFMenu(HWND, UINT, WPARAM, LPARAM)                           *
 *  Message handler for editing a new function.                                 *
 *  Ex varmgr_edit + varmgr_edit_handler in varmgr.c                            *
 ********************************************************************************/
BOOL CALLBACK WndNewFMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$NEW FUNCTION")));
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            hWnd = GetDlgItem(hDlg, IDC_PARAM);
            SetWindowText(hWnd, libLang->translate(_T("$$PARAM:")));
            // Set default parameter name
            hWnd = GetDlgItem(hDlg, IDC_EDIT3);
            SetWindowText(hWnd, parameter_name);
            hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            SetWindowText(hWnd, libLang->translate(_T("$$NAME:")));
            hWnd = GetDlgItem(hDlg, IDC_EDIT1);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            //SetFocus(hWnd);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = GetDlgItem(hDlg, IDC_EDIT1);
                    int len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *fctName = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) fctName);
                    hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *text = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) text);
                    hWnd = GetDlgItem(hDlg, IDC_EDIT3);
                    len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *param = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) param);

                    TCHAR oldparam[MAX_FUNCNAME+1];
                    _tcscpy(oldparam, parameter_name);
                    if (varmgr_edit_save(fctName, text, param, function, true, hDlg)) {
                        EndDialog(hDlg, 1);
                    }
                    _tcscpy(parameter_name, oldparam);
                    mfree(fctName);
                    mfree(text);
                    mfree(param);
                    processed = TRUE;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_VAR) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndVMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_USERF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndFMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }  else if (notify_src == IDC_CALCF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndfMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_APP_ENDVMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SendMessage(hWnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) lParam);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            main_insert(cur_skin, hWnd, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndModVMenu(HWND, UINT, WPARAM, LPARAM)                           *
 *  Message handler for modifying an existing variable.                         *
 *  Ex varmgr_edit + varmgr_edit_handler in varmgr.c                            *
 ********************************************************************************/
BOOL CALLBACK WndModVMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static TCHAR *varName;
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            {
            // lParam can contain window title and name of variable to edit.
            TCHAR *title;
            if (lParam == NULL) {
                title = NULL;
                varName = def_name(defSelectedItem);
            } else {
                t_getcplx_param *param = (t_getcplx_param *) lParam;
                title = (TCHAR *) param->title;
                varName = (TCHAR *) param->label;
            }

            // Initialize window with current language strings
            if (title == NULL)
                title = (TCHAR *) libLang->translate(_T("$$VARIABLE MODIFICATION"));
            SetWindowText(hDlg, title);
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            SetWindowText(hWnd, libLang->translate(_T("$$NAME:")));
            hWnd = GetDlgItem(hDlg, IDC_EDIT1);
            SetWindowText(hWnd, varName);
            SendMessage(hWnd, EM_SETREADONLY, (WPARAM) TRUE, (LPARAM) 0);
            // Get variable value. This can be long, for big matrices ...
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            TCHAR *varDef;
            if (varmgr_getVarDef(varName, &varDef)) {
                SetWindowText(hWnd, varDef);
                mfree(varDef);
            }
            SetCursor(NULL);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            // By the way, this also selects any text already present in the Edit Box
            //SendMessage(hWnd, EM_SETSEL, (WPARAM) 0, (LPARAM) -1); // Select all text
            //SetFocus(hWnd);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    int len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *text = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) text);
                    if (varmgr_edit_save(varName, text, NULL, variable, false, hDlg)) {
                        EndDialog(hDlg, 1);
                    }
                    mfree(text);
                    processed = TRUE;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_VAR) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndVMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_USERF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndFMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }  else if (notify_src == IDC_CALCF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndfMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_APP_ENDVMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SendMessage(hWnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) lParam);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            main_insert(cur_skin, hWnd, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndModFMenu(HWND, UINT, WPARAM, LPARAM)                           *
 *  Message handler for modifying an existing function.                         *
 *  Ex varmgr_edit + varmgr_edit_handler in varmgr.c                            *
 ********************************************************************************/
BOOL CALLBACK WndModFMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static TCHAR *fctName;
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            {
            // lParam can contain window title and name of variable to edit.
            TCHAR *title;
            TCHAR *fctDef, fctParam[MAX_FUNCNAME+1];
            if (lParam == NULL) {
                title = _T("$$FUNCTION MODIFICATION");
                fctName = def_name(defSelectedItem);
                fctDef = NULL;
                varmgr_getFctDef(fctName, &fctDef, fctParam);
            } else {
                t_getcplx_param *param = (t_getcplx_param *) lParam;
                title = (TCHAR *) param->title;
                fctName = (TCHAR *) param->label;
                fctDef = NULL;
                varmgr_getFctDef(fctName, &fctDef, fctParam);
                _tcscpy(fctParam, param->param);
            }

            // Initialize window with current language strings
            SetWindowText(hDlg, libLang->translate(title));
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            SetWindowText(hWnd, libLang->translate(_T("$$NAME:")));
            hWnd = GetDlgItem(hDlg, IDC_PARAM);
            SetWindowText(hWnd, libLang->translate(_T("$$PARAM:")));
            hWnd = GetDlgItem(hDlg, IDC_EDIT1);
            SetWindowText(hWnd, fctName);
            SendMessage(hWnd, EM_SETREADONLY, (WPARAM) TRUE, (LPARAM) 0);
            hWnd = GetDlgItem(hDlg, IDC_EDIT3);
            SetWindowText(hWnd, fctParam);
            hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SetWindowText(hWnd, fctDef);
            if (fctDef != NULL)   mfree(fctDef);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            // By the way, this also selects any text already present in the Edit Box
            //SendMessage(hWnd, EM_SETSEL, (WPARAM) 0, (LPARAM) -1); // Select all text
            //SetFocus(hWnd);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    int len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *text = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) text);
                    hWnd = GetDlgItem(hDlg, IDC_EDIT3);
                    len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    TCHAR *param = (TCHAR *) mmalloc(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) param);

                    TCHAR oldparam[MAX_FUNCNAME+1];
                    _tcscpy(oldparam, parameter_name);
                    if (varmgr_edit_save(fctName, text, param, function, false, hDlg)) {
                        EndDialog(hDlg, 1);
                    }
                    _tcscpy(parameter_name, oldparam);
                    mfree(text);
                    mfree(param);
                    processed = TRUE;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_VAR) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndVMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_USERF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndFMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }  else if (notify_src == IDC_CALCF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndfMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_APP_ENDVMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SendMessage(hWnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) lParam);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            main_insert(cur_skin, hWnd, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndVMenu(HWND, UINT, WPARAM, LPARAM)                              *
 *  Message handler for the variables listbox.                                  *
 ********************************************************************************/
BOOL CALLBACK WndVMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static HWND hLst, hParent;

    switch (message) {
        case WM_INITDIALOG:
            // Remember parent window
            hParent = (HWND) lParam; // Passed by calling window for return message
            // Initialize window with list of variables
            hLst = GetDlgItem(hDlg, IDC_LIST_LARGE);
            varmgr_popup(cur_skin, hLst, variable);
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_LARGE) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hLst, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        // And store the variable name
                        destroying = true;
                        DestroyWindow(hDlg);
                        varPopup_name = varmgr_action(cur_sel);
                        PostMessage(hParent, WM_APP_ENDVMENU, (WPARAM) 0, (LPARAM) varPopup_name);
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // The window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    varmgr_action(-1);
                    destroying = true;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndFMenu(HWND, UINT, WPARAM, LPARAM)                              *
 *  Message handler for the user functions listbox.                             *
 ********************************************************************************/
BOOL CALLBACK WndFMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static HWND hLst, hParent;

    switch (message) {
        case WM_INITDIALOG:
            // Remember parent window
            hParent = (HWND) lParam; // Passed by calling window for return message
            // Initialize window with list of user functions
            hLst = GetDlgItem(hDlg, IDC_LIST_LARGE);
            varmgr_popup(cur_skin, hLst, function);
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_LARGE) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hLst, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        // And store the function name
                        destroying = true;
                        DestroyWindow(hDlg);
                        fctPopup_name = varmgr_action(cur_sel);
                        PostMessage(hParent, WM_APP_ENDFMENU, (WPARAM) 0, (LPARAM) fctPopup_name);
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // The window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    varmgr_action(-1);
                    destroying = true;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndfMenu(HWND, UINT, WPARAM, LPARAM)                              *
 *  Message handler for the calc functions listbox.                             *
 ********************************************************************************/
BOOL CALLBACK WndfMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static HWND hLst, hParent;

    switch (message) {
        case WM_INITDIALOG:
            // Remember parent window
            hParent = (HWND) lParam; // Passed by calling window for return message
            // Initialize window with list of calc functions
            hLst = GetDlgItem(hDlg, IDC_LIST_SLIM);
            varmgr_popup_builtin(cur_skin, hLst);
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_SLIM) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hLst, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        // And store the function name
                        destroying = true;
                        DestroyWindow(hDlg);
                        builtinFctPopup_name = varmgr_builtinAction(cur_sel);
                        PostMessage(hParent, WM_APP_ENDFCMENU, (WPARAM) 0, (LPARAM) builtinFctPopup_name);
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // The window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    varmgr_builtinAction(-1);
                    destroying = true;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndAskTxtPopup(HWND, UINT, WPARAM, LPARAM)                        *
 *  Message handler for answering the ask() function.                           *
 * This procedure is called through DialogBoxParam. lParam contains a pointer   *
 * at a structure with ask text and default value (if any).                     *
 * Answer contains a piece of memory to free.                                   *
 ********************************************************************************/
BOOL CALLBACK WndAskTxtPopup (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
    static t_asktxt_param *param;

    switch (message) {
        case WM_INITDIALOG:
            param = (t_asktxt_param *) lParam;
            param->answertxt = NULL;
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, param->asktxt);
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            if (param->defaultvalue) {
                SetWindowText(hWnd, param->defaultvalue);
                //SendMessage(hWnd, EM_SETSEL, (WPARAM) 0, (LPARAM) -1); // Select all text
            }
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            // By the way, this also selects any text already present in the Edit Box
            //SetFocus(hWnd);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    int len = SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0) + 1;
                    param->answertxt = (TCHAR *) MemPtrNew(len * sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) len, (LPARAM) (param->answertxt));
                    EndDialog(hDlg, 1);
                    processed = TRUE;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_VAR) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndVMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_USERF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndFMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }  else if (notify_src == IDC_CALCF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndfMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_APP_ENDVMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SendMessage(hWnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) lParam);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            main_insert(cur_skin, hWnd, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  Functions for handling display of lists in the ListEditor window.           *
 ********************************************************************************/
static HWND hDlg_ListEditor;
static HWND hDlg_MatrixEditor;
static HWND hDlg_Solver;
static int newCbChoice, deleteCbChoice, saveCbChoice;
// Select label in a column header combo box
void LstEditSetLabel(int listNb, TCHAR *label) {
    HWND hWnd;
    if (listNb == LIST1_ID) {
        hWnd = GetDlgItem(hDlg_ListEditor, IDC_COMBO_LIST1);
    } else if (listNb == LIST2_ID) {
        hWnd = GetDlgItem(hDlg_ListEditor, IDC_COMBO_LIST2);
    } else if (listNb == LIST3_ID) {
        hWnd = GetDlgItem(hDlg_ListEditor, IDC_COMBO_LIST3);
    } else if (listNb == MATRIX_ID) {
        hWnd = GetDlgItem(hDlg_MatrixEditor, IDC_COMBO_MATRIX);
    } else if (listNb == SOLVER_ID) {
        hWnd = GetDlgItem(hDlg_Solver, IDC_COMBO_WKSHT);
    } else
        return;

    int cur_sel = -1;
    if (label[0] != _T('\0')) { // A selection
        int res = SendMessage(hWnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) label);
        if (res != CB_ERR)
            cur_sel = res;
    }
    SendMessage(hWnd, CB_SETCURSEL, (WPARAM) cur_sel, (LPARAM) 0);
}

// Set label in a first column item, and values in cells.
// If rowNb = -1, delete contents of the List View.
void LstEditSetRow (int rowNb, int value, TCHAR *cell1, TCHAR *cell2, TCHAR *cell3) {
    HWND hWnd = GetDlgItem(hDlg_ListEditor, IDC_EDITLIST);

    if (rowNb == -1) {
        SendMessage((HWND) hWnd, LVM_DELETEALLITEMS, (WPARAM) 0, (LPARAM) 0);
    } else {
        TCHAR text[10];
        _stprintf(text, _T("%d"), value);

        LVITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = rowNb;
        lvi.iSubItem = 0;
        lvi.pszText = text;
        int i = SendMessage((HWND) hWnd, LVM_INSERTITEM, (WPARAM) 0, (LPARAM) &lvi);

        // Note: there is no way to retrieve the length of text
        // stored in a subitem in Windows ..! So we will need to
        // use external means.
        if (cell1 != NULL) {
            lvi.iSubItem = 1;
            lvi.pszText = cell1;
            int i = SendMessage((HWND) hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
        }

        if (cell2 != NULL) {
            lvi.iSubItem = 2;
            lvi.pszText = cell2;
            int i = SendMessage((HWND) hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
        }

        if (cell3 != NULL) {
            lvi.iSubItem = 3;
            lvi.pszText = cell3;
            int i = SendMessage((HWND) hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
        }
    }
}

// Set values in a column header combo box for the List or Matrix editors.
void LstEditSetListChoices (int listNb, TCHAR **values, int size) {
    TCHAR loct1[MAX_RSCLEN], loct2[MAX_RSCLEN], loct3[MAX_RSCLEN];
    HWND hWnd;
    if (listNb == LIST1_ID) {
        hWnd = GetDlgItem(hDlg_ListEditor, IDC_COMBO_LIST1);
    } else if (listNb == LIST2_ID) {
        hWnd = GetDlgItem(hDlg_ListEditor, IDC_COMBO_LIST2);
    } else if (listNb == LIST3_ID) {
        hWnd = GetDlgItem(hDlg_ListEditor, IDC_COMBO_LIST3);
    } else if (listNb == MATRIX_ID) {
        hWnd = GetDlgItem(hDlg_MatrixEditor, IDC_COMBO_MATRIX);
    } else if (listNb == SOLVER_ID) {
        hWnd = GetDlgItem(hDlg_Solver, IDC_COMBO_WKSHT);
    } else
        return;

    SendMessage(hWnd, CB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);

    if (listNb == SOLVER_ID) {
        loct1[0] = _T(' '); // To try to get them first in sorted list
        _tcscpy(loct1+1, libLang->translate(_T("$$NEW WORKSHEET")));
        SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) loct1);
    } else {
        loct1[0] = loct2[0] = loct3[0] = _T(' '); // To try to get them first in sorted list
        _tcscpy(loct1+1, libLang->translate(_T("$$NEW")));
        _tcscpy(loct2+1, libLang->translate(_T("$$DELETE")));
        _tcscpy(loct3+1, libLang->translate(_T("$$MNSAVE AS")));
        SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) loct1);
        SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) loct2);
        SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) loct3);
    }

    for (int i=0 ; i<size ; i++) {
        SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) values[i]);
    }

    if (listNb == SOLVER_ID) {
        newCbChoice = SendMessage(hWnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) loct1);
    } else {
        newCbChoice = SendMessage(hWnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) loct1);
        deleteCbChoice = SendMessage(hWnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) loct2);
        saveCbChoice = SendMessage(hWnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) loct3);
    }
}

// Clear any selection in the ListView control
void LstEditDeselect (void) {
    HWND hWnd = GetDlgItem(hDlg_ListEditor, IDC_EDITLIST);
    SendMessage((HWND) hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) -1);
}

/********************************************************************************
 *  FUNCTION: WndListEditor (HWND, UINT, WPARAM, LPARAM)                        *
 *  Message handler for the List Editor dialog.                                 *
 *  lParam tells to init the first list with ans or not.                        *
 *  Ex ListEditHandleEvent + lstedit_reselect in lstedit.c                      *
 ********************************************************************************/
BOOL CALLBACK WndListEditor (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
//    static bool wHide;
//    static HWND hWnd_siEdit;
    static bool listc_ok[3];
    static int col, row;
    bool append;

    hDlg_ListEditor = hDlg; // Share it with other routines
    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$LIST EDITOR")));
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            // No initial selection, disable New, Insert and Delete
            hWnd = GetDlgItem(hDlg, IDC_APPEND);
            SetWindowText(hWnd, libLang->translate(_T("$$APPEND")));
            EnableWindow (hWnd, FALSE);
            hWnd = GetDlgItem(hDlg, IDC_INSERT);
            SetWindowText(hWnd, libLang->translate(_T("$$INSERT")));
            EnableWindow (hWnd, FALSE);
            hWnd = GetDlgItem(hDlg, IDC_DELETE);
            SetWindowText(hWnd, libLang->translate(_T("$$DELETE")));
            EnableWindow (hWnd, FALSE);
            // Set 4 columns in the list view
            hWnd = GetDlgItem(hDlg, IDC_EDITLIST);
            LVCOLUMN lvc;
            lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 24 * cur_skin->magnification;
            lvc.pszText = _T("#");
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 0, (LPARAM) (&lvc));
            lvc.fmt = LVCFMT_RIGHT;
            lvc.cx = 66 * cur_skin->magnification;
            lvc.pszText = _T("List1");
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 1, (LPARAM) (&lvc));
            lvc.pszText = _T("List2");
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 2, (LPARAM) (&lvc));
            lvc.pszText = _T("List3");
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 3, (LPARAM) (&lvc));
            lstedit_init(((int) lParam == TRUE));
            lstedit_tbl_load();
            // Tell ourselves to update combo boxes contents when they are opening
            // Tell ourselves that combo boxes contents are ok for now
            listc_ok[0] = listc_ok[1] = listc_ok[2] = true;
            // Nothing selected yet
            col = row = -1;
            // The subitem edit box is not visible yet
            // Note: it is created in EasyCalc.rc as follows:
            // CONTROL         "SubitemEdit",IDC_SUBITEMEDIT,"EDIT2",NOT WS_VISIBLE | WS_BORDER | ES_LOWERCASE | ES_AUTOHSCROLL,113,9,46,13
            // Re-note: not using that anymore, but rather a popup, to be able to have the V, F and f buttons
//            hWnd_siEdit = NULL;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR *) lParam;
            int i;
            if (pnmh->idFrom == IDC_EDITLIST) {
                if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_DBLCLK)
                    || (pnmh->code == NM_SETFOCUS) || (pnmh->code == NM_KILLFOCUS)) {
                    HWND hWnd = pnmh->hwndFrom;
                    if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_DBLCLK)) {
                        NMLISTVIEW *nmlv = (NMLISTVIEW *) lParam; // A bigger structure in fact ...
                        if (nmlv->iSubItem != 0) { // Click on a subitem, but we have -1 in iItem,
                                                   // so have to retrieve it.
                            TCHAR dummy[2]; // To get subitem text. Need to initialize it to give a
                                            // non-NULL value to LVM_GETITEMTEXT, or we get a system error.
                            int textLen = 0;
                             // Get the corresponding item = row number
                            LVHITTESTINFO lvhti;
                            lvhti.pt = nmlv->ptAction;
                            i = SendMessage(hWnd, LVM_SUBITEMHITTEST, (WPARAM) 0, (LPARAM) &lvhti);
                            // Modify state of subitem(s) and selection only if something is changing
                            LVITEM lvi;
                            memset(&lvi, 0, sizeof(lvi));
                            lvi.mask = LVIF_STATE;
                            lvi.stateMask = 0xF;
                            if (((lvhti.flags & LVHT_ONITEMLABEL) != 0)
                                && ((col != lvhti.iSubItem) || (row != lvhti.iItem))) {
                                if (col != -1) { // Clear previous one before lightening new one
                                    lvi.iItem = row;
                                    lvi.iSubItem = col;
                                    lvi.state = 0;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                }
                                row = lvi.iItem = lvhti.iItem;
                                col = lvi.iSubItem = lvhti.iSubItem;
                                // Get text in the item
                                lvi.mask = LVIF_TEXT;
                                lvi.pszText = dummy; // Cannot set this to NULL
                                lvi.cchTextMax = 2;  // to get only text length.
                                                     // And still, we do not get the real length,
                                                     // but the minimum of 1 and real length.
                                textLen = SendMessage(hWnd, LVM_GETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
                                if (pnmh->code == NM_DBLCLK) // Jump to edit mode
                                    goto subitem_edit;

                                if (textLen) { // Cannot select an empty subitem
                                    lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                                    lvi.mask = LVIF_STATE;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                    // Select corresponding item to show selection mark
                                    lvi.iSubItem = 0;
                                    lvi.state = LVIS_SELECTED;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                    i = SendMessage(hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
                                } else {
                                    col = row = -1;
                                }
                            } else { // Clicking not on label = clear selection,
                                     // or on same subitem a second time = edit
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                if ((lvhti.flags & LVHT_ONITEMLABEL) != 0) {
                                    // Second time click, edit subitem contents
                                    // First get text to edit
                                    lvi.mask = LVIF_TEXT;
                                    lvi.pszText = dummy; // Cannot set this to NULL
                                    lvi.cchTextMax = 2;  // to get only text length.
                                                         // And still, we do not get the real length,
                                                         // but the minimum of 1 and real length.
                                    textLen = SendMessage(hWnd, LVM_GETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
                                subitem_edit:
                                    if (textLen) { // Can only edit a non empty subitem
                                        // Show the popup window to get new value
                                        Complex cplx;
                                        lstedit_get_item(col, &cplx, row);
                                        t_getcplx_param param;
                                        param.title = libLang->translate(_T("$$LIST ITEM"));
                                        TCHAR text[32];
                                        _stprintf(text, _T("[%d] ="), row+1);
                                        param.label = text;
                                        param.value = &cplx;
                                        if (DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), hDlg,
                                                            WndGetComplex,
                                                            (LPARAM) &param
                                                           )
                                           ) {
                                            CError err;
                                            err = lstedit_replace_item(col, &cplxPopup_value, row);
                                            if (err) {
                                                alertErrorMessage(err);
                                            } else { // Accept modification
                                                TCHAR *tmptext = display_complex(cplxPopup_value);
                                                lvi.mask = LVIF_TEXT;
                                                lvi.pszText = tmptext;
                                                i = SendMessage(hWnd, LVM_SETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
                                                MemPtrFree(tmptext);
                                            }
                                        }
                                        lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                                        lvi.mask = LVIF_STATE;
                                        i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                        // Select corresponding item to show selection mark
                                        lvi.iSubItem = 0;
                                        lvi.state = LVIS_SELECTED;
                                        i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                        i = SendMessage(hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
//                                        SetFocus(hWnd); // Back to list

//                                        // Retrieve real item length
//                                        textLen = lstedit_item_length(col, row);
//                                        // Show the edit box for editing subitem contents
//                                        text = (TCHAR *) MemPtrNew((++textLen)*sizeof(TCHAR));
//                                        lvi.pszText = text;
//                                        lvi.cchTextMax = textLen;
//                                        i = SendMessage(hWnd, LVM_GETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
//                                        // Get position of subitem
//                                        RECT rc;
//                                        rc.top = col;
//                                        rc.left = LVIR_LABEL;
//                                        i = SendMessage(hWnd, LVM_GETSUBITEMRECT, (WPARAM) row, (LPARAM) &rc);
//                                        // Show the edit window on top of subitem
//                                        if (i) {
//                                            RECT rc2;
//                                            memset(&rc2, 0, sizeof(rc2));
//                                            // Retrieve offset to apply from List View start point
//                                            GetWindowRect(hDlg, &rc2);
//                                            hWnd_siEdit = GetDlgItem(hDlg, IDC_SUBITEMEDIT);
//                                            i = SendMessage(hWnd_siEdit, WM_SETTEXT, (WPARAM) 0, (LPARAM) text);
//                                            MoveWindow(hWnd_siEdit, rc2.left+rc.left,
//                                                                    rc2.top+rc.top-2,
//                                                                    rc.right-rc.left,
//                                                                    rc.bottom-rc.top+1,
//                                                                    TRUE);
//                                            wHide = false;
//                                            // Note: for this to work and show above the ListView,
//                                            // the control must be declared BEFORE the list view
//                                            // in EasyCalc.rc.
//                                            // I don't know why, I may be missing something, or go figure ...!
//                                            i = ShowWindow (hWnd_siEdit, SW_SHOWNORMAL);
//                                            // This is to ensure a KILLFOCUS msg when clicking elsewhere
//                                            SetFocus(hWnd_siEdit); // Not using WM_NEXTDLGCTL to not trap focus in a
//                                                                   // control disabled most of the time.
//                                        }
//                                        MemPtrFree(text);
                                    } else {
                                        col = row = -1;
                                    }
                                } else if (col != -1) { // Clear previous one
                                    col = row = -1;
                                    lvi.state = 0;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                }
                            }
                        } else { // Something has been clicked which is not a subitem
                            if (col != -1) { // Clear previous one if there was
                                LVITEM lvi;
                                lvi.mask = LVIF_STATE;
                                lvi.stateMask = 0xF;
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                col = -1;
                                row = -1;
                                lvi.state = 0;
                                i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                            }
                        }
                    }

                    // Enable Modify and Delete only if there is a selection
                    int enableBut = (col != -1);
                    hWnd = GetDlgItem(hDlg, IDC_APPEND);
                    EnableWindow (hWnd, enableBut);
                    hWnd = GetDlgItem(hDlg, IDC_INSERT);
                    EnableWindow (hWnd, enableBut);
                    hWnd = GetDlgItem(hDlg, IDC_DELETE);
                    EnableWindow (hWnd, enableBut);
                    processed = TRUE;
                } else if (pnmh->code == LVN_BEGINLABELEDIT) {
                    LV_DISPINFO *lvdi = (LV_DISPINFO *) lParam; // A bigger structure in fact ...
                    if (lvdi->item.iSubItem == 0)
                        res = TRUE; // Do not allow editing the item (0), only subitems. Note: true means no edit.
                    else
                        res = FALSE;
                    processed = TRUE;
//                } else if ((pnmh->code == LVN_ITEMCHANGING) || (pnmh->code == LVN_ITEMCHANGED)) {
//                    NMLISTVIEW *nmlv = (NMLISTVIEW *) lParam; // A bigger structure in fact ...
//                    res = FALSE; // Accept change
//                    processed = TRUE;
//                } else if (pnmh->code == NM_CUSTOMDRAW) {
//                    NMLVCUSTOMDRAW *nmlvcd = (NMLVCUSTOMDRAW *) lParam; // A bigger structure in fact ...
//                    res = CDRF_DODEFAULT;
//                    processed = TRUE;
//                } else if (pnmh->code == LVN_GETDISPINFO) {
//                    LV_DISPINFO *lvdi = (LV_DISPINFO *) lParam; // A bigger structure in fact ...
//                    res = TRUE;
//                    processed = TRUE;
                } else if (pnmh->code == NM_RECOGNIZEGESTURE) {
//                    NMRGINFO *nmrgi = (NMRGINFO *) lParam; // A bigger structure in fact ...
                    // This will ensure that double click will be recognized.
                    // Not all notification messages are processed, and it seems
                    // that answering TRUE to NM_RECOGNIZEGESTURE instead of FALSE
                    // allows to get the double click notification.
                    res = processed = TRUE;
                }
            }
            }
            break;

//        case WM_APP_EXEBUTTON:
//            // Coming from edit box, text has been validated
//            {
//            // Update subitem and hide the edit box, if not already done
//            HWND hWnd = GetDlgItem(hDlg, IDC_EDITLIST);
//            LVITEM lvi;
//            memset(&lvi, 0, sizeof(lvi));
//            lvi.iItem = row;
//            lvi.iSubItem = col;
//            // Update subitem contents, if it has changed
//            BOOL textUpdated = SendMessage(hWnd_siEdit, EM_GETMODIFY, (WPARAM) 0, (LPARAM) 0);
//            if (textUpdated) {
//                int textLen = SendMessage(hWnd_siEdit, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
//                if (textLen) { // Do not allow empty text .. have to use the Delete button
//                    // Retrieve input
//                    TCHAR *text = (TCHAR *) MemPtrNew((++textLen)*sizeof(TCHAR));
//                    int i = SendMessage(hWnd_siEdit, WM_GETTEXT, (WPARAM) textLen, (LPARAM) text);
//                    CodeStack *stack;
//                    CError err;
//
//                    stack = text_to_stack(text, &err);
//                    if (!err) {
//                        err = stack_compute(stack);
//                        if (!err)
//                            err = stack_get_val(stack, &cplxPopup_value, complex);
//                        stack_delete(stack);
//                    }
//                    if (!err)
//                        err = lstedit_replace_item(col, &cplxPopup_value, row);
//                    if (err) {
//                        alertErrorMessage(err);
//                    } else { // Accept modification and close edit box
//                        MemPtrFree(text);
//                        text = display_complex(cplxPopup_value);
//                        lvi.mask = LVIF_TEXT;
//                        lvi.pszText = text;
//                        i = SendMessage(hWnd, LVM_SETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
//                        ShowWindow (hWnd_siEdit, SW_HIDE); // This will send a KILLFOCUS msg
//                        SetFocus(hWnd); // Back to list
//                    }
//                    MemPtrFree(text);
//                }
//            } else { // Nothing changed
//                ShowWindow (hWnd_siEdit, SW_HIDE); // This will send a KILLFOCUS msg
//                SetFocus(hWnd); // Back to list
//            }
//            }
//            processed = TRUE;
//            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            int listNb;
            HWND hWnd = (HWND) lParam;
            switch (notify_src) {
                case IDOK:
                    if (notify_msg == BN_CLICKED) {
                        lstedit_destroy();
                        EndDialog(hDlg, 1);
                        processed = TRUE;
                    }
                    break;

//                case IDC_SUBITEMEDIT: // Message from the subitem edit box
//                    switch (notify_msg) {
//                        case EN_KILLFOCUS:
//                            // Hide the edit box, if not already done
//                            if (!wHide) {
//                                hWnd_siEdit = NULL; // Forget about this handle
//                                wHide = TRUE;
//                                ShowWindow (hWnd, SW_HIDE); // This will re-send a KILLFOCUS msg
//                            }
//                            LVITEM lvi;
//                            memset(&lvi, 0, sizeof(lvi));
//                            lvi.iItem = row;
//                            lvi.iSubItem = col;
//                            lvi.mask = LVIF_STATE ;
//                            lvi.stateMask = 0xF;
//                            lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
//                            hWnd = GetDlgItem(hDlg, IDC_EDITLIST);
//                            int i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
//                            // Select corresponding item to show selection mark
//                            lvi.iSubItem = 0;
//                            lvi.state = LVIS_SELECTED;
//                            i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
//                            i = SendMessage(hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
//                            processed = TRUE;
//                            break;
//                    }
//                    break;

                case IDC_COMBO_LIST1: // Message from a column header combo
                    listNb = LIST1_ID;
                    goto process_combo_msg;
                case IDC_COMBO_LIST2:
                    listNb = LIST2_ID;
                    goto process_combo_msg;
                case IDC_COMBO_LIST3:
                    listNb = LIST3_ID;
                process_combo_msg:
                    switch (notify_msg) {
                        case CBN_DROPDOWN:
                            // Lose subitem selection if any
                            if (col != -1) {
                                LVITEM lvi;
                                memset(&lvi, 0, sizeof(lvi));
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                lvi.mask = LVIF_STATE ;
                                lvi.stateMask = 0xF;
                                lvi.state = 0;
                                HWND hWndLV = GetDlgItem(hDlg, IDC_EDITLIST);
                                int i = SendMessage(hWndLV, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                col = row = -1;
                                hWnd = GetDlgItem(hDlg, IDC_APPEND);
                                EnableWindow(hWnd, FALSE);
                                hWnd = GetDlgItem(hDlg, IDC_INSERT);
                                EnableWindow(hWnd, FALSE);
                                hWnd = GetDlgItem(hDlg, IDC_DELETE);
                                EnableWindow(hWnd, FALSE);
                            }
                            // Set text in the drop down
                            if (!listc_ok[listNb-LIST1_ID]) {
                                listc_ok[listNb-LIST1_ID] = true;
                                lstedit_reselect(listNb);
                            }
                            processed = TRUE;
                            break;
                        case CBN_CLOSEUP:
                            {
                            CError err = c_interrupted;
                            TCHAR *lname = state->listPrefs.list[listNb-LIST1_ID];
                            int cur_sel = (int) SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            if (cur_sel != CB_ERR) { // Action for a list.
                                if ((cur_sel == saveCbChoice) && _tcslen(lname)) {
                                    /* Save as */
                                    t_saveAsVar_param param;
                                    param.title = libLang->translate(_T("$$SAVE LST AS TITLE"));
                                    param.saveAns = false; // Just verify we can save with
                                                           // this variable, do not do the
                                                           // save itself, done below.
                                    param.verify  = true;
                                    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VARENTRY), hDlg,
                                                       WndSaveAsVar, (LPARAM) &param
                                                      )
                                       ) {
                                        _tcscpy(lname, varPopup_name);
                                        MemPtrFree(varPopup_name);
                                        err = lstedit_save_lists();
                                        if (err) {
                                            alertErrorMessage(err);
                                        } else {
                                            listc_ok[0] = listc_ok[1] = listc_ok[2] = false;
                                        }
                                    }
                                } else if ((cur_sel == deleteCbChoice) && _tcslen(lname)) {
                                    /* Delete */
                                    if (!FrmCustomAlert(altConfirmDelete,
                                                        libLang->translate(_T("$$LIST")),
                                                        lname, NULL, NULL)) {
                                        db_delete_record(lname);
                                        lname[0] = _T('\0');
                                        lstedit_tbl_deselect();
                                        listc_ok[0] = listc_ok[1] = listc_ok[2] = false;
                                        err = c_noerror;
                                    }
                                } else if (cur_sel == newCbChoice) {
                                    /* New */
                                    Complex cplx;
                                    cplx.real = cplx.imag = 0.0;
                                    t_getcplx_param param_cplx;
                                    param_cplx.title = libLang->translate(_T("$$FIRST LIST ITEM"));
                                    param_cplx.label = _T("[1] =");
                                    param_cplx.value = &cplx;
                                    t_saveAsVar_param param_name;
                                    param_name.title = libLang->translate(_T("$$SAVE LST AS TITLE"));
                                    param_name.saveAns = false; // Just verify we can save with
                                                                // this variable, do not do the
                                                                // save itself, done below.
                                    param_name.verify  = true;
                                    if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VARENTRY), hDlg,
                                                       WndSaveAsVar, (LPARAM) &param_name
                                                      )
                                        && DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), hDlg,
                                                          WndGetComplex,
                                                          (LPARAM) &param_cplx
                                                         )
                                       ) {
                                        _tcscpy(lname, varPopup_name);
                                        MemPtrFree(varPopup_name);
                                        lstedit_init_firstitem(listNb, &cplxPopup_value);
                                        err = lstedit_save_lists();
                                        if (err) {
                                            alertErrorMessage(err);
                                        } else {
                                            listc_ok[0] = listc_ok[1] = listc_ok[2] = false;
                                        }
                                    }
                                } else {
                                    SendMessage(hWnd, CB_GETLBTEXT, (WPARAM) cur_sel, (LPARAM) lname);
                                    lstedit_tbl_deselect();
                                    err = c_noerror;
                                }
                            }
                            if (!err) { // Some list was modified
                                // Update header combo box of the column we are in
                                listc_ok[listNb-LIST1_ID] = true;
                                lstedit_reselect(listNb);

                                lstedit_load_lists();
                                lstedit_tbl_scroll(0);

                                if (cur_sel == newCbChoice) { // Continue appending values until user presses Cancel
                                    append = true;
                                    col = listNb;
                                    row = 0;
                                    goto add_values2;
                                }
                            } else { // Just re-establish the column header combo box value
                                LstEditSetLabel(listNb, lname);
                            }
                            }
                            processed = TRUE;
                            break;
                    }
                    break;

                case IDC_APPEND: // Message from the Append button
                    append = true;
                    goto add_values;
                case IDC_INSERT: // Insert button
                    append = false;
                add_values:
                    if ((notify_msg == BN_CLICKED) && (col != -1)) { // Just to make sure ..
                add_values2:
                        // Show a box for adding value
                        Complex cplx;
                        cplx.real = cplx.imag = 0.0;
                        t_getcplx_param param;
                        param.title = libLang->translate(_T("$$LIST ITEM"));
                        TCHAR text[32];
                        _stprintf(text, _T("[%d] ="), (append
                                                       ? lstedit_list_length(col)+1
                                                       : row+1
                                                      )
                                 );
                        param.label = text;
                        param.value = &cplx;
                        while (DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), hDlg,
                                                        WndGetComplex,
                                                        (LPARAM) &param
                                           )
                           ) {
                            CError err;
                            if (append)
                                err = lstedit_append_item(col, &cplxPopup_value);
                            else
                                err = lstedit_insert_item(col, &cplxPopup_value, row);
                            if (err) {
                                alertErrorMessage(err);
                                break;
                            } else { // All is ok, if insert, go to next item,
                                     // else just take care of lighting things correctly
                                LVITEM lvi;
                                lvi.mask = LVIF_STATE;
                                lvi.stateMask = 0xF;
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                lvi.state = 0;
                                HWND hWndLV = GetDlgItem(hDlg, IDC_EDITLIST);
                                int i = SendMessage(hWndLV, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                if (!append)
                                    lvi.iItem = ++row;

                                lstedit_tbl_scroll(0);

                                lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                                i = SendMessage(hWndLV, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                // Select corresponding item to show selection mark
                                lvi.iSubItem = 0;
                                lvi.state = LVIS_SELECTED;
                                i = SendMessage(hWndLV, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                i = SendMessage(hWndLV, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
                            }
                            // Continue inserting / appending .. until user presses cancel
                            // or there is an error ("break;" above).
                            _stprintf(text, _T("[%d] ="), (append
                                                           ? lstedit_list_length(col)+1
                                                           : row+1
                                                          )
                                     );
                        }
                        processed = TRUE;
                    }
                    break;

                case IDC_DELETE:
                    if ((notify_msg == BN_CLICKED) && (col != -1)) { // Just to make sure ..
                        CError err = lstedit_delete_item(col, row);
                        if (err) {
                            alertErrorMessage(err);
                        } else {
                            int llen = lstedit_list_length(col);
                            if (row >= llen)
                                row = llen-1;

                            LVITEM lvi;
                            lvi.mask = LVIF_STATE;
                            lvi.stateMask = 0xF;
                            lvi.iItem = row;
                            lvi.iSubItem = col;
                            lvi.state = 0;
                            HWND hWndLV = GetDlgItem(hDlg, IDC_EDITLIST);
                            int i = SendMessage(hWndLV, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);

                            lstedit_tbl_scroll(0);

                            lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                            i = SendMessage(hWndLV, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                            // Select corresponding item to show selection mark
                            lvi.iSubItem = 0;
                            lvi.state = LVIS_SELECTED;
                            i = SendMessage(hWndLV, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                            i = SendMessage(hWndLV, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
                        }
                        processed = TRUE;
                    }
                    break;
            }
            }
            break;

        case WM_CLOSE:
            lstedit_destroy();
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  Functions for handling display of a matrix in the MatrixEditor window.      *
 ********************************************************************************/
// Reset list view contents
void MtxEditResetView(int nb_cols) {
    HWND hWnd = GetDlgItem(hDlg_MatrixEditor, IDC_EDITMATRIX);

    // Clear all rows
    SendMessage(hWnd, LVM_DELETEALLITEMS, (WPARAM) 0, (LPARAM) 0);

    // Remove all columns
    for (int i=nb_cols ; i>=0 ; i--) {
        SendMessage(hWnd, LVM_DELETECOLUMN, (WPARAM) i, (LPARAM) 0);
    }
}

// Prepare list view with correct number of rows and columns
void MtxEditSetView(int nb_cols, int  nb_rows) {
    HWND hWnd = GetDlgItem(hDlg_MatrixEditor, IDC_EDITMATRIX);

    // Setup columns and headers
    TCHAR text[10];
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 24 * cur_skin->magnification;
    lvc.pszText = _T("#");
    SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 0, (LPARAM) (&lvc));
    lvc.fmt = LVCFMT_RIGHT;
    lvc.cx = 50 * cur_skin->magnification;
    lvc.pszText = text;
    for (int i=1 ; i<=nb_cols ; i++) {
        _stprintf(text, _T("%d"), i);
        SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) i, (LPARAM) (&lvc));
    }

    // Prepare number of rows
    SendMessage(hWnd, LVM_SETITEMCOUNT, (WPARAM) nb_rows, (LPARAM) 0);
}

// Set row label in the column item
void MtxEditSetRowLabel (int rowNb, int value) {
    HWND hWnd = GetDlgItem(hDlg_MatrixEditor, IDC_EDITMATRIX);

    TCHAR text[10];
    _stprintf(text, _T("%d"), value);

    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = rowNb;
    lvi.iSubItem = 0;
    lvi.pszText = text;
    int i = SendMessage((HWND) hWnd, LVM_INSERTITEM, (WPARAM) 0, (LPARAM) &lvi);
}

// Set cell value.
void MtxEditSetRowValue (int rowNb, int colNb, TCHAR *cell) {
    HWND hWnd = GetDlgItem(hDlg_MatrixEditor, IDC_EDITMATRIX);

    if (cell != NULL) {
        LVITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = rowNb;
        lvi.iSubItem = colNb;
        lvi.pszText = cell;
        // Note: there is no way to retrieve the length of text
        // stored in a subitem in Windows ..! So we will need to
        // use external means.
        int i = SendMessage((HWND) hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
    }
}

/********************************************************************************
 *  FUNCTION: WndMatrixEditor (HWND, UINT, WPARAM, LPARAM)                      *
 *  Message handler for the Matrix Editor dialog.                               *
 *  lParam tells to init the first list with ans or not.                        *
 *  Ex MatrixHandleEvent + mtxedit_select in mtxedit.c                          *
 ********************************************************************************/
BOOL CALLBACK WndMatrixEditor (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
    static bool matrixc_ok;
    static int col, row, nb_cols, nb_rows;

    hDlg_MatrixEditor = hDlg; // Share it with other routines
    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$MATRIX EDITOR")));
            HWND hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDC_COMBO_NBROWS);
            HWND hWnd2 = GetDlgItem(hDlg, IDC_COMBO_NBCOLS);
            TCHAR text[10];
            for (int i=0 ; i<MATRIX_MAX ; i++) {
                _stprintf(text, _T("%d"), i+1);
                SendMessage(hWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) text);
                SendMessage(hWnd2, CB_ADDSTRING, (WPARAM) 0, (LPARAM) text);
            }
            // Initialize matrix being edited, and dimension combos
            mtxedit_init(((int) lParam == TRUE));
            mtxedit_load_matrix(&nb_rows, &nb_cols);
            SendMessage(hWnd, CB_SETCURSEL, (WPARAM) nb_rows-1, (LPARAM) 0);
            SendMessage(hWnd2, CB_SETCURSEL, (WPARAM) nb_cols-1, (LPARAM) 0);
            // Prepare list view with number of columns and rows
            // and load it. This can be long ...
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            MtxEditSetView(nb_cols, nb_rows);
            mtxedit_tbl_load();
            SetCursor(NULL);
            // Tell ourselves that first combo box contents is ok
            matrixc_ok = true;
            // Nothing selected yet
            col = row = -1;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR *) lParam;
            int i;
            if (pnmh->idFrom == IDC_EDITMATRIX) {
                if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_DBLCLK)
                    || (pnmh->code == NM_SETFOCUS) || (pnmh->code == NM_KILLFOCUS)) {
                    HWND hWnd = pnmh->hwndFrom;
                    if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_DBLCLK)) {
                        NMLISTVIEW *nmlv = (NMLISTVIEW *) lParam; // A bigger structure in fact ...
                        if (nmlv->iSubItem != 0) { // Click on a subitem, but we have -1 in iItem,
                                                   // so have to retrieve it.
                            TCHAR dummy[2]; // To get subitem text. Need to initialize it to give a
                                            // non-NULL value to LVM_GETITEMTEXT, or we get a system error.
                            int textLen = 0;
                             // Get the corresponding item = row number
                            LVHITTESTINFO lvhti;
                            lvhti.pt = nmlv->ptAction;
                            i = SendMessage(hWnd, LVM_SUBITEMHITTEST, (WPARAM) 0, (LPARAM) &lvhti);
                            // Modify state of subitem(s) and selection only if something is changing
                            LVITEM lvi;
                            memset(&lvi, 0, sizeof(lvi));
                            lvi.mask = LVIF_STATE;
                            lvi.stateMask = 0xF;
                            if (((lvhti.flags & LVHT_ONITEMLABEL) != 0)
                                && ((col != lvhti.iSubItem) || (row != lvhti.iItem))) {
                                if (col != -1) { // Clear previous one before lightening new one
                                    lvi.iItem = row;
                                    lvi.iSubItem = col;
                                    lvi.state = 0;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                }
                                row = lvi.iItem = lvhti.iItem;
                                col = lvi.iSubItem = lvhti.iSubItem;
                                // Get text in the item
                                lvi.mask = LVIF_TEXT;
                                lvi.pszText = dummy; // Cannot set this to NULL
                                lvi.cchTextMax = 2;  // to get only text length.
                                                     // And still, we do not get the real length,
                                                     // but the minimum of 1 and real length.
                                textLen = SendMessage(hWnd, LVM_GETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
                                if (pnmh->code == NM_DBLCLK) // Jump to edit mode
                                    goto subitem_edit;

                                if (textLen) { // Cannot select an empty subitem
                                    lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                                    lvi.mask = LVIF_STATE;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                    // Select corresponding item to show selection mark
                                    lvi.iSubItem = 0;
                                    lvi.state = LVIS_SELECTED;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                    i = SendMessage(hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
                                } else {
                                    col = row = -1;
                                }
                            } else { // Clicking not on label = clear selection,
                                     // or on same subitem a second time = edit
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                if ((lvhti.flags & LVHT_ONITEMLABEL) != 0) {
                                    // Second time click, edit subitem contents
                                    // First get text to edit
                                    lvi.mask = LVIF_TEXT;
                                    lvi.pszText = dummy; // Cannot set this to NULL
                                    lvi.cchTextMax = 2;  // to get only text length.
                                                         // And still, we do not get the real length,
                                                         // but the minimum of 1 and real length.
                                    textLen = SendMessage(hWnd, LVM_GETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
                                subitem_edit:
                                    if (textLen) { // Can only edit a non empty subitem
                                        // Show the popup window to get new value
                                        Complex cplx;
                                        mtxedit_get_item(&cplx, row, col-1);
                                        t_getcplx_param param;
                                        param.title = libLang->translate(_T("$$MATRIX ITEM"));
                                        TCHAR text[32];
                                        _stprintf(text, _T("[%d:%d] ="), row+1, col);
                                        param.label = text;
                                        param.value = &cplx;
                                        if (DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), hDlg,
                                                            WndGetComplex,
                                                            (LPARAM) &param
                                                           )
                                           ) {
                                            CError err;
                                            err = mtxedit_replace_item(&cplxPopup_value, row, col-1);
                                            if (err) {
                                                alertErrorMessage(err);
                                            } else { // Accept modification
                                                TCHAR *tmptext = display_complex(cplxPopup_value);
                                                lvi.mask = LVIF_TEXT;
                                                lvi.pszText = tmptext;
                                                i = SendMessage(hWnd, LVM_SETITEMTEXT, (WPARAM) row, (LPARAM) &lvi);
                                                MemPtrFree(tmptext);
                                            }
                                        }
                                        lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                                        lvi.mask = LVIF_STATE;
                                        i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                        // Select corresponding item to show selection mark
                                        lvi.iSubItem = 0;
                                        lvi.state = LVIS_SELECTED;
                                        i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                        i = SendMessage(hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
                                    } else {
                                        col = row = -1;
                                    }
                                } else if (col != -1) { // Clear previous one
                                    col = row = -1;
                                    lvi.state = 0;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                }
                            }
                        } else { // Something has been clicked which is not a subitem
                            if (col != -1) { // Clear previous one if there was
                                LVITEM lvi;
                                lvi.mask = LVIF_STATE;
                                lvi.stateMask = 0xF;
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                col = -1;
                                row = -1;
                                lvi.state = 0;
                                i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                            }
                        }
                    }
                    processed = TRUE;
                } else if (pnmh->code == LVN_BEGINLABELEDIT) {
                    LV_DISPINFO *lvdi = (LV_DISPINFO *) lParam; // A bigger structure in fact ...
                    if (lvdi->item.iSubItem == 0)
                        res = TRUE; // Do not allow editing the item (0), only subitems. Note: true means no edit.
                    else
                        res = FALSE;
                    processed = TRUE;
//                } else if ((pnmh->code == LVN_ITEMCHANGING) || (pnmh->code == LVN_ITEMCHANGED)) {
//                    NMLISTVIEW *nmlv = (NMLISTVIEW *) lParam; // A bigger structure in fact ...
//                    res = FALSE; // Accept change
//                    processed = TRUE;
//                } else if (pnmh->code == NM_CUSTOMDRAW) {
//                    NMLVCUSTOMDRAW *nmlvcd = (NMLVCUSTOMDRAW *) lParam; // A bigger structure in fact ...
//                    res = CDRF_DODEFAULT;
//                    processed = TRUE;
//                } else if (pnmh->code == LVN_GETDISPINFO) {
//                    LV_DISPINFO *lvdi = (LV_DISPINFO *) lParam; // A bigger structure in fact ...
//                    res = TRUE;
//                    processed = TRUE;
                } else if (pnmh->code == NM_RECOGNIZEGESTURE) {
//                    NMRGINFO *nmrgi = (NMRGINFO *) lParam; // A bigger structure in fact ...
                    // This will ensure that double click will be recognized.
                    // Not all notification messages are processed, and it seems
                    // that answering TRUE to NM_RECOGNIZEGESTURE instead of FALSE
                    // allows to get the double click notification.
                    res = processed = TRUE;
                }
            }
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            switch (notify_src) {
                case IDOK:
                    if (notify_msg == BN_CLICKED) {
                        mtxedit_save_matrix();
                        mtxedit_destroy();
                        EndDialog(hDlg, 1);
                        processed = TRUE;
                    }
                    break;

                case IDC_COMBO_NBROWS: // Changing number of rows ?
                case IDC_COMBO_NBCOLS: // Changing number of cols ?
                    switch (notify_msg) {
                        case CBN_CLOSEUP:
                            int cur_sel = (int) SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                            if (cur_sel != CB_ERR) { // Action ... ?
                                cur_sel++;
                                if (((notify_src == IDC_COMBO_NBROWS) && (cur_sel != nb_rows))
                                    || ((notify_src == IDC_COMBO_NBCOLS) && (cur_sel != nb_cols))) {
                                    MtxEditResetView(nb_cols);
                                    if (notify_src == IDC_COMBO_NBROWS)
                                        mtxedit_redim(nb_rows=cur_sel, nb_cols);
                                    else
                                        mtxedit_redim(nb_rows, nb_cols=cur_sel);
                                    HWND hWnd = GetDlgItem(hDlg, IDC_COMBO_NBROWS);
                                    HWND hWnd2 = GetDlgItem(hDlg, IDC_COMBO_NBCOLS);
                                    SendMessage(hWnd, CB_SETCURSEL, (WPARAM) nb_rows-1, (LPARAM) 0);
                                    SendMessage(hWnd2, CB_SETCURSEL, (WPARAM) nb_cols-1, (LPARAM) 0);
                                    // Prepare list view with number of columns and rows
                                    // and load it.
                                    MtxEditSetView(nb_cols, nb_rows);
                                    mtxedit_tbl_load();
                                    // Nothing selected yet
                                    col = row = -1;
                                }
                            }
                    }
                    break;

                case IDC_COMBO_MATRIX: // Message from matrix combo
                    {
                    static bool sel_changed;
                    switch (notify_msg) {
                        case CBN_DROPDOWN:
                            // Set text in the drop down
                            sel_changed = false;
                            if (!matrixc_ok) {
                                matrixc_ok = true;
                                mtxedit_select();
                            }
                            processed = TRUE;
                            break;
                        case CBN_SELENDOK:
                            sel_changed = true;
                            processed = TRUE;
                            break;
                        case CBN_CLOSEUP:
                            if (sel_changed) {
                                CError err = c_interrupted;
                                int cur_sel = (int) SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                                if (cur_sel != CB_ERR) { // Action for a list.
                                    if ((cur_sel == saveCbChoice) && _tcslen(mtxedit_matrixName)) {
                                        /* Save as */
                                        t_saveAsVar_param param;
                                        param.title = libLang->translate(_T("$$SAVE MTX AS TITLE"));
                                        param.saveAns = false; // Just verify we can save with
                                                               // this variable, do not do the
                                                               // save itself, done below.
                                        param.verify  = true;
                                        if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VARENTRY), hDlg,
                                                           WndSaveAsVar, (LPARAM) &param
                                                          )
                                            ) {
                                            mtxedit_save_matrix();
                                            _tcscpy(mtxedit_matrixName, varPopup_name);
                                            MemPtrFree(varPopup_name);
                                            mtxedit_save_matrix();
                                            err = c_noerror;
                                        }
                                    } else if ((cur_sel == deleteCbChoice) && _tcslen(mtxedit_matrixName)) {
                                        /* Delete */
                                        if (!FrmCustomAlert(altConfirmDelete,
                                                            libLang->translate(_T("$$MATRIX")),
                                                            mtxedit_matrixName, NULL, NULL)) {
                                            db_delete_record(mtxedit_matrixName);
                                            mtxedit_matrixName[0] = _T('\0');
                                            err = c_noerror;
                                        }
                                    } else if (cur_sel == newCbChoice) {
                                        /* New */
                                        t_saveAsVar_param param_name;
                                        param_name.title = libLang->translate(_T("$$SAVE MTX AS TITLE"));
                                        param_name.saveAns = false; // Just verify we can save with
                                                                    // this variable, do not do the
                                                                    // save itself, done below.
                                        param_name.verify  = true;
                                        if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VARENTRY), hDlg,
                                                           WndSaveAsVar, (LPARAM) &param_name
                                                          )
                                           ) {
                                            mtxedit_save_matrix();
                                            _tcscpy(mtxedit_matrixName, varPopup_name);
                                            MemPtrFree(varPopup_name);
                                            mtxedit_new(1,1);
                                            mtxedit_save_matrix();
                                            err = c_noerror;
                                        }
                                    } else {
                                        mtxedit_save_matrix();
                                        SendMessage(hWnd, CB_GETLBTEXT, (WPARAM) cur_sel,
                                                    (LPARAM) mtxedit_matrixName);
                                        err = c_noerror;
                                    }
                                }
                                if (!err) { // Some matrix was modified
                                    // Update header combo box of the column we are in
                                    matrixc_ok = true;
                                    mtxedit_select();
                                    MtxEditResetView(nb_cols);
                                    mtxedit_load_matrix(&nb_rows, &nb_cols);
                                    HWND hWnd = GetDlgItem(hDlg, IDC_COMBO_NBROWS);
                                    HWND hWnd2 = GetDlgItem(hDlg, IDC_COMBO_NBCOLS);
                                    SendMessage(hWnd, CB_SETCURSEL, (WPARAM) nb_rows-1, (LPARAM) 0);
                                    SendMessage(hWnd2, CB_SETCURSEL, (WPARAM) nb_cols-1, (LPARAM) 0);
                                    // Prepare list view with number of columns and rows
                                    // and load it. This can be long
                                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                                    MtxEditSetView(nb_cols, nb_rows);
                                    mtxedit_tbl_load();
                                    SetCursor(NULL);
                                    // Nothing selected yet
                                    col = row = -1;
                                } else { // Just re-establish the column header combo box value
                                    LstEditSetLabel(MATRIX_ID, mtxedit_matrixName);
                                }
                            }
                            processed = TRUE;
                            break;
                    }
                    }
                    break;
            }
            }
            break;

        case WM_CLOSE:
            // Do not save current matrix, this means cancel ..
            mtxedit_destroy();
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  Functions for handling display of elements in the Solver window.            *
 ********************************************************************************/
// Disable controls
void SlvEnableAll (Boolean enable) {
    HWND hWnd = GetDlgItem(hDlg_Solver, IDC_CONFIG);
    EnableWindow(hWnd, enable);
    hWnd = GetDlgItem(hDlg_Solver, IDC_DELETE);
    EnableWindow(hWnd, enable);
    hWnd = GetDlgItem(hDlg_Solver, IDC_EDIT2);
    EnableWindow(hWnd, enable);
    if (enable) {
        // SetFocus doesn't work well in a Dialog, because of default buttons & co.
        // Have to use WM_NEXTDLGCTL instead.
        // By the way, this also selects any text already present in the Edit Box
        PostMessage(hDlg_Solver, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
    } else {
        // Make sure the equation field is empty
        SetWindowText(hWnd, _T(""));
    }
    hWnd = GetDlgItem(hDlg_Solver, IDC_VAR);
    EnableWindow(hWnd, enable);
    hWnd = GetDlgItem(hDlg_Solver, IDC_USERF);
    EnableWindow(hWnd, enable);
    hWnd = GetDlgItem(hDlg_Solver, IDC_CALCF);
    EnableWindow(hWnd, enable);
    hWnd = GetDlgItem(hDlg_Solver, IDC_UPDATE);
    EnableWindow(hWnd, enable);
    hWnd = GetDlgItem(hDlg_Solver, IDC_NOTES);
    EnableWindow(hWnd, enable);
    hWnd = GetDlgItem(hDlg_Solver, IDC_VARLIST);
    EnableWindow(hWnd, enable);
}

// Enable or disable the solve/calculate button, with appropriate text
static int exeButton = 0;
void SlvSetExeButton (int cmd) {
    HWND hWnd = GetDlgItem(hDlg_Solver, IDC_SOLVE);
    EnableWindow(hWnd, (cmd != SLV_OFF));
    if (cmd == SLV_SOLVE) {
        exeButton = cmd; // Remember button value
        TCHAR *text = (TCHAR *) libLang->translate(_T("$$SOLVE"));
        SetWindowText(hWnd, text);
    } else if (cmd == SLV_CALCULATE) {
        exeButton = cmd; // Remember button value
        TCHAR *text = (TCHAR *) libLang->translate(_T("$$CALCULATE"));
        SetWindowText(hWnd, text);
    }
}

// Set contents of the equation field
void SlvEqFldSet (TCHAR *eq) {
    HWND hWnd = GetDlgItem(hDlg_Solver, IDC_EDIT2);
    SetWindowText(hWnd, eq);
}

// Get contents of the equation field. Caller needs to free the returned value.
TCHAR *SlvEqFldGet (void) {
    HWND hWnd = GetDlgItem(hDlg_Solver, IDC_EDIT2);
    int siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
    TCHAR *text;
    text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
    SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
    return (text);
}

// Set label in first column item, and value in cell.
// If rowNb = -1, delete contents of the List View.
void SlvVarSetRow (int rowNb, TCHAR *label, TCHAR *value) {
    HWND hWnd = GetDlgItem(hDlg_Solver, IDC_VARLIST);

    if (rowNb == -1) {
        SendMessage((HWND) hWnd, LVM_DELETEALLITEMS, (WPARAM) 0, (LPARAM) 0);
    } else {
        LVITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = rowNb;
        lvi.iSubItem = 0;
        lvi.pszText = label;
        int i = SendMessage((HWND) hWnd, LVM_INSERTITEM, (WPARAM) 0, (LPARAM) &lvi);

        lvi.iSubItem = 1;
        if (value)
            lvi.pszText = value;
        else
            lvi.pszText = (TCHAR *) libLang->translate(_T("$$UNDEFINED"));
        SendMessage((HWND) hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
    }
}

// Display variable comment / help text.
void SlvVarHelp (TCHAR *text) {
    HWND hWnd = GetDlgItem(hDlg_Solver, IDC_VARCOMMENT);
    SetWindowText(hWnd, text);
}

/********************************************************************************
 *  FUNCTION: WndSolver (HWND, UINT, WPARAM, LPARAM)                            *
 *  Message handler for the Solver dialog.                                      *
 *  Ex SolverHandleEvent in solver.c                                            *
 ********************************************************************************/
BOOL CALLBACK WndSolver (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
    static bool wkshtc_ok;
    static int row;

    hDlg_Solver = hDlg; // Share it with other routines
    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$SOLVER")));
            HWND hWnd = GetDlgItem(hDlg, IDC_CONFIG);
            SetWindowText(hWnd, libLang->translate(_T("$$CONFIG")));
            hWnd = GetDlgItem(hDlg, IDC_DELETE);
            SetWindowText(hWnd, libLang->translate(_T("$$DELETE")));
            hWnd = GetDlgItem(hDlg, IDC_UPDATE);
            SetWindowText(hWnd, libLang->translate(_T("$$UPDATE")));
            hWnd = GetDlgItem(hDlg, IDC_NOTES);
            SetWindowText(hWnd, libLang->translate(_T("$$NOTE")));
            hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDC_MODIFY);
            SetWindowText(hWnd, libLang->translate(_T("$$MODIFY")));
            EnableWindow(hWnd, FALSE);
            hWnd = GetDlgItem(hDlg, IDC_SOLVE);
            SetWindowText(hWnd, libLang->translate(_T("$$SOLVE")));
            EnableWindow(hWnd, FALSE);
            // Set 2 columns in the list view
            hWnd = GetDlgItem(hDlg, IDC_VARLIST);
            LVCOLUMN lvc;
            lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 60 * cur_skin->magnification;
            lvc.pszText = (TCHAR *) libLang->translate(_T("$$NAME:"));
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 0, (LPARAM) (&lvc));
            lvc.cx = 170 * cur_skin->magnification;
            lvc.pszText = (TCHAR *) libLang->translate(_T("$$VALUE:"));
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 1, (LPARAM) (&lvc));

            slv_init();
            // Tell ourselves that first combo box contents is ok
            wkshtc_ok = true;
            // Nothing selected yet
            row = -1;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->idFrom == IDC_VARLIST) {
                if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_SETFOCUS)
                    || (pnmh->code == NM_KILLFOCUS)) {
                    // Enable Modify only if there is a selection
                    HWND hWnd = pnmh->hwndFrom;
                    int enableBut = ((row = SendMessage(hWnd, LVM_GETSELECTIONMARK, (WPARAM) 0, (LPARAM) 0))
                                     != -1);
                    slv_update_help(row);
                    hWnd = GetDlgItem(hDlg, IDC_MODIFY);
                    EnableWindow (hWnd, enableBut);
                    if (!enableBut)
                        SlvSetExeButton(SLV_OFF);
                    else
                        SlvSetExeButton(SLV_ON);
                    processed = TRUE;
                } else if (pnmh->code == NM_DBLCLK) {
                    // Retrieve selection
                    HWND hWnd = pnmh->hwndFrom;
                    row = SendMessage(hWnd, LVM_GETSELECTIONMARK, (WPARAM) 0, (LPARAM) 0);
                    slv_update_help(row);
                    if (row != -1) {
                        SlvSetExeButton(SLV_ON);
                        // Then call the action
                        goto modify_var;
                    } else {
                        SlvSetExeButton(SLV_OFF);
                    }
                    processed = TRUE;
                } else if (pnmh->code == NM_RECOGNIZEGESTURE) {
                    // This will ensure that double click will be recognized.
                    // Not all notification messages are processed, and it seems
                    // that answering TRUE to NM_RECOGNIZEGESTURE instead of FALSE
                    // allows to get the double click notification.
                    res = processed = TRUE;
                }
            }
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            switch (notify_src) {
                case IDOK:
                    if (notify_msg == BN_CLICKED) {
                        slv_close();
                        EndDialog(hDlg, 1);
                        processed = TRUE;
                    }
                    break;

                case IDC_COMBO_WKSHT: // Message from solver combo
                    {
                    static bool sel_changed;
                    switch (notify_msg) {
                        case CBN_DROPDOWN:
                            // Set text in the drop down
                            sel_changed = false;
                            if (!wkshtc_ok) {
                                wkshtc_ok = true;
                                slv_select_worksheet();
                            }
                            processed = TRUE;
                            break;
                        case CBN_SELENDOK:
                            sel_changed = true;
                            processed = TRUE;
                            break;
                        case CBN_CLOSEUP:
                            if (sel_changed) {
                                CError err = c_interrupted;
                                int cur_sel = (int) SendMessage(hWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                                if (cur_sel != CB_ERR) { // Action for a list.
                                    if (cur_sel == newCbChoice) {
                                        /* New */
                                        t_saveAsVar_param param_name;
                                        param_name.title = libLang->translate(_T("$$NEW WORKSHEET"));
                                        param_name.saveAns = false; // Don't verify we can save with
                                                                    // this variable, and do not do the
                                                                    // save itself, all done below.
                                        param_name.verify  = false;
                                        if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_VARENTRY), hDlg,
                                                           WndSaveAsVar, (LPARAM) &param_name
                                                          )
                                           ) {
                                            slv_save_worksheet();
                                            slv_selectedWorksheet = slv_new_worksheet(varPopup_name);
                                            MemPtrFree(varPopup_name);
                                            slv_select_worksheet();
                                            wkshtc_ok = true;
                                            err = c_noerror;
                                        }
                                    } else {
                                        slv_save_worksheet();
                                        slv_selectedWorksheet = cur_sel-1;
                                        err = c_noerror;
                                    }
                                }
                                if (!err) { // Some matrix was modified
                                    // Update rest of dialog.
                                    goto update_dlg;
                                } else { // Just re-establish the column header combo box value
                                    LstEditSetLabel(MATRIX_ID, mtxedit_matrixName);
                                }
                            }
                            processed = TRUE;
                            break;
                    }
                    }
                    break;

                case IDC_CONFIG:
                    if (notify_msg == BN_CLICKED) {
                        int rc = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SOLVER_CONFIG), hDlg, WndSolverOptions);
                        if (rc) {
                            slv_selectedWorksheet = slv_save_worksheet();
                            slv_select_worksheet(); // Name may have changed
                            wkshtc_ok = true;
                            goto update_dlg;
                        }
                        processed = TRUE;
                    }
                    break;

                case IDC_DELETE:
                    if (notify_msg == BN_CLICKED) {
                        if (FrmCustomAlert(altConfirmDelete,
                                           libLang->translate(_T("$$WORKSHEET")),
                                           worksheet_title,
                                           NULL,
                                           NULL)
                            == 0) {
                            DmRemoveRecord(slv_gDB, slv_selectedWorksheet);
                            slv_selectedWorksheet = -1;
                            wkshtc_ok = false; // Next time drop down is clicked, refresh it
                            goto update_dlg;
                        }
                        processed = TRUE;
                    }
                    break;

                case IDC_MODIFY:
                    if (notify_msg == BN_CLICKED) {
                    modify_var:
                        t_getcplx_param param;
                        param.title = NULL;
                        param.label = slv_getVar(row);
                        param.value = NULL;
                        int rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDITVAR), hDlg, WndModVMenu,
                                                (LPARAM) &param
                                               );
                        if (rc) {
                            goto update_vars;
                        }
                        processed = TRUE;
                    }
                    break;

                case IDC_SOLVE:
                    if (notify_msg == BN_CLICKED) {
                        if (exeButton == SLV_SOLVE) {
                            CError err = slv_solve(row);
                            if (!err)
                                goto update_vars;
                        } else if (exeButton == SLV_CALCULATE) {
                            slv_calculate();
                        }
                        processed = TRUE;
                    }
                    break;

                case IDC_UPDATE:
                    if (notify_msg == BN_CLICKED) {
                        goto update_vars;
                    update_dlg:
                        slv_update_worksheet();
                        row = -1; // No selected row
                    update_vars:
                        slv_destroy_varlist();
                        int nbvars = slv_init_varlist();
                        if (nbvars >= 0)
                            if (!StrLen(worksheet_note))
                                slv_create_initial_note();
                        if (nbvars < 1) { // No var
                            row = -1;
                        } else if (nbvars <= row) { // Number of vars has decreased
                            row = nbvars - 1;
                        }
                        slv_update_help(row);
                        HWND hWnd = GetDlgItem(hDlg, IDC_MODIFY);
                        if (row == -1) {
                            EnableWindow (hWnd, FALSE);
                            SlvSetExeButton(SLV_OFF);
                        } else {
                            EnableWindow (hWnd, TRUE);
                            SlvSetExeButton(SLV_ON);
                        }
                        processed = TRUE;
                    }
                    break;

                case IDC_NOTES:
                    if (notify_msg == BN_CLICKED) {
                        int rc = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_NOTES), hDlg, WndSolverNotes);
                        if (rc)
                            slv_update_help(row);
                        processed = TRUE;
                    }
                    break;

                case IDC_VAR:
                    if (notify_msg == BN_CLICKED) {
                        HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndVMenu,
                                                      (LPARAM) hDlg);
                        processed = TRUE;
                    }
                    break;

                case IDC_USERF:
                    if (notify_msg == BN_CLICKED) {
                        HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndFMenu,
                                                      (LPARAM) hDlg);
                        processed = TRUE;
                    }
                    break;

                case IDC_CALCF:
                    if (notify_msg == BN_CLICKED) {
                        HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndfMenu,
                                                      (LPARAM) hDlg);
                        processed = TRUE;
                    }
                    break;
            }
            }
            break;

        case WM_APP_ENDVMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SendMessage(hWnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) lParam);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            main_insert(cur_skin, hWnd, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            slv_close();
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndSolverOptions (HWND, UINT, WPARAM, LPARAM)                     *
 *  Message handler for the IDD_SOLVER_CONFIG dialog.                           *
 *  Ex slv_options in solver.c                                                  *
 ********************************************************************************/
BOOL CALLBACK WndSolverOptions (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$SOLVER CONFIG")));
            HWND hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            SetWindowText(hWnd, libLang->translate(_T("$$NAME:")));
            hWnd = GetDlgItem(hDlg, IDC_EDIT1);
            SetWindowText(hWnd, worksheet_title);
            hWnd = GetDlgItem(hDlg, IDC_MIN);
            SetWindowText(hWnd, libLang->translate(_T("$$MINIMUM:")));
            hWnd = GetDlgItem(hDlg, IDC_EDITMIN);
            TCHAR *text = display_real(worksheet_min);
            SetWindowText(hWnd, text);
            MemPtrFree(text);
            hWnd = GetDlgItem(hDlg, IDC_MAX);
            SetWindowText(hWnd, libLang->translate(_T("$$MAXIMUM:")));
            hWnd = GetDlgItem(hDlg, IDC_EDITMAX);
            text = display_real(worksheet_max);
            SetWindowText(hWnd, text);
            MemPtrFree(text);
            hWnd = GetDlgItem(hDlg, IDC_PREC);
            SetWindowText(hWnd, libLang->translate(_T("$$PRECISION:")));
            hWnd = GetDlgItem(hDlg, IDC_EDITPREC);
            text = display_real(worksheet_prec);
            SetWindowText(hWnd, text);
            MemPtrFree(text);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;

            hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            EnableWindow(hWnd, (_tcslen(worksheet_title) > 0));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDC_EDIT1) { // Message from the Edit Box
                switch (notify_msg) {
                    case EN_UPDATE:
                    case EN_CHANGE:
                        {
                        // Handle state of OK button
                        HWND hWnd = GetDlgItem(hDlg, IDC_EDIT1);
                        int siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        hWnd = GetDlgItem(hDlg, IDOK);
                        EnableWindow (hWnd, (siz > 0));
                        }
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDCANCEL) {
                unsigned int notify_msg = wParam >> 16;
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDOK) {
                unsigned int notify_msg = wParam >> 16;
                if (notify_msg == BN_CLICKED) {
                    // When input is made, retrieve it
                    processed = TRUE;

                    double min, max, prec;
                    HWND hWnd = GetDlgItem(hDlg, IDC_EDITMIN);
                    int siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                    TCHAR *text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                    Boolean err = slv_comp_field(text, &min);
                    MemPtrFree(text);
                    if (err)
                        break;
                    hWnd = GetDlgItem(hDlg, IDC_EDITMAX);
                    siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                    text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                    err = slv_comp_field(text, &max);
                    MemPtrFree(text);
                    if (err)
                        break;
                    hWnd = GetDlgItem(hDlg, IDC_EDITPREC);
                    siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                    text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                    err = slv_comp_field(text, &prec);
                    MemPtrFree(text);
                    if (err)
                        break;
                    if (min > max) {
                        FrmAlert(altGraphBadVal, NULL);
                        break;
                    }
                    worksheet_min = min;
                    worksheet_max = max;
                    worksheet_prec = prec;
                    hWnd = GetDlgItem(hDlg, IDC_EDIT1);
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) (MAX_WORKSHEET_TITLE+1), (LPARAM) worksheet_title);
                    EndDialog(hDlg, 1);
                }
            }
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndSolverNotes (HWND, UINT, WPARAM, LPARAM)                       *
 *  Message handler for the solver IDD_NOTES dialog.                            *
 *  Ex slv_note in solver.c                                                     *
 ********************************************************************************/
BOOL CALLBACK WndSolverNotes (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$NOTE")));
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SetWindowText(hWnd, worksheet_note);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            if (notify_src == IDCANCEL) {
                unsigned int notify_msg = wParam >> 16;
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDOK) {
                unsigned int notify_msg = wParam >> 16;
                if (notify_msg == BN_CLICKED) {
                    // When input is made, retrieve it
                    HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) (MAX_WORKSHEET_NOTE), (LPARAM) worksheet_note);
                    EndDialog(hDlg, 1);
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  Functions for handling display of lists in the Financial Calculator window. *
 ********************************************************************************/
static HWND hDlg_FinCalc;
// Set label in first column item, and value in cell.
// If rowNb = -1, delete contents of the List View.
void FinSetRow (int rowNb, TCHAR *label, TCHAR *value) {
    HWND hWnd = GetDlgItem(hDlg_FinCalc, IDC_VARLIST);

    if (rowNb == -1) {
        SendMessage((HWND) hWnd, LVM_DELETEALLITEMS, (WPARAM) 0, (LPARAM) 0);
    } else {
        LVITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = rowNb;
        lvi.iSubItem = 0;
        lvi.pszText = label;
        int i = SendMessage((HWND) hWnd, LVM_INSERTITEM, (WPARAM) 0, (LPARAM) &lvi);

        lvi.iSubItem = 1;
        if (value)
            lvi.pszText = value;
        else
            lvi.pszText = (TCHAR *) libLang->translate(_T("$$UNDEFINED"));
        SendMessage((HWND) hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
    }
}

// Set value of read-only result variables.
void FinShowValue (int rowNb, TCHAR *value) {
    HWND hWnd;

    if (rowNb == 0) {
        hWnd = GetDlgItem(hDlg_FinCalc, IDC_EDIT3);
    } else {
        hWnd = GetDlgItem(hDlg_FinCalc, IDC_EDIT4);
    }
    if (value)
        SetWindowText(hWnd, value);
    else
        SetWindowText(hWnd, libLang->translate(_T("$$UNDEFINED")));
}

/********************************************************************************
 *  FUNCTION: WndFinCalc (HWND, UINT, WPARAM, LPARAM)                           *
 *  Message handler for the Financial calculator dialog.                        *
 *  Ex FinHandleEvent in finance.c                                              *
 ********************************************************************************/
BOOL CALLBACK WndFinCalc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
    static int row, col;
    static TdispPrefs oldprefs;

    hDlg_FinCalc = hDlg; // Share it with other routines
    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, libLang->translate(_T("$$FINANCIAL TITLE")));
            HWND hWnd = GetDlgItem(hDlg, IDC_BEGINP);
            SetWindowText(hWnd, libLang->translate(_T("$$BEGIN")));
            if (calcPrefs.finBegin) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDC_ENDP);
            SetWindowText(hWnd, libLang->translate(_T("$$END")));
            if (!calcPrefs.finBegin) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            TCHAR text[64];
            hWnd = GetDlgItem(hDlg, IDC_VARNAME3);
            _tcscpy(text, libLang->translate(outFinForm[0].description));
            _tcscat(text, _T(" ("));
            _tcscat(text, outFinForm[0].var);
            _tcscat(text, _T(")"));
            SetWindowText(hWnd, text);
            hWnd = GetDlgItem(hDlg, IDC_VARNAME4);
            _tcscpy(text, libLang->translate(outFinForm[1].description));
            _tcscat(text, _T(" ("));
            _tcscat(text, outFinForm[1].var);
            _tcscat(text, _T(")"));
            SetWindowText(hWnd, text);
            // Set 2 columns in the list view
            hWnd = GetDlgItem(hDlg, IDC_VARLIST);
            LVCOLUMN lvc;
            lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 60 * cur_skin->magnification;
            lvc.pszText = (TCHAR *) libLang->translate(_T("$$NAME:"));
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 0, (LPARAM) (&lvc));
            lvc.cx = 170 * cur_skin->magnification;
            lvc.pszText = (TCHAR *) libLang->translate(_T("$$VALUE:"));
            SendMessage(hWnd, LVM_INSERTCOLUMN, (WPARAM) 1, (LPARAM) (&lvc));

            oldprefs = fp_set_prefs(finDPrefs);
            fin_update_fields();
            // Nothing selected yet
            row = col = -1;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_NOTIFY:
            {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->idFrom == IDC_VARLIST) {
                if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_SETFOCUS)
                    || (pnmh->code == NM_DBLCLK) || (pnmh->code == NM_KILLFOCUS)) {
                    HWND hWnd = pnmh->hwndFrom;
                    if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_DBLCLK)) {
                        NMLISTVIEW *nmlv = (NMLISTVIEW *) lParam; // A bigger structure in fact ...
                        if (nmlv->iSubItem != 0) { // Click on a subitem, but we have -1 in iItem,
                                                   // so have to retrieve it.
                             // Get the corresponding item = row number
                            LVHITTESTINFO lvhti;
                            lvhti.pt = nmlv->ptAction;
                            int i = SendMessage(hWnd, LVM_SUBITEMHITTEST, (WPARAM) 0, (LPARAM) &lvhti);
                            // Modify state of subitem(s) and selection only if something is changing
                            LVITEM lvi;
                            memset(&lvi, 0, sizeof(lvi));
                            lvi.mask = LVIF_STATE;
                            lvi.stateMask = 0xF;
                            if (((lvhti.flags & LVHT_ONITEMLABEL) != 0)
                                && ((col != lvhti.iSubItem) || (row != lvhti.iItem))) {
                                if (col != -1) { // Clear previous one before lightening new one
                                    lvi.iItem = row;
                                    lvi.iSubItem = col;
                                    lvi.state = 0;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                }
                                row = lvi.iItem = lvhti.iItem;
                                col = lvi.iSubItem = lvhti.iSubItem;

                                if (pnmh->code == NM_DBLCLK) // Jump to edit mode
                                    goto value_edit;

                                HWND hWnd_tmp = GetDlgItem(hDlg, IDC_VARCOMMENT);
                                SetWindowText(hWnd_tmp, libLang->translate(inpFinForm[row].description));

                                lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                                lvi.mask = LVIF_STATE;
                                i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                // Select corresponding item to show selection mark
                                lvi.iSubItem = 0;
                                lvi.state = LVIS_SELECTED;
                                i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                i = SendMessage(hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
                            } else { // Clicking not on label = clear selection,
                                     // or on same subitem a second time = edit value
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                if ((lvhti.flags & LVHT_ONITEMLABEL) != 0) {
                                    // Second time click = edit value
                                value_edit:
                                    // Show the popup window to get new value
                                    t_getcplx_param param;
                                    param.title = (TCHAR *) (libLang->translate(inpFinForm[row].description));
                                    param.label = inpFinForm[row].var;
                                    param.value = NULL;
                                    HWND hWnd_tmp = GetDlgItem(hDlg, IDC_VARCOMMENT);
                                    SetWindowText(hWnd_tmp, param.title);
                                    if (DialogBoxParam (g_hInst, MAKEINTRESOURCE(IDD_VARSTRINGENTRY), hDlg,
                                                        WndModVMenu,
                                                        (LPARAM) &param
                                                       )
                                       ) {
                                        fin_update_fields();
                                    }
                                    lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
                                    lvi.mask = LVIF_STATE;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                    // Select corresponding item to show selection mark
                                    lvi.iSubItem = 0;
                                    lvi.state = LVIS_SELECTED;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                    i = SendMessage(hWnd, LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) row);
                                } else if (col != -1) { // Clear previous one
                                    col = row = -1;
                                    lvi.state = 0;
                                    i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                                    HWND hWnd_tmp = GetDlgItem(hDlg, IDC_VARCOMMENT);
                                    SetWindowText(hWnd_tmp, _T(""));
                                }
                            }
                        } else { // Something has been clicked which is not a subitem = item
                            if (col != -1) { // Clear previous one if there was
                                LVITEM lvi;
                                lvi.mask = LVIF_STATE;
                                lvi.stateMask = 0xF;
                                lvi.iItem = row;
                                lvi.iSubItem = col;
                                col = -1;
                                lvi.state = 0;
                                int i = SendMessage(hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
                            }
                            row = nmlv->iItem;
                            HWND hWnd_tmp = GetDlgItem(hDlg, IDC_VARCOMMENT);
                            if (row == -1)
                                SetWindowText(hWnd_tmp, _T(""));
                            else {
                                SetWindowText(hWnd_tmp, libLang->translate(inpFinForm[row].description));
                                fin_compute(row, calcPrefs.finBegin);
                                fin_update_fields();
                            }
                        }
                    }
                    processed = TRUE;
                } else if (pnmh->code == LVN_BEGINLABELEDIT) {
                    LV_DISPINFO *lvdi = (LV_DISPINFO *) lParam; // A bigger structure in fact ...
                    res = TRUE; // Do not allow editing item (0) or subitem. Note: true means no edit.
                    processed = TRUE;
                } else if (pnmh->code == NM_RECOGNIZEGESTURE) {
                    // This will ensure that double click will be recognized.
                    // Not all notification messages are processed, and it seems
                    // that answering TRUE to NM_RECOGNIZEGESTURE instead of FALSE
                    // allows to get the double click notification.
                    res = processed = TRUE;
                }
            }
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            switch (notify_src) {
                case IDOK:
                    if (notify_msg == BN_CLICKED) {
                        fp_set_prefs(oldprefs);
                        EndDialog(hDlg, 1);
                        processed = TRUE;
                    }
                    break;

                case IDC_BEGINP:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            calcPrefs.finBegin = true;
                            processed = TRUE;
                            break;
                    }
                    break;

                case IDC_ENDP:
                    switch (notify_msg) {
                        case BN_CLICKED:
                            calcPrefs.finBegin = false;
                            processed = TRUE;
                            break;
                    }
                    break;

            }
            }
            break;

        case WM_CLOSE:
            fp_set_prefs(oldprefs);
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndGraphPrefs (HWND, UINT, WPARAM, LPARAM)                        *
 *  Message handler for the IDD_GRAPH_PREFS dialog.                             *
 *  Ex GraphPrefsHandleEvent in grprefs.c                                       *
 ********************************************************************************/
BOOL CALLBACK WndGraphPrefs (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static TgrPrefs oldPrefs;
    static TdispPrefs oldprefs;
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            oldPrefs = graphPrefs;
            oldprefs = fp_set_prefs(grDPrefs);

            SetWindowText(hDlg, libLang->translate(_T("$$GRAPH PREFERENCES")));
            HWND hWnd = GetDlgItem(hDlg, IDC_EDITXMIN);
            TCHAR *text = display_real(graphPrefs.xmin);
            SetWindowText(hWnd, text);
            MemPtrFree(text);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;

            hWnd = GetDlgItem(hDlg, IDC_EDITXMAX);
            text = display_real(graphPrefs.xmax);
            SetWindowText(hWnd, text);
            MemPtrFree(text);
            hWnd = GetDlgItem(hDlg, IDC_EDITYMIN);
            text = display_real(graphPrefs.ymin);
            SetWindowText(hWnd, text);
            MemPtrFree(text);
            hWnd = GetDlgItem(hDlg, IDC_EDITYMAX);
            text = display_real(graphPrefs.ymax);
            SetWindowText(hWnd, text);
            MemPtrFree(text);

            hWnd = GetDlgItem(hDlg, IDC_RADIOFCT);
            SetWindowText(hWnd, libLang->translate(_T("$$FUNCMODE")));
            if (graphPrefs.functype == graph_func) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
                hWnd = GetDlgItem(hDlg, IDC_THMIN);
                ShowWindow(hWnd, SW_HIDE);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                ShowWindow(hWnd, SW_HIDE);
                hWnd = GetDlgItem(hDlg, IDC_THMAX);
                ShowWindow(hWnd, SW_HIDE);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                ShowWindow(hWnd, SW_HIDE);
                hWnd = GetDlgItem(hDlg, IDC_THSTEP);
                ShowWindow(hWnd, SW_HIDE);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                ShowWindow(hWnd, SW_HIDE);
            }
            hWnd = GetDlgItem(hDlg, IDC_RADIOPOLAR);
            SetWindowText(hWnd, libLang->translate(_T("$$POLARMODE")));
            if (graphPrefs.functype == graph_polar) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
                hWnd = GetDlgItem(hDlg, IDC_THMIN);
                SetWindowText(hWnd, _T("th-min"));
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                text = display_real(graphPrefs.fimin);
                SetWindowText(hWnd, text);
                MemPtrFree(text);
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_THMAX);
                SetWindowText(hWnd, _T("th-max"));
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                text = display_real(graphPrefs.fimax);
                SetWindowText(hWnd, text);
                MemPtrFree(text);
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_THSTEP);
                SetWindowText(hWnd, _T("th-step"));
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                text = display_real(graphPrefs.fistep);
                SetWindowText(hWnd, text);
                MemPtrFree(text);
                ShowWindow(hWnd, SW_SHOWNA);
            }
            hWnd = GetDlgItem(hDlg, IDC_RADIOPARAM);
            SetWindowText(hWnd, libLang->translate(_T("$$PARAMMODE")));
            if (graphPrefs.functype == graph_param) {
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
                hWnd = GetDlgItem(hDlg, IDC_THMIN);
                SetWindowText(hWnd, _T("t-min"));
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                text = display_real(graphPrefs.tmin);
                SetWindowText(hWnd, text);
                MemPtrFree(text);
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_THMAX);
                SetWindowText(hWnd, _T("t-max"));
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                text = display_real(graphPrefs.tmax);
                SetWindowText(hWnd, text);
                MemPtrFree(text);
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_THSTEP);
                SetWindowText(hWnd, _T("t-step"));
                ShowWindow(hWnd, SW_SHOWNA);
                hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                text = display_real(graphPrefs.tstep);
                SetWindowText(hWnd, text);
                MemPtrFree(text);
                ShowWindow(hWnd, SW_SHOWNA);
            }

            if (graphPrefs.logx) {
                hWnd = GetDlgItem(hDlg, IDC_CHECKLOGX);
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }
            if (graphPrefs.logy) {
                hWnd = GetDlgItem(hDlg, IDC_CHECKLOGY);
                SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
            }

            hWnd = GetDlgItem(hDlg, IDC_BUTTONDEFAULT);
            SetWindowText(hWnd, libLang->translate(_T("$$DEFAULT")));
            hWnd = GetDlgItem(hDlg, IDC_BUTTONTRIGODEF);
            SetWindowText(hWnd, libLang->translate(_T("$$TRIGONO")));

            hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_CHECKLOGX) {
                switch (notify_msg) {
                    case BN_CLICKED:
                        int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                        graphPrefs.logx = (sel == BST_CHECKED);
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDC_CHECKLOGY) {
                switch (notify_msg) {
                    case BN_CLICKED:
                        int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                        graphPrefs.logy = (sel == BST_CHECKED);
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDC_RADIOFCT) {
                switch (notify_msg) {
                    case BN_CLICKED:
                        graphPrefs.functype = graph_func;
                        hWnd = GetDlgItem(hDlg, IDC_THMIN);
                        ShowWindow(hWnd, SW_HIDE);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                        ShowWindow(hWnd, SW_HIDE);
                        hWnd = GetDlgItem(hDlg, IDC_THMAX);
                        ShowWindow(hWnd, SW_HIDE);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                        ShowWindow(hWnd, SW_HIDE);
                        hWnd = GetDlgItem(hDlg, IDC_THSTEP);
                        ShowWindow(hWnd, SW_HIDE);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                        ShowWindow(hWnd, SW_HIDE);
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDC_RADIOPOLAR) {
                switch (notify_msg) {
                    case BN_CLICKED:
                        graphPrefs.functype = graph_polar;
                        hWnd = GetDlgItem(hDlg, IDC_THMIN);
                        SetWindowText(hWnd, _T("th-min"));
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                        TCHAR *text = display_real(graphPrefs.fimin);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_THMAX);
                        SetWindowText(hWnd, _T("th-max"));
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                        text = display_real(graphPrefs.fimax);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_THSTEP);
                        SetWindowText(hWnd, _T("th-step"));
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                        text = display_real(graphPrefs.fistep);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        ShowWindow(hWnd, SW_SHOWNA);
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDC_RADIOPARAM) {
                switch (notify_msg) {
                    case BN_CLICKED:
                        graphPrefs.functype = graph_param;
                        hWnd = GetDlgItem(hDlg, IDC_THMIN);
                        SetWindowText(hWnd, _T("t-min"));
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                        TCHAR *text = display_real(graphPrefs.tmin);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_THMAX);
                        SetWindowText(hWnd, _T("t-max"));
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                        text = display_real(graphPrefs.tmax);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_THSTEP);
                        SetWindowText(hWnd, _T("t-step"));
                        ShowWindow(hWnd, SW_SHOWNA);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                        text = display_real(graphPrefs.tstep);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        ShowWindow(hWnd, SW_SHOWNA);
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDC_BUTTONDEFAULT) {
                if (notify_msg == BN_CLICKED) {
                    hWnd = GetDlgItem(hDlg, IDC_EDITXMIN);
                    TCHAR *text = display_real(graphPrefs.xmin = -7.0);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    hWnd = GetDlgItem(hDlg, IDC_EDITXMAX);
                    text = display_real(graphPrefs.xmax = 7.0);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    hWnd = GetDlgItem(hDlg, IDC_EDITYMIN);
                    text = display_real(graphPrefs.ymin = -7.0);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    hWnd = GetDlgItem(hDlg, IDC_EDITYMAX);
                    text = display_real(graphPrefs.ymax = 7.0);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    graphPrefs.xscale = graphPrefs.yscale = 1.0;
                    if (graphPrefs.functype == graph_polar) {
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                        text = display_real(graphPrefs.fimin = 0.0);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                        text = display_real(math_rad_to_user(graphPrefs.fimax = 2.0 * M_PIl));
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                        text = display_real(math_rad_to_user(graphPrefs.fistep = 0.02 * M_PIl));
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                    } else if (graphPrefs.functype == graph_param) {
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                        text = display_real(graphPrefs.tmin = 0.0);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                        text = display_real(graphPrefs.tmax = 10.0);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                        hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                        text = display_real(graphPrefs.tstep = 0.1);
                        SetWindowText(hWnd, text);
                        MemPtrFree(text);
                    }
                    processed = TRUE;
                }
            } else if (notify_src == IDC_BUTTONTRIGODEF) {
                if (notify_msg == BN_CLICKED) {
                    hWnd = GetDlgItem(hDlg, IDC_EDITXMIN);
                    TCHAR *text = display_real(graphPrefs.xmin = -2.0 * M_PIl);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    hWnd = GetDlgItem(hDlg, IDC_EDITXMAX);
                    text = display_real(graphPrefs.xmax = 2.0 * M_PIl);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    hWnd = GetDlgItem(hDlg, IDC_EDITYMIN);
                    text = display_real(graphPrefs.ymin = -2.0 * M_PIl);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    hWnd = GetDlgItem(hDlg, IDC_EDITYMAX);
                    text = display_real(graphPrefs.ymax = 2.0 * M_PIl);
                    SetWindowText(hWnd, text);
                    MemPtrFree(text);
                    graphPrefs.xscale = graphPrefs.yscale = M_PIl / 2.0;
                    processed = TRUE;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    graphPrefs = oldPrefs;
                    fp_set_prefs(oldprefs);
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    // When input is made, retrieve it
                    processed = TRUE;

                    hWnd = GetDlgItem(hDlg, IDC_EDITXMIN);
                    int siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                    TCHAR *text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                    CError err = grpref_comp_field(text, &(graphPrefs.xmin));
                    MemPtrFree(text);
                    if (!err) {
                        hWnd = GetDlgItem(hDlg, IDC_EDITXMAX);
                        siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                        SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                        err = grpref_comp_field(text, &(graphPrefs.xmax));
                        MemPtrFree(text);
                    }
                    if (!err) {
                        hWnd = GetDlgItem(hDlg, IDC_EDITYMIN);
                        siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                        SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                        err = grpref_comp_field(text, &(graphPrefs.ymin));
                        MemPtrFree(text);
                    }
                    if (!err) {
                        hWnd = GetDlgItem(hDlg, IDC_EDITYMAX);
                        siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                        SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                        err = grpref_comp_field(text, &(graphPrefs.ymax));
                        MemPtrFree(text);
                    }
//                    if (grpref_comp_field(grPrefXscale, &xscale))
//                        return (0);
//                    if (grpref_comp_field(grPrefYscale, &yscale))
//                        return (0);
                    if (graphPrefs.functype != graph_func) {
                        double t1, t2, t3;
                        if (!err) {
                            hWnd = GetDlgItem(hDlg, IDC_EDITTHMIN);
                            siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                            text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                            SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                            err = grpref_comp_field(text, &t1);
                            MemPtrFree(text);
                        }
                        if (!err) {
                            hWnd = GetDlgItem(hDlg, IDC_EDITTHMAX);
                            siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                            text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                            SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                            err = grpref_comp_field(text, &t2);
                            MemPtrFree(text);
                        }
                        if (!err) {
                            hWnd = GetDlgItem(hDlg, IDC_EDITTHSTEP);
                            siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                            text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                            SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                            err = grpref_comp_field(text, &t3);
                            MemPtrFree(text);
                        }
                        if (graphPrefs.functype == graph_polar) {
                            graphPrefs.fimin  = math_user_to_rad(t1);
                            graphPrefs.fimax  = math_user_to_rad(t2);
                            graphPrefs.fistep = math_user_to_rad(t3);
                        } else {
                            graphPrefs.tmin = t1;
                            graphPrefs.tmax = t2;
                            graphPrefs.tstep = t3;
                        }
                    }

                    if (!err && grpref_verify_values()) {
                        fp_set_prefs(oldprefs);
                        EndDialog(hDlg, 1);
                    } else   FrmAlert(altGraphBadVal, NULL);
                }
            }
            }
            break;

        case WM_CLOSE:
            graphPrefs = oldPrefs;
            fp_set_prefs(oldprefs);
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

// Set values in cells.
static bool paramxy;
static int colobj[] = {IDC_COLORY1, IDC_COLORY2, IDC_COLORY3, IDC_COLORY4, IDC_COLORY5, IDC_COLORY6};
static int lineobj[] = {IDC_LINEY1, IDC_LINEY2, IDC_LINEY3, IDC_LINEY4, IDC_LINEY5, IDC_LINEY6};
static int fnobj[] = {IDC_CHECKY1, IDC_CHECKY2, IDC_CHECKY3, IDC_CHECKY4, IDC_CHECKY5, IDC_CHECKY6};
static int yobj[] = {IDC_EDITY1, IDC_EDITY2, IDC_EDITY3, IDC_EDITY4, IDC_EDITY5, IDC_EDITY6};
static int xobj[] = {IDC_EDITX1, IDC_EDITX2, IDC_EDITX3, IDC_EDITX4, IDC_EDITX5, IDC_EDITX6};
static HWND hDlg_GraphConf;
void GraphConfSetRow (int rowNb, TCHAR *celltext) {
    HWND hWnd;

    if (paramxy) {
        if (rowNb & 1)   hWnd = GetDlgItem(hDlg_GraphConf, xobj[rowNb >> 1]);
        else   hWnd = GetDlgItem(hDlg_GraphConf, yobj[rowNb >> 1]);
    } else {
        hWnd = GetDlgItem(hDlg_GraphConf, yobj[rowNb]);
    }

    SetWindowText(hWnd, celltext);
}

/********************************************************************************
 *  FUNCTION: WndGraphFctMenu(HWND, UINT, WPARAM, LPARAM)                       *
 *  Message handler for the function list in Graph Config.                      *
 *  Comes from grsetup_flist_tap() in grsetup.c.                                *
 *  In lParam contain a pointer at t_getGraphFct_param, identifying the         *
 *  parent window and the clicked row button (Yn, Xn).                          *
 ********************************************************************************/
BOOL CALLBACK WndGraphFctMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static t_getGraphFct_param *params;
    static dbList *dblist;
    static TCHAR **displist;
    static int pos;

    switch (message) {
        case WM_INITDIALOG:
            {
            // Remember parent window
            params = (t_getGraphFct_param *) lParam; // Passed by calling window for return message
            // Initialize window with list of functions to chose from, pointing at current
            // function if any
            TCHAR  *fname;
            TCHAR   txtuser[MAX_RSCLEN], txtnone[MAX_RSCLEN];

            dblist = db_get_list(function);
            displist = (TCHAR **) MemPtrNew(sizeof(TCHAR *) * (dblist->size + 2));

            /* displist will contain dynamic and static items, but the dynamic
             * will be released by db_delete_list with dblist */
            txtnone[0] = _T(' ');
            _tcscpy(txtnone+1, libLang->translate(_T("$$DELETE")));
            txtuser[0] = _T(' ');
            _tcscpy(txtuser+1, libLang->translate(_T("$$USER")));
            displist[0] = txtnone;
            displist[1] = txtuser;
            memcpy((void *)&displist[2], (void *)(dblist->list), dblist->size * sizeof(TCHAR *));

            /* Fname is actually pointer to the fct name saved in graphPrefs structure */
            fname = grsetup_get_fname(params->rownb);
            pos = -1; // No selection
            if (_tcslen(fname) != 0) {
                if (_tcscmp(fname, grsetup_def_name(params->rownb)) == 0)
                    pos = 1;
                else   for (int i=0 ; i<dblist->size ; i++)
                    if (_tcscmp(fname, dblist->list[i]) == 0)
                        pos = i+2;
            }

            HWND hLst = GetDlgItem(hDlg, IDC_LIST_SLIM);
            cur_skin->varMgrPopup(displist, dblist->size + 2, hLst);
            SendMessage(hLst, LB_SETCURSEL, pos, (LPARAM) 0);
            }
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_SLIM) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hWnd, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        int user_fct = 0;
                        // And store the function name
                        destroying = true;
                        DestroyWindow(hDlg);

                        if (cur_sel == 0) { // Delete entry
                            fctPopup_name = NULL;
                        } else if (cur_sel == 1) { // User function, the calling window
                                                     // will have to ask for it.
                            user_fct = 1;
                            const TCHAR *tmp = grsetup_def_name(params->rownb);
                            fctPopup_name = (TCHAR *) MemPtrNew((_tcslen(tmp)+1) * sizeof(TCHAR));
                            _tcscpy(fctPopup_name, tmp);
                        } else {
                            fctPopup_name = (TCHAR *) MemPtrNew((_tcslen(displist[cur_sel])+1) * sizeof(TCHAR));
                            _tcscpy(fctPopup_name, displist[cur_sel]);
                        }
                        db_delete_list(dblist);
                        MemPtrFree(displist);

                        PostMessage(params->parent, WM_APP_ENDFMENU, (WPARAM) user_fct, (LPARAM) fctPopup_name);
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // The window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    db_delete_list(dblist);
                    MemPtrFree(displist);
                    destroying = true;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndGraphConf (HWND, UINT, WPARAM, LPARAM)                         *
 *  Message handler for the IDD_GRAPH_CONFIG & IDD_GRAPH_CONFIGXY dialogs.      *
 *  Ex GraphSetupHandleEvent in grsetup.c                                       *
 ********************************************************************************/
BOOL CALLBACK WndGraphConf (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;
    static int m_nNormalWidth;
    static int m_nCurWidth;
    static int m_nHScrollPos;
    static RECT m_rect;
    static SCROLLINFO m_si;
    static bool ignore_edit_signal;
    static t_getGraphFct_param params;

    switch (message) {
        case WM_INITDIALOG:
            hDlg_GraphConf = hDlg;
            ignore_edit_signal = false;

            // Initialize window with current language strings
            {
            if (graphPrefs.functype == graph_param) {
                paramxy = true;

                GetWindowRect(hDlg, &m_rect);
                m_nNormalWidth = m_nCurWidth = m_rect.right - m_rect.left;
#define STEP_RATIO  10
                m_si.cbSize = sizeof(SCROLLINFO);
                m_si.fMask = SIF_ALL; // SIF_ALL = SIF_PAGE | SIF_RANGE | SIF_POS;
                m_si.nMin = m_si.nMax = m_si.nPage = m_si.nPos
                          = m_nHScrollPos = 0;
                SetScrollInfo(hDlg, SB_HORZ, &m_si, TRUE);
                // Resize if needed
                if (m_nNormalWidth > cur_skin->skin_width) {
                    MoveWindow(hDlg, m_rect.left, m_rect.top, cur_skin->skin_width, m_rect.bottom - m_rect.top, TRUE);
                }
            } else {
                paramxy = false;
            }

            SetWindowText(hDlg, libLang->translate(_T("$$SETUP GRAPHS")));
            HWND hWnd;
            for (int i=0 ; i<MAX_GRFUNCS ; i++) {
                TCHAR text[6];

                hWnd = GetDlgItem(hDlg, fnobj[i]);
                if (graphPrefs.grEnable[graphPrefs.functype][i]) {
                    SendMessage(hWnd, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
                }
                if (!paramxy) {
                    _tcscpy(text, grsetup_fn_descr(i));
                    _tcscat(text, _T(":"));
                    SetWindowText(hWnd, text);

                    grsetup_cell_contents(i);
                } else {
                    grsetup_cell_contents(i << 1);
                    grsetup_cell_contents((i << 1) | 1);
                }
            }
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            hWnd = GetDlgItem(hDlg, IDOK);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_CTLCOLORSTATIC:
            // Color boxes and line styles
            {
            HWND hWnd = (HWND) lParam;
            HDC hdc = (HDC) wParam;

            // Find out which control this is
            int id = GetDlgCtrlID(hWnd);
            for (int i=0 ; i<6 ; i++) {
                if ((colobj[i] == id) || (lineobj[i] == id)) { // Found it
                    SetTextColor(hdc, graphPrefs.colors[i]);
                    processed = (BOOL) GetStockObject(WHITE_BRUSH);
                }
            }
            }
            break;

        case WM_SIZE:
            if (paramxy) {
                unsigned int nHeight = lParam >> 16;
                m_nCurWidth = lParam & 0xFFFF;
                if (m_nCurWidth < m_nNormalWidth)	{
                    m_si.nMax  = m_nNormalWidth;
                    m_si.nPage = m_nCurWidth;
                } else {
                    m_si.nPage = m_si.nMax = 0;
                }

                if (m_nHScrollPos > (m_si.nMax - (int)(m_si.nPage) + 1)) {
                    m_nHScrollPos = m_si.nMax - m_si.nPage + 1;
                }
                m_si.nPos = m_nHScrollPos;
                SetScrollInfo(hDlg, SB_HORZ, &m_si, TRUE); 
            }
            break;

        case WM_HSCROLL:
            {
            int nSBCode = (int) LOWORD(wParam);
            int nPos = (int) HIWORD(wParam);
//            HWND hwndScrollBar = (HWND) lParam;
            int nDelta;
            int nMaxPos = m_nNormalWidth - m_nCurWidth + 1;

            switch (nSBCode) {
                case SB_LINERIGHT:
                    if (m_nHScrollPos >= nMaxPos) {
                        m_nHScrollPos = nMaxPos;
                        return (FALSE);
                    }
                    nDelta = min(max(nMaxPos/STEP_RATIO, 1), nMaxPos - m_nHScrollPos);
                    break;

                case SB_LINELEFT:
                    if (m_nHScrollPos <= 0) {
                        m_nHScrollPos = 0;
                        return (FALSE);
                    }
                    nDelta = -min(max(nMaxPos/STEP_RATIO, 1), m_nHScrollPos);
                    break;

                case SB_PAGERIGHT:
                    if (m_nHScrollPos >= nMaxPos) {
                        m_nHScrollPos = nMaxPos;
                        return (FALSE);
                    }
                    nDelta = min((int)(m_si.nPage), nMaxPos - m_nHScrollPos);
                    break;

                case SB_PAGELEFT:
                    if (m_nHScrollPos <= 0) {
                        m_nHScrollPos = 0;
                        return (FALSE);
                    }
                    nDelta = -min((int)(m_si.nPage), m_nHScrollPos);
                    break;

                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    nDelta = nPos - m_nHScrollPos;
                    break;

                default:
                    return (FALSE);
            }

            m_nHScrollPos += nDelta;
            SetScrollPos(hDlg, SB_HORZ, m_nHScrollPos, TRUE);
            ScrollWindowEx(hDlg, -nDelta, 0, NULL, &m_rect, NULL, NULL, SW_SCROLLCHILDREN);
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hWnd = (HWND) lParam;
            int k;
            if (notify_src == IDC_CHECKY1) {
                k = 0;
CheckY_lbl:
                switch (notify_msg) {
                    case BN_CLICKED:
                        int sel = (int) SendMessage(hWnd, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0);
                        graphPrefs.grEnable[graphPrefs.functype][k] = (sel == BST_CHECKED);
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDC_CHECKY2) {
                k = 1;
                goto CheckY_lbl;
            } else if (notify_src == IDC_CHECKY3) {
                k = 2;
                goto CheckY_lbl;
            } else if (notify_src == IDC_CHECKY4) {
                k = 3;
                goto CheckY_lbl;
            } else if (notify_src == IDC_CHECKY5) {
                k = 4;
                goto CheckY_lbl;
            } else if (notify_src == IDC_CHECKY6) {
                k = 5;
                goto CheckY_lbl;
            } else if (notify_src == IDC_EDITY1) {
                k = 0;
EditY_lbl:
                switch (notify_msg) {
                    case EN_SETFOCUS: // Click on it
                        // A trick is necessary here: when the fct edit window appears,
                        // focus on the edit is lost, and is then gained again when the
                        // dialog is dismissed. We need to ignore this second focus signal,
                        // since it is not a real click in the edit box.
                        if (ignore_edit_signal) {
                            ignore_edit_signal = false;
                            // By the way, make it lose focus, or clicking again in the
                            // box won't send the "gained focus" signal again.
                            hWnd = GetDlgItem(hDlg, IDOK);
                            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE);
                            SetFocus(hDlg); // Must do both !! (or abnormal things occur ... Windows GUI API is really crap !)
                        } else {
                            t_getcplx_param param;
                            TCHAR usrname[MAX_FUNCNAME+1]; /* grsetup_def_name returns static variable,
                                                            * that can change e.g. on form closure
                                                            * (and update on Debug ROM).
                                                            * DO not store a pointer
                                                            */
                            TCHAR *grname;

                            grname = grsetup_get_fname(k);
                            StrCopy(usrname, grsetup_def_name(k));
                            if (_tcslen(grname) == 0) { // || (_tcscmp(usrname, grname) == 0)) {
                                param.title = (TCHAR *) (libLang->translate(_T("$$GRAPH FUNCTION")));
                                param.label = usrname;
                            } else {
                                param.title = (TCHAR *) (libLang->translate(_T("$$FUNCTION MODIFICATION")));
                                param.label = grname;
                            }
                            param.param = grsetup_param_name();
                            param.value = NULL;
                            ignore_edit_signal = true; // Ignore next "gained focus" event.
                            int rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDITFCT), hDlg, WndModFMenu,
                                                    (LPARAM) &param
                                                   );
                            if (rc) {
                                StrCopy(grname, usrname);
                                grsetup_cell_contents(k);
                            }
                        }
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDC_EDITY2) {
                k = 1;
                if (paramxy)   k <<= 1;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITY3) {
                k = 2;
                if (paramxy)   k <<= 1;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITY4) {
                k = 3;
                if (paramxy)   k <<= 1;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITY5) {
                k = 4;
                if (paramxy)   k <<= 1;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITY6) {
                k = 5;
                if (paramxy)   k <<= 1;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITX1) {
                k = 1;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITX2) {
                k = 3;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITX3) {
                k = 5;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITX4) {
                k = 7;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITX5) {
                k = 9;
                goto EditY_lbl;
            } else if (notify_src == IDC_EDITX6) {
                k = 11;
                goto EditY_lbl;
            } else if (notify_src == IDC_BUTTONY1) {
                k = 0;
ButtonFct_lbl:
                if (notify_msg == BN_CLICKED) {
                    params.parent = hDlg;
                    params.rownb  = k;
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndGraphFctMenu,
                                                  (LPARAM) &params);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_BUTTONY2) {
                k = 1;
                if (paramxy)   k <<= 1;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONY3) {
                k = 2;
                if (paramxy)   k <<= 1;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONY4) {
                k = 3;
                if (paramxy)   k <<= 1;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONY5) {
                k = 4;
                if (paramxy)   k <<= 1;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONY6) {
                k = 5;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONX1) {
                k = 1;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONX2) {
                k = 3;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONX3) {
                k = 5;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONX4) {
                k = 7;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONX5) {
                k = 9;
                goto ButtonFct_lbl;
            } else if (notify_src == IDC_BUTTONX6) {
                k = 11;
                goto ButtonFct_lbl;
            } else if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    processed = TRUE;
                    EndDialog(hDlg, 1);
                }
            }
            }
            break;

        case WM_APP_ENDFMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            TCHAR *fname = grsetup_get_fname(params.rownb); // fname points at the fct name saved in graphPref structure
            TCHAR *newname = (TCHAR *) lParam;

            if (fctPopup_name == NULL) { // Delete
                db_delete_record(grsetup_def_name(params.rownb));
                fname[0] = 0; // Empty string
            } else {
                _tcscpy(fname, newname);
                mfree(newname);
                if (wParam == 1) { // User function, ask for it
                    t_getcplx_param param;
                    param.title = (TCHAR *) (libLang->translate(_T("$$GRAPH FUNCTION")));
                    param.label = newname;
                    param.param = grsetup_param_name();
                    param.value = NULL;
                    int rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_EDITFCT), hDlg, WndModFMenu,
                                            (LPARAM) &param
                                           );
                }
            }
            }
            grsetup_cell_contents(params.rownb); // Display new value
            processed = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndGraphTrackMenu(HWND, UINT, WPARAM, LPARAM)                     *
 *  Message handler for the Curve Track button, choose a curve.                 *
 ********************************************************************************/
BOOL CALLBACK WndGraphTrackMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static HWND hLst, hParent;

    switch (message) {
        case WM_INITDIALOG:
            // Remember parent window
            hParent = (HWND) lParam; // Passed by calling window for return message
            // Initialize window with list of curves
            hLst = GetDlgItem(hDlg, IDC_LIST_SLIM);
            for (int i=0 ; i<grphTrkCount ; i++) {
                SendMessage(hLst, LB_ADDSTRING, 0, (LPARAM) GrphTrkDescr[i]);
            }
            // Show selected one if any
            for (int i=0 ; i<grphTrkCount ; i++) {
                if (GrphTrkNums[i] == cur_skin->curve_nb)
                    SendMessage(hLst, LB_SETCURSEL, (WPARAM) i, (LPARAM) 0);
            }
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_SLIM) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hLst, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        // And store the curve number
                        destroying = true;
                        DestroyWindow(hDlg);
                        if (cur_sel == LB_ERR)
                            grphTrkCurve_nb = -1;
                        else
                            grphTrkCurve_nb = GrphTrkNums[cur_sel];
                        if (graphCalcIntersect) { // Coming from Calc process
                            PostMessage(hParent, WM_APP_ENDGCMENU, (WPARAM) 0, (LPARAM) 0);
                        } else { // Handling the Curve Track mode key
                            cur_skin->curve_nb = grphTrkCurve_nb;
                            PostMessage(hParent, WM_APP_ENDGTMENU, (WPARAM) 0, (LPARAM) 0);
                        }
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // The window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    destroying = true;
                    graphCalcIntersect = false;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndGraphCalcMenu(HWND, UINT, WPARAM, LPARAM)                      *
 *  Message handler for the Curve Calc button, choose a calcultion to make.     *
 ********************************************************************************/
BOOL CALLBACK WndGraphCalcMenu (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool destroying;
    BOOL processed = FALSE;
    LONG res = 0;
    static HWND hLst, hParent;
    static const t_grFuncType  *calccodelist;
    static int                  calcCodeList_sz, calcCodes_sz;
    static t_grFuncType calcCodes[cp_end];

    switch (message) {
        case WM_INITDIALOG:
            {
            // Remember parent window
            hParent = (HWND) lParam; // Passed by calling window for return message
            // Initialize window with list of curves
            if (graphPrefs.functype == graph_func) {
                calccodelist = (const t_grFuncType *) grcFunc;
                calcCodeList_sz = GRCFUNC_SZ;
            } else if (graphPrefs.functype == graph_polar) {
                calccodelist = (const t_grFuncType *) grcPolar;
                calcCodeList_sz = GRCPOLAR_SZ;
            } else {
                calccodelist = (const t_grFuncType *) grcParam;
                calcCodeList_sz = GRCPARAM_SZ;
            }
            hLst = GetDlgItem(hDlg, IDC_LIST_SLIM);
            calcCodes_sz = 0;
            t_grFuncType code;
            for (int i=0 ; i<calcCodeList_sz ; i++) {
                code = calccodelist[i];
                // If only a cross (not a range), only include point calculations
                if ((!cur_skin->is_selecting || (cur_skin->param == cur_skin->param0))
                    && ((code == cp_zero)
                        || (code == cp_value)
                        || (code == cp_min)
                        || (code == cp_max)
                        || (code == cp_integ)
                        || (code == cp_intersect)
                       )
                   )
                    continue;
                SendMessage(hLst, LB_ADDSTRING, 0, (LPARAM) libLang->translate((TCHAR *) (lstGraphCalc[code])));
                calcCodes[calcCodes_sz++] = code;
            }
            destroying = false;
            processed = TRUE; // Set default keyboard focus.
            }
            break;

        case WM_COMMAND:
            {
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (notify_src == IDC_LIST_SLIM) { // Message from the List Box
                switch (notify_msg) {
                    case LBN_DBLCLK:
                    case LBN_SELCHANGE:
                        {
                        // When selection is made, retrieve it (LB_GETSELITEMS Message)
                        int cur_sel = (int) SendMessage(hLst, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                        // And retain the element number
                        destroying = true;
                        DestroyWindow(hDlg);
                        if (cur_sel != LB_ERR) {
                            grFuncType = calcCodes[cur_sel];
                            calcResult_name = libLang->translate(lstGraphCalc[grFuncType]);
                            PostMessage(hParent, WM_APP_ENDGCMENU, (WPARAM) 0, (LPARAM) 0);
                        }
                        processed = TRUE;
                        }
                        break;
                }
            }
            }
            break;

        case WM_ACTIVATE:
            {
            unsigned int minimized = wParam >> 16;
            unsigned int activate_msg = wParam & 0xFFFF;
//            HWND hWnd = (HWND) lParam;
            if (activate_msg == WA_INACTIVE) {
                // The window is not the active one anymore.
                // Destroy it, instead of leaving it behind.
                if (!destroying) {
                    destroying = true;
                    DestroyWindow(hDlg);
                }
            }
            }
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndGetVarString (HWND, UINT, WPARAM, LPARAM)                      *
 *  Message handler for the IDD_VARSTRINGENTRY dialog.                          *
 *  lParam contains the window title.                                           *
 *  Ex varmgr_get_varstring in varmgr.c                                         *
 ********************************************************************************/
BOOL CALLBACK WndGetVarString (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            SetWindowText(hDlg, (TCHAR *) lParam);
            HWND hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            SetWindowText(hWnd, libLang->translate(_T("$$VALUE:")));
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            //SetFocus(hWnd);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            // Disable OK by default
            hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            EnableWindow (hWnd, FALSE);
            varStringPopup_name = NULL;
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            HWND hWnd;
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hLst = (HWND) lParam;
            if (notify_src == IDC_EDIT2) { // Message from the Edit Box
                switch (notify_msg) {
                    case EN_UPDATE:
                    case EN_CHANGE:
                        {
                        // Handle state of OK button
                        int siz = (int) SendMessage(hLst, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        hWnd = GetDlgItem(hDlg, IDOK);
                        EnableWindow (hWnd, (siz > 0));
                        }
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    // When input is made, retrieve it
                    hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    int siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                    TCHAR *text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                    if (!(*text) || !varfunc_name_ok(text, variable))
                        FrmAlert(altBadVariableName, NULL);
                    else {
                        varStringPopup_name = text;
                        EndDialog(hDlg, 1);
                    }
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: WndGetComplex (HWND, UINT, WPARAM, LPARAM)                        *
 *  Message handler for the IDD_VARSTRINGENTRY dialog, but used to get a complex*
 *  value.                                                                      *
 *  lParam contains a pointer at a t_getcplx_param structure.                   *
 *  Ex varmgr_get_complex in varmgr.c                                           *
 ********************************************************************************/
BOOL CALLBACK WndGetComplex (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL processed = FALSE;
    LONG res = 0;

    switch (message) {
        case WM_INITDIALOG:
            // Initialize window with current language strings
            {
            t_getcplx_param *param = (t_getcplx_param *) lParam;
            SetWindowText(hDlg, param->title);
            HWND hWnd = GetDlgItem(hDlg, IDC_VARNAME);
            if (param->label)
                SetWindowText(hWnd, param->label);
            else
                SetWindowText(hWnd, libLang->translate(_T("$$VALUE:")));
            TCHAR *text = display_complex(*(param->value));
            hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SetWindowText(hWnd, text);
            // SetFocus doesn't work well in a Dialog, because of default buttons & co.
            // Have to use WM_NEXTDLGCTL instead.
            //SetFocus(hWnd);
            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) hWnd, (LPARAM) TRUE) ;
            // Disable OK if no value by default
            hWnd = GetDlgItem(hDlg, IDOK);
            SetWindowText(hWnd, libLang->translate(_T("$$OK")));
            if (_tcslen(text) == 0)
                EnableWindow (hWnd, FALSE);
            MemPtrFree(text);
            hWnd = GetDlgItem(hDlg, IDCANCEL);
            SetWindowText(hWnd, libLang->translate(_T("$$CANCEL")));
            }
            processed = TRUE; // Set default keyboard focus.
            break;

        case WM_COMMAND:
            {
            HWND hWnd;
            unsigned int notify_msg = wParam >> 16;
            unsigned int notify_src = wParam & 0xFFFF;
            HWND hLst = (HWND) lParam;
            if (notify_src == IDC_EDIT2) { // Message from the Edit Box
                switch (notify_msg) {
                    case EN_UPDATE:
                    case EN_CHANGE:
                        {
                        // Handle state of OK button
                        int siz = (int) SendMessage(hLst, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                        hWnd = GetDlgItem(hDlg, IDOK);
                        EnableWindow (hWnd, (siz > 0));
                        }
                        processed = TRUE;
                        break;
                }
            } else if (notify_src == IDCANCEL) {
                if (notify_msg == BN_CLICKED) {
                    EndDialog(hDlg, 0);
                    processed = TRUE;
                }
            } else if (notify_src == IDOK) {
                if (notify_msg == BN_CLICKED) {
                    // When input is made, retrieve it
                    hWnd = GetDlgItem(hDlg, IDC_EDIT2);
                    int siz = (int) SendMessage(hWnd, WM_GETTEXTLENGTH, (WPARAM) 0, (LPARAM) 0);
                    TCHAR *text = (TCHAR *) MemPtrNew((++siz)*sizeof(TCHAR));
                    SendMessage(hWnd, WM_GETTEXT, (WPARAM) siz, (LPARAM) text);
                    if (!(*text))
                        FrmAlert(altCompute, NULL);
                    else {
                        CodeStack *stack;
                        CError err;

                        stack = text_to_stack(text, &err);
                        MemPtrFree(text);
                        if (!err) {
                            err = stack_compute(stack);
                            if (!err)
                                err = stack_get_val(stack, &cplxPopup_value, complex);
                            stack_delete(stack);
                        }
                        if (err) {
                            alertErrorMessage(err);
                        } else {
                            EndDialog(hDlg, 1);
                        }
                    }
                    processed = TRUE;
                }
            } else if (notify_src == IDC_VAR) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndVMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            } else if (notify_src == IDC_USERF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_LARGE_MENU), hDlg, WndFMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }  else if (notify_src == IDC_CALCF) {
                if (notify_msg == BN_CLICKED) {
                    HWND hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SLIM_MENU), hDlg, WndfMenu,
                                                  (LPARAM) hDlg);
                    processed = TRUE;
                }
            }
            }
            break;

        case WM_APP_ENDVMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            SendMessage(hWnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) lParam);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_APP_ENDFMENU:
        case WM_APP_ENDFCMENU:
            {
            HWND hWnd = GetDlgItem(hDlg, IDC_EDIT2);
            main_insert(cur_skin, hWnd, (TCHAR *) lParam, false, true, false, NULL);
            mfree(lParam);
            SetFocus(hWnd);
            }
            processed = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            processed = TRUE;
            break;
    }

    if (processed) // Send back message specific return value
        SetWindowLong(hDlg, DWL_MSGRESULT, res);
    return (processed);
}

/********************************************************************************
 *  FUNCTION: repeater(HWND, UINT, UINT, DWORD)                                 *
 *  Call handler for the repeater timer.                                        *
 ********************************************************************************/
static VOID CALLBACK repeater(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime) {
    KillTimer(NULL, timer);
    int repeat = core_repeat();
    if (repeat != 0)
        timer = SetTimer(NULL, 0, repeat == 1 ? 200 : 100, repeater);
    else
        timer = SetTimer(NULL, 0, 250, timeout1);
}

/********************************************************************************
 *  FUNCTION: timeout1(HWND, UINT, UINT, DWORD)                                 *
 *  Call handler for when the repeater timer passes the first 1/4 second.       *
 ********************************************************************************/
static VOID CALLBACK timeout1(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime) {
    KillTimer(NULL, timer);
    if (ckey != KEY_NONE) {
        core_keytimeout1();
        timer = SetTimer(NULL, 0, 1750, timeout2);
    } else   timer = 0;
}

/********************************************************************************
 *  FUNCTION: timeout2(HWND, UINT, UINT, DWORD)                                 *
 *  Call handler for when the repeater timer goes beyond 2 seconds.             *
 ********************************************************************************/
static VOID CALLBACK timeout2(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime) {
    KillTimer(NULL, timer);
    if (ckey != KEY_NONE)
        core_keytimeout2();
    timer = 0;
}

/********************************************************************************
 *  FUNCTION: matchBracket_timeout(HWND, UINT, UINT, DWORD)                     *
 *  Call handler for highlighting / hiding matching bracket.                    *
 ********************************************************************************/
static VOID CALLBACK matchBracket_timeout(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime) {
    static unsigned long start;

    if (BrH_timer) {
        KillTimer(NULL, BrH_timer);
        BrH_timer = 0;
    }
    if (BrH == 2) {
        unsigned long i, j, k = 1, end;
        TCHAR char1, char2, *text;

        cur_skin->get_select_text(NULL, &start, &end);
        if((start != end) || (end == 0)) {
           goto exit;
        }

        j = start-1;
        text = cur_skin->get_input_text();
        if (text[j] == _T(')')) {
            char1 = _T(')');
            char2 = _T('(');
            i = -1;
            end = 0;
        } else if (text[j] == _T('(')) {
            char1 = _T('(');
            char2 = _T(')');
            i = 1;
            end = StrLen(text);
        } else if (text[j] == _T(']')) {
            char1 = _T(']');
            char2 = _T('[');
            i = -1;
            end = 0;
        } else if (text[j] == _T('[')) {
            char1 = _T('[');
            char2 = _T(']');
            i = 1;
            end = StrLen(text);
        } else {
            goto exit;
        }

        while ((j != end) && k){
            j += i;
            if (text[j] == char1)
                k++;
            else if (text[j] == char2)
                k--;
        }

        if (k)
            goto exit;
        else {
            cur_skin->select_input_text(NULL, j, j+1);
            BrH=1;
            BrH_timer = SetTimer(NULL, 0, 400, matchBracket_timeout);
        }
    } else if (BrH == 1) {
        // Restore cursor position
        cur_skin->select_input_text(NULL, start, start);
    exit:
        BrH=0;
    }
}
