/* gpa_keys.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000 Free Software Foundation, Inc.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <gtk/gtk.h>

void keys_openPublic ( void ) {
g_print ( "Open public keyring\n" ); /*!!!*/
} /* keys_openPublic */

void keys_openSecret ( void ) {
g_print ( "Open secret keyring\n" ); /*!!!*/
} /* keys_openSecret */

void keys_open ( void ) {
g_print ( "Open keyring\n" ); /*!!!*/
} /* keys_open */

void keys_generateKey ( void ) {
g_print ( "Generate Key\n" ); /*!!!*/
} /* keys_generateKey */

void keys_generateRevocation ( void ) {
g_print ( "Generate Revocation Certificate\n" ); /*!!!*/
} /* keys_generateRevocation */

void keys_import ( void ) {
g_print ( "Import Keys\n" ); /*!!!*/
} /* keys_import */

void keys_importOwnertrust ( void ) {
g_print ( "Import Ownertrust\n" ); /*!!!*/
} /* keys_importOwnertrust */

void keys_updateTrust ( void ) {
g_print ( "Update Trust Database\n" ); /*!!!*/
} /* keys_updateTrust */

