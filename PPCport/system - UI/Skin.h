/*****************************************************************************
 * EasyCalc -- a scientific calculator
 * Copyright (C) 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
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
/* skin.h : Skin object for handling the skins of the calculator.
 *****************************************************************************/
#ifndef SKIN_H
#define SKIN_H 1

#include <windows.h>
#include "core/core_globals.h"
#include "compat/PalmOS.h"
#include "defuns.h"

#define GR_WHITE 0x00FFFFFF

typedef struct {
    int x, y;
} SkinPoint;

typedef struct {
    int x, y, width, height;
} SkinRect;

typedef struct {
    int code, shifted_code;
    SkinRect sens_rect;
    SkinRect disp_rect;
    SkinPoint src;
} SkinKey;

typedef struct {
        unsigned char r, g, b, pad;
} SkinColor;

#define IMGTYPE_MONO 1
#define IMGTYPE_GRAY 2
#define IMGTYPE_COLORMAPPED 3
#define IMGTYPE_TRUECOLOR 4

#define INPUTFONT_SIZE      14
#define BIGFONT_SIZE        18
#define SMALLFONT_SIZE      12
#define VERYSMALLFONT_SIZE  11

#define KEYMAP_MAX_MACRO_LENGTH 31
typedef struct {
    bool ctrl;
    bool alt;
    bool shift;
    bool cshift;
    int keycode;
    unsigned char macro[KEYMAP_MAX_MACRO_LENGTH + 1];
} keymap_entry;

#define SKIN_MAX_MACRO_LENGTH 31
typedef struct _SkinMacro {
    int code;
    unsigned char macro[SKIN_MAX_MACRO_LENGTH + 1];
    struct _SkinMacro *next;
} SkinMacro;

typedef struct {
    bool      exists;
    SkinRect  disp_rect;
    SkinPoint src;
    SkinPoint sel;
} SkinAnnunciator;

typedef enum {
    track_set,
    track_move,
    track_off
} TtrackAction;

typedef enum {
	COPYRESULT=0,	  
	VARSAVEAS,
	DEFMGR,
	GUESSIT,
	TODEGREE,
	TORADIAN,
	TOCIS,
	TOGONIO,
	ENGDISPLAY,
    SCIDISPLAY,
    NORMDISPLAY,
	TODEGREE2,
    TOGRAD
} resSelection;
#define SELECTION_COUNT 13

typedef enum {
	grphStart=0,	  
	grphInitComplete,
	grphZoneStarted,
	grphZoneComplete,
    grphViewStart,
	grphViewStarted,
	grphEnd
} t_graphState;

#define RESULTAREA_SIZE 128
#define INPUTAREA_SIZE  2048

#define MAX_GRFUNCS 6   // Max number of functions which can be graphed on screen

class Skin {
protected:
    SkinRect        skin;                   // Description of the skin on screen
    // Used while loading a skin; specifies how much to enlarge it.
    // This is necessary to force low-res skins to fill high-res screens.

    SkinPoint       display_loc;            // Description of the display in the skin
    int             display_w, display_h, display_basey, zonebmp_basey;
    SkinPoint       view_loc;               // Description of the wide view in the graph skin
    int             view_w, view_h, view_basey, viewbmp_basey;
    SkinPoint       display_scale;
    SkinPoint       view_scale;
    RECT            viewRect;               // Describes view area
    RECT            zoneOnView, tempZoneOnView;
    RECT            zv_intersect, tempzv_intersect;
    bool            intersect, tempIntersect;
    COLORREF        display_bg, display_fg;
    COLORREF        graphColors[MAX_GRFUNCS+3]; // 6 line colors + axis, grids and background
    HPEN            graphPens[MAX_GRFUNCS+3];   // 6 line colors + axis, grids and background
    HDC             resultDC;
    HDC             displayDC;
    HDC             zoneDC;                 // For keeping graph bitmap in memory
    HDC             viewDC;                 // For keeping wide view bitmap in memory
    HGDIOBJ         oldResultObj, oldZoneObj, oldViewObj;
    HBITMAP         resultBmp;
    HBITMAP         zoneBmp;
    HBITMAP         viewBmp;
    t_graphState    graphState;             // Tells where we are during graph drawing
    double          viewX, viewY, span, wonz_xr, wonz_yr;
    double          selx0, sely0, calcx, calcy, calcr;
    int             selArea, prevx, prevy, prevsx, prevsy, selsx0, selsy0, selprevsx, selprevsy;
    RECT            cross_rcx, cross_rcy, sel_rc;
    HRGN            zone_rgn, zgph_rgn, zcomplement_rgn;
    HRGN            view_rgn, vgph_rgn, vcomplement_rgn;
    HFONT           hFontBig_display;
    HFONT           hFontSmall_display;
    HFONT           hFontVerySmall_display;
    HFONT           hFont_input;
//    HANDLE          graph_thread;           // Handle to the graph drawing thread
    HPEN            hPenSelZone;
    SkinKey        *keylist;                // Array of keys in the skin
    int             nkeys;                  // Number of keys in the skin
    int             keys_cap;               // Number of allocated keys from memory
    SkinMacro      *macrolist;              // Array of macros specified in the layout file
    SkinAnnunciator annunciators[NB_ANNUN]; // Array of annunciators

    FILE *external_file;                    // Handle for external file, if not an internal file
                                            // NULL if internal file.
    const unsigned char *builtin_file;      // Handle for the internal file
    long  builtin_length;                   // Length of internal file
    long  builtin_pos;                      // Cur pos in the internal file

    int              skin_type;
    int              skin_ncolors;
    const SkinColor *skin_colors;
    int              skin_y;
    unsigned char   *skin_bitmap;           // Contains the bitmap of the skin
    int              skin_bytesperline;

    BITMAPINFOHEADER *skin_header;
    HBITMAP           skin_dib;
    unsigned char *disp_bitmap;             // Source bitmap to clear display parts
    int disp_bytesperline;
    bool display_enabled;                   // Enable or disable screen repaints.
    TCHAR inputText[INPUTAREA_SIZE];        // Used to get the input area string
    int   pow_pos;                          // Current position when writing power text.
    bool  result_size_recompute;            // Ask for recalculation of result_size
    SIZE  result_size;                      // Result "would-be" size on display without clipping
    TCHAR work_dispResult[RESULTAREA_SIZE]; // Working area for pow results
    bool landscape;

    keymap_entry *keymap;
    int           keymap_length;

    keymap_entry *parse_keymap_entry(char *line, int lineno);
    int open(const TCHAR *skinname, const TCHAR *basedir, int open_layout);
    void close();
    int gets(char *buf, int buflen);
    int make_dib(HDC hdc);

public:
    bool graph;                 // Tells if this is a graphic display skin
    bool recalc, recalc_zone, recalc_view;   // Signal to recompute graph or only parts of it on next repaint
    bool graphComplete, zoneComplete, viewComplete, continueOnView;
    int  skin_width,
         skin_height;
    int  magnification;
    HBRUSH editBgBrush;
    int  curve_nb;               // Curve being tracked
    bool is_cross, is_selecting; // Type of selection on curve
    double param0, param;        // Start and end of selection on input parameter.
                                 // If only a cross, value is in param, and param0 == NaN

    Skin();     // Constructor
    ~Skin();    // Destructor

    int load(TCHAR *skinname, const TCHAR *basedir, int width, int height, HDC hdc);
    int getchar(void);
    void rewind(void);
    int init_image(int type, int ncolors, const SkinColor *colors,
                        int width, int height);
    void put_pixels(unsigned const char *data);
    void finish_image(HDC hdc);

    void repaint(HDC hdc, HDC memdc);
    void repaint_annunciator(HDC hdc, HDC memdc, int which, int state);
    void find_key(int x, int y, bool cshift, int *skey, int *ckey);
    int find_skey(int ckey);
    unsigned char *find_macro(int ckey);
    unsigned char *keymap_lookup(int keycode, bool ctrl, bool alt, bool shift, bool cshift, bool *exact);
    void repaint_key(HDC hdc, HDC memdc, int key, int state);
    bool tapOnResult(HDC hdc, int x, int y, TtrackAction action);
    void repaint_result(HDC hdc);
    COLORREF getDisplayFgColor(void);
    COLORREF getDisplayBgColor(void);
    void create_input_area(HWND hwnd, HINSTANCE hinst);
    void clipboardAction (void *hwnd, int action);
    void select_input_text(void *hwnd);
    void select_input_text(void *hwnd, unsigned long start, unsigned long end);
    void get_select_text(void *hwnd, unsigned long *start, unsigned long *end);
    unsigned long get_insert_pos(void *hwnd);
    void set_insert_pos(void *hwnd, unsigned long pos);
    void back_delete(void *hwnd);
    void insert_input_text(void *hwnd, const TCHAR *text);
    void set_input_text(void *hwnd, const TCHAR *text);
    TCHAR *get_input_text(void);
    void print_result(void *hWnd_p, TCHAR *res_text);
    void print_result(HDC hdc);
    void paint_result(HDC hdc);
    HBITMAP allocResult(HDC hdc, LONG cx, LONG cy);
    void deleteResult(void);
    void erase_result(HDC hdc);
    void print_resultpow(void *hWnd_p, TCHAR *res_text);
    void print_resultpow(HDC hdc);
    void paint_resultpow(HDC hdc);
    void paint_resultpow_rec(HDC hdc, bool smallf, TCHAR *text);
    void paint_result_piece(HDC hdc, bool smallf, TCHAR *res_piece, int len);
    void scroll_result_left(HWND hWnd);
    void scroll_result_right(HWND hWnd);
    TCHAR *get_result(void);
    void clipCopy_result(void);
    void resultActionsPopup(resSelection choices[], int length, void *hWnd_p);
    void historyAddActionPopup(TCHAR *description, void *hWnd);
    void varSavePopup(TCHAR *choices[], int length, void *hWnd);
    void varMgrPopup(TCHAR *choices[], int length, void *hWnd);
    void varDefReset(void *hWnd);
    void varDefList(TCHAR *name, TCHAR *deftext, int itemNum, void *hWnd);
    void display_set_enabled(bool enable);

    void dragGraph(HDC hdc, int area, int dx, int dy);
    void showOnGraph(HDC hdc, double param);
    void tapOnGraph(HDC hdc, int area, int x, int y, TtrackAction action);
    void repaint_grphVal(HDC hdc, int annun);

    void refreshWonzRatio(HDC hdc);
    void scaleGraph(HDC hdc, int area, double scale_in);
    void centerView(HDC hdc);
    void refreshView(HDC hdc);
    void refreshViewAndTrigRecalc(HDC hdc);
    void moveGraph(HDC hdc, int area, double x, double y);
    void normZone(HDC hdc);
    void selPen(int i);
    void drawline(int x1, int y1, int x2, int y2);
    void finishline(int color);
    void trigRecalcGraph(void);
    void trigRecalcZone(void);
    void trigRecalcView(void);
    void resetGraphSel(void);
    void resetZone(void);
    void resetView(void);
    void switchSelGraph(HDC hdc);
    int initGraph(void);
    int initView(void);
    void clearZoneGraph(void);
    void drawZoneGraph(void);
    void clearViewGraph(void);
    DWORD drawgraph_async(void);
//    DWORD drawgraph_thread(void);
    int draw_graph(HDC hdc);
    void stop_graph(void);
    void draw_tempZoneOnView(HDC hdc);
    void set_tempZoneOnView(int sx, int sy);
    void draw_zoneOnView(HDC hdc);
    void calc_zoneOnView(void);
};

void skin_init(void);

#endif