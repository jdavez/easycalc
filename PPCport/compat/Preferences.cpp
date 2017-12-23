#include "StdAfx.h"
#include "Preferences.h"
#include <winnls.h>

#define DEC_DOT 0
#define DEC_COM 1
#define TH_SPC  0
#define TH_COM  1
#define TH_DOT  2
#define TH_APO  3

unsigned int PrefGetPreference(SystemPreferencesChoice choice) {
    unsigned int returnVal;

    switch (choice) {
        case prefNumberFormat:
            {
            int dec, th;
            TCHAR cdec[10], cth[10];
            int rcdec = GetLocaleInfo(LOCALE_USER_DEFAULT, 
                                      LOCALE_SDECIMAL, 
                                      cdec, 
                                      9 
                                     );
            int rcth = GetLocaleInfo(LOCALE_USER_DEFAULT, 
                                     LOCALE_STHOUSAND, 
                                     cth, 
                                     9 
                                    );
            if (rcdec <= 0)   dec = DEC_DOT;    // By default ..
            else {
                cdec[rcdec] = 0;
                if (_tcsstr(cdec, _T(",")) != NULL) {
                    dec = DEC_COM;
                } else {
                    dec = DEC_DOT; // By default
                }
            }

            if (rcth <= 0)   th = TH_SPC;       // By default ..
            else {
                cth[rcth] = 0;
                if (_tcsstr(cth, _T("'")) != NULL) {
                    th = TH_APO;
                } else if (_tcsstr(cth, _T(".")) != NULL) {
                    th = TH_DOT;
                } else if (_tcsstr(cth, _T(",")) != NULL) {
                    th = TH_COM;
                } else {
                    th = TH_SPC; // By default
                }
            }

            if ((th == TH_APO) && (dec == DEC_COM))   returnVal = nfApostropheComma;
            else if ((th == TH_APO) && (dec == DEC_DOT))   returnVal = nfApostrophePeriod;
            else if ((th == TH_SPC) && (dec == DEC_COM))   returnVal = nfSpaceComma;
            else if ((th == TH_DOT) && (dec == DEC_COM))   returnVal = nfPeriodComma;
            else    returnVal = nfCommaPeriod;
            }
            break;

        default:
            returnVal = 0;
    }

    return (returnVal);
}