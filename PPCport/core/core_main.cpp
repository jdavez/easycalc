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
/* core_main.cpp : core functions which are not dependent on the running OS.
 *****************************************************************************/

#include "stdafx.h"
#include "core_main.h"
#include "core/core_globals.h"
#include "core_display.h"
#include "core/Main.h"
#include "core/mlib/fp.h"

#include <stdlib.h>


/*-------------------------------------------------------------------------------
 - Constants.                                                                   -
 -------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------
 - Type declarations.                                                           -
 -------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------
 - Global variables.                                                            -
 -------------------------------------------------------------------------------*/
int repeating = 0;
int repeating_shift;
int repeating_key;


/*-------------------------------------------------------------------------------
 - Module variables.                                                            -
 -------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------
 - Forward declarations.                                                        -
 -------------------------------------------------------------------------------*/
static void stop_interruptible() MAIN_SECT;


/*-------------------------------------------------------------------------------
 - Procedures.                                                                  -
 -------------------------------------------------------------------------------*/
/********************************************************************************
 * FUNCTION: set_shift()                                                        *
 * Set the shift state.                                                         *
 ********************************************************************************/
static void set_shift(bool state) {
    if (mode_shift != state) {
        mode_shift = state;
        shell_annunciators(ANNVAL_UNCH, state, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH);
    }
}


/********************************************************************************
 * FUNCTION: core_repeat()                                                      *
 * This function is called by the shell to signal auto-repeating key events.    *
 * It is the core's responsibility to keep track of *which* key is repeating.   *
 * The function can return 0, to request repeating to stop; 1, which requests   *
 * a slow repeat rate, for SST/BST; or 2, which requests a fast repeat rate,    *
 * for number/alpha entry.                                                      *
 ********************************************************************************/
int core_repeat() {
    // For EasyCalc, no repeat.
//    keydown(repeating_shift, repeating_key);
//    int rpt = repeating;
    int rpt = 0;
    repeating = 0;
    return (rpt);
}

/********************************************************************************
 * FUNCTION: core_keytimeout1()                                                 *
 * This function informs the emulator core that the currently pressed key has   *
 * been held down for 1/4 of a second. (If the key is released less than 1/4    *
 * second after being pressed, this function is not called.)                    *
 * For keys that do not execute immediately, this marks the moment when the     *
 * calculator displays the key's function name.                                 *
 ********************************************************************************/
void core_keytimeout1() {
    // Function not used by EasyCalc
//    if ((pending_command == CMD_LINGER1) || (pending_command == CMD_LINGER2))
//        return;
//    if ((pending_command == CMD_RUN) || (pending_command == CMD_SST)) {
//        int saved_pending_command = pending_command;
//        if (pc == -1)
//            pc = 0;
//        prgm_highlight_row = 1;
//        flags.f.prgm_mode = 2; /* HACK - magic value to tell redisplay() */
//                               /* not to suppress option menu highlights */
//        pending_command = CMD_NONE;
//        redisplay();
//        flags.f.prgm_mode = 0;
//        pending_command = saved_pending_command;
//    } else if ((pending_command != CMD_NONE) && (pending_command != CMD_CANCELLED)
//               && ((cmdlist(pending_command)->flags & FLAG_NO_SHOW) == 0)) {
//        display_command(0);
//        /* If the program catalog was left up by GTO or XEQ,
//         * don't paint over it */
//        if ((mode_transientmenu == MENU_NONE) || (pending_command == CMD_NULL))
//            display_x(1);
//            flush_display();
//    }
}

/********************************************************************************
 * FUNCTION: core_keytimeout2()                                                 *
 * This function informs the emulator core that 2 seconds have passed since     *
 * core_keytimeout1() was called. (If the key is released less than 2 seconds   *
 * after core_keytimeout1() is called, this function is not called.)            *
 * This marks the moment when the calculator switches from displaying the key's *
 * function name to displaying 'NULL' (informing the user that the key has been *
 * annulled and so no operation will be performed when it is released).         *
 ********************************************************************************/
void core_keytimeout2() {
    // Function not used by EasyCalc
//    if ((pending_command == CMD_LINGER1) || (pending_command == CMD_LINGER2))
//        return;
//    remove_program_catalog = 0;
//    if ((pending_command != CMD_NONE) && (pending_command != CMD_CANCELLED)
//        && ((cmdlist(pending_command)->flags & FLAG_NO_SHOW) == 0)) {
//        clear_row(0);
//        draw_string(0, 0, "NULL", 4);
//        display_x(1);
//        flush_display();
//        pending_command = CMD_CANCELLED;
//    }
}

/********************************************************************************
 * FUNCTION: core_keydown()                                                     *
 * This function informs the emulator core that a key was pressed. Keys         *
 * are identified using a numeric key code, which corresponds to the key        *
 * numbers returned by the 'GETKEY' function: 'Sigma+' is 1, '1/x' is 2,        *
 * and so on. The shift key is 28 -- 'shift' handling is performed by the       *
 * emulator core, so it needs to receive raw key codes, unlike the 'GETKEY'     *
 * function, which handles the shift key itself and then returns key codes in   *
 * the range 38..74 for shifted keys. The core_keydown() function should only   *
 * be called with key codes from 1 to 37, inclusive.                            *
 * Keys that cause immediate action should be handled immediately when this     *
 * function is called, and calls to core_keytimeout1(), core_keytimeout2(),     *
 * and core_keyup() should be ignored (until the next core_keydown(), that      *
 * is!). Keys that are handled only when *released* should not be handled       *
 * immediately when this function is called; the emulator core should store the *
 * key code, handle core_keytimeout1() and core_keytimeout2() as appropriate,   *
 * and not perform any action until core_keyup() is called.                     *
 * RETURNS: a flag indicating whether or not the front end should call this     *
 * function again as soon as possible. This will be 1 if the calculator is      *
 * running a user program, and is only returning execution to the shell because *
 * it has detected that there is a pending event.                               *
 * The 'enqueued' pointer is a return parameter that the emulator core uses to  *
 * tell the shell if it has enqueued the keystroke. If this is 1, the shell     *
 * should not send call timeout1(), timeout2(), or keyup() for this keystroke.  *
 * NOTE: a key code of 0 (zero) signifies 'no key'; this is needed if the shell *
 * is calling this function because it asked to be called (by returning 1 the   *
 * last time) but no keystrokes are available. It is not necessary to balance   *
 * the keydown call with a keyup in this case.                                  *
 * The 'repeat' pointer is a return parameter that the emulator core uses to    *
 * ask the shell to auto-repeat the current key. If this is set to 1 or 2, the  *
 * shell will not call timeout1() and timeout2(), but will repeatedly call      *
 * core_repeat() until the key is released. (1 requests a slow repeat rate, for *
 * SST/BST; 2 requests a fast repeat rate, for number/alpha entry.)             *
 ********************************************************************************/
int core_keydown(int key, int *enqueued, int *repeat) {

    *enqueued = 0;
    *repeat = 0;

#define keydown(a,b) (repeating_key = b)

    /* No program is running, or it is running but waiting for a
     * keystroke (GETKEY); handle any new keystroke that has just come in
     */
    if (key != KEY_NONE) {
        if (mode_getkey && mode_running)
            shell_annunciators(ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, ANNVAL_UNCH, true);
        keydown(mode_shift, key);
        if (repeating != 0) {
            *repeat = repeating;
            repeating = 0;
        }
        if (key == KEY_SHIFT) {
            set_shift(!mode_shift);
            return (mode_running || (keybuf_head != keybuf_tail));
        }
        return (mode_running && !mode_getkey);
    }

    /* Nothing going on at all! */
    return (0);
}


/********************************************************************************
 * FUNCTION: core_keyup()                                                       *
 * This function informs the emulator core that the currently pressed key (that *
 * is, the one whose key code was given to it in the most recent call to        *
 * core_keydown()) has been released.                                           *
 * This function is always called when a key is released, regardless of how     *
 * long it was held down, and regardless of whether or not core_keytimeout1()   *
 * and core_keytimeout2() were called following the most recent                 *
 * core_keydown().                                                              *
 * RETURNS: a flag indicating whether or not the front end should call this     *
 * function again as soon as possible. This will be 1 if the calculator is      *
 * running a user program, and is only returning execution to the shell because *
 * it has detected that there is a pending event.                               *
 ********************************************************************************/
int core_keyup(Skin *skin, void *hWnd_p) {
    if (mode_shift && (repeating_key != KEY_SHIFT)
        && (repeating_key != KEY_BCKSPC)) { // Release shift when another key is pressed,
                                            // except if back erase
        set_shift(false);
    }
    switch (repeating_key) {
        case KEY_EXE:
            input_exec(skin, hWnd_p);
            break;
        case KEY_CLR:
            skin->set_input_text(NULL, _T(""));
            result_set_text(skin, hWnd_p, _T(""), notype);
            break;
        case KEY_BCKSPC:
            skin->back_delete(NULL);
            break;
        case KEY_DOT:
            skin->insert_input_text(NULL, &flPointChar);
            break;
        case KEY_CTRZONE:
        case KEY_CTRWIDE:
        case KEY_GPREFS:
        case KEY_GCONF:
        case KEY_GCALC:
        case KEY_GSETX:
        case KEY_ZONEIN:
        case KEY_ZONEOUT:
        case KEY_WIDEIN:
        case KEY_WIDEOUT:
        case KEY_NORMZON:
            shell_graphAction(repeating_key);
            break;
        default:
            main_btnrow_click(skin, repeating_key);
    }
    return ((mode_running && !mode_getkey) || (keybuf_head != keybuf_tail));
}
