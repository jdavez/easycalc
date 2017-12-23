/*
 * $Id: Main.cpp,v 1.6 2009/12/24 16:35:58 mapibid Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000,2001 Ondrej Palkovsky
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA  02110-1301, USA.
 *
 *  You can contact me at 'ondrap@penguin.cz'.
 *
 *  2001-09-23 - John Hodapp <bigshot@email.msn.com>
 *               Added code to force insertion point for fwd/backspace
 *               and delete keys to work on single character even if
 *               entire selection is highlighted.
 *  2001-09-31 - John Hodapp - added code to display and select trig mode
 *               on Basic and Scientific screen.  Repeating fwd/backspace.
 *  2001-12-8  - John Hodapp - added code to dispaly and select Radix on
 *               Basic and Scien screens.
 *  2003-05-19 - Arno Welzel - added code for Sony Clie support
*/

#include "StdAfx.h"
#include "compat/PalmOS.h"
#include "konvert.h"
//#include "calc.h"
//#include "calcrsc.h"
#include "prefs.h"
#include "fp.h"
#include "varmgr.h"
#include "stack.h"
#include "funcs.h"
#include "ansops.h"
#include "history.h"
//#include "memo.h"
#define _MAIN_C_
#include "system - UI/Skin.h"
#include "Main.h"
#include "EasyCalc.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

Main::Main(void)
{
}

Main::~Main(void)
{
}

const static struct {
        TCHAR *string;
        Boolean operatr;       // Prepend Ans when in the beginning of line
        Boolean func;          // Add ')' and backspace one position
        Boolean nostartparen;
        Boolean wide;
        const TCHAR *helptext;
}buttonString[BUTTON_COUNT+1]={
        {_T("0"),false},
        {_T("1"),false},
        {_T("2"),false},
        {_T("3"),false},
        {_T("4"),false},
        {_T("5"),false},
        {_T("6"),false},
        {_T("7"),false},
        {_T("8"),false},
        {_T("9"),false},
        {_T("A"),false},
        {_T("B"),false},
        {_T("C"),false},
        {_T("D"),false},
        {_T("E"),false},
        {_T("F"),false},
        {_T("&"),true},
        {_T("|"),true},
        {_T("ˆ"),true},                 // xor
        {_T("<<"),true},
        {_T(">>"),true},
        {_T("+"),true},
        {_T("-"),true},
        {_T("-"),false},
        {_T("*"),true},
        {_T("/"),true},
        {_T("ans")},
        {_T(""),false,true},            // Open + close bracket
        {_T("^"),true,true},            // power
        {_T("log"),false,true},
        {_T("ln"),false,true},
        {_T("^(-1)"),true},             // 1/x
        {_T("^(1/"),true,true,true},    // nth root
        {_T("^2"),true},
        {_T("sqrt"),false,true},
        {_T("sin"),false,true},
        {_T("cos"),false,true},
        {_T("tan"),false,true},
        {_T("exp"),false,true},
        {_T("pi")},
        {_T("°")},                      // degrees
        {_T("'")},                      // minutes
        {_T("abs"),false,true},         // Magnitude
        {_T("angle"),false,true},
        {_T("i")},                      // Complex_i
        {_T("e")},                      // e constant
        {_T(":")},                      // column
        {_T(")")},
        {_T("E")},
        {_T("asin"),false,true},
        {_T("acos"),false,true},
        {_T("atan"),false,true},
        {_T("sinh"),false,true},
        {_T("cosh"),false,true},
        {_T("tanh"),false,true},
        {_T("asinh"),false,true},
        {_T("acosh"),false,true},
        {_T("atanh"),false,true},
        {_T("10^"),false,true},
        {_T("log2"),false,true},
        {_T("2^"),false,true},
        {_T("fact"),false,true},
        {_T("ncr"),false,true,false,false,_T("n:r")},
        {_T("npr"),false,true,false,false,_T("n:r")},
        {_T("round"),false,true,false,true},
        {_T("trunc"),false,true,false,true},
        {_T("ceil"),false,true,false,true},
        {_T("floor"),false,true,false,true},
        {_T("gamma"),false,true,false,true},
        {_T("beta"),false,true,false,true,_T("z:w")},
        {_T("rand"),false,false},
        {_T("fsimps"),false,true,false,true,_T("min:max:func[:error[:params]]")},
        {_T("real"),false,true,false,true},
        {_T("imag"),false,true,false,true},
        {_T("conj"),false,true},
        {_T("exp(i*"),false,true,true,false},
        {_T("fzero"),false,true,false,true,_T("min:max:f[:err[:params]]")},
        {_T("fvalue"),false,true,false,true,_T("min:max:val:f[:err[:params]]")},
        {_T("fmin"),false,true,false,true,_T("min:max:f[:err]")},
        {_T("fmax"),false,true,false,true,_T("min:max:f[:err]")},
        {_T("fd_dx"),false,true,false,true,_T("x:f[:err[:params]]")},            // d/dx
        {_T("fromberg"),false,true,false,true,_T("min:max:func[:degree[:params]]")},     // integ
        {_T("fd2_dx"),false,true,false,true,_T("x:f[:err[:params]]")},           // d2/dx
        {_T("()=\""),false,true,true,true},
        {_T("\"")},
        {_T("x")},
        {_T("list"),false,true,false,true},
        {_T("median"),false,true,false,true},
        {_T("mean"),false,true,false,true},
        {_T("sum"),false,true,false,true},
        {_T("lmin"),false,true},                                        // min
        {_T("lmax"),false,true},                                        // max
        {_T("prod"),false,true},
        {_T("variance"),false,true,false,true},
        {_T("stddev"),false,true,false,true},
        {_T("dim"),false,true},
        {_T("["),false,true,true},   // Special handling to create corresponding ]
        {_T("matrix"),false,true,false,true},
        {_T("identity"),false,true,false,true},
        {_T("det"),false,true},
        {_T("qrs"),false,true},
        {_T("rref"),false,true},
        {_T("qrq"),false,true},
        {_T("qrr"),false,true},
        {_T("fintersect"),false,true,false,true,_T("min:max:f1:f2:[:err[:params]]")},
        {_T("]")},
        {_T("qBinomial"),false,true,false,true,_T("c:n:p")},
        {_T("qBeta"),false,true,false,true,_T("x:a:b")},
        {_T("qChiSq"),false,true,false,true,_T("ChiSq:df")},
        {_T("qF"),false,true,false,true,_T("F:df1:df2")},
        {_T("qPoisson"),false,true,false,true,_T("c:lam")},
        {_T("qStudentt"),false,true,false,true,_T("t:df")},
        {_T("qWeibull"),false,true,false,true,_T("t:a:b")},
        {_T("qNormal"),false,true,false,true,_T("z")},
        {_T("range"),false,true,false,true,_T("n:start[:step]")},
        {_T("rNorm"),false,true,false,true},
        {_T("find"),false,true,false,true,_T("expr:data")},
        {_T("sample"),false,true,false,true,_T("data:indices")},
        {_T("filter"),false,true,false,true,_T("b_coefs:a_coefs:data")},
        {_T("conv"),false,true,false,true,_T("x:y")},
        {_T("fft"),false,true,false,true,_T("data[:N]")},
        {_T("ifft"),false,true,false,true,_T("data[:N]")},
        {_T("prevprime"),false,true},
        {_T("isprime"),false,true},
        {_T("nextprime"),false,true},
        {_T("gcd"),false,true},
        {_T("lcm"),false,true},
        {_T("phi"),false,true},
        {_T("gcdex"),false,true,false,true,_T("a:b")},
        {_T("chinese"),false,true,false,true,_T("a1:n1:a2:n2[:a3:n3:...]")},
        {_T("modinv"),false,true,false,true,_T("a:b")},
        {_T("modpow"),false,true,false,true,_T("a:b:c")},
        {_T("factor"),false,true,false,true},
        {_T("besseli"),false,true,false,true,_T("n:x")},
        {_T("besselj"),false,true,false,true,_T("n:x")},
        {_T("besselk"),false,true,false,true,_T("n:x")},
        {_T("bessely"),false,true,false,true,_T("n:x")},
        {_T("ellc1"),false,true,false,true},
        {_T("ellc2"),false,true,false,true},
        {_T("elli1"),false,true,false,true,_T("m:phi")},
        {_T("elli2"),false,true,false,true,_T("m:phi")},
        {_T("euler"),false,false,false,true},
        {_T("cn"),false,true,false,false,_T("m:u")},
        {_T("dn"),false,true,false,false,_T("m:u")},
        {_T("sn"),false,true,false,false,_T("m:u")},
        {_T("igamma"),false,true,false,true,_T("a:x")},
        {_T("ibeta"),false,true,false,true,_T("a:b:x")},
        {_T("erf"),false,true,false,true},
        {_T("erfc"),false,true,false,true},
        {_T("="),true},
        {_T("Inf")},        // Infinity constant
        {_T("%"),true},     // Modulo operator
        {_T("j")},          // Complex_j
        {_T("299792458")},    //   Speed of light in vacuum (m s-1) c=299792458
        {_T("6.67428E-11")},    //   Newtonian constant of gravitation (m3 kg-1 s-2) G=6.67428E-11
        {_T("9.80665")},    //   Standard gravitational acc. (N kg-1) g=9.80665
        {_T("9.10938215E-31")},    //   Electron mass (kg) me=9.10938215E-31
        {_T("1.672621637E-27")},    //   Proton mass (kg) mp=1.672621637E-27
        {_T("1.67492729E-27")},    //   Neutron mass (kg) mn=1.67492729E-27
        {_T("1.660538782E-27")},    //   Unified Atomic mass unit (kg) u=1.660538782E-27
        {_T("6.62606896-34")},    //   Planck constant (J s) h=6.62606896-34
        {_T("1.3806504-23")},    //   Boltzmann constant (J K-1) k=1.380 6504-23
        {_T("1.2566370614E-6")},    //   Magnetic permeability - vacuum (magnetic constant) (H m-1) µ0=1.2566370614E-6
        {_T("8.854187817E-12")},    //   Dielectric permittivity (electric constant) (F m-1) e0=8.854187817E-12
        {_T("7.2973525376E-3")},    //   Fine structure constant alpha=7.2973525376E-3
        {_T("10973731.568527")},    //   Rydberg constant (m-1) r=10973731.568527
        {_T("2.8179402894E-15")},    //   Classical electron radius (m) re=2.8179402894E-15
        {_T("5.2917720859E-11")},    //   Bohr radius (m) a0=5.2917720859E-11
        {_T("2.067833667E-15")},    //   Fluxoid quantum (magnetic flux quantum) (W b) phi0=2.067833667E-15
        {_T("9.27400915E-24")},    //   Bohr magneton (J T-1) µb=9.27400915E-24
        {_T("-9.28476377E-24")},    //   Electron magnetic moment (J T-1) µe=-9.28476377E-24
        {_T("5.05078324E-27")},    //   Nuclear magneton (J T-1) µN=5.05078324E-27
        {_T("1.410606662E-26")},    //   Proton magnetic moment (J T-1) µp=1.410606662E-26
        {_T("-9.6623641E-27")},    //   Neutron magnetic moment (J T-1) µn=-9.6623641E-27
        {_T("2.4263102175E-12")},    //   Compton wavelength (electron) (m) lc=2.4263102175E-12
        {_T("1.3214098446E-15")},    //   Compton wavelength (proton) (m) lcp=1.3214098446E-15
        {_T("5.670400E-8")},    //   Stefan-Boltzmann constant (W m-2 K-4) sigma=5.670400E-8
        {_T("6.02214179E+23")},    //   Avogadro's constant (mol-1) Na=6.02214179E+23
        {_T("2.2413996E-2")},    //   Ideal gas volume at STP (m3 mol-1) Vm=2.2413996E-2
        {_T("8.314472")},    //   Universal molar gas constant (J mol-1 K-1) R=8.314472
        {_T("1.602176487E-19")},    //   Electron charge (elementary charge) (C) e=1.602176487E-19
        {_T("96485.3399")},    //   Faraday constant (C mol-1) F=96485.3399
        {_T("25812.807")},    //   Quantum Hall resistance = von Klitzing constant (Ohm) Rk=25812.807
        {_T(">=")},
        {_T("<=")},
        {_T("==")},
        {_T(";")},
        {0,NULL,false}
};
// Currently 187 entries, not counting the last one (NULL).
// Don't forget to update BUTTON_COUNT in Main.h if this evolves !


/***********************************************************************
 *
 * FUNCTION:     main_replace_enters
 *
 * DESCRIPTION:  Replace \n by ;
 *
 ***********************************************************************/
static void main_replace_enters (TCHAR *text) {
    for (;*text;text++)
        if (*text=='\n')   *text=';';
}

/***********************************************************************
 *
 * FUNCTION:     main_input_exec
 *
 * DESCRIPTION:  Execute contents of input line and show the result
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       err - Possible error that occurred during computation
 *
 ***********************************************************************/
CError main_input_exec (TCHAR *inp, Trpn *result) {
    CError err;
    int inpsize;
    CodeStack *stack;

    if (*inp == _T('\0'))
        return c_syntax;
    inpsize = (int) _tcslen(inp);

    main_replace_enters(inp);
    stack = text_to_stack(inp, &err);
    if (!err) {
        err = stack_compute(stack);
        /* Add line to history After computing and After succesful
         * compilation */
        if (inpsize)
            history_add_line(inp);

            if (!err) {
                (*result) = stack_pop(stack);
            }
        stack_delete(stack);
    }

    return err;
}

/***********************************************************************
 *
 * FUNCTION:     main_insert
 *
 * DESCRIPTION:  Insert a text on an input line, handle cases like
 *               embracing a selected text with a function, auto-closing
 *               of brackets etc.
 *
 * PARAMETERS:   fieldid - id of field where to insert text
 *               text - text to add to input line
 *               operatr - prepend 'ans' if on the beginning of line
 *               func - add opening and possibly closing bracket
 *               nostartparen - is a function without opening bracket
 *               helptext - description of parameters of a function
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
void
main_insert(Skin *skin, void *hwnd_edit, const TCHAR *text, Boolean operatr, Boolean func,
            Boolean nostartparen, const TCHAR *helptext)
{
    unsigned long startp, endp;

    skin->get_select_text(hwnd_edit, &startp, &endp);

    if (operatr && !func) {
        /* Insert 'ans' if on the beginning */
        if ((skin->get_insert_pos(hwnd_edit) == 0) ||
            ((startp != endp) && (startp == 0)))
            skin->insert_input_text(hwnd_edit, _T("ans"));
        skin->insert_input_text(hwnd_edit, text);
    } else if (func) {
        /* if selected, insert in front of selection and
         * put brackets around selection */
        if (startp != endp) {
            skin->select_input_text(hwnd_edit, startp, startp);
        }
        if ((skin->get_insert_pos(hwnd_edit) == 0) && operatr) {
            skin->insert_input_text(hwnd_edit, _T("ans"));
            if (startp != endp) {
                startp += 3;
                endp += 3;
            }
        }
        int len = _tcslen(text);
        if (len)
            skin->insert_input_text(hwnd_edit, text);
        if (!nostartparen) {
            skin->insert_input_text(hwnd_edit, _T("("));
            startp += 1;
            endp += 1;
        }
        if (startp != endp) {
            startp += len;
            endp += len;
            skin->set_insert_pos(hwnd_edit, endp);
            if (text[len-1] == '"')
                skin->insert_input_text(hwnd_edit, _T("\""));
            else if (text[len-1] == '[')
                skin->insert_input_text(hwnd_edit, _T("]"));
            else
                skin->insert_input_text(hwnd_edit, _T(")"));
            skin->select_input_text(hwnd_edit, startp, endp);
        } else if (calcPrefs.matchParenth) { /* nothing selected */
            if (text[len-1] == '"')
                skin->insert_input_text(hwnd_edit, _T("\""));
            else if (text[len-1] == '[')
                skin->insert_input_text(hwnd_edit, _T("]"));
            else
                skin->insert_input_text(hwnd_edit, _T(")"));
            skin->set_insert_pos(hwnd_edit, skin->get_insert_pos(hwnd_edit)-1);
        }
        if ((startp == endp) && helptext && calcPrefs.insertHelp) {
            startp = skin->get_insert_pos(hwnd_edit);
            skin->insert_input_text(hwnd_edit, helptext);
            skin->select_input_text(hwnd_edit, startp, startp+_tcslen(helptext));
        }
    } else {
        skin->insert_input_text(hwnd_edit, text);
        if ((StrLen(text) == 1) && (StrChr(_T("()[]"), text[0]) != NULL)) {
            shell_clBracket();
        }
    }
}

/***********************************************************************
 *
 * FUNCTION:     main_btnrow_click
 *
 * DESCRIPTION:  Handle the tap on the button on the screen,
 *               insert the proper text on the input line
 *
 * PARAMETERS:   btnid - id of pressed button
 *
 * RETURN:       true - button is in the group
 *               false - button doesn't exist
 *
 ***********************************************************************/
Boolean main_btnrow_click(Skin *skin, int btnid) IFACE;
Boolean main_btnrow_click(Skin *skin, int btnid) {
    if ((btnid < 0) || (btnid >= BUTTON_COUNT))
        return false;

    int index = btnid;
    main_insert(skin, NULL,
                buttonString[index].string,
                buttonString[index].operatr,
                buttonString[index].func,
                buttonString[index].nostartparen,
                buttonString[index].helptext
               );
    return true;
}
