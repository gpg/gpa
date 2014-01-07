/* keygendlg.c  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008, 2009 g10 Code GmbH.

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

#include <sys/stat.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <gpgme.h>

#include "gpa.h"
#include "icons.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "qdchkpwd.h"
#include "gpgmetools.h"
#include "keygenwizard.h"


#define STANDARD_KEY_LENGTH 2048


/* The key generation wizard.

   New users should not be overwhelmed by too many options most of which
   are not easily explained and will only confuse them. To solve that
   problem we use default values for the algorithm and size of the keys
   and we use a wizard interface to present the necessary options like
   name and email address in a step by step manner.  */

/* Helper functions.  */

/* Return a copy of string with leading and trailing whitespace
   stripped.  */
static char *
string_strip_dup (const char *string)
{
  return g_strstrip (g_strdup (string));
}


/* The wizard itself.  */

typedef struct
{
  GtkWidget *window;
  GtkWidget *wizard;
  GtkWidget *name_page;
  GtkWidget *email_page;
  GtkWidget *wait_page;
  GtkWidget *final_page;
  GtkWidget *backup_page;
  GtkWidget *backup_dir_page;

  GpaKeyGenWizardGenerateCb generate;
  gpointer generate_data;
} GPAKeyGenWizard;


/* Internal API.  */
static gboolean gpa_keygen_wizard_generate_action (gpointer data);


/* The user ID pages.  */

static GtkWidget *
gpa_keygen_wizard_simple_page (GPAKeyGenWizard *keygen_wizard,
			       const gchar *description_text,
			       const gchar *label_text)
{
  GtkWidget *align;
  guint pt, pb, pl, pr;
  GtkWidget *vbox;
  GtkWidget *description;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;

  align = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_get_padding (GTK_ALIGNMENT (align), &pt, &pb, &pl, &pr);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align),
			     pt + 5, pb + 5, pl + 5, pr + 5);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (align), vbox);

  description = gtk_label_new (description_text);
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);

  label = gtk_label_new (label_text);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 5);

  g_object_set_data (G_OBJECT (align), "gpa_keygen_entry", entry);
  g_object_set_data (G_OBJECT (align), "gpa_wizard_focus_child", entry);

  return align;
}


static gchar *
gpa_keygen_wizard_simple_get_text (GtkWidget *vbox)
{
  GtkWidget *entry;

  entry = g_object_get_data (G_OBJECT (vbox), "gpa_keygen_entry");
  return string_strip_dup ((gchar *) gtk_entry_get_text (GTK_ENTRY (entry)));
}


static void
gpa_keygen_wizard_simple_grab_focus (GtkWidget *vbox)
{
  GtkWidget *entry;

  entry = g_object_get_data (G_OBJECT (vbox), "gpa_keygen_entry");
  gtk_widget_grab_focus (entry);
}


static gboolean
name_validate_cb (GtkWidget *widget, gpointer data)
{
  GPAKeyGenWizard *wizard = data;
  const gchar *name;

  name = gtk_entry_get_text (GTK_ENTRY (widget));
  while (*name && g_unichar_isspace (g_utf8_get_char (name)))
    name = g_utf8_next_char (name);
  gtk_assistant_set_page_complete (GTK_ASSISTANT (wizard->window),
				   wizard->name_page,
                                   !gpa_validate_gpg_name (name));

  return FALSE;
}


static GtkWidget *
keygen_wizard_name_page (GPAKeyGenWizard *wizard)
{
  GtkWidget *widget;
  GtkWidget *entry;

  widget = gpa_keygen_wizard_simple_page
    (wizard,
     _("Please insert your full name.\n\n"
       "Your name will be part of the new key to make it easier for others"
       " to identify keys."),
     _("Your Name:"));

  entry = g_object_get_data (G_OBJECT (widget), "gpa_keygen_entry");
  g_signal_connect (G_OBJECT (entry), "changed",
		    G_CALLBACK (name_validate_cb), wizard);
  return widget;
}


static gboolean
email_validate_cb (GtkWidget *widget, gpointer data)
{
  GPAKeyGenWizard *wizard = data;
  const gchar *email;

  email = gtk_entry_get_text (GTK_ENTRY (widget));
  while (*email && g_unichar_isspace (g_utf8_get_char (email)))
    email = g_utf8_next_char (email);
  gtk_assistant_set_page_complete (GTK_ASSISTANT (wizard->window),
				   wizard->email_page,
                                   !gpa_validate_gpg_email (email));

  return FALSE;
}


static GtkWidget *
keygen_wizard_email_page (GPAKeyGenWizard *wizard)
{
  GtkWidget *widget;
  GtkWidget *entry;

  widget = gpa_keygen_wizard_simple_page
    (wizard,
     _("Please insert your email address.\n\n"
       "Your email address will be part of the new key to make it easier"
       " for others to identify keys. If you have several email addresses,"
       " you can add further email addresses later."),
     _("Your Email Address:"));

  entry = g_object_get_data (G_OBJECT (widget), "gpa_keygen_entry");
  g_signal_connect (G_OBJECT (entry), "changed",
		    G_CALLBACK (email_validate_cb), wizard);
  return widget;
}

/* Backup copies and revocation certificate.  */
static GtkWidget *
gpa_keygen_wizard_backup_page (GPAKeyGenWizard *wizard)
{
  GtkWidget *vbox;
  GtkWidget *description;
  GtkWidget *radio;

  vbox = gtk_vbox_new (FALSE, 0);

  description = gtk_label_new
    (_("It is recommended that you create a backup copy of your new key,"
       " once it has been generated.\n\n"
       "Do you want to create a backup copy?"));
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  radio = gtk_radio_button_new_with_mnemonic (NULL, _("Create _backup copy"));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, TRUE, 5);
  g_object_set_data (G_OBJECT (vbox), "gpa_keygen_backup", radio);

  radio = gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radio), _("Do it _later"));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, TRUE, 5);

  return vbox;
}


static GtkWidget *
gpa_keygen_wizard_message_page (const gchar *description_text)
{
  GtkWidget *vbox;
  GtkWidget *description;

  vbox = gtk_vbox_new (FALSE, 0);

  description = gtk_label_new (description_text);
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  return vbox;
}


static GtkWidget *
gpa_keygen_wizard_wait_page (GPAKeyGenWizard *wizard)
{
  return gpa_keygen_wizard_message_page
    (_("Your key is being generated.\n\n"
       "Even on fast computers this may take a while. Please be patient."));
}


static GtkWidget *
gpa_keygen_wizard_final_page (GPAKeyGenWizard * keygen_wizard)
{
  GtkWidget *widget;
  char *desc;

  desc = g_strdup_printf
    (_("Congratulations!\n\n"
       "You have successfully generated a key."
       " The key is indefinitely valid and has a length of %d bits."),
     STANDARD_KEY_LENGTH);
  widget = gpa_keygen_wizard_message_page (desc);
  g_free (desc);
  return widget;
}


/* Extract the values the user entered and call gpa_generate_key.
   Return TRUE if the key was created successfully.  */
static gboolean
gpa_keygen_wizard_generate_action (gpointer data)
//GtkAssistant *assistant, GtkWidget *page, gpointer data)
{
  GPAKeyGenWizard *wizard = data;
  gpa_keygen_para_t *para;
  gboolean do_backup;
  GtkWidget *radio;

  para = gpa_keygen_para_new ();

  /* Shall we make backups?  */
  radio = g_object_get_data (G_OBJECT (wizard->backup_page),
			     "gpa_keygen_backup");
  do_backup = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio));

  /* The User ID.  */
  para->name = gpa_keygen_wizard_simple_get_text (wizard->name_page);
  para->email = gpa_keygen_wizard_simple_get_text (wizard->email_page);

  /* Default values for newbie mode.  */
  para->algo = GPA_KEYGEN_ALGO_RSA_RSA;
  para->keysize = STANDARD_KEY_LENGTH;

  wizard->generate (para, do_backup, wizard->generate_data);

  gpa_keygen_para_free (para);

  return FALSE;
}



/* Handler for the close button. Destroy the main window */
static void
keygen_wizard_close (GtkWidget *widget, gpointer param)
{
  GPAKeyGenWizard *wizard = param;

  gtk_widget_destroy (wizard->window);
}


static void
free_keygen_wizard (gpointer data)
{
  GPAKeyGenWizard *keygen_wizard = data;

  g_free (keygen_wizard);
}


void
keygen_wizard_prepare_cb (GtkAssistant *assistant, GtkWidget *page,
			  gpointer data)
{
  GPAKeyGenWizard *wizard = data;

  if (page == wizard->name_page || page == wizard->email_page)
    gpa_keygen_wizard_simple_grab_focus (page);
  else if (page == wizard->wait_page)
    gpa_keygen_wizard_generate_action (wizard);
}


GtkWidget *
gpa_keygen_wizard_new (GtkWidget *parent,
		       GpaKeyGenWizardGenerateCb generate_action,
		       gpointer data)
{
  GPAKeyGenWizard *wizard;
  GtkWidget *window;
  GdkPixbuf *genkey_pixbuf;
  GdkPixbuf *backup_pixbuf;


  wizard = g_malloc (sizeof (*wizard));
  genkey_pixbuf = gpa_create_icon_pixbuf ("wizard_genkey");
  backup_pixbuf = gpa_create_icon_pixbuf ("wizard_backup");

  wizard->generate = generate_action;
  wizard->generate_data = data;


  window = gtk_assistant_new ();
  wizard->window = window;
  gpa_window_set_title (GTK_WINDOW (window), _("Generate key"));
  g_object_set_data_full (G_OBJECT (window), "user_data",
			  wizard, free_keygen_wizard);

  /* Set the forward button to be the default.  */
  GTK_WIDGET_SET_FLAGS (GTK_ASSISTANT (window)->forward, GTK_CAN_DEFAULT);
  gtk_window_set_default (GTK_WINDOW (window), GTK_ASSISTANT (window)->forward);

  wizard->name_page = keygen_wizard_name_page (wizard);
  gtk_assistant_append_page (GTK_ASSISTANT (window), wizard->name_page);
  gtk_assistant_set_page_type (GTK_ASSISTANT (window), wizard->name_page,
			       GTK_ASSISTANT_PAGE_CONTENT);
  gtk_assistant_set_page_title (GTK_ASSISTANT (window), wizard->name_page,
				/* FIXME */ _("Generate key"));
  gtk_assistant_set_page_side_image (GTK_ASSISTANT (window), wizard->name_page,
				     genkey_pixbuf);


  wizard->email_page = keygen_wizard_email_page (wizard);
  gtk_assistant_append_page (GTK_ASSISTANT (window), wizard->email_page);
  gtk_assistant_set_page_type (GTK_ASSISTANT (window), wizard->email_page,
			       GTK_ASSISTANT_PAGE_CONTENT);
  gtk_assistant_set_page_title (GTK_ASSISTANT (window), wizard->email_page,
				/* FIXME */ _("Generate key"));
  gtk_assistant_set_page_side_image (GTK_ASSISTANT (window), wizard->email_page,
				     genkey_pixbuf);

  /* FIXME: A better GUI would have a "Generate backup" button on the
     finish page after the key was generated.  */
  wizard->backup_page = gpa_keygen_wizard_backup_page (wizard);
  gtk_assistant_append_page (GTK_ASSISTANT (window), wizard->backup_page);
  gtk_assistant_set_page_type (GTK_ASSISTANT (window), wizard->backup_page,
			       GTK_ASSISTANT_PAGE_CONTENT);
  gtk_assistant_set_page_title (GTK_ASSISTANT (window), wizard->backup_page,
				/* FIXME */ _("Generate key"));
  gtk_assistant_set_page_side_image (GTK_ASSISTANT (window),
				     wizard->backup_page,
				     backup_pixbuf);
  gtk_assistant_set_page_complete (GTK_ASSISTANT (wizard->window),
				   wizard->backup_page, TRUE);


  /* FIXME: We need to integrate the progress bar for the operation
     into the page.  Also, after the operation completes, the
     assistant is destroyed, so the final page will never be
     shown.  The whole thing is upside down, and should be redone.  */
  wizard->wait_page = gpa_keygen_wizard_wait_page (wizard);
  gtk_assistant_append_page (GTK_ASSISTANT (window), wizard->wait_page);
  gtk_assistant_set_page_type (GTK_ASSISTANT (window), wizard->wait_page,
			       GTK_ASSISTANT_PAGE_PROGRESS);
  gtk_assistant_set_page_title (GTK_ASSISTANT (window), wizard->wait_page,
				/* FIXME */ _("Generate key"));
  gtk_assistant_set_page_side_image (GTK_ASSISTANT (window), wizard->wait_page,
				     genkey_pixbuf);

  /* The final page does not contain information about the generated
     key.  It should also offer a "generate backup" button, then the
     backup page can be removed.  */
  wizard->final_page = gpa_keygen_wizard_final_page (wizard);
  gtk_assistant_append_page (GTK_ASSISTANT (window), wizard->final_page);
  gtk_assistant_set_page_type (GTK_ASSISTANT (window), wizard->final_page,
			       GTK_ASSISTANT_PAGE_SUMMARY);
  gtk_assistant_set_page_title (GTK_ASSISTANT (window), wizard->final_page,
				/* FIXME */ _("Generate key"));
  gtk_assistant_set_page_side_image (GTK_ASSISTANT (window), wizard->final_page,
				     genkey_pixbuf);

  g_signal_connect (G_OBJECT (window), "prepare",
		    G_CALLBACK (keygen_wizard_prepare_cb), wizard);
  g_signal_connect (G_OBJECT (window), "close",
		    G_CALLBACK (keygen_wizard_close), wizard);
  g_signal_connect (G_OBJECT (window), "cancel",
		    G_CALLBACK (keygen_wizard_close), wizard);


  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);

  g_object_unref (genkey_pixbuf);
  g_object_unref (backup_pixbuf);

  return window;
}
