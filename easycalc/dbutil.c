/*
 *   $Id: dbutil.c,v 1.1 2007/09/28 01:23:25 tvoverbe Exp $
 *
 *   Scientific Calculator for Palms.
 *   Copyright (C) 1999,2000,2007 Ondrej Palkovsky
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
#include "defuns.h" 

static Err
db_open_byid(LocalID dbid, DmOpenRef *dbref)
{
        *dbref = DmOpenDatabase(CARDNO, dbid, dmModeReadWrite);
        if (*dbref)
        	return DmGetLastErr();
        return 0;
}

/***********************************************************************
 *
 * FUNCTION:     open_db
 * 
 * DESCRIPTION:  Open a database for EasyCalc. Creates a new one,
 *               if the database does not exist or if the old database
 *               has the wrong version
 *
 * PARAMETERS:   
 *
 * RETURN:       0 - on success
 *               other - on error
 *      
 ***********************************************************************/
Int16 open_db(const Char *name, UInt16 dbversion, UInt32 crid, UInt32 dbtype,
              DmOpenRef *dbref)
{
	Err err;
	LocalID dbid;
	UInt16 version;
	UInt32 creator;
	UInt32 type;
	UInt16 attr = 0;

	dbid = DmFindDatabase(CARDNO, name);	
	if (dbid) {  /* Database exists */
		DmDatabaseInfo(CARDNO, dbid,
		               NULL, /* name */
		               &attr, /* attrib */
		               &version, /* version */
		               NULL, /* crDate */
		               NULL, /* modDate */
		               NULL, /* bckUpDate */
		               NULL, /* modNum */
		               NULL, /* appinfoID */
		               NULL, /* sortInfoID */
		               &type, /* Type */
		               &creator); /* Creator */

		if (version == dbversion && creator == crid && type == dbtype) {
			if ((attr &  dmHdrAttrBackup) == 0) {
				attr |= dmHdrAttrBackup;
				DmSetDatabaseInfo(CARDNO, dbid,
				                  NULL, /* name */
				                  &attr, /* attrib */
				                  NULL, /* version */
				                  NULL, /* crDate */
				                  NULL, /* modDate */
				                  NULL, /* bckupDate */
				                  NULL, /* modNum */
				                  NULL, /* appinfoID */
				                  NULL, /* sortInfoID */
				                  NULL, /* Type */
				                  NULL); /* Creator */
			}
			return db_open_byid(dbid, dbref);
		}
		
		/* Database exists, but with incorrect version */
		err = DmDeleteDatabase(CARDNO, dbid);
		if (err)
			return err;
	}

	/* Database doesn't exist or old version */
	err = DmCreateDatabase(CARDNO, name, crid, dbtype, false);
	if (err)
		return err;

	dbid = DmFindDatabase(CARDNO, name);

	version = dbversion;
	attr |= dmHdrAttrBackup;
	DmSetDatabaseInfo(CARDNO, dbid,
			  NULL, /* name */
			  &attr, /* attrib */
			  &version, /* version */
			  NULL, /* crDate */
			  NULL, /* modDate */
			  NULL, /* bckUpDate */
			  NULL, /* modNum */
			  NULL, /* appinfoID */
			  NULL, /* sortInfoID */
			  NULL, /* Type */
			  NULL); /* Creator */
	return db_open_byid(dbid, dbref);
}
