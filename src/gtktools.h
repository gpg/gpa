/* gtktools.h  -  The GNU Privacy Assistant
 *      Copyright (C) 2000, 2001 G-N-U GmbH.
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

#ifndef GTK_TOOLS_H_
#define GTK_TOOLS_H_

#include <gtk/gtk.h>

extern void gpa_window_destroy (gpointer param);
extern GtkWidget *gpa_space_new (void);
extern GtkWidget *gpa_widget_hjustified_new (GtkWidget * widget,
					     GtkJustification jtype);
extern GtkWidget *gpa_button_new (GtkAccelGroup * accelGroup,
				  gchar * labelText);
extern GtkWidget *gpa_xpm_label_box( GtkWidget *parent,
				     const char *xpm_name,
                                     gchar     *label_text,
                                     GtkWidget * button,
                                     GtkAccelGroup * accelGroup);
extern GtkWidget *gpa_buttonCancel_new (GtkAccelGroup * accelGroup,
					gchar * labelText, gpointer * param);
extern GtkWidget *gpa_button_cancel_new (GtkAccelGroup * accel, gchar * label,
					 GtkSignalFunc func, gpointer param);
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
void gpa_button_set_text (GtkWidget * button, gchar * text, GtkAccelGroup *);
extern void gpa_window_show_centered (GtkWidget * widget, GtkWidget * parent);
extern void gpa_window_error (const gchar * message, GtkWidget * messenger);
extern void gpa_window_message (gchar * message, GtkWidget * messenger);
gchar * gpa_message_box_run (GtkWidget * parent, const gchar * title,
			     const gchar * message,
			     const gchar ** buttons);
extern void gpa_window_passphrase (GtkWidget * messenger, GtkSignalFunc func,
				   gchar * tip, gpointer data);
gchar * gpa_get_save_file_name (GtkWidget * parent, const gchar * title,
				const gchar * directory);
gchar * gpa_get_load_file_name (GtkWidget * parent, const gchar * title,
				const gchar * directory);
#endif /* GTK_TOOLS_H_ */
