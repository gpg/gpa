/* filemenu.h  -  The GNU Privacy Assistant
 *      Copyright (C) 2000 Free Software Foundation, Inc.
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

extern GList *filesOpened;
extern GList *filesSelected;

extern void file_open ( void );
extern void file_showDetail ( void );
extern void file_sign ( void );
extern void file_encrypt ( void );
extern void file_encryptAs ( void );
extern void file_protect ( void );
extern void file_protectAs ( void );
extern void file_decrypt ( void );
extern void file_decryptAs ( void );
extern void file_close ( void );
extern void file_quit ( void );

extern void file_browse ( gpointer param );
extern void gpa_fileOpenSelect_init ( char *title );
