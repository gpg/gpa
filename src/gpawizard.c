/* keygendlg.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2001 G-N-U GmbH.
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

/*
 * A simple general purpose Wizard implementation
 */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawizard.h"

typedef struct {
  GtkWidget * notebook;
  GtkWidget * prev_button;
  GtkWidget * next_button;
  GtkWidget * close_button;
  GtkAccelGroup * accel_group;
} GPAWizard;

typedef struct {
  gchar * prev_label;
  gchar * next_label;
  GPAWizardAction action;
  gpointer user_data;
} GPAWizardPage;


/* Return the page data associated with the current page of the wizard */
static GPAWizardPage *
gpa_wizard_get_current_page (GPAWizard * wizard)
{
  GtkWidget * page_widget;
  int page_number;

  page_number = gtk_notebook_get_current_page (GTK_NOTEBOOK(wizard->notebook));
  if (page_number < 0)
      page_number = 0;
  page_widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK(wizard->notebook),
					   page_number);
  return gtk_object_get_data (GTK_OBJECT (page_widget), "gpa_wizard_page");
}

/* Update the buttons of the wizard depending on the current page */
static void
gpa_wizard_update_buttons (GtkWidget * widget)
{
  GPAWizard * wizard = gtk_object_get_data (GTK_OBJECT (widget), "user_data");
  GPAWizardPage * page;
  int page_number;

  page_number = gtk_notebook_get_current_page (GTK_NOTEBOOK(wizard->notebook));
  if (page_number < 0)
      page_number = 0;

  page = gpa_wizard_get_current_page (wizard);

  gpa_button_set_text (wizard->prev_button, page->prev_label,
		       wizard->accel_group);
  gpa_button_set_text (wizard->next_button, page->next_label,
		       wizard->accel_group);

  /* if we're at the first page, make the prev-button insensitive. */
  if (page_number == 0)
    {
      gtk_widget_set_sensitive (wizard->prev_button, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (wizard->prev_button, TRUE);
    }
  /* if we're at the last page, assume that whatever the wizard was
   * supposed to do has been achieved. Therefore make both prev and next
   * button insensitive and change the label of the cancel button to
   * "close"
   */
  /* There doesn't seem to be a simple way to get the number of pages in
   * a notebook, so try to get the next page and if that is NULL, we're
   * at the last page.
   */
  if (!gtk_notebook_get_nth_page (GTK_NOTEBOOK (wizard->notebook),
				  page_number + 1))
    {
      gtk_widget_set_sensitive (wizard->prev_button, FALSE);
      gtk_widget_set_sensitive (wizard->next_button, FALSE);
      gpa_button_set_text (wizard->close_button, _("_Close"),
			   wizard->accel_group);
    }
}

static void
gpa_wizard_prev (GtkWidget * button, gpointer data)
{
  GPAWizard * wizard = data;
  gtk_notebook_prev_page (GTK_NOTEBOOK (wizard->notebook));
}

static void
gpa_wizard_next (GtkWidget * button, gpointer data)
{
  GPAWizard * wizard = data;
  GPAWizardPage * page;
  
  page = gpa_wizard_get_current_page (wizard);
  if (page->action)
    {
      if (!page->action(page->user_data))
	return;
    }
  gtk_notebook_next_page (GTK_NOTEBOOK (wizard->notebook));
}

static void
gpa_wizard_page_switched (GtkWidget *notebook, GtkNotebookPage *page,
			  gint page_num, gpointer user_data)
{
  GtkWidget * page_widget;
  int page_number;
  GtkWidget * focus;

  /* switch-page is emitted also when pages are added to the notebook,
   * even when it's not even displayed yet. In that case the page number
   * is < 0, so we simply ignore that case and only try to set the focus
   * if the page number is >= 0.
   */
  page_number = gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook));
  if (page_number >= 0)
    {
      gpa_wizard_update_buttons ((GtkWidget *)user_data);
      page_widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK(notebook),
					       page_number);
      focus = gtk_object_get_data (GTK_OBJECT (page_widget),
				   "gpa_wizard_focus_child");
      if (focus)
	{
	  gtk_widget_grab_focus (focus);
	}
    }
}

GtkWidget *
gpa_wizard_new (GtkAccelGroup * accel_group,
		GtkSignalFunc close_func, gpointer close_data)
{
  GtkWidget * vbox;
  GtkWidget * notebook;
  GtkWidget * hbox;
  GtkWidget * button_box;
  GtkWidget * button;

  GPAWizard * wizard = xmalloc (sizeof (*wizard));
  wizard->accel_group = accel_group;

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_object_set_data_full (GTK_OBJECT (vbox), "user_data", (gpointer)wizard,
			    free);

  notebook = gtk_notebook_new ();
  wizard->notebook = notebook;
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 5);
  /* use *_connect_after so that the callback is called after the
   * current page number has been updated so that
   * gpa_wizard_update_buttons uses the new page */
  gtk_signal_connect_after (GTK_OBJECT (notebook), "switch-page",
			    GTK_SIGNAL_FUNC (gpa_wizard_page_switched),
			    (gpointer)vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  
  button_box = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button_box, TRUE, TRUE, 5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (button_box), 0);

  button = gtk_button_new_with_label (_("Prev"));
  wizard->prev_button = button;
  gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gpa_wizard_prev), (gpointer) wizard);
  
  button = gtk_button_new_with_label (_("Next"));
  wizard->next_button = button;
  gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gpa_wizard_next), (gpointer) wizard);

  button = gpa_button_new (wizard->accel_group, _("_Cancel"));
  wizard->close_button = button;
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", close_func, close_data);
  gtk_widget_add_accelerator (button, "clicked", accel_group, GDK_Escape,
			      0, 0);

  return vbox;
}

/* Append page_widget as a new page to the wizard. prev_label and
 * next_label are the labels to be used for the prev- and next button.
 * If they're NULL, the default values "Prev" and "Next" will be used.
 * action is a callback to be called when the user clicks the next
 * button. user_data is passed through to the callback. If the callback
 * returns FALSE, the wizard does not switch to the next page, other
 * wise it does. If action is NULL, assume a noop action that returns
 * TRUE.
 */
void
gpa_wizard_append_page (GtkWidget * widget, GtkWidget * page_widget,
			gchar * prev_label, gchar * next_label,
			GPAWizardAction action, gpointer user_data)
{
  GPAWizard * wizard = gtk_object_get_data (GTK_OBJECT (widget), "user_data");
  GPAWizardPage * page = xmalloc (sizeof (*page));

  if (!prev_label)
    prev_label = _("_Prev");
  if (!next_label)
    next_label = _("_Next");

  page->prev_label = prev_label;
  page->next_label = next_label;
  page->action = action;
  page->user_data = user_data;
  
  gtk_object_set_data_full (GTK_OBJECT (page_widget), "gpa_wizard_page",
			    (gpointer)page, free);
  gtk_notebook_append_page (GTK_NOTEBOOK (wizard->notebook), page_widget,
			    NULL);
}


/* Turn to the next page of the wizard and don't run the page action.
 * This is used e.g. in keygenwizard.c by the action callback that is
 * invoked by the finish button to display a "wait" message.
 */
void
gpa_wizard_next_page_no_action (GtkWidget * widget)
{
  GPAWizard * wizard = gtk_object_get_data (GTK_OBJECT (widget), "user_data");
  gtk_notebook_next_page (GTK_NOTEBOOK (wizard->notebook));
}
