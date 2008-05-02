/* settingsdlg.c - The GNU Privacy Assistant
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


/* Definitions to define the object.  */
#define SETTINGS_DLG_TYPE \
          (settings_dlg_get_type ())

#define SETTINGS_DLG(obj) \
          (G_TYPE_CHECK_INSTANCE_CAST \
            ((obj), SETTINGS_DLG_TYPE,\
              SettingsDlg))

#define SETTINGS_DLG_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_CAST \
            ((klass), SETTINGS_DLG_TYPE, \
              SettingsDlgClass))

#define IS_SETTINGS_DLG(obj) \
          (G_TYPE_CHECK_INSTANCE_TYPE \
            ((obj), SETTINGS_DLG_TYPE))

#define IS_SETTINGS_DLG_CLASS(klass) \
          (G_TYPE_CHECK_CLASS_TYPE \
            ((klass), SETTINGS_DLG_TYPE))

#define SETTINGS_DLG_GET_CLASS(obj) \
          (G_TYPE_INSTANCE_GET_CLASS \
            ((obj), SETTINGS_DLG_TYPE, \
              SettingsDlgClass))

typedef struct _SettingsDlg SettingsDlg;
typedef struct _SettingsDlgClass SettingsDlgClass;


GType settings_dlg_get_type (void) G_GNUC_CONST;


/************************************
 ************ Public API ************
 ************************************/

/* Create and show the settings dialog.  */
void settings_dlg_new (GtkWidget *parent);

/* Tell whether the settings dialog is open.  */
gboolean settings_is_open (void);


#endif /*SETTINGSDLG_H*/
