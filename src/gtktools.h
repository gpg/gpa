/* gtktools.h  -  The GNU Privacy Assistant
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

#include <gtk/gtk.h>

extern void gpa_window_destroy (gpointer param);
extern GtkWidget *gpa_space_new (void);
extern GtkWidget *gpa_widget_hjustified_new (GtkWidget * widget,
					     GtkJustification jtype);
extern GtkWidget *gpa_button_new (GtkAccelGroup * accelGroup,
				  gchar * labelText);
extern GtkWidget *gpa_xpm_label_box( GtkWidget *parent,
					gchar     **xpm,
					gchar     *label_text,
					GtkWidget * button,
					GtkAccelGroup * accelGroup);
extern GtkWidget *gpa_buttonCancel_new (GtkAccelGroup * accelGroup,
					gchar * labelText, gpointer * param);
extern GtkWidget *gpa_check_button_new (GtkAccelGroup * accelGroup,
					gchar * labelText);
extern GtkWidget *gpa_radio_button_new (GtkAccelGroup * accelGroup,
					gchar * labelText);
extern GtkWidget *gpa_radio_button_new_from_widget (GtkRadioButton * widget,
						    GtkAccelGroup *
						    accelGroup,
						    gchar * labelText);
extern GtkWidget *gpa_toggle_button_new (GtkAccelGroup * accelGroup,
					 gchar * labelText);
extern void gpa_connect_by_accelerator (GtkLabel * label, GtkWidget * widget,
					GtkAccelGroup * accelGroup,
					gchar * labelText);
extern void gpa_widget_set_centered (GtkWidget * widget, GtkWidget * parent);
extern void gpa_widget_show (GtkWidget * widget, GtkWidget * parent,
			     gchar * tip);
extern void gpa_window_error (gchar * message, GtkWidget * messenger);
extern void gpa_window_message (gchar * message, GtkWidget * messenger);
extern void gpa_window_passphrase (GtkWidget * messenger, GtkSignalFunc func,
				   gchar * tip, gpointer data);
