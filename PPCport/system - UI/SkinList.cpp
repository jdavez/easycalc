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
/* SkinList.cpp : Methods for the SkinList object.
 *****************************************************************************/

#include "stdafx.h"

#include "SkinList.h"

/*-------------------------------------------------------------------------------
 - SkinUsage constructor and destructor.                                        -
 -------------------------------------------------------------------------------*/
SkinUsage::SkinUsage(int nb) {
    skin_nb = nb;
    usage = 0;
    prev = next = NULL;
}

SkinUsage::~SkinUsage() {
//    if (next != NULL)
//        delete next;
}

/*-------------------------------------------------------------------------------
 - SkinList constructor and destructor.                                         -
 -------------------------------------------------------------------------------*/
SkinList::SkinList() {
    list = cur_skin = NULL;
    nb_skins = 0;
}

SkinList::~SkinList() {
    clear();
}

/*-------------------------------------------------------------------------------
 - Methods.                                                                     -
 -------------------------------------------------------------------------------*/
/********************************************************************************
 * Clear list.                                                                  *
 ********************************************************************************/
void SkinList::clear (void) {
    SkinUsage *temp;

    while (list != NULL) {
        temp = list;
        list = list->next;
        delete temp;
    }
    cur_skin = NULL;
    nb_skins = 0;
}


/********************************************************************************
 * Add new skin.                                                                *
 ********************************************************************************/
void SkinList::add (int nb) {
    // Verify it doesn't exist already. If so reset the object and reuse it
    SkinUsage *temp = list;
    while (temp != NULL) {
        if (temp->skin_nb == nb)
            break;
        temp = temp->next;
    }
    if (temp != NULL) { // Remove it from list
        if (temp->prev == NULL)
            list = temp->next;
        else {
            temp->prev->next = temp->next;
            if (temp->next != NULL)
                temp->next->prev = temp->prev;
            temp->prev = NULL;
        }
        // Reset usage count
        temp->usage = 0;
    } else {
        temp = new SkinUsage(nb);
    }
    // Add at head
    temp->next = list;
    if (list != NULL)
        list->prev = temp;
    list = temp;
}


/********************************************************************************
 * Register usage.                                                              *
 ********************************************************************************/
void SkinList::add_use (int nb) {
    // Find it
    SkinUsage *temp = list;
    while (temp != NULL) {
        if (temp->skin_nb == nb)
            break;
        temp = temp->next;
    }
    if (temp != NULL) { // Register usage, and keep list sorted in increasing usage order
        cur_skin = temp; // Remember last used
        temp->usage++;
        // Find new position
        SkinUsage *temp2 = temp;
        while ((temp2->next != NULL) && (temp2->next->usage < temp->usage)) {
            temp2 = temp2->next;
        }
        if (temp2 != temp) { // Move ...
            if (temp->prev == NULL) {
                list = temp->next;
                temp->next->prev = NULL;
            } else {
                temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
            }
            temp->next = temp2->next;
            if (temp->next != NULL)
                temp->next->prev = temp;
            temp2->next = temp;
            temp->prev = temp2;
        }
    }
}


/********************************************************************************
 * Get least used, different from last used (if possible).                      *
 ********************************************************************************/
int SkinList::least_used (void) {
    int nb = -1;
    // Get first, or second if exists and first is last used.
    if ((list == cur_skin) && (list->next != NULL)) {
        nb = list->next->skin_nb;
    } else {
        nb = list->skin_nb;
    }
    return (nb);
}
