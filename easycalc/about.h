/*
 * $Id: about.h,v 1.3 2006/09/12 19:40:55 tvoverbe Exp $
 *
 * Copyright (C) 2006 Ton van Overbeek
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Summary
 *  Resizable about and help form routines for EasyCalc.
 *
 * Author
 *  Ton van Overbeek, ton@v-overbeek.nl
 *
 */

Boolean aboutEventHandler(EventPtr event);
void doAbout(UInt16 aboutStringID);
void doHelp(UInt16 hlpTitleID, UInt16 hlpTextID);
