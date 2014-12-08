/* confdialog.c - The GNU Privacy Assistant
 * Copyright (C) 2008 g10 Code GmbH
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

#ifndef CONFDIALOG_H
#define CONFDIALOG_H

GtkWidget *gpa_backend_config_dialog_new (void);


char *gpa_load_gpgconf_string (const char *cname, const char *name);
void gpa_store_gpgconf_string (const char *cname,
                               const char *name, const char *value);



char *gpa_load_configured_keyserver (void);
void gpa_store_configured_keyserver (const char *value);
char *gpa_configure_keyserver (GtkWidget *parent);


#endif
