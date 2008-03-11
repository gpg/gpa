/* settingsdlg.c - The GNU Privacy Assistant
   Copyright (C) 2002, Miguel Coca
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

/* Create a new settings dialog and return it.  The dialog is shown
   but not run.  */
GtkWidget *gpa_settings_dialog_new (void);

#endif
