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
/* Skin.cpp : Methods for the Skin object of the calculator.
 *****************************************************************************/

#include "stdafx.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compat/PalmOS.h"
#include "compat/Lang.h"
#include "Skin.h"
#include "shell_loadimage.h"
#include "core/core_display.h"
#include "core/Main.h"
#include "EasyCalc.h"
#include "core/Graph.h"
#include "core/mlib/fp.h"
#include "compat/MathLib.h"
#include "core/mlib/mathem.h"

/*-------------------------------------------------------------------------------
 - Constants.                                                                   -
 -------------------------------------------------------------------------------*/
// Which file to open
#define OPEN_IMAGE  0
#define OPEN_LAYOUT 1

/**********************************************************/
/* Language objects, as manipulated by EasyCall.cpp.      */
/**********************************************************/
extern LibLang *libLang;

static int skin_count;
static const TCHAR *skin_name[1];
static long skin_layout_size[1];
static unsigned char *skin_layout_data[1];
static long skin_bitmap_size[1];
static unsigned char *skin_bitmap_data[1];

static TCHAR g_dispResult[RESULTAREA_SIZE];      // Contains the result area string
static int   g_result_len;                       // Length of the result area string
static bool  g_result_pow;                       // Signals that we are mixing small and big fonts (exponential display)
//static int   g_scroll_result;                    // DT_LEFT if scrolled left (left align), DT_RIGHT if scrolled right (right align)
static int   g_scroll_result;                    // Position of display window from right of BMP
/*-------------------------------------------------------------------------------
 - Initialisation of globals.                                                   -
 -------------------------------------------------------------------------------*/
void skin_init (void) {
    *g_dispResult = 0; // Empty string
    g_result_len = 0;
    g_result_pow = false;
//    g_scroll_result = DT_RIGHT;
    g_scroll_result = 0;
}

/*-------------------------------------------------------------------------------
 - Constructor and destructor.                                                  -
 -------------------------------------------------------------------------------*/
Skin::Skin() {
    for (int i=0 ; i<NB_ANNUN ; i++) {
        annunciators[i].exists = false;
    }
    keylist = NULL;
    nkeys = 0;
    keys_cap = 0;
    macrolist = NULL;
    skin_colors = NULL;
    skin_bitmap = NULL;
    skin_header = NULL;
    skin_dib = NULL;
    editBgBrush = NULL;
    disp_bitmap = NULL;
    keymap = NULL;
    keymap_length = 0;
    result_size_recompute = true;
    zone_rgn = zgph_rgn = zcomplement_rgn = view_rgn = vgph_rgn = vcomplement_rgn = NULL;
    hFontBig_display = hFontSmall_display = hFontVerySmall_display = hFont_input = NULL;
//    graph_thread = NULL;
    hPenSelZone = NULL;
    displayDC = resultDC = zoneDC = viewDC = NULL;
    oldZoneObj = oldViewObj = NULL;
    zoneBmp = viewBmp = NULL;
    graph = graphComplete = zoneComplete = viewComplete = intersect = recalc_zone
          = recalc_view = false;
    recalc = true;
    display_enabled = true;
    for (int i=0 ; i<MAX_GRFUNCS+3 ; i++) {
        graphColors[i] = WinRGBToIndex(&graphRGBColors[i]);
        graphPens[i] = NULL;
    }
    selArea = curve_nb = prevsx = prevsy = selprevsx = -1; // No curve selected
                                                           // No area with selection in
    is_cross = is_selecting = false;
    param0 = param = calcx = calcy = calcr = NaN; // No cross
}

Skin::~Skin() {
    stop_graph();

    if (keylist != NULL) {
        mfree(keylist);
        keylist = NULL;
    }
    if (skin_header != NULL) {
        mfree(skin_header);
        skin_header = NULL;
    }
    if (disp_bitmap != NULL) {
        mfree(disp_bitmap);
        disp_bitmap = NULL;
    }
    if (skin_dib != NULL) {
        DeleteObject(skin_dib);
        skin_dib = NULL;
    }
    if (skin_bitmap != NULL) {
        mfree(skin_bitmap);
        skin_bitmap = NULL;
    }
    if (zone_rgn != NULL)
        DeleteObject(zone_rgn);
    if (zgph_rgn != NULL)
        DeleteObject(zgph_rgn);
    if (zcomplement_rgn != NULL)
        DeleteObject(zcomplement_rgn);
    if (view_rgn != NULL)
        DeleteObject(view_rgn);
    if (vgph_rgn != NULL)
        DeleteObject(vgph_rgn);
    if (vcomplement_rgn != NULL)
        DeleteObject(vcomplement_rgn);
    if (hFontBig_display != NULL)
        DeleteObject(hFontBig_display);
    if (hFontSmall_display != NULL)
        DeleteObject(hFontSmall_display);
    if (hFontVerySmall_display != NULL)
        DeleteObject(hFontVerySmall_display);
    if (hFont_input != NULL)
        DeleteObject(hFont_input);
    if (hPenSelZone != NULL)
        DeleteObject(hPenSelZone);
    if (editBgBrush != NULL)
        DeleteObject(editBgBrush);

    if (resultDC != NULL) {
        deleteResult();
        DeleteDC(resultDC);
        resultDC = NULL;
    }
    if (displayDC != NULL)
        ReleaseDC(g_hWnd, displayDC);
    if (zoneDC != NULL) {
        if (oldZoneObj != NULL) {
            SelectObject(zoneDC, oldZoneObj);
            oldZoneObj = NULL;
            if (zoneBmp != NULL) {
                DeleteObject(zoneBmp);
                zoneBmp = NULL;
            }
        }
        DeleteDC(zoneDC);
        zoneDC = NULL;
    }
    if (viewDC != NULL) {
        if (oldViewObj != NULL) {
            SelectObject(viewDC, oldViewObj);
            oldViewObj = NULL;
            if (viewBmp != NULL) {
                DeleteObject(viewBmp);
                viewBmp = NULL;
            }
        }
        DeleteDC(viewDC);
        viewDC = NULL;
    }
    for (int i=0 ; i<MAX_GRFUNCS+3 ; i++) {
        if (graphPens[i] != NULL)
            DeleteObject(graphPens[i]);
    }
}

/*-------------------------------------------------------------------------------
 - Methods.                                                                     -
 -------------------------------------------------------------------------------*/
/********************************************************************************
 * Keymap parser.                                                               *
 ********************************************************************************/
/* private */
keymap_entry *Skin::parse_keymap_entry (char *line, int lineno) {
    char *p;
    static keymap_entry entry;

    p =  strchr(line, '#');
    if (p != NULL)
        *p = 0;
    p = strchr(line, '\n');
    if (p != NULL)
        *p = 0;
    p = strchr(line, '\r');
    if (p != NULL)
        *p = 0;

    p = strchr(line, ':');
    if (p != NULL) {
        char *val = p + 1;
        char *tok;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;
        bool cshift = false;
        int keycode = 0;
        int done = 0;
        unsigned char macro[KEYMAP_MAX_MACRO_LENGTH + 1];
        int macrolen = 0;

        /* Parse keycode */
        *p = 0;
        tok = strtok(line, " \t");
        while (tok != NULL) {
            if (done) {
                fprintf(stderr, "Keymap, line %d: Excess tokens in key spec.\n", lineno);
                return NULL;
            }
            if (_stricmp(tok, "ctrl") == 0)
                ctrl = true;
            else if (_stricmp(tok, "alt") == 0)
                alt = true;
            else if (_stricmp(tok, "shift") == 0)
                shift = true;
            else if (_stricmp(tok, "cshift") == 0)
                cshift = true;
            else {
                char *endptr;
                long k = strtol(tok, &endptr, 10);
                if (k < 1 || *endptr != 0) {
                    fprintf(stderr, "Keymap, line %d: Bad keycode.\n", lineno);
                    return NULL;
                }
                keycode = k;
                done = 1;
            }
            tok = strtok(NULL, " \t");
        }
        if (!done) {
            fprintf(stderr, "Keymap, line %d: Unrecognized keycode.\n", lineno);
            return NULL;
        }

        /* Parse macro */
        tok = strtok(val, " \t");
        while (tok != NULL) {
            char *endptr;
            long k = strtol(tok, &endptr, 10);
            if (*endptr != 0 || k < 1 || k > 255) {
                fprintf(stderr, "Keymap, line %d: Bad value (%s) in macro.\n", lineno, tok);
                return NULL;
            } else if (macrolen == KEYMAP_MAX_MACRO_LENGTH) {
                fprintf(stderr, "Keymap, line %d: Macro too long (max=%d).\n", lineno, KEYMAP_MAX_MACRO_LENGTH);
                return NULL;
            } else
                macro[macrolen++] = (unsigned char) k;
            tok = strtok(NULL, " \t");
        }
        macro[macrolen] = 0;

        entry.ctrl = ctrl;
        entry.alt = alt;
        entry.shift = shift;
        entry.cshift = cshift;
        entry.keycode = keycode;
        strcpy((char *) entry.macro, (const char *) macro);
        return (&entry);
    } else   return (NULL);
}


/********************************************************************************
 * Open a skin file (LAYOUT or IMAGE).                                          *
 * Returns true if could find and open the file (whether internal or external). *
 ********************************************************************************/
/* private */
int Skin::open (const TCHAR *skinname, const TCHAR *basedir, int open_layout) {
    int i;
    TCHAR namebuf[1024];

    /* Look for built-in skin first */
    skin_count = 0;
    for (i = 0; i < skin_count; i++) {
        if (_tcscmp(skinname, skin_name[i]) == 0) {
            external_file = NULL;
            builtin_pos = 0;
            if (open_layout) {
                builtin_length = skin_layout_size[i];
                builtin_file = skin_layout_data[i];
            } else {
                builtin_length = skin_bitmap_size[i];
                builtin_file = skin_bitmap_data[i];
            }
            return (1);
        }
    }

    /* name did not match a built-in skin; look for file */
    _stprintf(namebuf, _T("%s\\%s.%s"), basedir, skinname,
                       open_layout ? _T("layout") : _T("gif"));
    external_file = _tfopen(namebuf, _T("rb"));
    return (external_file != NULL);
}

/********************************************************************************
 * Close the open skin file.                                                    *
 ********************************************************************************/
/* private */
void Skin::close (void) {
    if (external_file != NULL)   fclose(external_file);
}

/********************************************************************************
 * Read a char from the open skin file.                                         *
 ********************************************************************************/
int Skin::getchar (void) {
    if (external_file != NULL)
        return (fgetc(external_file));
    else if (builtin_pos < builtin_length)
        return (builtin_file[builtin_pos++]);
    return (EOF);
}

/********************************************************************************
 * Read a line from the open skin file into buf of size buflen.                 *
 * Detects EOF. Returns true if something read or not EOF.                      *
 ********************************************************************************/
/* private */
int Skin::gets (char *buf, int buflen) {
    int p       = 0;
    int eof     = -1;
    int comment = 0;

    while (p < buflen - 1) {
        int c = this->getchar();
        if (eof == -1)
            eof = (c == EOF);
        if ((c == EOF) || (c == '\n') || (c == '\r'))
            break;
        /* Remove comments */
        if (c == '#')
            comment = 1;
        if (comment)
            continue;
        /* Suppress leading spaces */
        if ((p == 0) && (isspace(c)))
            continue;
        buf[p++] = c;
    }
    buf[p++] = 0;
    return ((p > 1) || !eof);
}

/********************************************************************************
 * Come back to beginning of open skin file.                                    *
 ********************************************************************************/
void Skin::rewind (void) {
    if (external_file != NULL)
        fseek(external_file, 0, SEEK_SET);
    else
        builtin_pos = 0;
}

/********************************************************************************
 * Load skin descriptions in the object.                                        *
 * Return:  0 if all ok.                                                        *
 *         -1 if could not find the layout file.                                *
 *         -2 if could not load the gif file.                                   *
 ********************************************************************************/
int Skin::load (TCHAR *skinname, const TCHAR *basedir, int width, int height, HDC hdc) {
    char line[1024];
    int success;
    int size;
    int kmcap = 0;
    int lineno = 0;
    bool prev_landscape = landscape;

    graph = (towupper(skinname[_tcslen(skinname)-1]) == _T('G'));
    view_loc.x = view_loc.y = -1; // To detect if there is a wide view zone

    /*************************/
    /* Load skin description */
    /*************************/
    if (!this->open(skinname, basedir, OPEN_LAYOUT)) {
//        goto fallback_on_1st_builtin_skin;
        return (-1);
    }

    if (keylist != NULL)
        mfree(keylist);
    keylist = NULL;
    nkeys = 0;
    keys_cap = 0;

    while (macrolist != NULL) {
        SkinMacro *m = macrolist->next;
        mfree(macrolist);
        macrolist = m;
    }

    if (keymap != NULL)
        mfree(keymap);
    keymap = NULL;
    keymap_length = 0;

    landscape = false;

    while (this->gets(line, 1024)) {
        lineno++;
        if (*line == 0)
            continue;
        if (_strnicmp(line, "skin:", 5) == 0) {
            int x, y, skin_width, skin_height;
            if (sscanf(line + 5, " %d,%d,%d,%d", &x, &y, &skin_width, &skin_height) == 4){
                skin.x = x;
                skin.y = y;
                skin.width = skin_width;
                skin.height = skin_height;
            }
        } else if (_strnicmp(line, "display:", 8) == 0) {
            int x, y, xscale, yscale;
            unsigned long bg, fg;
            if (sscanf(line + 8, " %d,%d %d %d %lx %lx", &x, &y,
                                 &xscale, &yscale, &bg, &fg) == 6) {
                display_loc.x = x;
                display_loc.y = y;
                display_scale.x = xscale;
                display_scale.y = yscale;
                // Set the foreground and background colors in Windows
                // COLORREF format.
                graphColors[8] = display_bg = ((bg >> 16) & 255) | (bg & 0x0FF00) | ((bg & 255) << 16);
                graphColors[6] = display_fg = ((fg >> 16) & 255) | (fg & 0x0FF00) | ((fg & 255) << 16);
            }
        } else if (_strnicmp(line, "wideview:", 9) == 0) {
            int x, y, xscale, yscale;
            unsigned long bg, fg;
            if (sscanf(line + 9, " %d,%d %d %d %lx %lx", &x, &y,
                                 &xscale, &yscale, &bg, &fg) == 6) {
                view_loc.x = x;
                view_loc.y = y;
                view_scale.x = xscale;
                view_scale.y = yscale;
                // Set the foreground and background colors in Windows
                // COLORREF format.
                graphColors[8] = display_bg = ((bg >> 16) & 255) | (bg & 0x0FF00) | ((bg & 255) << 16);
                graphColors[6] = display_fg = ((fg >> 16) & 255) | (fg & 0x0FF00) | ((fg & 255) << 16);
                // Initialize view position and size
                centerView(NULL);
            }
        } else if (_strnicmp(line, "graph:", 6) == 0) {
            int c;
            unsigned long color;
            if (sscanf(line + 6, " %d %lx", &c, &color) == 2) {
                // Set color in Windows COLORREF format.
                c--;
                if ((c >= 0) && (c <= 8))
                    graphPrefs.colors[c]
                        = graphColors[c]
                        = ((color >> 16) & 255) | (color & 0x0FF00) | ((color & 255) << 16);
            }
        } else if (_strnicmp(line, "key:", 4) == 0) {
            char keynumbuf[20];
            int keynum, shifted_keynum;
            int sens_x, sens_y, sens_width, sens_height;
            int disp_x, disp_y, disp_width, disp_height;
            int act_x, act_y;
            if (sscanf(line + 4, " %s %d,%d,%d,%d %d,%d,%d,%d %d,%d",
                                 keynumbuf,
                                 &sens_x, &sens_y, &sens_width, &sens_height,
                                 &disp_x, &disp_y, &disp_width, &disp_height,
                                 &act_x, &act_y) == 11) {
                int n = sscanf(keynumbuf, "%d,%d", &keynum, &shifted_keynum);
                if (n > 0) {
                    if (n == 1)
                        shifted_keynum = keynum;
                    // Special code for 'shift' key
                    if ((keynum >= KEY_SHIFT) && (keynum < BUTTON_COUNT)
                        && (shifted_keynum >= KEY_SHIFT) && (shifted_keynum < BUTTON_COUNT)) {
                        SkinKey *key;
                        if (nkeys == keys_cap) {
                            keys_cap += 50;
                            keylist = (SkinKey *) mrealloc(keylist, keys_cap * sizeof(SkinKey));
                            // TODO - handle memory allocation failure
                        }
                        key = keylist + nkeys;
                        key->code = keynum;
                        key->shifted_code = shifted_keynum;
                        key->sens_rect.x = sens_x;
                        key->sens_rect.y = sens_y;
                        key->sens_rect.width = sens_width;
                        key->sens_rect.height = sens_height;
                        key->disp_rect.x = disp_x;
                        key->disp_rect.y = disp_y;
                        key->disp_rect.width = disp_width;
                        key->disp_rect.height = disp_height;
                        key->src.x = act_x;
                        key->src.y = act_y;
                        nkeys++;
                    }
                }
            }
        } else if (_strnicmp(line, "landscape:", 10) == 0) {
            int ls;
            if (sscanf(line + 10, " %d", &ls) == 1)
                landscape = ls != 0;
        } else if (_strnicmp(line, "macro:", 6) == 0) {
            char *tok = strtok(line + 6, " ");
            int len = 0;
            SkinMacro *macro = NULL;
            while (tok != NULL) {
                char *endptr;
                long n = strtol(tok, &endptr, 10);
                if (*endptr != 0) {
                    /* Not a proper number; ignore this macro */
                    if (macro != NULL) {
                        mfree(macro);
                        macro = NULL;
                        break;
                    }
                }
                if (macro == NULL) {
                    if (n < 38 || n > 255)
                        /* Macro code out of range; ignore this macro */
                        break;
                    macro = (SkinMacro *) mmalloc(sizeof(SkinMacro));
                    // TODO - handle memory allocation failure
                    macro->code = n;
                } else if (len < SKIN_MAX_MACRO_LENGTH) {
                    if (n < 1 || n > NB_KEYS) {
                        /* Key code out of range; ignore this macro */
                        mfree(macro);
                        macro = NULL;
                        break;
                    }
                    macro->macro[len++] = (unsigned char) n;
                }
                tok = strtok(NULL, " ");
            }
            if (macro != NULL) {
                macro->macro[len++] = 0;
                macro->next = macrolist;
                macrolist = macro;
            }
        } else if (_strnicmp(line, "annunciator:", 12) == 0) {
            int annnum;
            int disp_x, disp_y, disp_width, disp_height;
            int act_x, act_y, sel_x, sel_y;
            if (sscanf(line + 12, " %d %d,%d,%d,%d %d,%d %d,%d",
                                  &annnum,
                                  &disp_x, &disp_y, &disp_width, &disp_height,
                                  &act_x, &act_y, &sel_x, &sel_y) == 9) {
                if ((annnum >= 1) && (annnum <= NB_ANNUN)) {
                    SkinAnnunciator *ann = annunciators + (annnum - 1);
                    ann->exists = true;
                    ann->disp_rect.x = disp_x;
                    ann->disp_rect.y = disp_y;
                    ann->disp_rect.width = disp_width;
                    ann->disp_rect.height = disp_height;
                    ann->src.x = act_x;
                    ann->src.y = act_y;
                    ann->sel.x = sel_x;
                    ann->sel.y = sel_y;
                }
            }
        } else if (strchr(line, ':') != 0) {
            keymap_entry *entry = parse_keymap_entry(line, lineno);
            if (entry != NULL) {
                if (keymap_length == kmcap) {
                    kmcap += 50;
                    keymap = (keymap_entry *) mrealloc(keymap, kmcap * sizeof(keymap_entry));
                    // TODO - handle memory allocation failure
                }
                memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
            }
        }
    }

    if (display_scale.x == 0)
        landscape = false;

    this->close();

    /********************************************************************/
    /* Compute optimum magnification level, and adjust skin description */
    /********************************************************************/
    int xs = width / skin.width;
    int ys = height / skin.height;
    magnification = (xs < ys) ? xs : ys;
    if (magnification < 1)
        magnification = 1;
    else if (magnification > 1) {
        if (magnification > 4)
            // In order to support magnifications of more than 4,
            // the monochrome pixel-replication code in skin_put_pixels()
            // needs to be modified; currently, it only supports magnifications
            // of 1, 2, 3, and 4.
            magnification = 4;
        skin.x *= magnification;
        skin.y *= magnification;
        skin.width *= magnification;
        skin.height *= magnification;
        display_loc.x *= magnification;
        display_loc.y *= magnification;
        view_loc.x *= magnification;
        view_loc.y *= magnification;
        if (display_scale.x == 0)
            // This is the special hack to get the most out of the QVGA's
            // 240-pixel width by doubling 4 out of every 6 pixels; if we're
            // on a larger screen, fall back on a tidy integral scale factor
            // and just leave some screen space unused.
//            display_scale.x = (int) (magnification * 1.67);
            display_scale.x = magnification;
        else
            display_scale.x *= magnification;
        display_scale.y *= magnification;
        view_scale.x *= magnification;
        view_scale.y *= magnification;
        int i;
        for (i = 0; i < nkeys; i++) {
            SkinKey *key = keylist + i;
            key->sens_rect.x *= magnification;
            key->sens_rect.y *= magnification;
            key->sens_rect.width *= magnification;
            key->sens_rect.height *= magnification;
            key->disp_rect.x *= magnification;
            key->disp_rect.y *= magnification;
            key->disp_rect.width *= magnification;
            key->disp_rect.height *= magnification;
            key->src.x *= magnification;
            key->src.y *= magnification;
        }
        for (i=0 ; i<NB_ANNUN ; i++) {
            SkinAnnunciator *ann = annunciators + i;
            ann->disp_rect.x *= magnification;
            ann->disp_rect.y *= magnification;
            ann->disp_rect.width *= magnification;
            ann->disp_rect.height *= magnification;
            ann->src.x *= magnification;
            ann->src.y *= magnification;
            ann->sel.x *= magnification;
            ann->sel.y *= magnification;
        }
    }
    if (graph) {
        display_w = 200 * display_scale.x;
        display_h = 200 * display_scale.y;
        zonebmp_basey = display_h - 1;
        display_basey = display_loc.y + zonebmp_basey;
        view_w = 55 * display_scale.x;
        view_h = 55 * display_scale.y;
        viewbmp_basey = view_h - 1;
        view_basey = view_loc.y + viewbmp_basey;
        viewRect.top  = view_loc.y;
        viewRect.left = view_loc.x;
        viewRect.bottom = view_basey;
        viewRect.right  = viewRect.left + view_w - 1;
    } else if (landscape) {
        display_w = 16 * display_scale.x;
        display_h = 131 * display_scale.y;
    } else {
        if (display_scale.x == 0)
            display_w = 216;
        else
//            display_w = 131 * display_scale.x;
            display_w = 216 * display_scale.x;
        display_h = 16 * display_scale.y;
    }

    /********************/
    /* Load skin bitmap */
    /********************/
    if (!this->open(skinname, basedir, OPEN_IMAGE)) {
//        goto fallback_on_1st_builtin_skin;
        return (-2);
    }

    /* shell_loadimage() calls skin_getchar() and skin_rewind() to load the
     * image from the compiled-in or on-disk file; it calls skin_init_image(),
     * skin_put_pixels(), and skin_finish_image() to create the in-memory
     * representation.
     */
    success = shell_loadimage(this, hdc);
    this->close();

    if (success != 1) {
//        goto fallback_on_1st_builtin_skin;
        // If success is not 1, then it is:
        //  0 (error loading GIF file) => return -3
        // -1 (error allocating space for loading color map) => return -4
        // -2 (error allocating space for loading bitmap) => return -5
        // -3 (error allocating space for building screen scaled display) => return -6
        return (-3 + success);
    }

    /********************************/
    /* (Re)build the display bitmap */
    /********************************/
    if (disp_bitmap != NULL) {
        mfree(disp_bitmap);
        disp_bitmap = NULL;
    }

    int lcd_w, lcd_h;
    if (graph) {
        lcd_w = lcd_h = 200;
        disp_bytesperline = ((lcd_w * display_scale.x + 15) >> 3) & ~1;
    } else {
        if (landscape) {
            lcd_w = 16;
            lcd_h = 131;
        } else {
            lcd_w = 131;
            lcd_h = 16;
        }
        if (display_scale.x == 0)
            disp_bytesperline = 28;
        else
            disp_bytesperline = ((lcd_w * display_scale.x + 15) >> 3) & ~1;
    }
    size = disp_bytesperline * lcd_h * display_scale.y;
    disp_bitmap = (unsigned char *) mmalloc(size);
    if (disp_bitmap == NULL) // Memory allocation failure
        return (-7);
    memset(disp_bitmap, 255, size);

    return (0);
}

/********************************************************************************
 * Prepare the image structure.                                                 *
 ********************************************************************************/
int Skin::init_image (int type, int ncolors, const SkinColor *colors,
                      int width, int height) {
    if (skin_bitmap != NULL) {
        mfree(skin_bitmap);
        skin_bitmap = NULL;
    }
    if (skin_dib != NULL) {
        DeleteObject(skin_dib);
        skin_dib = NULL;
    }
    if (skin_header != NULL) {
        mfree(skin_header);
        skin_header = NULL;
    }

    skin_type = type;
    skin_ncolors = ncolors;
    skin_colors = colors;

    width *= magnification;
    height *= magnification;

    switch (type) {
        case IMGTYPE_MONO:
            skin_bytesperline = ((width + 15) >> 3) & ~1;
            break;
        case IMGTYPE_GRAY:
        case IMGTYPE_COLORMAPPED:
            skin_bytesperline = (width + 3) & ~3;
            break;
        case IMGTYPE_TRUECOLOR:
            skin_bytesperline = (width * 3 + 3) & ~3;
            break;
        default:
            return 0;
    }

    skin_bitmap = (unsigned char *) mmalloc(skin_bytesperline * height);
    skin_width = width;
    skin_height = height;
    skin_y = 0;
    return (skin_bitmap != NULL);
}

/********************************************************************************
 * Load image data into the skin bitmap.                                        *
 ********************************************************************************/
void Skin::put_pixels (unsigned const char *data) {
    unsigned char *dst = skin_bitmap + skin_y * skin_bytesperline;
    if (magnification == 1) {
        if (skin_type == IMGTYPE_MONO) {
            for (int i = 0; i < skin_bytesperline; i++) {
                unsigned char c = data[i];
                c = (c >> 7) | ((c >> 5) & 2) | ((c >> 3) & 4) | ((c >> 1) & 8)
                    | ((c << 1) & 16) | ((c << 3) & 32) | ((c << 5) & 64) | (c << 7);
                dst[i] = c;
            }
        } else if (skin_type == IMGTYPE_TRUECOLOR) {
            for (int i = 0; i < skin_width; i++) {
                data++;
                *dst++ = *data++;
                *dst++ = *data++;
                *dst++ = *data++;
            }
        } else
            memcpy(dst, data, skin_bytesperline);
        skin_y++;
    } else {
        if (skin_type == IMGTYPE_MONO) {
            if (magnification == 2) {
                int i = 0;
                while (true) {
                    unsigned char c = *data++;
                    dst[i++] = (((c << 6) & 64) | ((c << 3) & 16) | (c & 4) | ((c >> 3) & 1)) * 3;
                    if (i == skin_bytesperline)
                        break;
                    dst[i++] = (((c << 2) & 64) | ((c >> 1) & 16) | ((c >> 4) & 4) | (c >> 7)) * 3;
                    if (i == skin_bytesperline)
                        break;
                }
            } else if (magnification == 3) {
                int i = 0;
                while (true) {
                    unsigned char c = *data++;
                    dst[i++] = (((c << 5) & 32) | ((c << 1) & 4)) * 7 + ((c >> 2) & 1) * 3;
                    if (i == skin_bytesperline)
                        break;
                    dst[i++] = ((c << 5) & 128) + (((c << 1) & 16) | ((c >> 3) & 2)) * 7 + ((c >> 5) & 1);
                    if (i == skin_bytesperline)
                        break;
                    dst[i++] = (((c << 1) & 64) | ((c >> 3) & 8) | (c >> 7)) * 7;
                    if (i == skin_bytesperline)
                        break;
                }
            } else {
                // magnification must be 4 now; it's the highest value this bitmap
                // scaling code supports.
                int i = 0, j = 0;
                while (true) {
                    unsigned char c = *data++;
                    dst[i++] = (((c << 4) & 16) | ((c >> 1) & 1)) * 15;
                    if (i == skin_bytesperline)
                        break;
                    dst[i++] = (((c << 2) & 16) | ((c >> 3) & 1)) * 15;
                    if (i == skin_bytesperline)
                        break;
                    dst[i++] = ((c & 16) | ((c >> 5) & 1)) * 15;
                    if (i == skin_bytesperline)
                        break;
                    dst[i++] = (((c >> 2) & 16) | ((c >> 7) & 1)) * 15;
                    if (i == skin_bytesperline)
                        break;
                }
            }
        } else if (skin_type == IMGTYPE_TRUECOLOR) {
            unsigned char *p = dst;
            for (int i = 0; i < skin_width; i++) {
                unsigned char r, g, b;
                data++;
                r = *data++;
                g = *data++;
                b = *data++;
                for (int j = 0; j < magnification; j++) {
                    *p++ = r;
                    *p++ = g;
                    *p++ = b;
                }
            }
        } else {
            unsigned char *p = dst;
            for (int i = 0; i < skin_width; i++) {
                unsigned char c = *data++;
                for (int j = 0; j < magnification; j++)
                    *p++ = c;
            }
        }
        for (int i = 1; i < magnification; i++) {
            unsigned char *p = dst + skin_bytesperline;
            memcpy(p, dst, skin_bytesperline);
            dst = p;
        }
        skin_y += magnification;
    }
}

/********************************************************************************
 * Prepare the skin bitmap internal object.                                     *
 ********************************************************************************/
void Skin::finish_image (HDC hdc) {
    BITMAPINFOHEADER *bh;

    if (skin_type == IMGTYPE_MONO) {
        skin_dib = CreateBitmap(skin_width, skin_height, 1, 1, skin_bitmap);
        mfree(skin_bitmap);
        skin_bitmap = NULL;
        return;
    }

    if (skin_type == IMGTYPE_COLORMAPPED) {
        RGBQUAD *cmap;
        int i;
        bh = (BITMAPINFOHEADER *) mmalloc(sizeof(BITMAPINFOHEADER) + skin_ncolors * sizeof(RGBQUAD));
        // TODO - handle memory allocation failure
        cmap = (RGBQUAD *) (bh + 1);
        for (i = 0; i < skin_ncolors; i++) {
            cmap[i].rgbRed = skin_colors[i].r;
            cmap[i].rgbGreen = skin_colors[i].g;
            cmap[i].rgbBlue = skin_colors[i].b;
            cmap[i].rgbReserved = 0;
        }
    } else if (skin_type == IMGTYPE_GRAY) {
        RGBQUAD *cmap;
        int i;
        bh = (BITMAPINFOHEADER *) mmalloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
        // TODO - handle memory allocation failure
        cmap = (RGBQUAD *) (bh + 1);
        for (i = 0; i < 256; i++) {
            cmap[i].rgbRed = cmap[i].rgbGreen = cmap[i].rgbBlue = i;
            cmap[i].rgbReserved = 0;
        }
    } else
        bh = (BITMAPINFOHEADER *) mmalloc(sizeof(BITMAPINFOHEADER));
        // TODO - handle memory allocation failure

    bh->biSize = sizeof(BITMAPINFOHEADER);
    bh->biWidth = skin_width;
    bh->biHeight = -skin_height;
    bh->biPlanes = 1;
    switch (skin_type) {
        case IMGTYPE_MONO:
            bh->biBitCount = 1;
            bh->biClrUsed = 0;
            break;
        case IMGTYPE_GRAY:
            bh->biBitCount = 8;
            bh->biClrUsed = 256;
            break;
        case IMGTYPE_COLORMAPPED:
            bh->biBitCount = 8;
            bh->biClrUsed = skin_ncolors;
            break;
        case IMGTYPE_TRUECOLOR:
            bh->biBitCount = 24;
            bh->biClrUsed = 0;
            break;
    }
    bh->biCompression = BI_RGB;
    bh->biSizeImage = skin_bytesperline * skin_height;
    bh->biXPelsPerMeter = 2835;
    bh->biYPelsPerMeter = 2835;
    bh->biClrImportant = 0;

    skin_header = bh;

    make_dib(hdc);
}

/********************************************************************************
 * Create the skin_dib for display if not yet existing.                         *
 * Return 0 if problem.                                                         *
 ********************************************************************************/
/* private */
int Skin::make_dib (HDC hdc) {
    void *bits;
    if (skin_type == IMGTYPE_MONO) {
        /* Not using a DIB, but a regular monochrome bitmap;
         * this bitmap was already created in skin_finish_image(),
         * since, unlike a DIB, no DC is required to create it.
         */
        return (1);
    }
    if (skin_header == NULL)
        return (0);
    if (skin_dib == NULL) {
        skin_dib = CreateDIBSection(hdc, (BITMAPINFO *) skin_header, DIB_RGB_COLORS, &bits, NULL, 0);
        if (skin_dib == NULL)
            return (0);
        memcpy(bits, skin_bitmap, skin_bytesperline * skin_height);
        mfree(skin_bitmap);
        skin_bitmap = NULL;
    }
    return (1);
}

/********************************************************************************
 * Find the key and keycode from coordinates on the skin.                       *
 ********************************************************************************/
void Skin::find_key (int x, int y, bool cshift, int *skey, int *ckey) {
    int i;

    // First search within the active annunciators
    int rx, ry;
    for (i=0 ; i<NB_ANNUN ; i++) {
        SkinAnnunciator *ann = annunciators + i;
        if (!ann->exists
            || (ann->sel.x == -1)) // This annunciator does not exist or is not selectable
            continue;

        rx = x - ann->disp_rect.x;
        ry = y - ann->disp_rect.y;
        if ((rx >= 0) && (rx < ann->disp_rect.width)
            && (ry >= 0) && (ry < ann->disp_rect.height)) {
            *skey = ANNUNBASE - i;
            *ckey = i + 1;
            return;
        }
    }

    // Then verify if this is inside the result area, or one of the graph areas.
    if (graph) {
        rx = x - display_loc.x;
        ry = y - display_loc.y;
        if ((rx >= 0) && (rx < display_w)
            && (ry >= 0) && (ry < display_h)) {
            *skey = ZONE_AREA;
            *ckey = KEY_NONE;
            return;
        }
        rx = x - view_loc.x;
        ry = y - view_loc.y;
        if ((rx >= 0) && (rx < view_w)
            && (ry >= 0) && (ry < view_h)) {
	        *skey = WVIEW_AREA;
            *ckey = KEY_NONE;
            return;
        }
    } else {
        rx = x - display_loc.x;
        ry = y - display_loc.y;
        if ((rx >= 0) && (rx < display_w)
            && (ry >= 0) && (ry < display_h)) {
            *skey = RESULT_AREA;
            *ckey = KEY_NONE;
            return;
        }
    }

    // Not found, search within the keys
    *skey = NOSKEY;
    *ckey = KEY_NONE;
    for (i=0 ; i<nkeys ; i++) {
        SkinKey *k  = keylist + i;
        int      rx = x - k->sens_rect.x;
        int      ry = y - k->sens_rect.y;

        if ((rx >= 0) && (rx < k->sens_rect.width)
            && (ry >= 0) && (ry < k->sens_rect.height)) {
            *skey = i;
            *ckey = cshift ? k->shifted_code : k->code;
            break;
        }
    }
}

/********************************************************************************
 * Find the keymap entry for a keycode.                                         *
 ********************************************************************************/
int Skin::find_skey (int ckey) {
    int i;
    for (i=0 ; i<nkeys ; i++)
        if ((keylist[i].code == ckey) || (keylist[i].shifted_code == ckey))
            return (i);
    return (-1);
}

/********************************************************************************
 * Find the macro string for a keycode.                                         *
 ********************************************************************************/
unsigned char *Skin::find_macro (int ckey) {
    SkinMacro *m = macrolist;
    while (m != NULL) {
        if (m->code == ckey)
            return (m->macro);
        m = m->next;
    }
    return (NULL);
}

/********************************************************************************
 * Lookup for a macro string in the keymap, using a keycode.                    *
 ********************************************************************************/
unsigned char *Skin::keymap_lookup (int keycode, bool ctrl, bool alt, bool shift,
                                    bool cshift, bool *exact) {
    int            i;
    unsigned char *macro = NULL;

    for (i=0 ; i<keymap_length ; i++) {
        keymap_entry *entry = keymap + i;

        if ((keycode == entry->keycode)
            && (ctrl == entry->ctrl)
            && (alt == entry->alt)
            && (shift == entry->shift)) {
            macro = entry->macro;
            if (cshift == entry->cshift) {
                *exact = true;
                return (macro);
            }
        }
    }
    *exact = false;
    return (macro);
}

/********************************************************************************
 * OS specific: display the bitmap on the given Windows Handle Context.         *
 ********************************************************************************/
void Skin::repaint (HDC hdc, HDC memdc) {
    COLORREF old_bg, old_fg;
    if (!make_dib(memdc))
        return;
    HGDIOBJ oldObject = SelectObject(memdc, skin_dib);
    if (skin_type == IMGTYPE_MONO) {
        old_bg = SetBkColor(hdc, 0x00ffffff);
        old_fg = SetTextColor(hdc, 0x00000000);
    }
    BitBlt(hdc, skin.x, skin.y, skin.width, skin.height, memdc, 0, 0, SRCCOPY);
    if (skin_type == IMGTYPE_MONO) {
        SetBkColor(hdc, old_bg);
        SetTextColor(hdc, old_fg);
    }
    SelectObject(memdc, oldObject); // Set back default object
}

/********************************************************************************
 * OS specific: display an annunciator on the given Windows Handle Context.     *
 ********************************************************************************/
void Skin::repaint_annunciator (HDC hdc, HDC memdc, int which, int state) {
    if (!display_enabled)
        return;
    SkinAnnunciator *ann = annunciators + which;
    if (!ann->exists)
        return;
    if ((which >= ANN_ZONMAXY) && (which <= ANN_CURVENB)) { // Special handling for graph values annunciators
        if (graph) { // Do not display them is not in a graph skin
            repaint_grphVal(hdc, which);
        }
        return;
    }
    COLORREF old_bg, old_fg;
    if (!make_dib(memdc))
        return;
    HGDIOBJ oldObject = SelectObject(memdc, skin_dib);
    if (skin_type == IMGTYPE_MONO) {
        old_bg = SetBkColor(hdc, 0x00ffffff);
        old_fg = SetTextColor(hdc, 0x00000000);
    }

    // Special handling for the skin and menu annunciators, they are always visible or selected
    if ((which >= ANN_S1) && (which <= ANN_HSTMENU)) {
        state += 1;
        if (state > 2)   state = 2;
    }

    if (state == 1) // Just display
        BitBlt(hdc, ann->disp_rect.x, ann->disp_rect.y, ann->disp_rect.width, ann->disp_rect.height,
               memdc, ann->src.x, ann->src.y, SRCCOPY);
    else if (state == 2) // Selected mode
        BitBlt(hdc, ann->disp_rect.x, ann->disp_rect.y, ann->disp_rect.width, ann->disp_rect.height,
               memdc, ann->sel.x, ann->sel.y, SRCCOPY);
    else // Blank the area
        BitBlt(hdc, ann->disp_rect.x, ann->disp_rect.y, ann->disp_rect.width, ann->disp_rect.height,
               memdc, ann->disp_rect.x, ann->disp_rect.y, SRCCOPY);
    if (skin_type == IMGTYPE_MONO) {
        SetBkColor(hdc, old_bg);
        SetTextColor(hdc, old_fg);
    }
    SelectObject(memdc, oldObject); // Set back default object
}

/********************************************************************************
 * OS specific: repaint a key on screen.                                        *
 ********************************************************************************/
void Skin::repaint_key (HDC hdc, HDC memdc, int key, int state) {
    SkinKey *k;
    COLORREF old_bg, old_fg;

    if (key < 0 || key >= nkeys)
        return;
    if (!make_dib(memdc))
        return;
    HGDIOBJ oldObject = SelectObject(memdc, skin_dib);
    k = keylist + key;
    if (skin_type == IMGTYPE_MONO) {
        old_bg = SetBkColor(hdc, 0x00ffffff);
        old_fg = SetTextColor(hdc, 0x00000000);
    }
    if (state)
        BitBlt(hdc, k->disp_rect.x, k->disp_rect.y, k->disp_rect.width, k->disp_rect.height,
               memdc, k->src.x, k->src.y, SRCCOPY);
    else
        BitBlt(hdc, k->disp_rect.x, k->disp_rect.y, k->disp_rect.width, k->disp_rect.height,
               memdc, k->disp_rect.x, k->disp_rect.y, SRCCOPY);
    if (skin_type == IMGTYPE_MONO) {
        SetBkColor(hdc, old_bg);
        SetTextColor(hdc, old_fg);
    }
    SelectObject(memdc, oldObject); // Set back default object
}

/********************************************************************************
 * Handle result scrolling.                                                     *
 ********************************************************************************/
bool Skin::tapOnResult (HDC hdc, int x, int y, TtrackAction action) {
    bool rc = false;

    if (result_size.cx > display_w) {
        if (action == track_set) { // Directly point in result area, or enter it
            prevx = selsx0 = x;
            prevsx = g_scroll_result;
        } else if ((action == track_move) && (prevx != x)) {
            prevx = x;

            int dx = x - selsx0;
            g_scroll_result = prevsx + dx;
            if (g_scroll_result < 0)
                g_scroll_result = 0;
            if (g_scroll_result > result_size.cx - display_w)
                g_scroll_result = result_size.cx - display_w;

            repaint_result(hdc);
            rc = true; // We are moving ...
        }
    }

    return (rc);
}

/********************************************************************************
 * OS specific: repaint the result area.                                        *
 ********************************************************************************/
void Skin::repaint_result (HDC hdc) {
    if (!display_enabled || graph)
        return;

    erase_result(hdc);
    if (*g_dispResult != 0)
        if (g_result_pow)
            paint_resultpow(hdc);
        else
            paint_result(hdc);
}

/********************************************************************************
 * Return the display foreground color.                                         *
 ********************************************************************************/
COLORREF Skin::getDisplayFgColor (void) {
    return (display_fg);
}

/********************************************************************************
 * Return the display background color.                                         *
 ********************************************************************************/
COLORREF Skin::getDisplayBgColor (void) {
    return (display_bg);
}

/********************************************************************************
 * OS specific: create the input text zone in the display area.                 *
 ********************************************************************************/
void Skin::create_input_area (HWND hWnd_p, HINSTANCE hinst) {
    if (g_hwndE != NULL) {
        if (graph) {
            ShowWindow(g_hwndE, SW_HIDE);
            EnableWindow(g_hwndE, FALSE);
        } else {
            ShowWindow(g_hwndE, SW_SHOWNA);
            EnableWindow(g_hwndE, TRUE);
            BOOL res = MoveWindow(g_hwndE,
                                  display_loc.x, display_loc.y + 18 * magnification,
                                  display_w, 30 * magnification,
                                  TRUE);
        }
    } else {
        DWORD dwStyle = (graph ? WS_DISABLED : WS_VISIBLE)
                        | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_LEFT;
//        g_hwndE = CreateWindow(_T("EDIT2"), NULL, dwStyle,
        g_hwndE = CreateWindow(_T("EDIT"), NULL, dwStyle,
                               display_loc.x, display_loc.y + 18 * magnification,
                               display_w, 30 * magnification,
                               hWnd_p, NULL, hinst, NULL);
        g_cedit2_obj = subclassEdit(hWnd_p, g_hwndE);
    }
#ifdef POCKETPC2003_UI_MODEL
#define ES_EX_FOCUSBORDERDISABLED       0x00000002  // Prevent control from drawing a border when it gains focus.
#endif
    Edit_SetExtendedStyle(g_hwndE, 0xFFFFFFFF, Edit_GetExtendedStyle(g_hwndE) | ES_EX_FOCUSBORDERDISABLED);

    if (hFont_input == NULL) {
        // Set the font of the edit control.
        LOGFONT lf;
        // Clear out the lf structure to use when creating the font.
        memset(&lf, 0, sizeof(LOGFONT));
        lf.lfHeight = INPUTFONT_SIZE * magnification;
        _tcscpy(lf.lfFaceName, _T("Tahoma"));
        hFont_input = CreateFontIndirect(&lf);
    }
    long res = SendMessage (g_hwndE, WM_SETFONT, (WPARAM) hFont_input, (LPARAM) TRUE);
    // No softline break character when getting the text.
    res = SendMessage (g_hwndE, EM_FMTLINES, FALSE, 0);

//TCHAR buf[100];
//int nchar = GetClassName(g_hwndE, buf, 100);
//print_result(hWnd_p, buf);
    if (graph) {
        /* Draw axes values */
//        graph_draw_axes_labels(&stdbounds);
    } else   SetFocus(g_hwndE);
}

/********************************************************************************
 * OS specific: interaction between the Edit Control passed as parameter and    *
 *              clipboard.                                                      *
 *              If parameter is NULL, this is the input area Edit Control.      *
 * action is one of WM_OPY, WM_CU, WM_PASTE.                                    *
 ********************************************************************************/
void Skin::clipboardAction (void *hwnd, int action) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;
    long res = SendMessage (hwnd_edit, action, (WPARAM) 0, (LPARAM) -1);
    res = SendMessage (hwnd_edit, EM_SCROLLCARET, (WPARAM) 0, (LPARAM) 0);
}

/********************************************************************************
 * OS specific: selects all the text in the Edit Control passed as parameter.   *
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
void Skin::select_input_text (void *hwnd) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;
    long res = SendMessage (hwnd_edit, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
    res = SendMessage (hwnd_edit, EM_SCROLLCARET, (WPARAM) 0, (LPARAM) 0);
}

/********************************************************************************
 * OS specific: selects some text in the the Edit Control passed as parameter.  *
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
void Skin::select_input_text (void *hwnd, unsigned long start, unsigned long end) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;
    long res = SendMessage (hwnd_edit, EM_SETSEL, (WPARAM) start, (LPARAM) end);
    res = SendMessage (hwnd_edit, EM_SCROLLCARET, (WPARAM) 0, (LPARAM) 0);
}

/********************************************************************************
 * OS specific: get boundaries of selected text in the Edit Control passed as   *
 *              parameter.                                                      *
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
void Skin::get_select_text (void *hwnd, unsigned long  *start, unsigned long  *end) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;
    long res = SendMessage (hwnd_edit, EM_GETSEL, (WPARAM) start, (LPARAM) end);
}

/********************************************************************************
 * OS specific: get insert position in the Edit Control passed as parameter.    *
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
unsigned long Skin::get_insert_pos(void *hwnd) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;

    unsigned long pos; 
    long res = SendMessage (hwnd_edit, EM_GETSEL, (WPARAM) &pos, (LPARAM) NULL);
    return (pos);
}

/********************************************************************************
 * OS specific: set cursor position in the Edit Control passed as parameter.    *
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
void Skin::set_insert_pos (void *hwnd, unsigned long pos) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;

    long res = SendMessage (hwnd_edit, EM_SETSEL, (WPARAM) pos, (LPARAM) pos);
}

/********************************************************************************
 * OS specific: remove selection, or remove character at left of cursor  in     *
 *              the Edit Control passed as parameter.                           *
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
void Skin::back_delete (void *hwnd) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;

    unsigned long start, end;
    long res = SendMessage (hwnd_edit, EM_GETSEL, (WPARAM) &start, (LPARAM) &end);
    if (start != end) {
        res = SendMessage (hwnd_edit, WM_CLEAR, (WPARAM) 0, (LPARAM) 0);
    } else if (start > 0) {
        res = SendMessage (hwnd_edit, EM_SETSEL, (WPARAM) start, (LPARAM) (start-1));
        res = SendMessage (hwnd_edit, WM_CLEAR, (WPARAM) 0, (LPARAM) 0);
    }
}

/********************************************************************************
 * OS specific: inserts text in the Edit Control passed as parameter, at the    *
 *              cursor (replacing selected text if any).                        *
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
void Skin::insert_input_text (void *hwnd, const TCHAR *text) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;
    long res = SendMessage (hwnd_edit, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) text);
}

/********************************************************************************
 * OS specific: sets text in the Edit Control passed as parameter, replacing all*
 *              If parameter is NULL, this is the input area Edit Control.      *
 ********************************************************************************/
void Skin::set_input_text (void *hwnd, const TCHAR *text) {
    HWND hwnd_edit = (HWND) hwnd;
    if (hwnd_edit == NULL)
        hwnd_edit = g_hwndE;
    BOOL res = SetWindowText(hwnd_edit, text);
}

/********************************************************************************
 * OS specific: gets text from the input area Edit Control.                     *
 ********************************************************************************/
TCHAR *Skin::get_input_text() {
    int res = GetWindowText(g_hwndE, inputText, INPUTAREA_SIZE);
    return (inputText);
}

/********************************************************************************
 * OS specific: display the specified result text in the display area.          *
 ********************************************************************************/
void Skin::print_result (void *hWnd_p, TCHAR *res_text) {
    // Save the text and call the main method
    //_tcscpy(dispResult, res_text);
    g_result_len = _tcslen(res_text);
    if (g_result_len >= RESULTAREA_SIZE) {
        g_result_len = RESULTAREA_SIZE - 1;
    }
    _tcsncpy(g_dispResult, res_text, g_result_len);
    g_dispResult[g_result_len] = _T('\0');
    g_result_pow = false; // Just a mono font result

    if (!graph) {
        HDC hdc = GetDC((HWND) hWnd_p);
        print_result(hdc);
        ReleaseDC((HWND) hWnd_p, hdc);
    }
}

/********************************************************************************
 * OS specific: remove result BMP.                                              *
 ********************************************************************************/
HBITMAP Skin::allocResult (HDC hdc, LONG cx, LONG cy) {
    HBITMAP bm;

//    bm = CreateCompatibleBitmap(resultDC, cx, cy);
    bm = CreateCompatibleBitmap(hdc, cx, cy);
    if (bm != NULL) {
        oldResultObj = SelectObject(resultDC, bm);
    }
    return (bm);
}

/********************************************************************************
 * OS specific: remove result BMP.                                              *
 ********************************************************************************/
void Skin::deleteResult (void) {
    if (oldResultObj != NULL) {
        SelectObject(resultDC, oldResultObj);
        oldResultObj = NULL;
        if (resultBmp != NULL) {
            DeleteObject(resultBmp);
            resultBmp = NULL;
        }
    }
}

/********************************************************************************
 * OS specific: erase the display area.                                         *
 ********************************************************************************/
void Skin::erase_result (HDC hdc) {
//    HGDIOBJ oldObject = SelectObject(memdc, result_hbitmap);
//    COLORREF old_bg = SetBkColor(hdc, display_bg);
//    COLORREF old_fg = SetTextColor(hdc, display_fg);
//    BitBlt(hdc, display_loc.x, display_loc.y, display_w, 18 * magnification, memdc, 0, 0, SRCCOPY);
//    SelectObject(memdc, oldObject); // Set back default object
    RECT rc;
    rc.top  = display_loc.y;
    rc.left = display_loc.x;
    rc.bottom = rc.top + BIGFONT_SIZE * magnification;
    rc.right  = rc.left + display_w;
    FillRect(hdc, &rc, editBgBrush);
}

/********************************************************************************
 * OS specific: erase and display the saved result text in the display area.    *
 ********************************************************************************/
void Skin::print_result (HDC hdc) {
    if (!graph) {
        erase_result(hdc);
        deleteResult();
        result_size_recompute = true;    // Recalculate result_size in paint_result()
//        g_scroll_result = DT_RIGHT;
        g_scroll_result = 0;
        paint_result(hdc);
    }
}

/********************************************************************************
 * OS specific: overlay saved result text in the result display area.           *
 ********************************************************************************/
void Skin::paint_result (HDC hdc) {
//    // Set colors and font
//    COLORREF oldFg = SetTextColor(hdc, display_fg);
//    int oldBg = SetBkMode(hdc, TRANSPARENT);
//    if (hFontBig_display == NULL) {
//        LOGFONT lf;
//        // Clear out the lf structure to use when creating the font.
//        memset(&lf, 0, sizeof(LOGFONT));
//        lf.lfHeight = BIGFONT_SIZE * magnification;
//        lf.lfWeight = FW_BOLD;
//        _tcscpy(lf.lfFaceName, _T("Tahoma"));
//        hFontBig_display = CreateFontIndirect(&lf);
//    }
//    HFONT oldFont = (HFONT)SelectObject(hdc, hFontBig_display);

    if (result_size_recompute) {
        if (resultDC == NULL) {
            resultDC = CreateCompatibleDC(hdc);
        }
        // Set font
        if (hFontBig_display == NULL) {
            LOGFONT lf;
            // Clear out the lf structure to use when creating the font.
            memset(&lf, 0, sizeof(LOGFONT));
            lf.lfHeight = BIGFONT_SIZE * magnification;
            lf.lfWeight = FW_BOLD;
            _tcscpy(lf.lfFaceName, _T("Tahoma"));
            hFontBig_display = CreateFontIndirect(&lf);
        }
        HFONT oldFont = (HFONT) SelectObject(resultDC, hFontBig_display);
//        GetTextExtentPoint32(hdc, g_dispResult, g_result_len, &result_size);
        GetTextExtentPoint32(resultDC, g_dispResult, g_result_len, &result_size);
        resultBmp = allocResult(hdc, result_size.cx, result_size.cy);
        if (resultBmp == NULL) {
            SelectObject(resultDC, oldFont); // Set back default object
            return;
        }

        // We apparently get a monochrome bitmap .. so no need to set colors,
        // only set them when BitBlt-ing !!
        RECT rcBmp = {0, 0, result_size.cx, result_size.cy};
        COLORREF oldFg = SetTextColor(resultDC, display_fg);
        COLORREF oldBg = SetBkColor(resultDC, display_bg);
        int oldBgM = SetBkMode(resultDC, OPAQUE);
        DrawText(resultDC, g_dispResult, -1, &rcBmp,
                           DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
        SelectObject(resultDC, oldFont); // Set back default object
        result_size_recompute = false;
    }
    // Display text - scroll result is either DT_LEFT or DT_RIGHT
//    RECT rc = {display_loc.x, display_loc.y,
//               display_loc.x + display_w, display_loc.y + BIGFONT_SIZE * magnification};
//    DrawText(hdc, g_dispResult, -1, &rc,
//             g_scroll_result | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);
    int scroll = ANNVAL_SCR_NONE;
    if (g_result_len > 0) { // Handle scroll annunciators when necessary
        if (result_size.cx > display_w) {
//            if (g_scroll_result == DT_RIGHT) { // Light the scroll left annunciator
            if (result_size.cx > g_scroll_result + display_w) { // Light the scroll left annunciator
                scroll |= ANNVAL_SCR_LEFT;
//            } else { // Light the scroll right annunciator
            }
            if (g_scroll_result > 0) { // Light the scroll right annunciator
                scroll |= ANNVAL_SCR_RIGHT;
            }
            // Set colors now, because BitBlt-ing a monochrome bitmap
            COLORREF oldFg = SetTextColor(hdc, display_fg);
            COLORREF oldBg = SetBkColor(hdc, display_bg);
            int oldBgM = SetBkMode(hdc, OPAQUE);
            BitBlt(hdc, display_loc.x, display_loc.y,
                        display_w, BIGFONT_SIZE * magnification,
                        resultDC, result_size.cx - display_w - g_scroll_result, 0, SRCCOPY);
        } else {
            // Set colors now, because BitBlt-ing a monochrome bitmap
            COLORREF oldFg = SetTextColor(hdc, display_fg);
            COLORREF oldBg = SetBkColor(hdc, display_bg);
            int oldBgM = SetBkMode(hdc, OPAQUE);
            BitBlt(hdc, display_loc.x + display_w - result_size.cx, display_loc.y,
                        result_size.cx, BIGFONT_SIZE * magnification,
                        resultDC, 0, 0, SRCCOPY);
        }
    }
    shell_scroll_annunciators(scroll);

//    SelectObject(hdc, oldFont); // Set back default object
}

/***********************************************************************
 * OS specific: initialize the display of a result with power text.    *
 * Formats a a^b in a nice way on the display.                         *
 * This writes pieces to output from right to left, or left to right   *
 * depending on scroll, using recursivity.                             *
 * res_text should contain a '^' character. If not, this will revert   *
 * to normal print_result().                                           *
 ***********************************************************************/
void Skin::print_resultpow (void *hWnd_p, TCHAR *res_text) {
    // Save the text and call the appropriate method
    //_tcscpy(dispResult, res_text);
    g_result_len = _tcslen(res_text);
    if (g_result_len >= RESULTAREA_SIZE) {
        g_result_len = RESULTAREA_SIZE - 1;
    }
    _tcsncpy(g_dispResult, res_text, g_result_len);
    g_dispResult[g_result_len] = _T('\0');

    if (!graph) {
        HDC hdc = GetDC((HWND) hWnd_p);
        if (_tcschr(res_text, _T('^')) == NULL) {
            g_result_pow = false; // Just a mono font result
            print_result(hdc);
        } else {
            g_result_pow = true; // Special routine to mix fonts
            print_resultpow(hdc);
        }
        ReleaseDC((HWND) hWnd_p, hdc);
    }
}

/********************************************************************************
 * OS specific: erase and display the saved result text in the display area.    *
 ********************************************************************************/
void Skin::print_resultpow (HDC hdc) {
    if (!graph) {
        erase_result(hdc);
        deleteResult();
        result_size_recompute = true;    // Recalculate result_size in paint_resultpow()
//        g_scroll_result = DT_RIGHT;
        g_scroll_result = 0;
        paint_resultpow(hdc);
    }
}

/********************************************************************************
 * OS specific: overlay a mixed font result with power text in the result       *
 * display area.                                                                *
 ********************************************************************************/
void Skin::paint_resultpow (HDC hdc) {
//    // Set colors
//    COLORREF oldFg = SetTextColor(hdc, display_fg);
//    int oldBg = SetBkMode(hdc, TRANSPARENT);

    if (result_size_recompute) {
        if (resultDC == NULL) {
            resultDC = CreateCompatibleDC(hdc);
        }
        // Calculate overestimated size with big font only
        if (hFontBig_display == NULL) {
            LOGFONT lf;
            // Clear out the lf structure to use when creating the font.
            memset(&lf, 0, sizeof(LOGFONT));
            lf.lfHeight = BIGFONT_SIZE * magnification;
            lf.lfWeight = FW_BOLD;
            _tcscpy(lf.lfFaceName, _T("Tahoma"));
            hFontBig_display = CreateFontIndirect(&lf);
        }
        HFONT oldFont = (HFONT) SelectObject(resultDC, hFontBig_display);

        // result_size.cx will get recomputed by the method, but
        // result_size.cy will be correct at least.
//        GetTextExtentPoint32(hdc, g_dispResult, g_result_len, &result_size);
        GetTextExtentPoint32(resultDC, g_dispResult, g_result_len, &result_size);
//        SelectObject(hdc, oldFont); // Set back default object
        resultBmp = allocResult(hdc, result_size.cx, result_size.cy);
        if (resultBmp == NULL) {
            SelectObject(resultDC, oldFont); // Set back default object
            return;
        }
        // Clear bitmap, we'll have to write in transparent mode since small
        // fonts can't set background on the whole height.
        RECT rcBmp = {0, 0, result_size.cx, result_size.cy};
        COLORREF oldBg = SetBkColor(resultDC, display_bg);
        FillRect(resultDC, &rcBmp, editBgBrush);
        result_size.cx = 0;
//    }
    pow_pos = 0;
    _tcscpy(work_dispResult, g_dispResult); // Use a work area, since text will be altered
//    paint_resultpow_rec(hdc, false, work_dispResult); // Start with normal text.
        // We apparently get monochrome bitmaps .. so no need to set colors,
        // only set them when BitBlt-ing !!
        int oldBgM = SetBkMode(resultDC, TRANSPARENT);
        COLORREF oldFg = SetTextColor(resultDC, display_fg);
        paint_resultpow_rec(resultDC, false, work_dispResult); // Start with normal text.
        SelectObject(resultDC, oldFont); // Set back default object
//    if (result_size_recompute) { // Was left at true to provoke recomputation of result_size.cx
//        HBITMAP bm = CreateCompatibleBitmap(resultDC, result_size.cx, result_size.cy);
        HBITMAP bm = CreateCompatibleBitmap(hdc, result_size.cx, result_size.cy);
        if (bm == NULL) {
            return;
        }
        SelectObject(resultDC, bm);
        HDC tempDC = CreateCompatibleDC(hdc);
        HGDIOBJ tempOld = SelectObject(tempDC, resultBmp);
        BitBlt(resultDC, 0, 0, result_size.cx, result_size.cy, tempDC, 0, 0, SRCCOPY);
        SelectObject(tempDC, tempOld);
        DeleteObject(tempDC);
        DeleteObject(resultBmp);
        resultBmp = bm;
        result_size_recompute = false;
    }

    int scroll = ANNVAL_SCR_NONE;
    if (g_result_len > 0) { // Handle scroll annunciators when necessary
        if (result_size.cx > display_w) {
//            if (g_scroll_result == DT_RIGHT) { // Light the scroll left annunciator
            if (result_size.cx > g_scroll_result + display_w) { // Light the scroll left annunciator
                scroll |= ANNVAL_SCR_LEFT;
//            } else { // Light the scroll right annunciator
            }
            if (g_scroll_result > 0) { // Light the scroll right annunciator
                scroll |= ANNVAL_SCR_RIGHT;
            }
            // Set colors now, because BitBlt-ing a monochrome bitmap
            COLORREF oldFg = SetTextColor(hdc, display_fg);
            COLORREF oldBg = SetBkColor(hdc, display_bg);
            int oldBgM = SetBkMode(hdc, OPAQUE);
            BitBlt(hdc, display_loc.x, display_loc.y,
                        display_w, BIGFONT_SIZE * magnification,
                        resultDC, result_size.cx - display_w - g_scroll_result, 0, SRCCOPY);
        } else {
            // Set colors now, because BitBlt-ing a monochrome bitmap
            COLORREF oldFg = SetTextColor(hdc, display_fg);
            COLORREF oldBg = SetBkColor(hdc, display_bg);
            int oldBgM = SetBkMode(hdc, OPAQUE);
            BitBlt(hdc, display_loc.x + display_w - result_size.cx, display_loc.y,
                        result_size.cx, BIGFONT_SIZE * magnification,
                        resultDC, 0, 0, SRCCOPY);
        }
    }
    shell_scroll_annunciators(scroll);
}

/***********************************************************************
 * Recursive body of print_resultpow().                                *
 * smallf indicates small power characters or normal characters.       * 
 ***********************************************************************/
void Skin::paint_resultpow_rec (HDC hdc, bool smallf, TCHAR *text) {
    TCHAR *expon, *expoff;
    int pdepth;

    // Look for '^' or go to end of string
    expon = _tcschr(text, _T('^'));
    if (expon)
        *expon = NULL; // Skip '^'
//    if (g_scroll_result == DT_LEFT)
        // Display the selected part first in current font size. A NULL is in *expon or at end.
        paint_result_piece(hdc, smallf, text, _tcslen(text));
    if (expon) {
        // We have a part after a '^'. It can be between parenthesises,
        // or end with a next term separated by '*' or '/', or simply end of string.
        expon++;
        if (expon[0] == _T('(')) { // .. between () ..
            expoff = expon;
            pdepth = 1;
            do { // Look for the corresponding closing ')'
                expoff++; // expoff is on the previously examined char 
                          // when entering the loop body.
                expoff = _tcspbrk(expoff, _T("()")); // Search for next ')' or '('.
                if (expoff && (*expoff == _T('('))) // Do we have a '(' before the next ')' ?
                    pdepth++; // Yes ! Get in one more depth level.
                else   pdepth--; // No, close that depth level.
            } while (pdepth > 0);
            // If aligned right,
            // print text on the right of expoff first, if there is one.
//            if (*(expoff+1) && (g_scroll_result == DT_RIGHT))
//                paint_resultpow_rec(hdc, false, expoff+1);
            // Print exponent, without enclosing ()
            expon++;
            *expoff = NULL;
            paint_result_piece(hdc, true, expon, _tcslen(expon));
            // If aligned left,
            // print text on the right of expoff after, if there is one.
//            if (*(expoff+1) && (g_scroll_result == DT_LEFT))
            if (*(expoff+1))
                paint_resultpow_rec(hdc, false, expoff+1);
        } else { // Find * or / maybe
            TCHAR c;
            expoff = _tcschr(expon, _T('*'));
            if (!expoff)
                expoff = _tcschr(expon, _T('/'));
            if (expoff) {
//                if (g_scroll_result == DT_RIGHT) // If aligned right,
//                                               // print text after on the right first, including * or /
//                    paint_resultpow_rec(hdc, false, expoff);
                c = *expoff; // Remember the char in case of aligned left ...
                *expoff = NULL; // Terminate string here for the left piece to display
            }
            // Print expon string in small chars.
            paint_result_piece(hdc, true, expon, _tcslen(expon));
//            if (expoff && (g_scroll_result == DT_LEFT)) {
            if (expoff) {
                // If aligned left,
                // print text on the right after, including * or /
                *expoff = c; // Restore remembered char
                paint_resultpow_rec(hdc, false, expoff);
                *expoff = NULL; // Just to be consistent :-)
            }
        }
    }
//    if (g_scroll_result == DT_RIGHT)
//        // Display the selected part after in current font size. A NULL is in *expon or at end.
//        paint_result_piece(hdc, smallf, text, _tcslen(text));
}

/********************************************************************************
 * OS specific: increments of power text in the result display area.            *
 ********************************************************************************/
void Skin::paint_result_piece (HDC hdc, bool smallf, TCHAR *res_piece, int len) {
//    COLORREF oldFg;
//    int      oldBg;
    HFONT    oldFont;
    SIZE     sz;

    if (smallf) { // Set small colors and font
//        oldFg = SetTextColor(hdc, display_fg);
//        oldBg = SetBkMode(hdc, TRANSPARENT);
        if (hFontSmall_display == NULL) {
            LOGFONT  lf;
            // Clear out the lf structure to use when creating the font.
            memset(&lf, 0, sizeof(LOGFONT));
            lf.lfHeight = SMALLFONT_SIZE * magnification;
            lf.lfWeight = FW_BOLD;
            _tcscpy(lf.lfFaceName, _T("Tahoma"));
            hFontSmall_display = CreateFontIndirect(&lf);
        }
        oldFont = (HFONT)SelectObject(hdc, hFontSmall_display);
    } else { // Set normal colors and font
//        oldFg = SetTextColor(hdc, display_fg);
//        oldBg = SetBkMode(hdc, TRANSPARENT);
        if (hFontBig_display == NULL) {
            LOGFONT  lf;
            // Clear out the lf structure to use when creating the font.
            memset(&lf, 0, sizeof(LOGFONT));
            lf.lfHeight = BIGFONT_SIZE * magnification;
            lf.lfWeight = FW_BOLD;
            _tcscpy(lf.lfFaceName, _T("Tahoma"));
            hFontBig_display = CreateFontIndirect(&lf);
        }
        oldFont = (HFONT)SelectObject(hdc, hFontBig_display);
    }

    // Get the size of the object we are going to display
    if (!GetTextExtentPoint(hdc, res_piece, len, &sz)) {
        // Error ! Stop here.
        SelectObject(hdc, oldFont); // Set back default object
        return;
    }

    if (result_size_recompute) // update result size
        result_size.cx += sz.cx;

    // Print the object at its place, and calculate position for next piece.
    RECT rc;
//    if (g_scroll_result == DT_RIGHT) {
//        if (pow_pos + sz.cx < display_w) { // Do not go beyond display area limits.
//            rc.left   = display_loc.x + display_w - pow_pos - sz.cx;
//            rc.top    = display_loc.y;
//            rc.right  = display_loc.x + display_w - pow_pos;
//            rc.bottom = display_loc.y + BIGFONT_SIZE * magnification;
//            pow_pos += sz.cx;
//        } else {
//            rc.left   = display_loc.x;
//            rc.top    = display_loc.y;
//            rc.right  = display_loc.x + display_w - pow_pos;
//            rc.bottom = display_loc.y + BIGFONT_SIZE * magnification;
//            pow_pos = display_w;
//        }
//    } else { // Text going from left to right
//        if (pow_pos + sz.cx < display_w) { // Do not go beyond display area limits.
//            rc.left   = display_loc.x + pow_pos;
//            rc.top    = display_loc.y;
//            rc.right  = display_loc.x + pow_pos + sz.cx;
//            rc.bottom = display_loc.y + BIGFONT_SIZE * magnification;
//            pow_pos += sz.cx;
//        } else {
//            rc.left   = display_loc.x + pow_pos;
//            rc.top    = display_loc.y;
//            rc.right  = display_loc.x + display_w;
//            rc.bottom = display_loc.y + BIGFONT_SIZE * magnification;
//            pow_pos = display_w;
//        }
//    }
    rc.left   = pow_pos;
    rc.top    = 0;
    rc.right  = pow_pos + sz.cx;
    rc.bottom = BIGFONT_SIZE * magnification;
    pow_pos += sz.cx;
    if (smallf) { // Small is aligned on top, normal is on bottom.
//        DrawText(hdc, res_piece, -1, &rc, g_scroll_result | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);	
        DrawText(resultDC, res_piece, -1, &rc, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);	
    } else {
//        DrawText(hdc, res_piece, -1, &rc, g_scroll_result | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);	
        DrawText(resultDC, res_piece, -1, &rc, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);	
    }

    SelectObject(hdc, oldFont); // Set back default object
}

/********************************************************************************
 * OS specific: scroll result left in the display area.                         *
 ********************************************************************************/
void Skin::scroll_result_left (HWND hWnd) {
    if (!graph) {
        HDC hdc = GetDC(hWnd);
        erase_result(hdc);

//        g_scroll_result = DT_LEFT;
        if (g_scroll_result < result_size.cx - (display_w << 1))
            g_scroll_result += display_w;
        else
            g_scroll_result = result_size.cx - display_w;
        if (g_result_pow)
            paint_resultpow(hdc);
        else
            paint_result(hdc);
        ReleaseDC(hWnd, hdc);
    }
}

/********************************************************************************
 * OS specific: scroll result right in the display area.                        *
 ********************************************************************************/
void Skin::scroll_result_right (HWND hWnd) {
    if (!graph) {
        HDC hdc = GetDC(hWnd);
        erase_result(hdc);

//        g_scroll_result = DT_RIGHT;
        if (g_scroll_result > display_w)
            g_scroll_result -= display_w;
        else
            g_scroll_result = 0;
        if (g_result_pow)
            paint_resultpow(hdc);
        else
            paint_result(hdc);
        ReleaseDC(hWnd, hdc);
    }
}

/********************************************************************************
 * Get the result text from the display area.                                   *
 ********************************************************************************/
TCHAR *Skin::get_result (void) {
    return (g_dispResult);
}

/********************************************************************************
 * OS specific: put result text from the display area to the clipboard.         *
 ********************************************************************************/
void Skin::clipCopy_result (void) {
    if (*g_dispResult != 0) {
        int len = (_tcslen(g_dispResult) + 1)*sizeof(TCHAR); // Number of bytes + 1
        // Get the clipboard and put data in there
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, len);
        void *p = GlobalLock(h);
        memcpy(p, g_dispResult, len);
        GlobalUnlock(h);
        OpenClipboard(NULL);
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, h);
        CloseClipboard();
    }
}

/********************************************************************************
 * OS specific: displays the result actions menu.                               *
 ********************************************************************************/
void Skin::resultActionsPopup (resSelection choices[], int length, void *hWnd) {
    const TCHAR *description;
    int          i;

    for (i=0 ; i<length ; i++) {
        description = libLang->translate(resMenuDesc[choices[i]]);
        SendMessage((HWND) hWnd, LB_ADDSTRING, 0, (LPARAM) description);
    }
}

/********************************************************************************
 * OS specific: displays the result actions menu.                               *
 ********************************************************************************/
void Skin::historyAddActionPopup (TCHAR *description, void *hWnd) {
    SendMessage((HWND) hWnd, LB_ADDSTRING, 0, (LPARAM) description);
}

/********************************************************************************
 * OS specific: displays the varSave values menu.                               *
 ********************************************************************************/
void Skin::varSavePopup (TCHAR *choices[], int length, void *hWnd) {
    for (int i=0 ; i<length ; i++) {
        SendMessage((HWND) hWnd, CB_ADDSTRING, 0, (LPARAM) choices[i]);
    }
}

/********************************************************************************
 * OS specific: displays the varMgr values menu.                                *
 ********************************************************************************/
void Skin::varMgrPopup (TCHAR *choices[], int length, void *hWnd) {
    for (int i=0 ; i<length ; i++) {
        SendMessage((HWND) hWnd, LB_ADDSTRING, 0, (LPARAM) choices[i]);
    }
}

/********************************************************************************
 * OS specific: reset the varDef list of values.                                *
 ********************************************************************************/
void Skin::varDefReset (void *hWnd) {
    SendMessage((HWND) hWnd, LVM_DELETEALLITEMS, (WPARAM) 0, (LPARAM) 0);
}

/********************************************************************************
 * OS specific: displays the varDef list of values.                             *
 ********************************************************************************/
void Skin::varDefList (TCHAR *name, TCHAR *deftext, int itemNum, void *hWnd) {
    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = itemNum;
    lvi.iSubItem = 0;
    lvi.pszText = name;
    int i = SendMessage((HWND) hWnd, LVM_INSERTITEM, (WPARAM) 0, (LPARAM) &lvi);

    lvi.iItem = i;
    lvi.iSubItem = 1;
    lvi.pszText = deftext;
    SendMessage((HWND) hWnd, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvi);
}

/********************************************************************************
 * Enable or disable screen repaints.                                           *
 ********************************************************************************/
void Skin::display_set_enabled (bool enable) {
    display_enabled = enable;
}

/********************************************************************************
 * Shift graph when drawing it by (dx, dy).                                     *
 ********************************************************************************/
void Skin::dragGraph (HDC hdc, int area, int dx, int dy) {
    if (area == ZONE_AREA) {
        // Calculate parts which need to be blanked
        SetRectRgn(zgph_rgn, display_loc.x + dx, display_loc.y + dy,
                             display_loc.x + display_w + dx, display_loc.y + display_h + dy);
        CombineRgn(zcomplement_rgn, zone_rgn, zgph_rgn, RGN_DIFF);
        FillRgn(hdc, zcomplement_rgn, editBgBrush);
        // Copy the visible part of graph in the rest
        int dxt, dxb;
        int dyt, dyb;
        if (dx < 0) {
            dxt = 0;
            dxb = -dx;
        } else {
            dxt = dx;
            dxb = 0;
        }
        if (dy < 0) {
            dyt = 0;
            dyb = -dy;
        } else {
            dyt = dy;
            dyb = 0;
        }
        BitBlt(hdc, display_loc.x + dxt, display_loc.y + dyt,
                    display_w - dxt - dxb, display_h - dyt - dyb,
               zoneDC, dxb, dyb, SRCCOPY);
    } else {
        // Calculate parts which need to be blanked
        SetRectRgn(vgph_rgn, view_loc.x + dx, view_loc.y + dy,
                             view_loc.x + view_w + dx, view_loc.y + view_h + dy);
        CombineRgn(vcomplement_rgn, view_rgn, vgph_rgn, RGN_DIFF);
        FillRgn(hdc, vcomplement_rgn, editBgBrush);
        // Copy the visible part of graph in the rest
        int dxt, dxb;
        int dyt, dyb;
        if (dx < 0) {
            dxt = 0;
            dxb = -dx;
        } else {
            dxt = dx;
            dxb = 0;
        }
        if (dy < 0) {
            dyt = 0;
            dyb = -dy;
        } else {
            dyt = dy;
            dyb = 0;
        }
        BitBlt(hdc, view_loc.x + dxt, view_loc.y + dyt,
                    view_w - dxt - dxb, view_h - dyt - dyb,
               viewDC, dxb, dyb, SRCCOPY);
    }
}

/********************************************************************************
 * Retrieve screen coordinates from input parameter, set graph on it, and       *
 * show selection cross.                                                        *
 * Corresponds to grtaps_track_manual() in original cade.                       *
 * Can only be used in curve track mode.                                        *
 ********************************************************************************/
void Skin::showOnGraph (HDC hdc, double value) {
    double realx, realy, r;

    if (graphPrefs.functype == graph_polar) {
        value = math_user_to_rad(value);
    }
    zoneGraph.graph_get_vals(curve_nb, value, &realx, &realy, &r);

    if (is_cross) { // Remove previous cross
        if (prevsy != -1)
            InvertRect(hdc, &cross_rcy); // Remove previous horizontal part of cross
        if (prevsx != -1)
            InvertRect(hdc, &cross_rcx); // Remove previous vertical part of cross
    } else if (is_selecting) { // Remove previous sel zone
        if (selprevsx != -1) {
            InvertRect(hdc, &sel_rc); // Remove previous sel zone
            selprevsx = -1; // Remember that no sel zone set
        }
    }

    // Move to new position; and show new cross
    moveGraph(hdc, ZONE_AREA, realx, realy);
    param0 = NaN;
    param = value;
    calcx = realx;
    calcy = realy;
    if (graphPrefs.functype == graph_polar) {
        calcr = r;
    } else {
        calcr = NaN;
    }

    int sx = zoneGraph.graph_xgr2scr(calcx);
    int sy = zoneGraph.graph_ygr2scr(calcy);

    is_cross = true;

    cross_rcy.top = display_basey - sy;
    cross_rcy.bottom = cross_rcy.top + 1;
    cross_rcy.left = display_loc.x;
    cross_rcy.right = display_loc.x + display_w;
    InvertRect(hdc, &cross_rcy); // Set new one
    prevsy = sy;

    cross_rcx.top = display_loc.y;
    cross_rcx.bottom = display_loc.y + display_h;
    cross_rcx.left = sx + display_loc.x;
    cross_rcx.right = cross_rcx.left + 1;
    InvertRect(hdc, &cross_rcx); // Set new one
    prevsx = sx;

    repaint_grphVal(hdc, ANN_PARAM0);
    repaint_grphVal(hdc, ANN_PARAM);
    repaint_grphVal(hdc, ANN_YVALUE);
    repaint_grphVal(hdc, ANN_XVALUE);
    repaint_grphVal(hdc, ANN_RVALUE);
}

/********************************************************************************
 * Convert screen coordinates to values and display them in annunciators,       *
 * or move graph, or select, depending on graph mode.                           *
 ********************************************************************************/
void Skin::tapOnGraph (HDC hdc, int area, int x, int y, TtrackAction action) {
    bool removeCross_x, removeCross_y, removeSel, setCross_x, setCross_y, setSel;
    removeCross_x = removeCross_y = removeSel = setCross_x = setCross_y = setSel = false;
    Graph grph;

    if ((action == track_set) || (selArea != area)) { // Directly point in zone, or enter it
                                                      // from the other one or none
        prevx = x;
        prevy = y;
        if (dispPrefs.penMode == pen_trackpt) {
            param0 = NaN;
            if (is_cross) { // Remove previous cross)
                removeCross_x = removeCross_y = true;
            } else if (is_selecting) { // Remove previous selection
                is_selecting = false;
                removeSel = removeCross_y = true;
                if (graphPrefs.functype != graph_func)
                    removeCross_x = true;
            }
            is_cross = setCross_x = setCross_y = true;
        } else /* if (dispPrefs.penMode == pen_selzone) */ {
            param0 = NaN;
            if (is_selecting) { // Remove previous selection
                is_selecting = false;
                removeSel = true;
            }
            is_cross = true; // To signal start of selection, but will be handled like a selection ..
            if ((dispPrefs.penMode == pen_centerwide) && (area == WVIEW_AREA))
                setSel = true;
        }
    } else if (action == track_move) {
        if ((prevx == x) && (prevy == y)) { // No move (yes, Windows API again ..!)
            return;
        }
        if (dispPrefs.penMode == pen_trackpt) {
            if (is_cross) {
                removeCross_x = true;
                is_selecting = true;
                is_cross = false;
                // Remember start point
                param0 = param;
            }
            removeSel = removeCross_y = setCross_y = setSel = true;
            if (graphPrefs.functype != graph_func)
                removeCross_x = setCross_x = true;
        } else /* if (dispPrefs.penMode == pen_selzone) */ {
            if (is_cross) {
                is_selecting = true;
                is_cross = false;
                // Remember start point
                param0 = param;
                selx0 = calcx; // These two will be used when pressing ZOOM_IN keys
                sely0 = calcy;
            }
            removeSel = setSel = true;
        }
    } else {
        if ((dispPrefs.penMode == pen_movzone) || (dispPrefs.penMode == pen_centerwide)) {
            if (x != -1) { // We are getting out of zone, not shifting pen up,
                           // so set back screen into place
                is_selecting = false;
                removeSel = true;
                param0 = param = calcx = calcy = calcr = NaN;
                action = track_move;
            } else if (is_cross) {
                is_cross = false;
                selx0 = calcx;
                sely0 = calcy;
            }

        }
    }

    if (action == track_off) { // Action on pen up ?
        if (dispPrefs.penMode == pen_movzone) { // Move graph ...
            if (area == ZONE_AREA) {
                double zX = (graphPrefs.xmax + graphPrefs.xmin) / 2.0;
                double zY = (graphPrefs.ymax + graphPrefs.ymin) / 2.0;
                moveGraph(hdc, area, zX + selx0 - calcx, zY + sely0 - calcy);
            } else {
                moveGraph(hdc, area, viewX + selx0 - calcx, viewY + sely0 - calcy);
            }
        } else if (dispPrefs.penMode == pen_centerwide) { // Move zone only
            double zX = (graphPrefs.xmax + graphPrefs.xmin) / 2.0;
            double zY = (graphPrefs.ymax + graphPrefs.ymin) / 2.0;
            if (area == ZONE_AREA) {
                moveGraph(hdc, area, zX + selx0 - calcx, zY + sely0 - calcy);
            } else {
                moveGraph(hdc, ZONE_AREA, calcx, calcy);
            }
        }
    } else {
        int sx, sy, selsx;
        int top, bottom, left, right, basey;
        if (area == ZONE_AREA) {
            sx = x - (left = display_loc.x);
            top = display_loc.y;
            sy = (basey = display_basey) - y;
            right = left + display_w;
            bottom = top + display_h;
            grph = zoneGraph;

            if (dispPrefs.penMode == pen_trackpt) { // Snap to curve
                selsx = sx; // Keep current pen position (in polar or param, sx is modified)
                if (graphPrefs.functype == graph_func) {
                    zoneGraph.grtaps_track_func(&sx, &sy, &calcx, &calcy, curve_nb);
                    param = calcx;
                } else if (graphPrefs.functype == graph_polar) {
                    zoneGraph.grtaps_track_pol(&sx, &sy, &calcx, &calcy, &param, &calcr, curve_nb);
                } else {
                    zoneGraph.grtaps_track_param(&sx, &sy, &calcx, &calcy, &param, curve_nb);
                }
            } else { // Just where the pen is ..
                param = calcx = zoneGraph.graph_xscr2gr(sx);
                calcy = zoneGraph.graph_yscr2gr(sy);
            }
        } else {
            sx = x - (left = view_loc.x);
            top = view_loc.y;
            sy = (basey = view_basey) - y;
            right = left + view_w;
            bottom = top + view_h;
            grph = viewGraph;

            if (dispPrefs.penMode == pen_trackpt) { // Snap to curve
                selsx = sx; // Keep current pen position (in polar or param, sx is modified)
                if (graphPrefs.functype == graph_func) {
                    viewGraph.grtaps_track_func(&sx, &sy, &calcx, &calcy, curve_nb);
                    param = calcx;
                } else if (graphPrefs.functype == graph_polar) {
                    viewGraph.grtaps_track_pol(&sx, &sy, &calcx, &calcy, &param, &calcr, curve_nb);
                } else {
                    viewGraph.grtaps_track_param(&sx, &sy, &calcx, &calcy, &param, curve_nb);
                }
            } else { // Just where the pen is ..
                param = calcx = viewGraph.graph_xscr2gr(sx);
                calcy = viewGraph.graph_yscr2gr(sy);
            }
        }

        // Erase and draw a cross only if in curve track mode
        if (dispPrefs.penMode == pen_trackpt) {
            // Cut out positions which can't be drawn
            if ((sx < 0) || (sx > grph.ScrPrefs.xmax))
                sx = -1;
            if ((sy < 0) || (sy > grph.ScrPrefs.ymax))
                sy = -1;
            // 4 possible cases for horizontal line:
            // - remove hor. line <== remove && exist(prev) && (!set || !exist(new))
            // - remove hor. line and draw a new line
            //      <== remove && exist(prev) && set && exist(new) && (prev != new)
            // - draw a new line <== (!remove || !exist(prev)) && set && exist(new)
            // - remain with existing line or no-line situation <== rest of possibilities
            // The above decision tree is combined and simplified (Karnaugh diagram ..)
            if (removeCross_y && (prevsy != -1)) {
                if (!setCross_y || (sy == -1)) {
                    InvertRect(hdc, &cross_rcy); // Remove previous horizontal part of cross
                    prevsy = -1; // Remember that no line set
                } else if (sy != prevsy) {
                    InvertRect(hdc, &cross_rcy); // Remove previous horizontal part of cross
                    cross_rcy.top = basey - sy;
                    cross_rcy.bottom = cross_rcy.top + 1;
                    cross_rcy.left = left;
                    cross_rcy.right = right;
                    InvertRect(hdc, &cross_rcy); // Set new one
                    prevsy = sy;
                }
            } else if (setCross_y && (sy != -1)) {
                cross_rcy.top = basey - sy;
                cross_rcy.bottom = cross_rcy.top + 1;
                cross_rcy.left = left;
                cross_rcy.right = right;
                InvertRect(hdc, &cross_rcy);
                prevsy = sy;
            }
            // 4 possible cases for vertical line:
            // - remove vert. line <== remove && exist(prev) && (!set || !exist(new))
            // - remove vert. line and draw a new line
            //      <== remove && exist(prev) && set && exist(new) && (prev != new)
            // - draw a new line <== (!remove || !exist(prev)) && set && exist(new)
            // - remain with existing line or no-line situation <== rest of possibilities
            // The above decision tree is combined and simplified (Karnaugh diagram ..)
            if (removeCross_x && (prevsx != -1)) {
                if (!setCross_x || (sx == -1)) {
                    InvertRect(hdc, &cross_rcx); // Remove previous vertical part of cross
                    prevsx = -1; // Remember that no line set
                } else if (sx != prevsx) {
                    InvertRect(hdc, &cross_rcx); // Remove previous vertical part of cross
                    cross_rcx.top = top;
                    cross_rcx.bottom = bottom;
                    cross_rcx.left = sx + left;
                    cross_rcx.right = cross_rcx.left + 1;
                    InvertRect(hdc, &cross_rcx); // Set new one
                    prevsx = sx;
                }
            } else if (setCross_x && (sx != -1)) {
                cross_rcx.top = top;
                cross_rcx.bottom = bottom;
                cross_rcx.left = sx + left;
                cross_rcx.right = cross_rcx.left + 1;
                InvertRect(hdc, &cross_rcx); // Set new one
                prevsx = sx;
            }
            // 4 possible cases for selection zone:
            // - remove sel zone <== remove && exist(prev) && !set
            // - remove sel zone and draw a new zone
            //      <== remove && exist(prev) && set && exist(new) && (prev != new)
            // - draw a new sel zone <== (!remove || !exist(prev)) && set && exist(new)
            // - remain with existing sel zone or no-sel zone situation <== rest of possibilities
            // The above decision tree is combined and simplified (Karnaugh diagram ..)
            if (is_cross) {
                selsx0 = selsx; // Remember sel zone start point
            }
            if (removeSel && (selprevsx != -1)) {
                if (!setSel) {
                    InvertRect(hdc, &sel_rc); // Remove previous sel zone
                    selprevsx = -1; // Remember that no sel zone set
                } else if (selsx != selprevsx) {
                    InvertRect(hdc, &sel_rc); // Remove previous sel zone
                    sel_rc.top = top;
                    sel_rc.bottom = bottom;
                    if (selsx0 <= selsx) {
                        sel_rc.left = selsx0 + left;
                        sel_rc.right = selsx + left + 1;
                    } else {
                        sel_rc.left = selsx + left;
                        sel_rc.right = selsx0 + left + 1;
                    }
                    InvertRect(hdc, &sel_rc); // Set new one
                    selprevsx = selsx;
                }
            } else if (setSel && (selsx != -1)) {
                sel_rc.top = top;
                sel_rc.bottom = bottom;
                if (selsx0 <= selsx) {
                    sel_rc.left = selsx0 + left;
                    sel_rc.right = selsx + left + 1;
                } else {
                    sel_rc.left = selsx + left;
                    sel_rc.right = selsx0 + left + 1;
                }
                InvertRect(hdc, &sel_rc); // Set new one
                selprevsx = selsx;
            }
        } else if (dispPrefs.penMode == pen_selzone) {
            if (hPenSelZone == NULL) {
                hPenSelZone = CreatePen(PS_DASH, 1, graphColors[6]);
            }
            int oldBkMode = SetBkMode(hdc, OPAQUE);
            int oldBkColor = SetBkColor(hdc, GR_WHITE);
            int oldMixMode = SetROP2(hdc, R2_XORPEN); // Draw in XOR mode ...
            HGDIOBJ oldPen = SelectObject(hdc, hPenSelZone);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            // 4 possible cases for selection zone:
            // - remove sel zone <== remove && exist(prev) && !set
            // - remove sel zone and draw a new zone
            //      <== remove && exist(prev) && set && exist(new) && (prev != new)
            // - draw a new sel zone <== (!remove || !exist(prev)) && set && exist(new)
            // - remain with existing sel zone or no-sel zone situation <== rest of possibilities
            // The above decision tree is combined and simplified (Karnaugh diagram ..)
            if (is_cross) {
                selsx0 = sx; // Remember sel zone start point
                selsy0 = sy;
            }
            if (removeSel && (selprevsx != -1)) {
                if (!setSel) {
                    // Remove previous sel rectangle
                    Rectangle(hdc, sel_rc.left, sel_rc.top, sel_rc.right, sel_rc.bottom);
                    selprevsx = -1; // Remember that no sel rectangle set
                } else if ((sx != selprevsx) || (sy != selprevsy)){
                    // Remove previous sel rectangle
                    Rectangle(hdc, sel_rc.left, sel_rc.top, sel_rc.right, sel_rc.bottom);
                    if (selsy0 <= sy) {
                        sel_rc.top = bottom - sy - 1;
                        sel_rc.bottom = bottom - selsy0;
                    } else {
                        sel_rc.top = bottom - selsy0 - 1;
                        sel_rc.bottom = bottom - sy;
                    }
                    if (selsx0 <= sx) {
                        sel_rc.left = selsx0 + left;
                        sel_rc.right = sx + left + 1;
                    } else {
                        sel_rc.left = sx + left;
                        sel_rc.right = selsx0 + left + 1;
                    }
                    // Set new one
                    Rectangle(hdc, sel_rc.left, sel_rc.top, sel_rc.right, sel_rc.bottom);
                    selprevsx = sx;
                    selprevsy = sy;
                }
            } else if (setSel && (sx != -1)) {
                if (selsy0 <= sy) {
                    sel_rc.top = bottom - sy - 1;
                    sel_rc.bottom = bottom - selsy0;
                } else {
                    sel_rc.top = bottom - selsy0 - 1;
                    sel_rc.bottom = bottom - sy;
                }
                if (selsx0 <= selsx) {
                    sel_rc.left = selsx0 + left;
                    sel_rc.right = sx + left + 1;
                } else {
                    sel_rc.left = sx + left;
                    sel_rc.right = selsx0 + left + 1;
                }
                // Set new one
                Rectangle(hdc, sel_rc.left, sel_rc.top, sel_rc.right, sel_rc.bottom);
                selprevsx = sx;
                selprevsy = sy;
            }
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            SetROP2(hdc, oldMixMode);
            SetBkColor(hdc, oldBkColor);
            SetBkMode(hdc, oldBkMode);
        } else if (dispPrefs.penMode == pen_movzone) {
            if (is_cross) {
                selsx0 = sx; // Remember start point
                selsy0 = sy;
            }
            if (removeSel && (selprevsx != -1)) {
                if (!setSel) {
                    // Restore graph as normal
                    if (selArea == ZONE_AREA) {
                        BitBlt(hdc, display_loc.x, display_loc.y, display_w, display_h,
                               zoneDC, 0, 0, SRCCOPY);
                        // Redraw zone annunciators
                        HDC memdc = CreateCompatibleDC(hdc);
                        for (int i=0 ; i<=ANN_SG ; i++)
                            repaint_annunciator(hdc, memdc, i, ann_state[i]);
                        DeleteDC(memdc);
                    } else {
                        BitBlt(hdc, view_loc.x, view_loc.y, view_w, view_h,
                               viewDC, 0, 0, SRCCOPY);
                        // Redraw zone on view
                        draw_zoneOnView(hdc);
                    }
                    selprevsx = -1; // Remember that no sel rectangle set
                } else if ((sx != selprevsx) || (sy != selprevsy)){
                    // Restore graph as normal
                    if ((selArea == ZONE_AREA) && (selArea != area)) {
                        BitBlt(hdc, display_loc.x, display_loc.y, display_w, display_h,
                               zoneDC, 0, 0, SRCCOPY);
                        // Redraw zone annunciators
                        HDC memdc = CreateCompatibleDC(hdc);
                        for (int i=0 ; i<=ANN_SG ; i++)
                            repaint_annunciator(hdc, memdc, i, ann_state[i]);
                        DeleteDC(memdc);
                    } else if ((selArea == WVIEW_AREA) && (selArea != area)) {
                        BitBlt(hdc, view_loc.x, view_loc.y, view_w, view_h,
                               viewDC, 0, 0, SRCCOPY);
                        // Redraw zone on view
                        draw_zoneOnView(hdc);
                    }
                    // Set new one
                    dragGraph(hdc, area, sx - selsx0, selsy0 - sy);
                    selprevsx = sx;
                    selprevsy = sy;
                }
            } else if (setSel && (sx != -1)) {
                dragGraph(hdc, area, sx - selsx0, selsy0 - sy);
                selprevsx = sx;
                selprevsy = sy;
            }
        } else if (dispPrefs.penMode == pen_centerwide) {
            if (is_cross) {
                selsx0 = sx; // Remember start point
                selsy0 = sy;
            }
            if (removeSel && (selprevsx != -1)) {
                if (!setSel) {
                    // Restore graph as normal
                    if (selArea == ZONE_AREA) {
                        BitBlt(hdc, display_loc.x, display_loc.y, display_w, display_h,
                               zoneDC, 0, 0, SRCCOPY);
                        // Redraw zone annunciators
                        HDC memdc = CreateCompatibleDC(hdc);
                        for (int i=0 ; i<=ANN_SG ; i++)
                            repaint_annunciator(hdc, memdc, i, ann_state[i]);
                        DeleteDC(memdc);
                    } else {
                        // Remove temp zone on view
                        draw_tempZoneOnView(hdc);
                        // Redraw zone on view
                        draw_zoneOnView(hdc);
                    }
                    selprevsx = -1; // Remember that no sel rectangle set
                } else if ((sx != selprevsx) || (sy != selprevsy)){
                    // Restore graph as normal
                    if ((selArea == ZONE_AREA) && (selArea != area)) {
                        BitBlt(hdc, display_loc.x, display_loc.y, display_w, display_h,
                               zoneDC, 0, 0, SRCCOPY);
                        // Redraw zone annunciators
                        HDC memdc = CreateCompatibleDC(hdc);
                        for (int i=0 ; i<=ANN_SG ; i++)
                            repaint_annunciator(hdc, memdc, i, ann_state[i]);
                        DeleteDC(memdc);
                    } else if ((selArea == WVIEW_AREA) && (selArea != area)) {
                        // Remove temp zone on view
                        draw_tempZoneOnView(hdc);
                        // Redraw zone on view
                        draw_zoneOnView(hdc);
                    }
                    if (area == ZONE_AREA) {
                        // Set new one
                        dragGraph(hdc, area, sx - selsx0, selsy0 - sy);
                    } else {
                        // Remove temp zone on view
                        draw_tempZoneOnView(hdc);
                        // Set new temp zone on view coordinates
                        set_tempZoneOnView(sx + left, basey - sy);
                        // Draw new temp zone on view
                        draw_tempZoneOnView(hdc);
                    }
                    selprevsx = sx;
                    selprevsy = sy;
                }
            } else if (setSel && (sx != -1)) {
                if (area == ZONE_AREA) {
                    // Set new one
                    dragGraph(hdc, area, sx - selsx0, selsy0 - sy);
                } else {
                    // Set new temp zone on view coordinates
                    set_tempZoneOnView(sx + left, basey - sy);
                    // Remove zone on view
                    draw_zoneOnView(hdc);
                    // Draw new temp zone on view
                    draw_tempZoneOnView(hdc);
                }
                selprevsx = sx;
                selprevsy = sy;
            }
        }
    }
    selArea = area;

    repaint_grphVal(hdc, ANN_PARAM0);
    repaint_grphVal(hdc, ANN_PARAM);
    repaint_grphVal(hdc, ANN_YVALUE);
    repaint_grphVal(hdc, ANN_XVALUE);
    repaint_grphVal(hdc, ANN_RVALUE);
}

/********************************************************************************
 * OS specific: display graph annunciator values.                               *
 * Corresponds to grtaps_print_val() of grtaps.c in original code.              *
 ********************************************************************************/
void Skin::repaint_grphVal (HDC hdc, int annun) {
    RECT rc;
    SkinRect *sr = &(annunciators[annun].disp_rect);
    rc.top  = sr->y;
    rc.left = sr->x;
    rc.bottom = rc.top + sr->height;
    rc.right  = rc.left + sr->width;
    FillRect(hdc, &rc, editBgBrush);

    // Set colors and font
    COLORREF oldFg = SetTextColor(hdc, display_fg);
    int oldBg = SetBkMode(hdc, TRANSPARENT);
    if (hFontVerySmall_display == NULL) {
        LOGFONT lf;
        // Clear out the lf structure to use when creating the font.
        memset(&lf, 0, sizeof(LOGFONT));
        lf.lfHeight = VERYSMALLFONT_SIZE * magnification;
//        lf.lfWeight = FW_BOLD;
        _tcscpy(lf.lfFaceName, _T("Tahoma"));
        hFontVerySmall_display = CreateFontIndirect(&lf);
    }
    HFONT oldFont = (HFONT)SelectObject(hdc, hFontVerySmall_display);

    // Display text
#define TXTSZ_GRPHANNUN 31
    TCHAR text[TXTSZ_GRPHANNUN+1];
    bool txt = true;
    switch (annun) {
        case ANN_ZONMAXY:
            //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), graphPrefs.ymax);
            fp_print_g_double(text, graphPrefs.ymax, 6);
            break;
        case ANN_ZONMINY:
            //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), graphPrefs.ymin);
            fp_print_g_double(text, graphPrefs.ymin, 6);
            break;
        case ANN_ZONMINX:
            //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), graphPrefs.xmin);
            fp_print_g_double(text, graphPrefs.xmin, 6);
            break;
        case ANN_ZONMAXX:
            //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), graphPrefs.xmax);
            fp_print_g_double(text, graphPrefs.xmax, 6);
            break;
        case ANN_WONZ_XR:
            //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), wonz_xr);
            fp_print_g_double(text, wonz_xr, 5);
            break;
        case ANN_WONZ_YR:
            //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), wonz_yr);
            fp_print_g_double(text, wonz_yr, 5);
            break;
        case ANN_PARAM0:
            if (!isnan(param0)) {
                //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), param0);
                fp_print_g_double(text, param0, 6);
            } else {
                txt = false;
            }
            break;
        case ANN_PARAM:
            if (!isnan(param)) {
                //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), param);
                fp_print_g_double(text, param, 6);
            } else {
                txt = false;
            }
            break;
        case ANN_YVALUE:
            if (!isnan(calcy)) {
                //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), calcy);
                fp_print_g_double(text, calcy, 6);
            } else {
                txt = false;
            }
            break;
        case ANN_XVALUE:
            if (!isnan(calcx)) {
                //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), calcx);
                fp_print_g_double(text, calcx, 6);
            } else {
                txt = false;
            }
            break;
        case ANN_RVALUE:
            if (!isnan(calcr)) {
                //_sntprintf(text, TXTSZ_GRPHANNUN, _T("%.4G"), calcr);
                fp_print_g_double(text, calcr, 6);
            } else {
                txt = false;
            }
            break;
        case ANN_CURVENB:
            if (curve_nb >= 0) {
                _sntprintf(text, TXTSZ_GRPHANNUN, _T("Y%d"), curve_nb+1);
            } else {
                txt = false;
            }
            break;
    }
    if (txt)
        DrawText(hdc, text, -1, &rc, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, oldFont); // Set back default object
}

/********************************************************************************
 * Refresh ratio.                                                               *
 ********************************************************************************/
void Skin::refreshWonzRatio (HDC hdc) {
    wonz_xr = 2 * span / (graphPrefs.xmax - graphPrefs.xmin);
    wonz_yr = 2 * span / (graphPrefs.ymax - graphPrefs.ymin);
    if (hdc != NULL) {
        repaint_grphVal(hdc, ANN_WONZ_XR);
        repaint_grphVal(hdc, ANN_WONZ_YR);
    }
    calc_zoneOnView(); // Refresh bounds of zone inside wide view
}

/********************************************************************************
 * Zoom in or zoom out.                                                         *
 ********************************************************************************/
void Skin::scaleGraph (HDC hdc, int area, double scale_out) {
    if (area == ZONE_AREA) {
        if ((dispPrefs.penMode == pen_selzone) && is_selecting && (scale_out < 1.0)
            && (selx0 != calcx) && (sely0 != calcy)) {
            // Zoom to selected area in selzone mode
            if (selx0 < calcx) {
                graphPrefs.xmin = selx0;
                graphPrefs.xmax = calcx;
            } else {
                graphPrefs.xmin = calcx;
                graphPrefs.xmax = selx0;
            }
            if (sely0 < calcy) {
                graphPrefs.ymin = sely0;
                graphPrefs.ymax = calcy;
            } else {
                graphPrefs.ymin = calcy;
                graphPrefs.ymax = sely0;
            }
            repaint_grphVal(hdc, ANN_ZONMAXY);
            repaint_grphVal(hdc, ANN_ZONMINY);
            repaint_grphVal(hdc, ANN_ZONMINX);
            repaint_grphVal(hdc, ANN_ZONMAXX);
            draw_zoneOnView(hdc); // Toggle off zone inside view
            refreshWonzRatio(hdc);
            draw_zoneOnView(hdc); // Toggle on zone inside view
            trigRecalcZone();
        } else {
            // Apply ratio to zone, refresh whole graph
            double cx = (graphPrefs.xmin + graphPrefs.xmax) / 2.0;
            double cy = (graphPrefs.ymin + graphPrefs.ymax) / 2.0;
            double spanx = (graphPrefs.xmax - graphPrefs.xmin) / 2.0 * scale_out;
            double spany = (graphPrefs.ymax - graphPrefs.ymin) / 2.0 * scale_out;
            graphPrefs.xmin = cx - spanx;
            graphPrefs.xmax = cx + spanx;
            graphPrefs.ymin = cy - spany;
            graphPrefs.ymax = cy + spany;
            repaint_grphVal(hdc, ANN_ZONMAXY);
            repaint_grphVal(hdc, ANN_ZONMINY);
            repaint_grphVal(hdc, ANN_ZONMINX);
            repaint_grphVal(hdc, ANN_ZONMAXX);
            draw_zoneOnView(hdc); // Toggle off zone inside view
            double vX, vY, s;
            vX = viewX;
            vY = viewY;
            s = span;
            refreshView(hdc);
            if ((vX != viewX) || (vY != viewY) || (s != span))
                trigRecalcGraph(); // Recalc both
            else {
                draw_zoneOnView(hdc); // Toggle on zone inside view
                trigRecalcZone();
            }
        }
    } else {
        if ((dispPrefs.penMode == pen_selzone) && is_selecting && (scale_out < 1.0)
            && (selx0 != calcx) && (sely0 != calcy)) {
            // Zoom to selected area in selzone mode
            viewX = (selx0 + calcx) / 2.0;
            viewY = (sely0 + calcy) / 2.0;
            double spanx = calcx - selx0;
            if (spanx < 0.0)   spanx = -spanx;
            double spany = calcy - sely0;
            if (spany < 0.0)   spany = -spany;
            span = ((spanx > spany) ? spanx : spany) / 2.0;
        } else {
            // Apply ratio to wide view, refresh only wide view
            viewX = (graphPrefs.xmin + graphPrefs.xmax) / 2.0;
            viewY = (graphPrefs.ymin + graphPrefs.ymax) / 2.0;
            span *= scale_out;
        }
        refreshWonzRatio(hdc);
        trigRecalcView();
    }
}

/********************************************************************************
 * Center view on zone.                                                         *
 ********************************************************************************/
void Skin::centerView (HDC hdc) {
    // Center on zone, and size it to 3 times the biggest zone edge
    viewX = (graphPrefs.xmin + graphPrefs.xmax) / 2.0;
    viewY = (graphPrefs.ymin + graphPrefs.ymax) / 2.0;
    span = max((graphPrefs.xmax - graphPrefs.xmin),
               (graphPrefs.ymax - graphPrefs.ymin)
              ) * 1.5;
    refreshWonzRatio(hdc);
}

/********************************************************************************
 * Refresh view, depending on selected mode.                                    *
 ********************************************************************************/
void Skin::refreshView (HDC hdc) {
    if (dispPrefs.penMode == pen_centerwide)
        centerView(hdc);
    else
        refreshWonzRatio(hdc);
}

void Skin::refreshViewAndTrigRecalc (HDC hdc) {
    double vX, vY, s;
    vX = viewX;
    vY = viewY;
    s = span;
    refreshView(hdc);
    if ((vX != viewX) || (vY != viewY) || (s != span))
        trigRecalcView();
}

/********************************************************************************
 * Move center to (X, Y), keeping current span.                                 *
 ********************************************************************************/
void Skin::moveGraph (HDC hdc, int area, double cx, double cy) {
    if (area == ZONE_AREA) {
        // Move zone, refresh whole graph
        double spanx = (graphPrefs.xmax - graphPrefs.xmin) / 2.0;
        double spany = (graphPrefs.ymax - graphPrefs.ymin) / 2.0;
        graphPrefs.xmin = cx - spanx;
        graphPrefs.xmax = cx + spanx;
        graphPrefs.ymin = cy - spany;
        graphPrefs.ymax = cy + spany;
        repaint_grphVal(hdc, ANN_ZONMAXY);
        repaint_grphVal(hdc, ANN_ZONMINY);
        repaint_grphVal(hdc, ANN_ZONMINX);
        repaint_grphVal(hdc, ANN_ZONMAXX);
        draw_zoneOnView(hdc); // Toggle off zone inside view
        double vX, vY, s;
        vX = viewX;
        vY = viewY;
        s = span;
        refreshView(hdc);
        if ((vX != viewX) || (vY != viewY) || (s != span))
            trigRecalcGraph(); // Recalc both
        else {
            draw_zoneOnView(hdc); // Toggle on zone inside view
            trigRecalcZone();
        }
    } else {
        // Move wide view, refresh only wide view
        viewX = cx;
        viewY = cy;
        trigRecalcView();
    }
}

/********************************************************************************
 * Normalize zone dimension to a square => spanx = spany, keeping greater.      *
 ********************************************************************************/
void Skin::normZone (HDC hdc) {
    double cx = (graphPrefs.xmin + graphPrefs.xmax) / 2.0;
    double cy = (graphPrefs.ymin + graphPrefs.ymax) / 2.0;
    double spanx = (graphPrefs.xmax - graphPrefs.xmin) / 2.0;
    double spany = (graphPrefs.ymax - graphPrefs.ymin) / 2.0;
    if (spanx > spany)  spany = spanx;
    else   spanx = spany;
    graphPrefs.xmin = cx - spanx;
    graphPrefs.xmax = cx + spanx;
    graphPrefs.ymin = cy - spany;
    graphPrefs.ymax = cy + spany;
    repaint_grphVal(hdc, ANN_ZONMAXY);
    repaint_grphVal(hdc, ANN_ZONMINY);
    repaint_grphVal(hdc, ANN_ZONMINX);
    repaint_grphVal(hdc, ANN_ZONMAXX);
    draw_zoneOnView(hdc); // Toggle off zone inside view
    double vX, vY, s;
    vX = viewX;
    vY = viewY;
    s = span;
    refreshView(hdc);
    if ((vX != viewX) || (vY != viewY) || (s != span))
        trigRecalcGraph(); // Recalc both
    else {
        draw_zoneOnView(hdc); // Toggle on zone inside view
        trigRecalcZone();
    }
}

/********************************************************************************
 * Select proper pen color for graph drawing.                                   *
 ********************************************************************************/
void Skin::selPen (int i) {
    if (graphPens[i] == NULL) {
        graphPens[i] = CreatePen(PS_SOLID, 0, graphColors[i]);
    }
    SelectObject(displayDC, graphPens[i]);
    if (graphState <= grphZoneComplete) {
        SelectObject(zoneDC, graphPens[i]);
    } else {
        SelectObject(viewDC, graphPens[i]);
    }
}

static int oldx, oldy;
/********************************************************************************
 * Draws a line on the graph screen.                                            *
 ********************************************************************************/
void Skin::drawline (int x1, int y1, int x2, int y2) {
    if (graphState <= grphZoneComplete) {
        if ((oldx != x1) || (oldy != y1)) {
            MoveToEx(displayDC, display_loc.x+x1, display_basey-y1, NULL);
            MoveToEx(zoneDC, x1, zonebmp_basey-y1, NULL);
        }
        LineTo(displayDC, display_loc.x+x2, display_basey-y2);
        LineTo(zoneDC, x2, zonebmp_basey-y2);
    } else {
        if ((oldx != x1) || (oldy != y1)) {
            MoveToEx(displayDC, view_loc.x+x1, view_basey-y1, NULL);
            MoveToEx(viewDC, x1, viewbmp_basey-y1, NULL);
        }
        LineTo(displayDC, view_loc.x+x2, view_basey-y2);
        LineTo(viewDC, x2, viewbmp_basey-y2);
    }

    oldx = x2;
    oldy = y2;
}

// Given that LineTo doesn't draw its last point, whenever we have a discontinuity
// in line (getting out of screen or else), we need to draw the last point.
void Skin::finishline (int color) {
    if (graphState <= grphZoneComplete) {
        SetPixel(displayDC, display_loc.x+oldx, display_basey-oldy, color);
        SetPixel(zoneDC, oldx, zonebmp_basey-oldy, color);
    } else {
        SetPixel(displayDC, view_loc.x+oldx, view_basey-oldy, color);
        SetPixel(viewDC, oldx, viewbmp_basey-oldy, color);
    }
}

/********************************************************************************
 * Trigger recalc & repaint of graphes.                                         *
 ********************************************************************************/
void Skin::trigRecalcGraph (void) {
    RECT rc;

    stop_graph();
    recalc = true;

    rc.top  = display_loc.y;
    rc.left = display_loc.x;
    rc.bottom = rc.top + display_h;
    rc.right  = rc.left + display_w;
    InvalidateRect(g_hWnd, &rc, FALSE);

    rc.top  = view_loc.y;
    rc.left = view_loc.x;
    rc.bottom = rc.top + view_h;
    rc.right  = rc.left + view_w;
    InvalidateRect(g_hWnd, &rc, FALSE);

    UpdateWindow(g_hWnd);
}

void Skin::trigRecalcZone (void) {
    RECT rc;

    stop_graph();
    recalc_zone = true;

    rc.top  = display_loc.y;
    rc.left = display_loc.x;
    rc.bottom = rc.top + display_h;
    rc.right  = rc.left + display_w;
    InvalidateRect(g_hWnd, &rc, FALSE);

    UpdateWindow(g_hWnd);
}

void Skin::trigRecalcView (void) {
    RECT rc;

    stop_graph();
    recalc_view = true;

    rc.top  = view_loc.y;
    rc.left = view_loc.x;
    rc.bottom = rc.top + view_h;
    rc.right  = rc.left + view_w;
    InvalidateRect(g_hWnd, &rc, FALSE);

    UpdateWindow(g_hWnd);
}

/********************************************************************************
 * Reset selections.                                                            *
 ********************************************************************************/
void Skin::resetGraphSel (void) {
    selArea = prevsx = prevsy = selprevsx = -1; // No selection
    is_cross = is_selecting = false;
    param0 = param = calcx = calcy = calcr = NaN; // No cross
}

/********************************************************************************
 * Reset zone state.                                                            *
 ********************************************************************************/
void Skin::resetZone (void) {
    graphComplete = zoneComplete = false;
    graphState = grphStart;
    if (selArea == ZONE_AREA)
        resetGraphSel();
}

/********************************************************************************
 * Reset view state.                                                            *
 ********************************************************************************/
void Skin::resetView (void) {
    graphComplete = viewComplete = false;
    graphState = grphViewStart;
    if (selArea == WVIEW_AREA)
        resetGraphSel();
}

/********************************************************************************
 * Switch cross / selection on-off, for ex. to be able to redraw annunciators.  *
 ********************************************************************************/
void Skin::switchSelGraph (HDC hdc) {
    if (dispPrefs.penMode == pen_trackpt) {
        if (is_cross || is_selecting) { // Draw cross and/or sel zone again
            if (prevsy != -1)
                InvertRect(hdc, &cross_rcy);
            if (prevsx != -1)
                InvertRect(hdc, &cross_rcx);
            if (is_selecting && (selprevsx != -1)) // Set again selection zone
                InvertRect(hdc, &sel_rc);
        }
    } else if (dispPrefs.penMode == pen_selzone) {
        if (is_selecting && (selprevsx != -1)) { // Draw sel rectangle again
            int oldBkMode = SetBkMode(hdc, OPAQUE);
            int oldBkColor = SetBkColor(hdc, GR_WHITE);
            int oldMixMode = SetROP2(hdc, R2_XORPEN); // Draw in XOR mode ...
            HGDIOBJ oldPen = SelectObject(hdc, hPenSelZone);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, sel_rc.left, sel_rc.top, sel_rc.right, sel_rc.bottom);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            SetROP2(hdc, oldMixMode);
            SetBkColor(hdc, oldBkColor);
            SetBkMode(hdc, oldBkMode);
        }
    }
}

/********************************************************************************
 * Start graph.                                                                 *
 ********************************************************************************/
int Skin::initGraph (void) {
    if (zoneDC == NULL) {
        zoneDC = CreateCompatibleDC(displayDC);
    }
    if (zoneBmp == NULL) {
        zoneBmp = CreateCompatibleBitmap(displayDC, display_w, display_h);
    }
    if ((zoneDC == NULL) || (zoneBmp == NULL)) {
        return (-1);
    }
    if (oldZoneObj == NULL) {
        oldZoneObj = SelectObject(zoneDC, zoneBmp);
    }

    if (zone_rgn == NULL) {
        zone_rgn = CreateRectRgn(display_loc.x, display_loc.y,
                                 display_loc.x + display_w, display_loc.y + display_h);
    }

    if (zgph_rgn == NULL) {
        zgph_rgn = CreateRectRgn(1, 1, 3, 3);
    }

    if (zcomplement_rgn == NULL) {
        zcomplement_rgn = CreateRectRgn(1, 1, 3, 3);
    }

    if (view_loc.x != -1) {
        return (initView());
    }

    return (0);
}

int Skin::initView (void) {
    if (viewDC == NULL) {
        viewDC = CreateCompatibleDC(displayDC);
    }
    if (viewBmp == NULL) {
        viewBmp = CreateCompatibleBitmap(displayDC, view_w, view_h);
    }
    if ((viewDC == NULL) || (viewBmp == NULL)) {
        return (-2);
    }
    if (oldViewObj == NULL) {
        oldViewObj = SelectObject(viewDC, viewBmp);
    }

    if (view_rgn == NULL) {
        view_rgn = CreateRectRgn(view_loc.x, view_loc.y,
                                 view_loc.x + view_w, view_loc.y + view_h);
    }

    if (vgph_rgn == NULL) {
        vgph_rgn = CreateRectRgn(1, 1, 3, 3);
    }

    if (vcomplement_rgn == NULL) {
        vcomplement_rgn = CreateRectRgn(1, 1, 3, 3);
    }

    return (0);
}

/********************************************************************************
 * Clear zone graph.                                                            *
 ********************************************************************************/
void Skin::clearZoneGraph (void) {
    /* Erase drawing area */
    RECT rc;
    rc.top  = display_loc.y;
    rc.left = display_loc.x;
    rc.bottom = rc.top + display_h;
    rc.right  = rc.left + display_w;
    FillRect(displayDC, &rc, editBgBrush);
    rc.top  = 0;
    rc.left = 0;
    rc.bottom = display_h;
    rc.right  = display_w;
    FillRect(zoneDC, &rc, editBgBrush);
}

/********************************************************************************
 * Clear view graph.                                                            *
 ********************************************************************************/
void Skin::clearViewGraph (void) {
    /* Erase drawing area */
    RECT rc;
    rc.top  = view_loc.y;
    rc.left = view_loc.x;
    rc.bottom = rc.top + view_h;
    rc.right  = rc.left + view_w;
    FillRect(displayDC, &rc, editBgBrush);
    rc.top  = 0;
    rc.left = 0;
    rc.bottom = view_h;
    rc.right  = view_w;
    FillRect(viewDC, &rc, editBgBrush);
}

/********************************************************************************
 * Draw graph with functions on screen (asynchronous proc).                     *
 ********************************************************************************/
DWORD Skin::drawgraph_async (void) {
    switch (graphState) {
        case grphViewStart:
            if (view_loc.x != -1) {
                displayDC = GetDC(g_hWnd);
                int rc = initView();
                if (rc != 0) {
                    ReleaseDC(g_hWnd, displayDC);
                    displayDC = NULL;
                    return (rc);
                }
                graphState = grphZoneComplete;
                goto l_grphZoneComplete;
            }
            graphState = grphEnd;
            graphComplete = true;
            continueOnView = false;
            break;

        case grphStart:
            displayDC = GetDC(g_hWnd);
            {
            int rc = initGraph();
            if (rc != 0) {
                ReleaseDC(g_hWnd, displayDC);
                displayDC = NULL;
                return (rc);
            }
            }
            graphState = grphInitComplete;

        case grphInitComplete:
            clearZoneGraph();
            zoneGraph.graph_draw_start(this, display_w, display_h, &graphPrefs);
            oldx = oldy = -1;
            graphState = grphZoneStarted;

        case grphZoneStarted:
            if (!is_graph_active()) {
                ReleaseDC(g_hWnd, displayDC);
                displayDC = NULL;
                return (-1);
            }
            zoneGraph.graph_draw_incr(this);
            if (!is_graph_complete()) {
                break; // Come back on it as many times as needed
            }
            graphState = grphZoneComplete;

        case grphZoneComplete:
            // Force redraw of zone + annunciators
            {
            RECT rc;
            rc.top  = display_loc.y;
            rc.left = display_loc.x;
            rc.bottom = rc.top + display_h;
            rc.right  = rc.left + display_w;
            InvalidateRect(g_hWnd, &rc, FALSE);
            }
            if (!continueOnView || (view_loc.x == -1)) {
                if (view_loc.x != -1) { // Erase zone highlight inside view
                    draw_zoneOnView(displayDC);
                }
                graphState = grphEnd;
                goto l_grphEnd;
            }
l_grphZoneComplete:
            clearViewGraph();
            graphState = grphViewStarted;
            viewPrefs.xmin = viewX - span;
            viewPrefs.xmax = viewX + span;
            viewPrefs.ymin = viewY - span;
            viewPrefs.ymax = viewY + span;
            viewGraph.graph_draw_start(this, view_w, view_h, &viewPrefs);
            oldx = oldy = -1;

        case grphViewStarted:
            if (!is_graph_active()) {
                ReleaseDC(g_hWnd, displayDC);
                displayDC = NULL;
                return (-1);
            }
            viewGraph.graph_draw_incr(this);
            if (!is_graph_complete()) {
                break; // Come back on it as many times as needed
            }
            graphState = grphEnd;

        case grphEnd:
l_grphEnd:
            if (view_loc.x != -1) { // Draw zone highlight inside view
                calc_zoneOnView(); // Calculate bounds of zone inside wide view
                draw_zoneOnView(displayDC);
            }
            graphComplete = true;
            continueOnView = false;
            ReleaseDC(g_hWnd, displayDC);
            displayDC = NULL;
            break;
    }

    return (0);
}

/********************************************************************************
 * Draw graph with functions on screen (thread proc).                           *
 ********************************************************************************/
//DWORD Skin::drawgraph_thread (void) {
//    displayDC = GetDC(g_hWnd);
//    int rc;
//    if ((rc = initGraph()) != 0)   return (rc);
//
//    if (zoneComplete) {
//        BitBlt(displayDC, display_loc.x, display_loc.y, display_w, display_h, zoneDC, 0, 0, SRCCOPY);
//    } else {
//        clearZoneGraph();
//        zoneGraph.graph_draw_start(this, display_w, display_h, &graphPrefs);
//        oldx = oldy = -1;
//        while (is_graph_active() && !is_graph_complete()) {
//            zoneGraph.graph_draw_incr(this);
//        }
//
//        if (is_graph_complete()) {
//            // Copy result to memory for later repaint(s)
//            BitBlt(zoneDC, 0, 0, display_w, display_h, displayDC, display_loc.x, display_loc.y, SRCCOPY);
//            zoneComplete = true;
//        }
//    }
//
//    if (view_loc.x != -1)
//        if (viewComplete) {
////            BitBlt(displayDC, view_loc.x, view_loc.y, view_w, view_h, viewDC, 0, 0, SRCCOPY);
//        } else {
//            clearViewGraph();
////            viewGraph.graph_draw_start(this, view_loc.x, view_loc.y, view_w, view_h, &viewPrefs);
//            oldx = oldy = -1;
////            while (is_graph_active() && !is_graph_complete()) {
////                graph_draw_incr(this);
////            }
//            if (is_graph_complete()) {
//                // Copy result to memory for later repaint(s)
////                BitBlt(viewDC, 0, 0, view_w, view_h, displayDC, view_loc.x, view_loc.y, SRCCOPY);
//                viewComplete = true;
//            }
//        }
//
//    /*Draw grid */
//    //    if (graphPrefs.grEnable[7])
//    //        graph_grid(&natbounds);
//    graphComplete = true;
//    ReleaseDC(g_hWnd, displayDC);
//    // Force redraw of all + annunciators
//    RECT r;
//    r.top  = display_loc.y;
//    r.left = display_loc.x;
//    r.bottom = display_loc.y + display_h;
//    r.right  = r.left + display_w;
//    InvalidateRect(g_hWnd, &r, FALSE);
//    UpdateWindow(g_hWnd);
//
//    return (0);
//}

/********************************************************************************
 * Draw graph thread entry point.                                               *
 ********************************************************************************/
//DWORD WINAPI drawgraph_threadEntry (LPVOID lpParameter) {
//    Skin *skin = (Skin *) lpParameter;
//    DWORD rc;
//
//    do {
//       rc = skin->drawgraph_async();
//    } while ((rc == 0) && !skin->graphComplete);
//
////    ExitThread(rc);
//    return (rc); // This does the ExitThread().
//}

/********************************************************************************
 * Draw graph with functions on screen (uses another thread).                   *
 ********************************************************************************/
int Skin::draw_graph (HDC hdc) {
    int rc = 1;
    if (recalc) {
        recalc_zone = recalc_view = true;
        recalc = false;
    }
    if (!recalc_zone) { // Redraw memorized bitmap
        BitBlt(hdc, display_loc.x, display_loc.y, display_w, display_h, zoneDC, 0, 0, SRCCOPY);
    }
    if (!recalc_view) { // Redraw memorized bitmap
        BitBlt(hdc, view_loc.x, view_loc.y, view_w, view_h, viewDC, 0, 0, SRCCOPY);
    }
    if (recalc_zone || recalc_view) { // Rebuild one or both bitmaps
//        stop_graph();
        continueOnView = (recalc_zone && recalc_view);
        if (recalc_view) {
            resetView();
            recalc_view = false;
        }
        if (recalc_zone) {
            resetZone();
            recalc_zone = false;
        }
        drawgraph_async();
        if (is_graph_active() && !graphComplete)
            rc = 0; // Signal drawing is not complete, and subsequent calls to _async are needed
//        graph_thread = CreateThread(NULL, NULL, drawgraph_threadEntry, this,
//                                    0, NULL);
////        int r = GetThreadPriority(graph_thread);
////        r = SetThreadPriority(graph_thread, THREAD_PRIORITY_NORMAL);
////        r = SetThreadPriority(graph_thread, THREAD_PRIORITY_HIGHEST);
    }
    return (rc);
}

void Skin::stop_graph (void) {
    graph_draw_stop();
//    if (graph_thread != NULL) { // Still running, stop it
//        BOOL rc;
//        DWORD trc;
////        rc = TerminateThread(graph_thread, 0);
//        rc = GetExitCodeThread(graph_thread, &trc);
//        while (trc != STILL_ACTIVE) {
//            Sleep(1);
//            rc = GetExitCodeThread(graph_thread, &trc);
//        }
//        rc = CloseHandle(graph_thread);
//        graph_thread = NULL;
//    }
}

/********************************************************************************
 * Flip temp zone inside view on/off.                                           *
 ********************************************************************************/
void Skin::draw_tempZoneOnView (HDC hdc) {
    if (!tempIntersect)
        return;

    // Draw rectangle
    InvertRect(hdc, &tempzv_intersect);
}

/********************************************************************************
 * Set temp zone inside view coordinates.                                       *
 ********************************************************************************/
void Skin::set_tempZoneOnView (int sx, int sy) {
    // Calculate screen coordinates of the zone rectangle inside view
    int dsx = sx - (zoneOnView.left + zoneOnView.right) / 2;
    int dsy = sy - (zoneOnView.top + zoneOnView.bottom) / 2;
    tempZoneOnView.top = zoneOnView.top + dsy;
    tempZoneOnView.left = zoneOnView.left + dsx;
    tempZoneOnView.bottom = zoneOnView.bottom + dsy;
    tempZoneOnView.right = zoneOnView.right + dsx;

    // Calculate intersection with view rectangle
    tempIntersect = (IntersectRect(&tempzv_intersect, &tempZoneOnView, &viewRect) != 0);
    // Adapt for use by InvertRect .. (one more Windows API oddity !)
    tempzv_intersect.bottom++;
    tempzv_intersect.right++;
}

/********************************************************************************
 * Flip zone inside view on/off.                                                *
 ********************************************************************************/
void Skin::draw_zoneOnView (HDC hdc) {
    if ((is_graph_active() && (graphState >= grphViewStart)) || !intersect)
        return;

    // Draw rectangle
    InvertRect(hdc, &zv_intersect);
}

/********************************************************************************
 * Calculate zone inside view coordinates.                                      *
 ********************************************************************************/
void Skin::calc_zoneOnView (void) {
    // Calculate screen coordinates of the zone rectangle inside view
    graph_get_zoneOnView(&(zoneOnView.top), &(zoneOnView.left),
                         &(zoneOnView.bottom), &(zoneOnView.right));
    zoneOnView.top = view_basey - zoneOnView.top;
    zoneOnView.left += view_loc.x;
    zoneOnView.bottom = view_basey - zoneOnView.bottom;
    zoneOnView.right += view_loc.x;

    // Calculate intersection with view rectangle
    intersect = (IntersectRect(&zv_intersect, &zoneOnView, &viewRect) != 0);
    // Adapt for use by InvertRect .. (one more Windows API oddity !)
    zv_intersect.bottom++;
    zv_intersect.right++;
}
