/* keygendlg.c  -  The GNU Privacy Assistant
   Copyright (C) 2001 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH.
   This file is part of GPA

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


/*
 * A simple general purpose Wizard implementation
 */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawizard.h"

typedef struct
{
  GtkWidget *notebook;
  GtkWidget *prev_button;
  GtkWidget *next_button;
  GtkWidget *finish_button;
  GtkWidget *cancel_button;
  GtkWidget *close_button;
  GtkAccelGroup *accel_group;
  GPAWizardPageSwitchedFunc page_switched;
  gpointer page_switched_data;
}
GPAWizard;

typedef struct
{
  gboolean is_last;
  GPAWizardAction action;
  gpointer user_data;
}
GPAWizardPage;

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
  return g_object_get_data (G_OBJECT (page_widget), "gpa_wizard_page");
}

/* Update the buttons of the wizard depending on the current page */
void
gpa_wizard_update_buttons (GtkWidget * widget)
{
  GPAWizard * wizard = g_object_get_data (G_OBJECT (widget), "user_data");
  GPAWizardPage * page;
  int page_number;

  page_number = gtk_notebook_get_current_page (GTK_NOTEBOOK(wizard->notebook));
  if (page_number < 0)
      page_number = 0;

  page = gpa_wizard_get_current_page (wizard);

  /* Choose whether to show "Next" or "Finish" button based on the data
   * provided */
  if (page->is_last) 
    {
      gtk_widget_hide (wizard->next_button);
      gtk_widget_show (wizard->finish_button);
    }
  else
    {
      gtk_widget_show (wizard->next_button);
      gtk_widget_hide (wizard->finish_button);
    }

  /* if we're at the last page, assume that whatever the wizard was
   * supposed to do has been achieved. Therefore make both prev and next
   * button insensitive, hide the cancel button and show the close one.
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
      gtk_widget_set_sensitive (wizard->finish_button, FALSE);
      gtk_widget_set_sensitive (wizard->close_button, TRUE);
      gtk_widget_set_sensitive (wizard->cancel_button, TRUE);
      gtk_widget_show (wizard->close_button);
      gtk_widget_hide (wizard->cancel_button);
    }
  else
    {
      gtk_widget_set_sensitive (wizard->prev_button, TRUE);
      gtk_widget_set_sensitive (wizard->next_button, TRUE);
      gtk_widget_set_sensitive (wizard->finish_button, TRUE);
      gtk_widget_set_sensitive (wizard->close_button, TRUE);
      gtk_widget_set_sensitive (wizard->cancel_button, TRUE);
      gtk_widget_hide (wizard->close_button);
      gtk_widget_show (wizard->cancel_button);
    }

  /* if we're at the first page, make the prev-button insensitive. */
  if (page_number == 0)
    {
      gtk_widget_set_sensitive (wizard->prev_button, FALSE);
    }

  /* FIXME: If we are on the "wait" page, disable all buttons.
   */
  if (gtk_notebook_get_nth_page (GTK_NOTEBOOK (wizard->notebook),
				  page_number + 1) &&
      !gtk_notebook_get_nth_page (GTK_NOTEBOOK (wizard->notebook),
                                  page_number + 2))
    {
      gtk_widget_set_sensitive (wizard->prev_button, FALSE);
      gtk_widget_set_sensitive (wizard->next_button, FALSE);
      gtk_widget_set_sensitive (wizard->finish_button, FALSE);
      gtk_widget_set_sensitive (wizard->close_button, FALSE);
      gtk_widget_set_sensitive (wizard->cancel_button, FALSE);
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
  gpa_wizard_next_page ((GtkWidget *)data);
}

static void
gpa_wizard_page_switched (GtkWidget *notebook, GtkNotebookPage *page,
			  gint page_num, gpointer user_data)
{
  GtkWidget * page_widget;
  int page_number;
  GtkWidget * focus;
  GtkWidget * main_widget = user_data;
  GPAWizard * wizard = g_object_get_data (G_OBJECT (main_widget), "user_data");

  /* Switch-page is emitted also when pages are added to the notebook,
     even when it's not even displayed yet.  In that case the page
     number is < 0, so we simply ignore that case and only try to set
     the focus if the page number is >= 0.  */
  page_number = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
  if (page_number >= 0)
    {
      gpa_wizard_update_buttons ((GtkWidget *)user_data);
      page_widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK(notebook),
					       page_number);
      focus = g_object_get_data (G_OBJECT (page_widget),
				 "gpa_wizard_focus_child");
      if (focus)
	{
	  gtk_widget_grab_focus (focus);
	}

      /* Call the page switched callback */
      /* FIXME: this should really be a proper GTK signal */
      if (wizard->page_switched)
	{
	  wizard->page_switched (page_widget, wizard->page_switched_data);
	}
    }
}


/* Handler for the notebook's destroy signal. Remove the page_switch
 * callback. For whatever reason, the notebook would emit page_switch
 * signals during the destroy which could cause segfaults in the
 * callback */
static void
gpa_wizard_notebook_destroy (GtkWidget * widget, gpointer param)
{
  GPAWizard * wizard = param;

  wizard->page_switched = NULL;
  wizard->page_switched_data = NULL;
}


/* Create a new GPA Wizard.  */
GtkWidget *
gpa_wizard_new (GtkAccelGroup *accel_group,
		GtkSignalFunc close_func, gpointer close_data)
{
  GtkWidget *vbox;
  GtkWidget *notebook;
  GtkWidget *hbox;
  GtkWidget *button_box;
  GtkWidget *button;

  GPAWizard *wizard = g_malloc (sizeof (*wizard));
  wizard->accel_group = accel_group;
  wizard->page_switched = NULL;
  wizard->page_switched_data = NULL;

  vbox = gtk_vbox_new (FALSE, 3);
  g_object_set_data_full (G_OBJECT (vbox), "user_data", wizard, g_free);

  notebook = gtk_notebook_new ();
  wizard->notebook = notebook;
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 5);
  /* use *_connect_after so that the callback is called after the
     current page number has been updated so that
     gpa_wizard_update_buttons uses the new page.  */
  g_signal_connect_after (G_OBJECT (notebook), "switch-page",
			  G_CALLBACK (gpa_wizard_page_switched), vbox);
  g_signal_connect (G_OBJECT (notebook), "destroy",
		    G_CALLBACK (gpa_wizard_notebook_destroy), wizard);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  
  button_box = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button_box, TRUE, TRUE, 5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (button_box), 10);

  button = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
  wizard->prev_button = button;
  gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (gpa_wizard_prev), wizard);
  
  button = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  wizard->next_button = button;
  gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (gpa_wizard_next), vbox);

  button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  wizard->finish_button = button;
  gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (gpa_wizard_next), vbox);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  wizard->close_button = button;
  gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (close_func), close_data);
  gtk_widget_add_accelerator (button, "clicked", accel_group, GDK_Escape,
			      0, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  wizard->cancel_button = button;
  gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (close_func), close_data);
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
			gboolean is_last,
			GPAWizardAction action, gpointer user_data)
{
  GPAWizard * wizard = g_object_get_data (G_OBJECT (widget), "user_data");
  GPAWizardPage * page = g_malloc (sizeof (*page));

  page->is_last = is_last;
  page->action = action;
  page->user_data = user_data;
  
  g_object_set_data_full (G_OBJECT (page_widget), "gpa_wizard_page",
			  page, g_free);
  gtk_notebook_append_page (GTK_NOTEBOOK (wizard->notebook), page_widget,
			    NULL);
}


/* Turn to the next page of the wizard and run the page action.
 */
void
gpa_wizard_next_page (GtkWidget * widget)
{
  GPAWizard *wizard = g_object_get_data (G_OBJECT (widget), "user_data");
  GPAWizardPage *page;
  
  page = gpa_wizard_get_current_page (wizard);
  if (page->action)
    {
      if (!page->action(page->user_data))
	return;
    }
  gtk_notebook_next_page (GTK_NOTEBOOK (wizard->notebook));
}


/* Turn to the next page of the wizard and don't run the page action.
   This is used e.g. in keygenwizard.c by the action callback that is
   invoked by the finish button to display a "wait" message.  */
void
gpa_wizard_next_page_no_action (GtkWidget *widget)
{
  GPAWizard *wizard = g_object_get_data (G_OBJECT (widget), "user_data");
  gtk_notebook_next_page (GTK_NOTEBOOK (wizard->notebook));
}

/***/
void gpa_wizard_set_page_switched (GtkWidget *widget,
				   GPAWizardPageSwitchedFunc page_switched,
				   gpointer param)
{
  GPAWizard *wizard = g_object_get_data (G_OBJECT (widget), "user_data");
  wizard->page_switched = page_switched;
  wizard->page_switched_data = param;
}
