/* gpawizard.h  -  The GNU Privacy Assistant
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

#ifndef GPAWIZARD_H
#define GPAWIZARD_H

#include <gtk/gtk.h>

typedef gboolean (*GPAWizardAction)(gpointer user_data);

GtkWidget * gpa_wizard_new (GtkAccelGroup * accel_group,
			    GtkSignalFunc close_func, gpointer close_data);
void gpa_wizard_append_page (GtkWidget * widget, GtkWidget * page_widget,
			     gchar * back_label, gchar * next_label,
			     GPAWizardAction action, gpointer user_data);

void gpa_wizard_next_page (GtkWidget * widget);

void gpa_wizard_next_page_no_action (GtkWidget * widget);

typedef void (*GPAWizardPageSwitchedFunc)(GtkWidget * page_widget,
					   gpointer param);
void gpa_wizard_set_page_switched (GtkWidget * widget,
				   GPAWizardPageSwitchedFunc,
				   gpointer param);

#endif /* GPAWIZARD_H */
