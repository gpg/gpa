/* keyring.h  -  The GNU Privacy Assistant
   Copyright (C) 2000 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  */

#ifndef KEYRINGEDITOR_H
#define KEYRINGEDITOR_H

/* Create and return a new keyring editor dialog.  The dialog is
   hidden by default.  */
GtkWidget *keyring_editor_new (void);

#endif /* KEYRINGEDITOR_H */
