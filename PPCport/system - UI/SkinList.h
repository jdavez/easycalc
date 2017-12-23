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
/* SkinList.h : SkinList object for registering Skin usage of the calculator.
 *****************************************************************************/
#ifndef SKINLIST_H
#define SKINLIST_H 1

// Object to contain a skin number and usage
class SkinUsage {
public:
    int skin_nb;
    int usage;
    SkinUsage *prev, *next;

    SkinUsage(int nb);  // Constructor
    ~SkinUsage();       // Destructor
};

// SkinList object
class SkinList {
protected:
    SkinUsage *list;
    SkinUsage *cur_skin;
    int        nb_skins;

public:
    SkinList();    // Constructor
    ~SkinList();   // Destructor

    void clear(void);
    void add(int nb);
    void add_use(int nb);
    int least_used(void);
};

#endif