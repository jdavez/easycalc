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
/* Lang.cpp : Lang and LibLang objects for handling language translations.
 *****************************************************************************/

#include "StdAfx.h"
#include "compat/Lang.h"
#include "winnls.h"
#include <locale.h>

#ifdef  UNICODE
#ifdef WINCE
static TCHAR _tlocaleInfo[64];
TCHAR *Lang::_tsetlocale (int category, const TCHAR *locale) {
    char localeTmp[64];

    wcstombs(localeTmp, locale, 64);
// This uses the current locale, which I didn't find how to set in Win CE.
// mbstowcs(_tlocaleInfo, localeInfo, 64);MultiByteToWideChar();
    if (MultiByteToWideChar(codePage, MB_PRECOMPOSED, localeTmp, -1, _tlocaleInfo, 64) == 0) {
        // Failed to convert to the target codepage
        return (NULL);
    }
    return (_tlocaleInfo);
}
 #else
    #define _tsetlocale _wsetlocale
 #endif
#else
    #define _tsetlocale setlocale
    #define mbstowcs(a,b,c)  ((a == NULL) ? (strlen(b)) : ((strcpy(a,b) == NULL) ? -1 : c))
#endif
#include <stdlib.h>
#include "PalmOS.h"


/*-------------------------------------------------------------------------------
 - The Lang object Constructor and Destructor.                                  -
 -------------------------------------------------------------------------------*/
Lang::Lang(const TCHAR *name) {
    localeInfo = NULL;

    langName.assign(name);
    // Try to set the locale with the provided string first.
#ifdef WINCE
    {
#else
    if ((localeInfo = _tsetlocale(LC_ALL, name)) != NULL) {
    } else { // Not recognized by windows.
#endif
             // Map using the Palm EasyCalc PilRC names
        if (_tcscmp(name, _T("cs")) == 0) {
            codePage = 1250;
            systLangId = MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT);
            localeInfo = _tsetlocale(LC_ALL, _T("Czech"));
        } else if (_tcscmp(name, _T("de")) == 0) {
            codePage = 1252;
            systLangId = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
            localeInfo = _tsetlocale(LC_ALL, _T("German"));
        } else if (_tcscmp(name, _T("en")) == 0) {
            codePage = 1252;
            systLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            localeInfo = _tsetlocale(LC_ALL, _T("English"));
        } else if (_tcscmp(name, _T("fr")) == 0) {
            codePage = 1252;
            systLangId = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
            localeInfo = _tsetlocale(LC_ALL, _T("French"));
        } else if (_tcscmp(name, _T("it")) == 0) {
            codePage = 1252;
            systLangId = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
            localeInfo = _tsetlocale(LC_ALL, _T("Italian"));
        } else if (_tcscmp(name, _T("ja")) == 0) {
            codePage = 932;
            systLangId = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
            localeInfo = _tsetlocale(LC_ALL, _T("Japanese_Japan.932"));
            // This may not load correctly on western systems with Japan non installed,
            // so try a bypass .., which should set at least the code page correcly.
            if (localeInfo == NULL) {
                localeInfo = _tsetlocale(LC_ALL, _T(".932"));
            }
        } else if (_tcscmp(name, _T("pt")) == 0) {
            codePage = 1252;
            systLangId = MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE);
            localeInfo = _tsetlocale(LC_ALL, _T("Portuguese"));
        } else if (_tcscmp(name, _T("ru_koi8")) == 0) {
            // Note that koi8-u, for Ukrainia, is 21866
            codePage = 20866;
            systLangId = MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT);
            localeInfo = _tsetlocale(LC_ALL, _T("Russian_Russia.20866"));
        } else if (_tcscmp(name, _T("ru_win")) == 0) {
            codePage = 1251;
            systLangId = MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT);
            localeInfo = _tsetlocale(LC_ALL, _T("Russian"));
        } else if (_tcscmp(name, _T("sp")) == 0) {
            codePage = 1252;
            systLangId = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
            localeInfo = _tsetlocale(LC_ALL, _T("Spanish"));
        }
        if (localeInfo == NULL) { // Cannot recognize the language, use a character set by default
                                  // for converting from what is read to wide characters.
            codePage = 1252;
            systLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            localeInfo = _tsetlocale(LC_ALL, _T("English"));
        }
    }

    // Since the string returned by setlocale can be overwritten, save a copy
    localeInfo = _mtcsdup(localeInfo);
}

Lang::~Lang(void) {
    stdext::hash_map<String,String>::const_iterator res;

//    res = translation.begin();
//    while (res != translation.end()) {
//        delete (res->first);
//        delete (res->second);
//        res++;
//    }

    langName.clear();
//    delete langName;
    translation.clear();
//    delete translation;
    mfree(localeInfo);
}

/*-------------------------------------------------------------------------------
 - Lang object methods.                                                         -
 -------------------------------------------------------------------------------*/
/********************************************************************************
 * Insert a translation pair.                                                   *
 ********************************************************************************/
bool Lang::insert(const TCHAR *key, const TCHAR *value) {
    String s1 (key);
    String s2 (value);
    std::pair<stdext::hash_map<String,String>::iterator, bool> res;

    res = translation.insert(Pair(s1, s2));
    return (res.second);
}

/********************************************************************************
 * Get the text of a translation entry.                                         *
 ********************************************************************************/
const TCHAR *Lang::get(TCHAR *key) {
    String s1 (key);
    stdext::hash_map<String,String>::const_iterator res;

    res = translation.find(s1);
    if (res == translation.end())   return (NULL);
    return (res->second.c_str());
}


/*-------------------------------------------------------------------------------
 - The LibLang object Constructor and Destructor.                               -
 -------------------------------------------------------------------------------*/
#define SPACECHARS " \t"
#define EOLCHARS "\r\n"
#define ESCAPECHARS "\\"
#define STRINGDELIMITERS "\""
#define ST1COMMENTCHARS "/"
#define ST2COMMENTCHARS "*"
#define EQUALCHARS "="

#define TRANSLATION    1
#define LANGNAME       2
#define BEGIN          3
#define STRING1_OR_END 4
#define EQUAL          6
#define STRING2        7

#define LEX_IN_NONE         0
#define LEX_IN_WORD         1
#define LEX_IN_STRING       2
#define LEX_IN_ESCAPE       3
#define LEX_END_STRING      4
#define LEX_BETWEEN_STRINGS 5
#define LEX_IN_STARTCOMMENT 6
#define LEX_IN_COMMENT      7
#define LEX_IN_ENDCOMMENT   8

#define TOK_NONE   0
#define TOK_WORD   1
#define TOK_STRING 2
#define TOK_EQUAL  3

LibLang::LibLang(FILE *langFile) {
    wcstr = NULL;
    allocLen = nbLang = 0;
    firstCalled = false;

    // Parse the lang.rcp file until EOF.
    // Simple parser, line oriented.
    // As per http://www.dtek.chalmers.se/groups/pilot/doc/pilrc.htm
    //    TRANSLATION <Language.s>
    //BEGIN
    //        <STRINGTRANSLATIONS>
    //END
    //Where <STRINGTRANSLATIONS> is one or more of:
    //    <Original.s> = <Translated.s>
    //Note: a '\' at end of line means "line continued on next line".
    //      <...> means a string between "".
    //      /* ... */ delimit comments, which can be on multiple lines.
    //      No nested comment support.
    int c, clast;
    int nextToken = TRANSLATION; // Start looking for TRANSLATION
    int token, lexState;
    bool complete;
    std::string s;
    std::string s1;
    Lang *lang = NULL;
    while (!feof(langFile)) {
        // Get a new token from the lexical "level".
        s.clear();
        lexState = LEX_IN_NONE; // Start the lexical level in no mode
        token = TOK_NONE;       // No lexical token recognized yet
        complete = false;
        while (!complete && (c = fgetc(langFile)) != EOF) {
            switch (lexState) {
                case LEX_IN_NONE:
                    if ((strchr(SPACECHARS, c) != NULL)
                        || (strchr(EOLCHARS, c) != NULL))
                        break;
                    else if (strchr(STRINGDELIMITERS, c) != NULL) { // We're getting the start of a string
                        token = TOK_STRING;
                        lexState = LEX_IN_STRING;
                    } else if (strchr(ST1COMMENTCHARS, c) != NULL) { // We're getting the start of a comment
                        clast = c;
                        lexState = LEX_IN_STARTCOMMENT;
                    } else if (strchr(EQUALCHARS, c) != NULL) {
                        token = TOK_EQUAL;
                        s.append(1, c);
                        complete = true;
                    } else { // Anything else is a word, by default
                        token = TOK_WORD;
                        s.append(1, c);
                        lexState = LEX_IN_WORD;
                    }
                    break;

                case LEX_IN_WORD:
                    if ((strchr(SPACECHARS, c) != NULL)
                        || (strchr(EOLCHARS, c) != NULL)) {
                        complete = true;
                    } else if ((strchr(STRINGDELIMITERS, c) != NULL)
                        || (strchr(ST1COMMENTCHARS, c) != NULL)
                        || (strchr(EQUALCHARS, c) != NULL)) {
                        ungetc (c, langFile); // Give back the character for the next
                                              // lexical round.
                        complete = true;
                    } else { // Anything else is part of the word
                        s.append(1, c);
                    }
                    break;

                case LEX_IN_STRING:
                    if (strchr(ESCAPECHARS, c) != NULL) { // Escape character, keep the next one,
                                                          // whatever it is (except end of line)
                        clast = c;
                        lexState = LEX_IN_ESCAPE;
                    } else if (strchr(STRINGDELIMITERS, c) != NULL) { // We're getting the end of the string
                        lexState = LEX_END_STRING;
                    } else if (strchr(EOLCHARS, c) != NULL) { // End of line reached, stop the string there
                        complete = true;
                    } else {
                        s.append(1, c);
                    }
                    break;

                case LEX_IN_ESCAPE:
                    if (strchr(EOLCHARS, c) != NULL) { // End of line reached, stop the string there
                        complete = true;
                    } else {
                        switch (c) {
                            case 'n':
#ifdef _WINDOWS
                                s.append("\r\n");
#else
                                s.append(1, '\n');
#endif
                                break;
                            case 'r':
                                s.append(1, '\r');
                                break;
                            case 't':
                                s.append(1, '\t');
                                break;
                            default:
                                s.append(1, clast); // Keep the '\'
                                s.append(1, c);
                        }
                        lexState = LEX_IN_STRING;
                    }
                    break;

                case LEX_END_STRING:
                    if (strchr(ESCAPECHARS, c) != NULL) { // Escape character, meaning the string
                                                          // goes on on next line.
                        lexState = LEX_BETWEEN_STRINGS;
                    } else if (strchr(SPACECHARS, c) == NULL) { // Consume spaces between '"' and '\'
                        // Not space and not '\'
                        ungetc (c, langFile); // Give back the character for the next
                                              // lexical round.
                        complete = true;
                    }
                    break;

                case LEX_BETWEEN_STRINGS:
                    if (strchr(STRINGDELIMITERS, c) != NULL) { // We're getting the start of the string continuation
                        lexState = LEX_IN_STRING;
                    }
                    break;

                case LEX_IN_STARTCOMMENT:
                    if (strchr(ST2COMMENTCHARS, c) != NULL) {
                        lexState = LEX_IN_COMMENT;
                    } else { // No start of comment, so go back to a word by default
                        s.append(1, clast);
                        s.append(1, c);
                        lexState = LEX_IN_WORD;
                    }
                    break;

                case LEX_IN_COMMENT:
                    if (strchr(ST2COMMENTCHARS, c) != NULL) {
                        lexState = LEX_IN_ENDCOMMENT; // Possible end of comment
                    }
                    break;

                case LEX_IN_ENDCOMMENT:
                    if (strchr(ST1COMMENTCHARS, c) != NULL) {
                        lexState = LEX_IN_NONE; // Back to normal
                    } else if (strchr(ST2COMMENTCHARS, c) == NULL) {
                        lexState = LEX_IN_COMMENT; // Not '*', go back to comment mode
                    }
                    break;
            }
        }

        // The lexical level is done, analyze at "grammar" level
        if (token != TOK_NONE)   switch (nextToken) {
            case TRANSLATION:
                if (token == TOK_WORD) {
                    const char *str = s.c_str();
                    char *strup = _mstrdup(str);
                    _strupr(strup);
                    if (strcmp(strup, "TRANSLATION") == 0) {
                        nextToken = LANGNAME;
                    }
                    mfree(strup);
                }
                break;

            case LANGNAME:
                if (token == TOK_STRING) { // We've got a new language translation
                    nextToken = BEGIN;

                    // Create or retrieve a language object
                    const TCHAR *langName = convert(s.c_str());
                    lang = (Lang *) getLang (langName);
                    if (lang == NULL) { // Create a new one
                        lang = new Lang (langName);
                        insertLang(lang);
                    }
                } else    nextToken = TRANSLATION;
                break;

            case BEGIN:
                if (token == TOK_WORD) {
                    const char *str = s.c_str();
                    char *strup = _mstrdup(str);
                    _strupr(strup);
                    if (strcmp(strup, "BEGIN") == 0) {
                        nextToken = STRING1_OR_END;
                    } else   nextToken = TRANSLATION;
                    mfree(strup);
                } else   nextToken = TRANSLATION;
                break;

            case STRING1_OR_END:
                if (token == TOK_WORD) {
                    const char *str = s.c_str();
                    char *strup = _mstrdup(str);
                    _strupr(strup);
                    if (strcmp(strup, "END") == 0) {
                        nextToken = TRANSLATION;
                    }
                    mfree(strup);
                } else if (token == TOK_STRING) {
                    s1.assign(s);
                    nextToken = EQUAL;
                }
                break;

            case EQUAL:
                if (token == TOK_EQUAL) {
                    nextToken = STRING2;
                } else   nextToken = STRING1_OR_END;
                break;

            case STRING2:
                if (token == TOK_STRING) {
                    // We have a new entry for translation
                    TCHAR *str1 = _mtcsdup(convert(s1.c_str()));
                    const TCHAR *str2 = convert(s.c_str());
                    lang->insert(str1, str2);
                    mfree(str1);
                }
                nextToken = STRING1_OR_END;
                break;
        }
    }

    // All this has manipulated the locale several times ...
    // Set it to english by default.
    setLang(_T("en"));
    defLang = curLang; // Remember it as default language
}

LibLang::~LibLang(void) {
    stdext::hash_map<String,Lang*>::const_iterator res;

    res = libLang.begin();
    while (res != libLang.end()) {
        delete (res->second);
        res++;
    }
    libLang.clear();
//    delete libLang;
    if (wcstr != NULL)
        mfree(wcstr);
}

/*-------------------------------------------------------------------------------
 - LibLang object methods.                                                      -
 -------------------------------------------------------------------------------*/
/********************************************************************************
 * Insert a new Lang object in library.                                         *
 ********************************************************************************/
bool LibLang::insertLang(Lang *lang) {
    std::pair<stdext::hash_map<String,Lang*>::iterator, bool> res;

    res = libLang.insert(LibPair(lang->langName, lang));
    nbLang++;
    return (res.second);
}

/********************************************************************************
 * Get a Lang name by its system identifier.                                    *
 ********************************************************************************/
const TCHAR *LibLang::getLang(int systLangId) {
    stdext::hash_map<String,Lang*>::const_iterator res;

    res = libLang.begin();
    while (res != libLang.end()) {
        if (res->second->systLangId == systLangId)
            break;
        res++;
    }
    if (res == libLang.end())   return (NULL);
    return (res->second->langName.c_str());
}

/********************************************************************************
 * Get a Lang name by its indexe in the language library.                       *
 ********************************************************************************/
const TCHAR *LibLang::getLangByIndex(int index) {
    stdext::hash_map<String,Lang*>::const_iterator res;

    if (index >= nbLang)
        return (NULL);

    res = libLang.begin();
    while (index > 0) {
        res++;
        index--;
    }
    if (res == libLang.end())   return (NULL);
    return (res->second->langName.c_str());
}

/********************************************************************************
 * Get a Lang object by its name.                                               *
 ********************************************************************************/
const Lang *LibLang::getLang(const TCHAR *langName) {
    String s1 (langName);
    stdext::hash_map<String,Lang*>::const_iterator res;

    res = libLang.find(s1);
    if (res == libLang.end())   return (NULL);
    return (res->second);
}

/********************************************************************************
 * Set current language.                                                        *
 * Returns 0 if ok, something else if not.                                      *
 ********************************************************************************/
int LibLang::setLang(TCHAR *langName) {
    curLang = (Lang *) getLang(langName);

    if (curLang != NULL) {
#ifdef WINCE
        return (0);
#else
        TCHAR *localeInfo = _tsetlocale(LC_ALL, curLang->localeInfo);
        return (localeInfo == NULL);
#endif
    } else   return (-1);
}

/********************************************************************************
 * Get current language.                                                        *
 ********************************************************************************/
const TCHAR *LibLang::getLang(void) {
    return (curLang->langName.c_str());
}

/********************************************************************************
 * Translate a token with current language.                                     *
 * If not found, try the default one.                                           *
 * If not found, return initial string.                                         *
 ********************************************************************************/
const TCHAR *LibLang::translate(TCHAR *token) {
    const TCHAR *text = curLang->get(token);
    if (text == NULL) {
        // Use default application language if not found.
        // Note: desactivated for now, we want to see unmapped tokens to
        //       complement language files.
//        if (defLang != curLang) {
//            text = defLang->get(token);
//        }
//        if (text == NULL)
            text = token;
    }
    return (text);
}

/********************************************************************************
 * Convert from chars in a locale to TCHAR.                                     *
 * When the string is no more needed, it must be freed.                         *
 ********************************************************************************/
const TCHAR *LibLang::convert(const char *str) {
    size_t len = mbstowcs(NULL, str, 0);
    size_t len2 = len * 2 + 2;
    if (len2 > allocLen) {
        if (wcstr != NULL)   mfree(wcstr);
        allocLen = len2;
        wcstr = (TCHAR *) mmalloc(allocLen);
    }
    len2 = mbstowcs(wcstr, str, len+1);
    if (len2 < 0)   return (NULL);
    else   return (wcstr);
}

/********************************************************************************
 * Get the number of language translations held in library.                     *
 ********************************************************************************/
int LibLang::getNbLang(void) {
    return (nbLang);
}

/********************************************************************************
 * Get the first language in library.                                           *
 ********************************************************************************/
const TCHAR *LibLang::getFirst(void) {
    firstCalled = true;

    libIterator = libLang.begin();
    if (libIterator != libLang.end())
        return (libIterator->first.c_str());
    return (NULL);
}

/********************************************************************************
 * Get the next language in library. If first time called, return the first     *
 * entry.                                                                       *
 ********************************************************************************/
const TCHAR *LibLang::getNext(void) {
    if (!firstCalled)   return getFirst();

    if (libIterator != libLang.end())   libIterator++;
    else   return (NULL);

    if (libIterator != libLang.end())
        return (libIterator->first.c_str());
    return (NULL);
}
