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
/* Lang.h : Lang and LibLang objects for handling language translations.
 *****************************************************************************/
#pragma once

#include <stdio.h>
#include <string>
#include <string.h>
#include <hash_map>

typedef std::wstring String;
typedef std::pair<String,String> Pair;

// Object to contain one language
class Lang {
protected:
    int                             codePage;
    stdext::hash_map<String,String> translation;
public:
    int                             systLangId;
    String                          langName;
    TCHAR                          *localeInfo;

    Lang(const TCHAR *name);
    ~Lang(void);
#ifdef WINCE
    TCHAR *_tsetlocale (int category, const TCHAR *locale);
#endif
    bool insert(const TCHAR *key, const TCHAR *value);
    const TCHAR *get(TCHAR *key);
};

// Library of languages, as read from the concatenated lang.rcp file.
typedef std::pair<String,Lang*> LibPair;

class LibLang {
protected:
    int                            nbLang;
    bool                           firstCalled;
    Lang                          *curLang;
    Lang                          *defLang;
    TCHAR                         *wcstr;
    size_t                         allocLen; // Track size m-allocated to wcstr
    stdext::hash_map<String,Lang*> libLang;
    stdext::hash_map<String,Lang*>::const_iterator libIterator;

    bool insertLang(Lang *lang);
    const Lang *getLang(const TCHAR *langName);
public:
    LibLang(FILE *langFile);
    ~LibLang(void);
    const TCHAR *convert(const char *str);
    int getNbLang(void);
    const TCHAR *getFirst(void);
    const TCHAR *getNext(void);
    int setLang(TCHAR *langName);
    const TCHAR *getLang(int systLangId);
    const TCHAR *getLang(void);
    const TCHAR *getLangByIndex(int index);
    const TCHAR *translate(TCHAR *key);
};
