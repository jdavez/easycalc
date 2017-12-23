/*   
 * $Id: main.c,v 1.73 2007/12/24 10:45:10 cluny Exp $
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
 *		 Added code to force insertion point for fwd/backspace
 *		 and delete keys to work on single character even if 
 *		 entire selection is highlighted.
 *  2001-09-31 - John Hodapp - added code to display and select trig mode
 *		 on Basic and Scientific screen.  Repeating fwd/backspace.
 *  2001-12-8  - John Hodapp - added code to dispaly and select Radix on
 *               Basic and Scien screens.
 *  2003-05-19 - Arno Welzel - added code for Sony Clie support
*/

#include <PalmOS.h>
#include <TxtGlue.h>

#include "konvert.h"
#include "calc.h"
#include "clie.h"
#include "calcrsc.h"
#include "prefs.h"
#include "result.h"
#include "fp.h"
#include "varmgr.h"
#include "stack.h"
#include "funcs.h"
#include "ansops.h"
#include "history.h"
#include "memo.h"
#define _MAIN_C_
#include "main.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

const struct {
	const char *dtext;
	UInt16 bitmap;
	const char *itext;
	Boolean operator;
	Boolean func;
	Boolean wide;
	Boolean nostartparen;
	const char *helptext;
}buttonRow[][BUTTON_COUNT] = 
{
	{
		{"sin",0,"sin",false,true},
		{"cos",0,"cos",false,true},
		{"tan",0,"tan",false,true},
		{"asin",0,"asin",false,true},
		{"acos",0,"acos",false,true},
		{"atan",0,"atan",false,true},
		{"sinh",0,"sinh",false,true},
		{"cosh",0,"cosh",false,true},
		{"tanh",0,"tanh",false,true},
		{"asinh",0,"asinh",false,true},
		{"acosh",0,"acosh",false,true},
		{"atanh",0,"atanh",false,true},
		{"pi",0,"pi"},
		{"'",0,"'"},
		{"°",0,"°"}
	},{
		{"ln",0,"ln",false,true},
		{"",bmpExpon,"exp",false,true},
		{"",bmpPower,"^",true},
		{"log10",0,"log",false,true},
		{"10^",0,"10^"},
		{"",bmpRootX,"^(1/",true,true,false,true},
		{"log2",0,"log2",false,true},
		{"2^",0,"2^"},
		{"",bmpSquare,"^2",true},
		{"",bmpInverse,"^(-1)",true},
		{"abs",0,"abs",false,true},
		{"",bmpSquareRoot,"sqrt",false,true}
	},{
		{"fact",0,"fact",false,true},
		{"ncr",0,"ncr",false,true},
		{"npr",0,"npr",false,true},
		{"round",0,"round",false,true,true},
		{"trunc",0,"trunc",false,true,true},
		{NULL},
		{"floor",0,"floor",false,true,true},
		{"abs",0,"abs",false,true,true},
		{NULL},
		{"gamma",0,"gamma",false,true,true},
		{"beta",0,"beta",false,true,true},
		{NULL},
		{"rand",0,"rand",false,false},
		{"rNrm",0,"rNorm",false,false},
		{":",0,":"},
	},{
		{"i",0,"i"},
		{NULL},
		{NULL},
		{"real",0,"real",false,true,true},
		{"imag",0,"imag",false,true,true},
		{NULL},
		{"conj",0,"conj",false,true},
		{"e^ix",0,"exp(i*",false,true,false,true},
		{"abs",0,"abs",false,true},
		{"angle",0,"angle",false,true,true},
#ifdef SPECFUN_ENABLED
	},{
		{"fzero",0,"fzero",false,true,true,false,"min:max:f[:err]"},
		{"fvalue",0,"fvalue",false,true,true,false,"min:max:val:f[:err]"},
		{NULL},
		{"fmin",0,"fmin",false,true,true,false,"min:max:f[:err]"},
		{"fmax",0,"fmax",false,true,true,false,"min:max:f[:err]"},
		{NULL},
		{"d/dx",0,"fd_dx",false,true,true,false,"x:f[:err]"},
		{"integ",0,"fromberg",false,true,true,false,"min:max:f[:n]"},
		{NULL},
		{"d2/dx",0,"fd2_dx",false,true,true,false,"x:f[:err]"},
		{"()=\"",0,"()=\"",false,true,true,true},
		{NULL},
		{"\"",0,"\""},
		{"x",0,"x"},
		{":",0,":"}
	},{
		{"list",0,"list",false,true,true},
		{"median",0,"median",false,true,true},
		{NULL},
		{"mean",0,"mean",false,true,true},
		{"sum",0,"sum",false,true,true},
		{NULL},
		{"min",0,"lmin",false,true},
		{"max",0,"lmax",false,true},
		{"prod",0,"prod",false,true},
		{"variance",0,"variance",false,true,true},
		{"stddev",0,"stddev",false,true,true},
		{NULL},
		{"dim",0,"dim",false,true},
		{"[",0,"["},
		{":",0,":"}
	},{
		{"matrix",0,"matrix",false,true,true},
		{"identity",0,"identity",false,true,true},
		{NULL},
		{"",bmpInverse,"^(-1)",true},
		{"'",0,"'",true},
		{NULL},
		{"det",0,"det",false,true},
		{"dim",0,"dim",false,true},
		{"qrs",0,"qrs",false,true},
		{"rref",0,"rref",false,true},
		{"qrq",0,"qrq",false,true},
		{"qrr",0,"qrr",false,true},
		{"[",0,"[",true},
		{"]",0,"]"},
		{":",0,":"}
	},{
		{"qBinom",0,"qBinomial",false,true,true,false,"c:n:p"},
		{"qBeta",0,"qBeta",false,true,true,false,"x:a:b"},
		{NULL},
		{"qChiSq",0,"qChiSq",false,true,true,false,"ChiSq:df"},
		{"qF",0,"qF",false,true,true,false,"F:df1:df2"},
		{NULL},
		{"qPoiss",0,"qPoisson",false,true,true,false,"c:lam"},
		{"qStudt",0,"qStudentt",false,true,true,false,"t:df"},
		{NULL},
		{"qWeib",0,"qWeibull",false,true,true,false,"t:a:b"},
		{"qNorm",0,"qNormal",false,true,true,false,"z"},
		{NULL},
		{NULL},
		{NULL},
		{":",0,":"}
	},{
		{"range",0,"range",false,true,true,false,"n:start[:step]"},
		{"rNorm",0,"rNorm",false,true,true,false},
		{NULL},
		{"find",0,"find",false,true,true,false,"expr:data"},
		{"sample",0,"sample",false,true,true,false,"data:indices"},
		{NULL},
		{"filter",0,"filter",false,true,true,false,"b_coefs:a_coefs:data"},
		{"conv",0,"conv",false,true,true,false},
		{NULL},
		{"fft",0,"fft",false,true,true,false,"data[:N]"},
		{"ifft",0,"ifft",false,true,true,false,"data[:N]"},
		{NULL},
		{"[",0,"["},
		{"]",0,"]"},
		{":",0,":"}
	},{
		{"",bmpPrevPrime,"prevprime",false,true},
		{"",bmpIsPrime,"isprime",false,true},
		{"",bmpNextPrime,"nextprime",false,true},
		{"gcd",0,"gcd",false,true},
		{"lcm",0,"lcm",false,true},
		{"phi",0,"phi",false,true},
		{"gcdex",0,"gcdex",false,true,true},
		{"chinese",0,"chinese",false,true,true},
		{NULL},
		{"modinv",0,"modinv",false,true,true},
		{"modpow",0,"modpow",false,true,true},
		{NULL},
		{"factor",0,"factor",false,true,true},
		{NULL},
		{":",0,":"}
	},{
		{"besseli",0,"besseli",false,true,true,false,"n:x"},
		{"besselj",0,"besselj",false,true,true,false,"n:x"},
		{NULL},
		{"besselk",0,"besselk",false,true,true,false,"n:x"},
		{"bessely",0,"bessely",false,true,true,false,"n:x"},
		{NULL},
		{"ellc1",0,"ellc1",false,true,true},
		{"ellc2",0,"ellc2",false,true,true},
		{NULL},
		{"elli1",0,"elli1",false,true,true,false,"m:phi"},
		{"elli2",0,"elli2",false,true,true,false,"m:phi"},
		{NULL},
		{"euler",0,"euler",false,false,true},
		{NULL},
		{":",0,":"}
	},{
		{"cn",0,"cn",false,true},
		{"dn",0,"dn",false,true},
		{"sn",0,"sn",false,true},
		{"gamma",0,"gamma",false,true,true},
		{"beta",0,"beta",false,true,true},
		{NULL},
		{"igamma",0,"igamma",false,true,true,false,"a:x"},
		{"ibeta",0,"ibeta",false,true,true,false,"a:b:x"},
		{NULL},
		{"erf",0,"erf",false,true,true},
		{"erfc",0,"erfc",false,true,true},
		{NULL},
		{NULL},
		{NULL},
		{":",0,":"}
#endif
	}
};


const static struct {
	UInt16 id;
	char *string;
	Boolean operator; /* Prepend Ans when in the beginning of line */
	Boolean func;  /* Add ')' and backspace one position */
	Boolean nostartparen;
}buttonString[]={	
	{btnMain0,"0",false},
	{btnMain1,"1",false},
	{btnMain2,"2",false},
	{btnMain3,"3",false},
	{btnMain4,"4",false},
	{btnMain5,"5",false},
	{btnMain6,"6",false},
	{btnMain7,"7",false},
	{btnMain8,"8",false},
	{btnMain9,"9",false},
	{btnMainA,"A",false},
	{btnMainB,"B",false},
	{btnMainC,"C",false},
	{btnMainD,"D",false},
	{btnMainE,"E",false},
	{btnMainF,"F",false},
	{btnMainAnd,"&",true},
	{btnMainOr,"|",true},
	{btnMainXor,"ˆ",true},
	{btnMainShl,"<<",true},
	{btnMainShr,">>",true},
	{btnMainPlus,"+",true},
	{btnMainMinus,"-",true},
	{btnMainNeg,"-",false},
	{btnMainMult,"*",true},
	{btnMainDivide,"/",true},
	{btnMainAns,"ans"},
	{btnMainOpBr,"",false,true},
	{btnMainPow,"^",true}, 
	{btnMainLog,"log",false,true},
	{btnMainLn,"ln",false,true},
	{btnMainInv,"^(-1)",true},
	{btnMainSq1,"^(1/",true,true,true},
	{btnMainSqr,"^2",true},
	{btnMainSqrt,"sqrt",false,true},
	{btnMainSin,"sin",false,true},
	{btnMainCos,"cos",false,true},
	{btnMainTan,"tan",false,true},
	{btnMainExp,"exp",false,true},
	{btnMainFact,"fact",false,true},
	{btnMainPi,"pi"},
	{btnMainDeg,"°"},
	{btnMainMin,"'"},
	{btnMainMagn,"abs",false,true},
	{btnMainAngl,"angle",false,true},
	{btnMain_i,"i"},
	{btnMainColn,":"},
	{btnMainClBr,")"},
	{btnMainEE,"E"},
	{0,NULL,false}
};

const struct {
	const char *trigtext;
}trigmode[] ={
	{"Deg"},
	{"Rad"},
	{"Grd"}
};

static void
main_replace_enters(char *text) IFACE;
static void
main_replace_enters(char *text)
{
	for (;*text;text++)
	  if (*text=='\n')
	    *text=';';
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
Int16
main_input_exec(void)
{
	char *inp;
	CError err;
	FieldPtr pole;
	int inpsize;
	Trpn result;
	CodeStack *stack;

	inp = FldGetTextPtr(GetObjectPtr(tdInput));
	if (!inp)
		return 1;
	inpsize=StrLen(inp);

	main_replace_enters(inp); 
	stack = text_to_stack(inp,&err);
	if (!err) {
		err = stack_compute(stack);
		/* Add line to history After computing and After succesful 
		* compilation */
		if (inpsize)
			history_add_line(inp);

		if (!err) {
			result = stack_pop(stack);
			result_set(result);
			err=set_ans_var(result);
			rpn_delete(result);
		}
		stack_delete(stack);
	}
	if (err) 
	  result_error(err);

	result_draw();

	pole=GetObjectPtr(tdInput);
	if (!err) {
		FldSetSelection(pole,0,inpsize);
		FrmSetFocus(FrmGetActiveForm(),FrmGetObjectIndex(FrmGetActiveForm(),tdInput));
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
 *               operator - prepend 'ans' if on the beginning of line
 *               func - add opening and possibly closing bracket
 *               nostartparen - is a function without opening bracket
 *               helptext - description of parameters of a function
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
void
main_insert(UInt16 fieldid,const char *text,Boolean operator,Boolean func,
	    Boolean nostartparen,const char *helptext)
{
	FieldPtr field;
	UInt16 startp,endp;
	Boolean insOK = true;

	field = GetObjectPtr(fieldid);
	
	FldGetSelection(field,&startp,&endp);
	
	if (operator && !func) {
		/* Insert 'ans' if on the beginning */
		if (FldGetInsPtPosition(field)==0 ||
			(startp!=endp && startp==0))
			FldInsert(field,"ans",3);
		FldInsert(field,text,StrLen(text));
	}
	else if (func) {
		/* if selected, insert in front of selection and
		 * put brackets around selection */
		if (startp!=endp) {
			FldSetSelection(field,0,0);
			FldSetInsPtPosition(field,startp);
		}
		if (FldGetInsPtPosition(field)==0 && operator) {
			insOK = FldInsert(field,"ans",3);
			if (startp!=endp && insOK)
			  endp+=3;
		}
		if (insOK && StrLen(text))
			insOK = FldInsert(field,text,StrLen(text));
		if (!nostartparen && insOK) {
			insOK = FldInsert(field,"(",1);
			if (insOK) {
				startp+=1;
				endp+=1;
			}
		}
		if (startp!=endp && insOK) {
			startp+=StrLen(text);
			endp+=StrLen(text);
			FldSetInsPtPosition(field,endp);
			if (text[StrLen(text)-1] == '"')
				insOK = FldInsert(field,"\"",1);
			else
				insOK = FldInsert(field,")",1);
			if (insOK)
				FldSetSelection(field,startp,endp);
		} else if (calcPrefs.matchParenth && insOK) { /* nothing selected */
			if (text[StrLen(text)-1] == '"')
				insOK = FldInsert(field,"\"",1);
			else
				insOK = FldInsert(field,")",1);
			if (insOK)
				FldSetInsPtPosition(field,FldGetInsPtPosition(field)-1);
		}
		if (startp == endp && helptext && calcPrefs.insertHelp && insOK) {
			startp = FldGetInsPtPosition(field);
			insOK = FldInsert(field,helptext,StrLen(helptext));
			if (insOK)
				FldSetSelection(field,startp,startp+StrLen(helptext));
		}

	}
	else 
		FldInsert(field,text,StrLen(text));
}

/***********************************************************************
 *
 * FUNCTION:     main_btnrow_click
 * 
 * DESCRIPTION:  Handle the tap on the button on the Scientific screen,
 *               insert the proper text on the input line
 *
 * PARAMETERS:   btnid - id of pressed button
 *
 * RETURN:       true - button is in the group
 *               false - button doesn't belong to the button row
 *      
 ***********************************************************************/
static Boolean
main_btnrow_click(UInt16 btnid) IFACE;
static Boolean
main_btnrow_click(UInt16 btnid)
{
	Int16 index;

	if (btnid >= btnMainW1)
		index = btnid - btnMainW1;
	else
		index = btnid - btnMainS1;
	
	if (index < 0 || index > BUTTON_COUNT)
		return false;

	main_insert(tdInput,
		    buttonRow[calcPrefs.btnRow][index].itext,
		    buttonRow[calcPrefs.btnRow][index].operator,
		    buttonRow[calcPrefs.btnRow][index].func,
		    buttonRow[calcPrefs.btnRow][index].nostartparen,
		    buttonRow[calcPrefs.btnRow][index].helptext);

	return true;
}

/***********************************************************************
 *
 * FUNCTION:     main_draw_graph_btn
 *
 * DESCRIPTION:  Draw the bitmap for a graphic button with the correct
 *               background color.
 *
 * PARAMETERS:   frm - Pointer to active form
 *               btnid - Object id of the button in the form
 *               bmpid - Resource id of the bitmap to be drawn
 *               dx - offset between button boundary box and bitmap in x
 *               dy - offset between button boundary box and bitmap in y
 *
 * RETURN:       Nothing
 *
 ***********************************************************************/
static void
main_draw_graph_btn(FormType *frm, UInt16 btnid, UInt16 bmpid,
                    UInt16 dx, UInt16 dy) IFACE;
static void
main_draw_graph_btn(FormType *frm, UInt16 btnid, UInt16 bmpid,
                    UInt16 dx, UInt16 dy)
{
	RectangleType bounds;
	MemHandle bmpres;
	BitmapType *bmp;

	if (colorDisplay) {
		WinPushDrawState();
		WinSetBackColor(UIColorGetTableEntryIndex(UIObjectFill));
	}

	FrmGetObjectBounds(frm, FrmGetObjectIndex(frm, btnid), &bounds);
	WinEraseRectangle(&bounds, 4);
	bmpres = DmGetResource(bitmapRsc, bmpid);
	bmp = MemHandleLock(bmpres);
	clie_drawbitmap(bmp, (bounds.topLeft.x + dx) * gSonyFactor,
	                (bounds.topLeft.y + dy) * gSonyFactor);
	MemHandleUnlock(bmpres);
	DmReleaseResource(bmpres);

	if (colorDisplay)
		WinPopDrawState();
}

/***********************************************************************
 *
 * FUNCTION:     main_btnrow_set
 * 
 * DESCRIPTION:  Show proper buttons with proper names on the 
 *               Scientific screen
 *
 * PARAMETERS:   row - which group a user selected
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void
main_btnrow_set(UInt16 row) IFACE;
static void
main_btnrow_set(UInt16 row)
{
	Int16 i;
	FormPtr frm = FrmGetActiveForm();
	UInt16 objid;

	/* Hide not displayed buttons */
	for (i=0;i < BUTTON_COUNT;i++) 
		if (buttonRow[row][i].itext) {
			if (buttonRow[row][i].wide) 
				FrmHideObject(frm,FrmGetObjectIndex(frm,btnMainS1 + i));
			else if ((i % 3) != 2)
				FrmHideObject(frm,FrmGetObjectIndex(frm,btnMainW1 + i));
		} else {
			FrmHideObject(frm,FrmGetObjectIndex(frm,btnMainS1 + i));
			if ((i % 3) != 2)
				FrmHideObject(frm,FrmGetObjectIndex(frm,btnMainW1 + i));
		}

	for (i=0;i < BUTTON_COUNT;i++)
		if (buttonRow[row][i].itext) {
			if (buttonRow[row][i].wide) 
				objid = btnMainW1 + i;
			else
				objid = btnMainS1 + i;

			CtlSetLabel(GetObjectPtr(objid), buttonRow[row][i].dtext);
			FrmShowObject(frm,FrmGetObjectIndex(frm,objid));
			if (buttonRow[row][i].bitmap) {
				main_draw_graph_btn(frm, objid, buttonRow[row][i].bitmap, 1, 1);
			}
		} 
}


/***********************************************************************
 *
 * FUNCTION:     main_fix_graph_btns
 *
 * DESCRIPTION:  Draw the fixed graphics buttons with the correct background.
 *
 * PARAMETERS:   formid
 *
 * RETURN:       Nothing
 *     
 ***********************************************************************/
static void
main_fix_graph_btns(UInt16 formid) IFACE;
static void
main_fix_graph_btns(UInt16 formid)
{
	FormType *frm = FrmGetActiveForm();

	switch (formid) {
	case frmScientific:
		main_draw_graph_btn(frm, btnMainDel, bmpLdel, 4, 2);
		break;
	case frmBasicS:
		main_draw_graph_btn(frm, btnMainExp, bmpExpon, 1, 1);
		main_draw_graph_btn(frm, btnMainInv, bmpInverse, 1, 1);
		main_draw_graph_btn(frm, btnMainDel, bmpLdel, 4, 2);
		/* fall through */
	case frmBasic:
		main_draw_graph_btn(frm, btnMainSqr, bmpSquare, 1, 1);
		main_draw_graph_btn(frm, btnMainSqrt, bmpSquareRoot, 1, 1);
		main_draw_graph_btn(frm, btnMainPow, bmpPower, 1, 1);
		main_draw_graph_btn(frm, btnMainSq1, bmpRootX, 1, 1);
		break;
	default:
	}
}


/***********************************************************************
 *
 * FUNCTION:     main_init
 * 
 * DESCRIPTION:  Initialize the main screen
 *
 * PARAMETERS:   formId
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void 
main_init(UInt16 formId) IFACE;
static void 
main_init(UInt16 formId)
{
	FieldPtr pole;
	UInt16 button;

	result_init(resultGadget);
	
	pole=GetObjectPtr(tdInput);
	FldInsert(pole,calcPrefs.input,StrLen(calcPrefs.input));
	FldSetInsPtPosition(pole,calcPrefs.insertPos);
	FldSetSelection(pole,calcPrefs.selPosStart,
					calcPrefs.selPosEnd);	
	
	switch (formId) {
	case frmBasic:
	case frmBasicS:
		button = btnBasic;
		break;
	case frmScientific:
		button = btnScientific;
		break;
	case frmInteger:
		button = btnInteger;
		break;
	default:
		button = btnBasic;
		break;
	}
	CtlSetValue(GetObjectPtr(button),true);
	result_clear_arrowflags();
}

/***********************************************************************
 *
 * FUNCTION:     main_update
 * 
 * DESCRIPTION:  Setup some things on a form
 *
 * PARAMETERS:   Nothing
 *
 * RETURN:       Nothing
 *      
 ***********************************************************************/
static void 
main_update(void) IFACE;
static void 
main_update(void)
{
	UInt16 formid;
	Tbase base; 
	Tbase trig;
	
	formid=FrmGetActiveFormID();
	main_fix_graph_btns(formid);
	switch (formid) {
	case frmInteger:
		base=dispPrefs.base;
		CtlSetValue(GetObjectPtr(tdIntDec),base==disp_decimal);
		CtlSetValue(GetObjectPtr(tdIntOct),base==disp_octal);
		CtlSetValue(GetObjectPtr(tdIntBin),base==disp_binary);
		CtlSetValue(GetObjectPtr(tdIntHex),base==disp_hexa);
		break;
	case frmScientific:
		main_btnrow_set(calcPrefs.btnRow);
		break;
	}
	switch (formid) {
	case frmScientific:
	case frmBasic:
	case frmBasicS:
		if (flPointChar == '.')
			CtlSetLabel(GetObjectPtr(btnMainDot),".");
		else
			CtlSetLabel(GetObjectPtr(btnMainDot),",");
		
		trig=calcPrefs.trigo_mode;
		base=dispPrefs.base;
		CtlSetLabel(GetObjectPtr(btnPrefMode),trigmode[trig].trigtext);

		switch (base) {
		case disp_binary:
		  CtlSetLabel(GetObjectPtr(btnRadixMode),"Bin");
		  break;
		case disp_octal:
		  CtlSetLabel(GetObjectPtr(btnRadixMode),"Oct");
		  break;
		case disp_hexa:
		  CtlSetLabel(GetObjectPtr(btnRadixMode),"Hex");
		  break;
		case disp_decimal:
		default:
		  CtlSetLabel(GetObjectPtr(btnRadixMode),"Dec");
		  break;
		}

		break;
	}

	result_draw();
}


Boolean 
MainFormHandleEvent(EventPtr event)
{
	Boolean    handled = false;
	FormPtr     frm;
	FieldPtr field;
	ListPtr  lst;
	char *text;
	UInt16 formid;
	UInt16 controlID;
	UInt16 insptr;
	Int16 i;
	Tbase trig;
	Tbase base;
	static UInt16 BrH=0,start;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif
	
	if(event->eType && BrH==1){
		EventType newevent;
		newevent.eType=nilEvent;
		EvtAddEventToQueue(&newevent);
		EvtAddEventToQueue(event);
		return true;
	}

	switch (event->eType) 
	{
	case penDownEvent:
		handled=result_track(event->screenX,event->screenY);
		break;
	case keyDownEvent:
		formid = FrmGetActiveFormID();
		if (TxtGlueCharIsValid(event->data.keyDown.chr) &&
		    TxtGlueCharIsCntrl(event->data.keyDown.chr) &&
		    event->data.keyDown.chr != chrBackspace &&
		    event->data.keyDown.chr != chrDelete &&
		    event->data.keyDown.chr != pageUpChr &&
		    event->data.keyDown.chr != pageDownChr) {
			/* Swallow control chars except Backspace, Delete and page up/down */
			handled = true;
 		} else if (event->data.keyDown.chr=='\n') {
			main_input_exec();
			handled=true;
		} else if (event->data.keyDown.chr==')' || event->data.keyDown.chr=='(') {
			calc_nil_timeout(0);
			BrH=2;
		} else if (formid == frmScientific
			   && event->data.keyDown.chr == pageDownChr) {
			lst = GetObjectPtr(lstMainBtnRow);
			calcPrefs.btnRow++;
			if (calcPrefs.btnRow >= LstGetNumberOfItems(lst))
				calcPrefs.btnRow = 0;
			main_btnrow_set(calcPrefs.btnRow);
			handled = true;
		} else if (formid == frmScientific
			   && event->data.keyDown.chr == pageUpChr) {
			lst = GetObjectPtr(lstMainBtnRow);
			if (calcPrefs.btnRow == 0) 
				calcPrefs.btnRow = LstGetNumberOfItems(lst) - 1;
			else 
				calcPrefs.btnRow--;
			main_btnrow_set(calcPrefs.btnRow);
			handled = true;
		}
		break;
	case frmCloseEvent:		 
		formid=event->data.frmClose.formID;
		  
		field = GetObjectPtr(tdInput);
		calcPrefs.insertPos = FldGetInsPtPosition(field);
		FldGetSelection(field, &calcPrefs.selPosStart,
				&calcPrefs.selPosEnd);
		if (FldGetTextPtr(field))
			StrCopy(calcPrefs.input,FldGetTextPtr(field));
		  
		handled=false; /* This is important - let the
				* FrmHandleEvent handle this
				* itself */
		break;
	case frmUpdateEvent:
		if (event->data.frmUpdate.updateCode == frmUpdateVars) {
		    if (FrmGetActiveFormID() == frmBasic && calcPrefs.dispScien)
			ChangeForm(frmBasicS);
		    else if (FrmGetActiveFormID() == frmBasicS && !calcPrefs.dispScien)
			ChangeForm(frmBasic);
		    else
			main_update();
		    handled = true;
		}
		if (event->data.frmUpdate.updateCode == frmUpdateResult) {
			result_draw();
			handled = true;
		}
		if (event->data.frmUpdate.updateCode == frmRedrawUpdateCode) {
			FrmDrawForm(FrmGetActiveForm());
			result_draw();
			handled = true;
		}
		break;
	case frmOpenEvent:		  
		frm=FrmGetActiveForm();
		formid=FrmGetActiveFormID();
		  
		calcPrefs.form=formid;
		  
		main_init(formid);
		FrmDrawForm(frm);
		main_update();

		FrmSetFocus(frm,FrmGetObjectIndex(frm,tdInput));
		  
		handled = true;
		break;
	case ctlRepeatEvent:
		frm = FrmGetActiveForm();
		controlID=event->data.ctlSelect.controlID;
		switch (controlID) {
		case btnMainBksp:
			//EvtEnqueueKey(chrLeftArrow,0,0);
			field = GetObjectPtr(tdInput);
			insptr = FldGetInsPtPosition(field);
			FldSetSelection(field,insptr,insptr);
			FldSetInsPtPosition(field,insptr-1);
			/* Make sure cursor is on. Needed in PalmOS > 5.4.0 */
			FrmSetFocus(frm, FrmGetObjectIndex(frm, tdInput));
			handled = false;
			break;
		case btnMainFwsp:
			//EvtEnqueueKey(chrRightArrow,0,0);
			field = GetObjectPtr(tdInput);
			insptr = FldGetInsPtPosition(field);
			if (insptr == FldGetTextLength(field)) {
				FldSetSelection(field,insptr,insptr);
				FldSetInsPtPosition(field,0);
				handled = false;
				break;
			}
			else
				FldSetSelection(field,insptr,insptr);
			FldSetInsPtPosition(field,insptr+1);
			/* Make sure cursor is on. Needed in PalmOS > 5.4.0 */
			FrmSetFocus(frm, FrmGetObjectIndex(frm, tdInput));
			handled = false;
			break;	
		case btnMainDel: 
			//EvtEnqueueKey(chrBackspace,0,0);
			field = GetObjectPtr(tdInput);
			insptr = FldGetInsPtPosition(field);
			if (insptr ==0)
				break;
			else {
				FldSetSelection(field,insptr,insptr);
				FldDelete(field,insptr-1,insptr);
				/* Make sure cursor is on. Needed in PalmOS > 5.4.0 */
				FrmSetFocus(frm, FrmGetObjectIndex(frm, tdInput));
				handled = false;
				break;
			}
			break;
		}
		break;
	case ctlSelectEvent:
		controlID=event->data.ctlSelect.controlID;
		
		if (chooseForm(controlID)) {
			handled=true;
			break;
		}
		
		handled=true;
		switch (controlID) {
		case btnPrefMode:
			trig=calcPrefs.trigo_mode;
			switch (trig) {
			case 1:
				trig = 0;
				break;
			case 2:
				trig = 1;
				break;
			case 0:
				trig = 2;
				break;
			}
			calcPrefs.trigo_mode = trig;
			CtlSetLabel(GetObjectPtr(btnPrefMode),trigmode[trig].trigtext);
			//result_draw();
			break;
			
		case btnRadixMode:
			base=dispPrefs.base;
			switch (base){
			case 16:
				base=10;
				CtlSetLabel(GetObjectPtr(btnRadixMode),"Dec");
				break;
			case 10:
				base = 8;
				CtlSetLabel(GetObjectPtr(btnRadixMode),"Oct");
				break;
			case 8:
				base = 2;
				CtlSetLabel(GetObjectPtr(btnRadixMode),"Bin");
				break;
			case 2:
				base = 16;
				CtlSetLabel(GetObjectPtr(btnRadixMode),"Hex");
				break;
			}
			dispPrefs.base = base;
			fp_set_base(base);
			ans_redisplay("ans");
			break;
		case calcResPopupBut: 
			result_popup();
			break;
		case varFuncListButton:
		case varBuiltinListButton:
			if (controlID == varFuncListButton)
				text = varmgr_popup(function);
			else
				text = varmgr_popup_builtin();
			if (text) {
				main_insert(tdInput,text,false,true,false,NULL);
				MemPtrFree(text);
			}
			break;
		case varListButton:
		case btnHistory:
			if (controlID==varListButton)
				text = varmgr_popup(variable);
			else 
				text = history_popup();
			if (text) {
				field = GetObjectPtr(tdInput);
				FldInsert(field, text, StrLen(text));
				MemPtrFree(text);
			}
			break;
		case btnMainSel:
			LstSetSelection(GetObjectPtr(lstMainBtnRow), calcPrefs.btnRow);
			i = LstPopupList(GetObjectPtr(lstMainBtnRow));
			if (i != noListSelection && i != calcPrefs.btnRow) {
				calcPrefs.btnRow = i;
				main_btnrow_set(i);
			}
			break;
		case tdIntBin:
		case tdIntHex:
		case tdIntDec:
		case tdIntOct:
			switch (controlID) {
			case tdIntBin:
				i=disp_binary;
				break;
			case tdIntHex:
				i=disp_hexa;
				break;
			case tdIntOct:
				i=disp_octal;
				break;
			case tdIntDec:
			default:
				i=disp_decimal;
				break;
			}
			fp_set_base(i);
			ans_redisplay("ans");
			break;
		case tdDoit:
			main_input_exec();
			break;
		case btnMainClear:
			field = GetObjectPtr(tdInput);
			FldDelete(field,0,FldGetTextLength(field));
			result_set_text("", notype);
			result_draw();
			break;
		case btnMainDot:
			FldInsert(GetObjectPtr(tdInput),&flPointChar,1);
			break;
		default:
			if(calcPrefs.matchParenth) {
			  handled=false;
			  break;
			}
		case btnMainClBr:
			BrH=2;
			calc_nil_timeout(0);
			handled=false;
			break;
		}

		if (main_btnrow_click(controlID)) 
			handled = true;

		if (handled)
			break;
		
		/* If we are in Octal mode and user tries to use decimal
		 * keys, warn him. This is the most frequent problem
		 * I got and this should solve it.
		 */
		if (dispPrefs.base == disp_octal &&
		    (controlID == btnMain8 || controlID == btnMain9)) {
		    FrmCustomAlert(altOctalMode,"Octal",NULL,NULL);
		    handled = true;
		    break;
		}
		else if (dispPrefs.base == disp_binary &&
			 controlID >= btnMain2 && controlID <=btnMain9) {
		    FrmCustomAlert(altOctalMode,"Binary",NULL,NULL);
		    handled = true;
		    break;
		}

		for (i=0;buttonString[i].id;i++) 
			if (buttonString[i].id == controlID) {
				main_insert(tdInput,buttonString[i].string,buttonString[i].operator,
					    buttonString[i].func,buttonString[i].nostartparen,NULL);
				FrmSetFocus(FrmGetActiveForm(),
					    FrmGetObjectIndex(FrmGetActiveForm(),
							      tdInput));
				handled=true;
				break;
			}
		break;
	case menuEvent:		  
		if (chooseForm(event->data.menu.itemID)) {
			handled=true;
			break;
		}
		switch (event->data.menu.itemID) {
		case mitDefMgr:
			FrmPopupForm(defForm);
			handled = true;
			break;
		case mitFinCalc:
			FrmPopupForm(finForm);
			handled = true;
			break;
		case tdBPref:
			FrmPopupForm(prefForm);
			handled=true;
			break;
		case mitClearHistory:
			history_shrink(0);
			handled = true;
			break;
		case mitMemoExport:
			memo_dump();
			handled = true;
			break;
		case mitMemoImport:
			FrmPopupForm(memoImportForm);
			handled = true;
			break;
#ifdef SPECFUN_ENABLED
		case mitListEdit:
			FrmPopupForm(frmListEdit);
			handled = true;
			break;
		case mitMatrixEdit:
			FrmPopupForm(frmMatrix);
			handled = true;
			break;
		case mitSolver:
			FrmPopupForm(slvForm);
			handled = true;
			break;
#endif
		}
		break;		    
	case fldEnterEvent:
		if(event->data.fldEnter.fieldID==tdInput){
		   BrH=2;
		   calc_nil_timeout(0);
		}
		break;	
	case nilEvent:
		if(BrH==2){
			UInt16 j,k=1,end;
			char char1,char2;

			field=GetObjectPtr(tdInput);
			FldGetSelection(field,&start,&end);

		   	if(start!=end || end==0){
			   goto exit;
			}

			j=start-1;
		   	text = FldGetTextPtr(field);
			if(text[j]==')'){
			   char1=')'; char2='(';
			   i=-1; end=0;
			}else if(text[j]=='('){
			   char1='('; char2=')';
			   i=1; end=StrLen(text);
			}else{
			   goto exit;
			}

			while(j!=end && k){
			  j+=i;
	           	  if(text[j]==char1) k++; 
	           	  else if(text[j]==char2) k--; 
			}

			if(k) goto exit;
			else{
			  FldSetSelection(field,j,j+1);
			  calc_nil_timeout(40);
			  BrH=1;
			}
		}
		else if(BrH==1)
		{
			FldSetSelection(GetObjectPtr(tdInput),start,start);
		exit:	calc_nil_timeout(evtWaitForever);
			BrH=0;
		}
		handled=true;
		break;
	}
	
	return handled;
}
