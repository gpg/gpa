/* gpa.h  -  main header
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
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

#ifndef GPA_H
#define GPA_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gpapa.h>
#include "xmalloc.h"
#include "logging.h"

#include "i18n.h"   /* fixme: file should go into each source file */
#include "options.h" /* ditto */


extern GtkWidget *global_windowMain;
extern GtkWidget *global_windowTip;
extern GpapaAction global_lastCallbackResult;
extern GList *global_defaultRecipients;

extern GtkWidget *gpa_get_global_clist_file (void);
extern void gpa_callback (GpapaAction action, gpointer actiondata,
			  gpointer calldata);
extern void sigs_append (gpointer data, gpointer userData);
extern void gpa_selectRecipient (GtkWidget * clist, gint row, gint column,
				 GdkEventButton * event, gpointer userData);
extern void gpa_unselectRecipient (GtkWidget * clist, gint row, gint column,
				   GdkEventButton * event, gpointer userData);
extern void gpa_removeRecipients (gpointer param);
extern void gpa_addRecipients (gpointer param);
extern void gpa_recipientWindow_close (gpointer param);

gboolean gpa_simplified_ui (void);
void gpa_set_simplified_ui (gboolean value);

gchar * gpa_default_key (void);
void gpa_set_default_key (gchar * key);
void gpa_update_default_key (void);

void gpa_open_keyring_editor (void);
void gpa_open_filemanager (void);
GtkWidget * gpa_get_keyring_editor (void);
GtkWidget * gpa_get_filenamager (void);

typedef void (*GPADefaultKeyChanged) (gpointer user_data);


#endif /*GPA_H */

