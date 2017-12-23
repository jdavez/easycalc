#pragma once

#ifndef PREFERENCES_H
#define PREFERENCES_H 1

typedef enum {
    nfCommaPeriod,
    nfPeriodComma,
    nfSpaceComma,
    nfApostrophePeriod,
    nfApostropheComma
} NumberFormatType;

typedef enum {
    prefNumberFormat
} SystemPreferencesChoice;

unsigned int PrefGetPreference(SystemPreferencesChoice choice);

#endif