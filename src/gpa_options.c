/* gpa_options.c  -  The GNU Privacy Assistant
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

void options_keyserver ( void ) {
g_print ( "Set Keyserver\n" ); /*!!!*/
} /* options_keyserver */

void options_recipients ( void ) {
g_print ( "Set Default Recipients\n" ); /*!!!*/
} /* options_recipients */

void options_key ( void ) {
g_print ( "Set Default Key\n" ); /*!!!*/
} /* options_key */

void options_homedir ( void ) {
g_print ( "Set Home Directory\n" ); /*!!!*/
} /* options_homedir */

void options_load ( void ) {
g_print ( "Load Options File\n" ); /*!!!*/
} /* options_load */

void options_save ( void ) {
g_print ( "Save Options File\n" ); /*!!!*/
} /* options_save */
