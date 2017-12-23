 /*   
 * $Id: prefs.c,v 1.18 2007/08/31 00:58:18 tvoverbe Exp $
 * 
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000 Ondrej Palkovsky
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
*/

#include <PalmOS.h>

#include "calcrsc.h"
#include "konvert.h"
#include "calc.h"
#include "prefs.h"
#include "calcDB.h"
#include "fp.h"
#include "about.h"

#ifdef SUPPORT_DIA
#include "DIA.h"
#endif

tPrefs calcPrefs;

void
prefs_read_preferences()
{
	UInt16 size;
	
	size=sizeof(calcPrefs);
	if (PrefGetAppPreferences(APP_ID,PREF_DEFAULT,&calcPrefs,&size,
				  true)!=PREF_VERSION) {
		calcPrefs.input[0]='\0';
		calcPrefs.form=frmBasic;
		calcPrefs.btnRow = 0;
		calcPrefs.insertPos=0;
		calcPrefs.selPosStart=calcPrefs.selPosEnd=0;
		calcPrefs.solverWorksheet = -1;

		calcPrefs.finBegin=false;
		calcPrefs.matchParenth=false;
		calcPrefs.trigo_mode=radian;
		
		calcPrefs.dispPrefs.forceInteger = false;		
		calcPrefs.dispPrefs.decPoints = 9;
		calcPrefs.dispPrefs.stripZeros = true;
		calcPrefs.reducePrecision = false;
		calcPrefs.dispPrefs.base = disp_decimal;
		calcPrefs.dispPrefs.mode = disp_normal;
		calcPrefs.dispPrefs.cvtUnits = true;
		calcPrefs.insertHelp = true;
		calcPrefs.acceptPalmPref = false;
		calcPrefs.dispScien = false;
		
		dispPrefs = calcPrefs.dispPrefs; /* db_recompile needs dispPrefs */
		db_recompile_all();
		doAbout(hlpMain1); /* calls FrmGotoForm(frmABout) */
	}
	else
		FrmGotoForm(calcPrefs.form);
	fp_set_prefs(calcPrefs.dispPrefs);
}
	
void 
prefs_save_preferences()
{
	calcPrefs.dispPrefs = dispPrefs;
	PrefSetAppPreferences(APP_ID,PREF_DEFAULT,PREF_VERSION,&calcPrefs,
			      sizeof(calcPrefs),true);
}

Boolean
PreferencesHandleEvent(EventPtr event)
{
	Boolean handled=false;
	int controlID;
	FormPtr frm;
	ControlPtr knoflik;
	ListPtr list;

#ifdef SUPPORT_DIA
	if (ResizeHandleEvent(event)) {
		return true;
	}
#endif

	switch (event->eType) {
	 case frmOpenEvent:
		frm=FrmGetActiveForm();		
		FrmDrawForm(frm);
		
		switch (dispPrefs.base) {
		 default:
		 case disp_decimal:
			knoflik=GetObjectPtr(prefDecimal);
			break;
		 case disp_binary:
			knoflik=GetObjectPtr(prefBinary);
			break;
		 case disp_octal:
			knoflik=GetObjectPtr(prefOctal);
			break;
		 case disp_hexa:
			knoflik=GetObjectPtr(prefHexa);
			break;
		}
		CtlSetValue(knoflik,1);
		
		switch (calcPrefs.trigo_mode) {
		 case degree:
			knoflik=GetObjectPtr(prefDegree);
			break;
		 case grad:
			knoflik=GetObjectPtr(prefGrad);
			break;
		 case radian:
		 default:
			knoflik=GetObjectPtr(prefRadian);
			break;
		}
		CtlSetValue(knoflik,1);
		
		
		switch (dispPrefs.mode) {
		 case disp_normal:
			knoflik=GetObjectPtr(prefNormal);
			break;
		 case disp_sci:		
			knoflik=GetObjectPtr(prefSci);
			break;
		 case disp_eng:
		 default:
			knoflik=GetObjectPtr(prefEng);
			break;
		}
		CtlSetValue(knoflik,1);
/* Set the precision */
		list = GetObjectPtr(prefPrecList);
		LstSetSelection(list,dispPrefs.decPoints);
		  {
			  static char tmp[4]; /* It's simpler than MemPtrNew/Free.. */
			  StrPrintF(tmp,"%d",dispPrefs.decPoints);
			  CtlSetLabel(GetObjectPtr(prefPrecPopup),tmp);
		  }
		/* Set the topItem of the list */
		LstMakeItemVisible(list,dispPrefs.decPoints);
                /* Set the checkboxes */
		CtlSetValue(GetObjectPtr(prefReducePrecision),
			     calcPrefs.reducePrecision);
		CtlSetValue(GetObjectPtr(prefStrip),dispPrefs.stripZeros);
		CtlSetValue(GetObjectPtr(prefIntInput),dispPrefs.forceInteger);
		CtlSetValue(GetObjectPtr(prefCvtUnits),dispPrefs.cvtUnits);
		CtlSetValue(GetObjectPtr(prefParenth),calcPrefs.matchParenth);
		CtlSetValue(GetObjectPtr(prefInsertHelp),calcPrefs.insertHelp);
		CtlSetValue(GetObjectPtr(prefAcceptPref),calcPrefs.acceptPalmPref);
		CtlSetValue(GetObjectPtr(prefDispScien),calcPrefs.dispScien);

		handled=true;
		break;
	 case ctlSelectEvent:
		controlID=event->data.ctlSelect.controlID;  
		switch (controlID) {
		 case prefOK:
			if (CtlGetValue(GetObjectPtr(prefNormal)))
			  dispPrefs.mode=disp_normal;
			else if (CtlGetValue(GetObjectPtr(prefSci)))
			  dispPrefs.mode=disp_sci;
			else if (CtlGetValue(GetObjectPtr(prefEng)))
			  dispPrefs.mode=disp_eng;
			
			if (CtlGetValue(GetObjectPtr(prefDegree)))
			  calcPrefs.trigo_mode = degree;
			else if (CtlGetValue(GetObjectPtr(prefRadian)))
			  calcPrefs.trigo_mode = radian;
			else if (CtlGetValue(GetObjectPtr(prefGrad)))
			  calcPrefs.trigo_mode = grad;
			
			if (CtlGetValue(GetObjectPtr(prefDecimal)))
			  dispPrefs.base = disp_decimal;
			else if (CtlGetValue(GetObjectPtr(prefBinary)))
			  dispPrefs.base = disp_binary;
			else if (CtlGetValue(GetObjectPtr(prefOctal)))
			  dispPrefs.base = disp_octal;
			else if (CtlGetValue(GetObjectPtr(prefHexa)))
			  dispPrefs.base = disp_hexa;

			calcPrefs.insertHelp=CtlGetValue(GetObjectPtr(prefInsertHelp));
			calcPrefs.acceptPalmPref=CtlGetValue(GetObjectPtr(prefAcceptPref));
			calcPrefs.matchParenth=CtlGetValue(GetObjectPtr(prefParenth));
			calcPrefs.reducePrecision=CtlGetValue(GetObjectPtr(prefReducePrecision));
			calcPrefs.dispScien=CtlGetValue(GetObjectPtr(prefDispScien));
			
			dispPrefs.stripZeros=CtlGetValue(GetObjectPtr(prefStrip));
			dispPrefs.decPoints = LstGetSelection(GetObjectPtr(prefPrecList));
			dispPrefs.forceInteger=CtlGetValue(GetObjectPtr(prefIntInput));
			dispPrefs.cvtUnits=CtlGetValue(GetObjectPtr(prefCvtUnits));
			fp_set_prefs(dispPrefs);
			FrmReturnToForm(0);
			FrmUpdateForm(calcPrefs.form,frmUpdateVars);
			handled=true;
			break;
		 case prefCancel:
			FrmReturnToForm(0);
			handled=true;
			break;
		}
	}	
	  
	return handled;
}

