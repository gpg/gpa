/* keygendlg.c  -  The GNU Privacy Assistant
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

#include <gtk/gtk.h>
#include "gpa.h"
#include "gpapastrings.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "gpawizard.h"


/*
 * The key generation wizard
 *
 * New users should not be overwhelmed by too many options most of which
 * are not easily explained and will only confuse them. To solve that
 * problem we use default values for the algorithm and size of the keys
 * and we use a wizard interface to present the necessary options like
 * user id and password in a step by step manner.
 */


/*
 * Helper functions
 */
 
/* Return a copy of string with leading and trailing whitespace stripped */
static gchar *
string_strip_dup (gchar * string)
{
  return g_strstrip (xstrdup (string));
}


/*
 * The wizard itself
 */



typedef struct {
  GtkWidget * window;
  GtkWidget * wizard;
  GtkWidget * name_page;
  GtkWidget * email_page;
  GtkWidget * comment_page;
  GtkWidget * passwd_page;
  GtkWidget * wait_page;
  GtkWidget * final_page;
  gboolean successful;
} GPAKeyGenWizard;

/*
 * The user ID pages
 */

static GtkWidget *
gpa_keygen_wizard_simple_page (const gchar * description_text,
			       const gchar * label_text)
{
  GtkWidget * vbox;
  GtkWidget * description;
  GtkWidget * hbox;
  GtkWidget * label;
  GtkWidget * entry;

  vbox = gtk_vbox_new (FALSE, 0);

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
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 5);

  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_entry", entry);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_wizard_focus_child", entry);
  return vbox;
}

static gchar *
gpa_keygen_wizard_simple_get_text (GtkWidget * vbox)
{
  GtkWidget * entry;

  entry = gtk_object_get_data (GTK_OBJECT (vbox), "gpa_keygen_entry");
  return string_strip_dup (gtk_entry_get_text (GTK_ENTRY (entry)));
}


static GtkWidget *
gpa_keygen_wizard_name_page (void)
{
  return gpa_keygen_wizard_simple_page
    (_("Please insert your full name."
       "\n\n"
       "Your name will be part of the new key"
       " to make it easier for others to identify"
       " keys."),
     _("Your Name:"));
}

static gboolean
gpa_keygen_wizard_name_validate (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  gboolean result = TRUE;
  gchar * name = gpa_keygen_wizard_simple_get_text (keygen_wizard->name_page);

  if (!*name)
    {
      /* The string is empty or consists entirely of whitespace */
      gpa_window_error (_("Please insert your name"), keygen_wizard->window);
      result = FALSE;
    }

  free (name);
  return result;
}

static GtkWidget *
gpa_keygen_wizard_email_page (void)
{
  return gpa_keygen_wizard_simple_page
    (_("Please insert your email address."
       "\n\n"
       " Your email adress will be part of the"
       " new key to make it easier for others to"
       " identify keys."
       " If you have several email adresses, you can add further email"
       " adresses later."),
     _("Your Email Adress:"));
}


static gboolean
gpa_keygen_wizard_email_validate (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  gboolean result = TRUE;
  gchar * email;

  email = gpa_keygen_wizard_simple_get_text (keygen_wizard->email_page); 
  if (!*email)
    {
      /* The string is empty or consists entirely of whitespace */
      gpa_window_error (_("Please insert your email address"),
			keygen_wizard->window);
      result = FALSE;
    }
  /* FIXME: we should do much more checking. Best would be exactly the
   * same checks gpg does in interactive mode with --gen-key */

  free (email);
  return result;
}


static GtkWidget *
gpa_keygen_wizard_comment_page (void)
{
  return gpa_keygen_wizard_simple_page
    (_("If you want you can supply a comment that further identifies"
       " the key to other users."
       " The comment is especially useful if you generate several keys"
       " for the same email adress."
       " The comment is completely optional."
       " Leave it empty if you don't have a use for it."),
     _("Comment:"));
}

static GtkWidget *
gpa_keygen_wizard_password_page (void)
{
  GtkWidget * vbox;
  GtkWidget * description;
  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * entry;

  vbox = gtk_vbox_new (FALSE, 0);

  description = gtk_label_new
    (_("As the final step of the key generation you have to supply"
       " a password for the new key."));
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 5);

  label = gtk_label_new (_("Password:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_passwd",
		       (gpointer)entry);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_wizard_focus_child",
		       (gpointer)entry);

  label = gtk_label_new (_("Repeat Password:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_passwd_repeat",
		       (gpointer)entry);

  return vbox;
}

static gchar *
gpa_keygen_wizard_password_get_password (GtkWidget * vbox)
{
  GtkWidget * entry = gtk_object_get_data (GTK_OBJECT (vbox),
					   "gpa_keygen_passwd");
  return gtk_entry_get_text (GTK_ENTRY (entry));
}

/* Validate the password in both entries and return NULL if they're OK,
 * otherwise return a suitable message to be shown to the user.
 */
/* FIXME: We should add some checks for whitespace because leading and
 * trailing whitespace is stripped somewhere in gpapa/gpg and whitespace
 * only passwords are not allowed */
static gchar *
gpa_keygen_wizard_password_validate (GtkWidget * vbox)
{
  GtkWidget * entry_passwd = gtk_object_get_data (GTK_OBJECT (vbox),
						  "gpa_keygen_passwd");
  GtkWidget * entry_repeat = gtk_object_get_data (GTK_OBJECT (vbox),
						  "gpa_keygen_passwd_repeat");
  gchar * message = NULL;

  if (strcmp (gtk_entry_get_text (GTK_ENTRY (entry_passwd)),
	      gtk_entry_get_text (GTK_ENTRY (entry_repeat))) != 0)
    message = _("Please insert the same password in "
		"\"Password\" and \"Repeat Password\".");
  else if (strlen (gtk_entry_get_text (GTK_ENTRY (entry_passwd))) == 0)
    message = _("Please insert a password\n");

  return message;
}


/* Generate a key pair for the given name, email, comment and password.
 * Algorithm and keysize are set to default values suitable for new
 * users. The parent parameter is used as the parent widget for error
 * message boxes */
static gboolean
gpa_keygen_generate_key(gchar * name, gchar * email, gchar * comment,
			gchar * password, GtkWidget * parent)
{
  GpapaPublicKey *publicKey;
  GpapaSecretKey *secretKey;

  /* default values for newbie mode */
  GpapaAlgo algo = GPAPA_ALGO_BOTH;
  gint keysize = 1024;

  gpapa_create_key_pair (&publicKey, &secretKey, password,
			 algo, keysize, name, email, comment,
			 gpa_callback, parent);
  return global_lastCallbackResult == GPAPA_ACTION_FINISHED;
}


/* Callback for the finish-button of the password page. Extract the
 * values the user entered and call gpa_keygen_generate_key */
static gboolean
gpa_keygen_wizard_password_action (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  gchar * name;
  gchar * email;
  gchar * comment;
  gchar * passwd;

  gchar * message;
  gboolean successful;

  message = gpa_keygen_wizard_password_validate (keygen_wizard->passwd_page);
  if (message)
    {
      gpa_window_error (message, keygen_wizard->window);
      return FALSE;
    }

  name = gpa_keygen_wizard_simple_get_text (keygen_wizard->name_page);
  email = gpa_keygen_wizard_simple_get_text (keygen_wizard->email_page);
  comment = gpa_keygen_wizard_simple_get_text (keygen_wizard->comment_page);
  passwd = gpa_keygen_wizard_password_get_password(keygen_wizard->passwd_page);

  /* Switch to the next page showing the "wait" message. */
  gpa_wizard_next_page_no_action (keygen_wizard->wizard);

  /* Handle some events so that the page is correctly displayed */
  while (gtk_events_pending())
    gtk_main_iteration();

  successful = gpa_keygen_generate_key (name, email, comment, passwd,
					keygen_wizard->window);

  free (name);
  free (email);
  free (comment);
  
  keygen_wizard->successful = successful;
  return successful;
}

static GtkWidget *
gpa_keygen_wizard_message_page (const gchar * description_text)
{
  GtkWidget * vbox;
  GtkWidget * description;

  vbox = gtk_vbox_new (FALSE, 0);

  description = gtk_label_new (description_text);
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  return vbox;
}


static GtkWidget *
gpa_keygen_wizard_wait_page (void)
{
  return gpa_keygen_wizard_message_page
    (_("Your key is being generated\n\n"
       "Even on fast computers this may take a while. Please be patient\n"));
}


static GtkWidget *
gpa_keygen_wizard_final_page (void)
{
  return gpa_keygen_wizard_message_page
    (_("Congratulations!\n\n"
       "You have successfully generated a key."
       " The key is indefinitely valid and has a length of 1024 bits."));
}


/* Handler for the close button. Destroy the main window */
static void
gpa_keygen_wizard_close (GtkWidget * widget, gpointer param)
{
  GPAKeyGenWizard * wizard = param;

  gtk_widget_destroy (wizard->window);
}


/* handler for the destroy signal. Quit the recursive main loop */
static void
gpa_keygen_wizard_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}

gboolean
gpa_keygen_wizard_run (GtkWidget * parent)
{
  GtkWidget * window;
  GtkWidget * wizard;
  GtkAccelGroup * accel_group;
  GPAKeyGenWizard * keygen_wizard;

  keygen_wizard = xmalloc (sizeof (*keygen_wizard));
  keygen_wizard->successful = FALSE;

  accel_group = gtk_accel_group_new ();

  window = gtk_window_new (GTK_WINDOW_DIALOG);
  keygen_wizard->window = window;
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_window_set_title (GTK_WINDOW (window), _("Generate key"));
  gtk_object_set_data_full (GTK_OBJECT (window), "user_data",
			    (gpointer) keygen_wizard, free);
  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
			     GTK_SIGNAL_FUNC (gpa_keygen_wizard_destroy),
			     (gpointer)keygen_wizard);

  wizard = gpa_wizard_new (accel_group, gpa_keygen_wizard_close,
			   keygen_wizard);
  keygen_wizard->wizard = wizard;
  gtk_container_add (GTK_CONTAINER (window), wizard);

  keygen_wizard->name_page = gpa_keygen_wizard_name_page ();
  gpa_wizard_append_page (wizard, keygen_wizard->name_page,
			  NULL, NULL,
			  gpa_keygen_wizard_name_validate, keygen_wizard);

  keygen_wizard->email_page = gpa_keygen_wizard_email_page ();
  gpa_wizard_append_page (wizard, keygen_wizard->email_page,
			  NULL, NULL,
			  gpa_keygen_wizard_email_validate, keygen_wizard);

  keygen_wizard->comment_page = gpa_keygen_wizard_comment_page ();
  gpa_wizard_append_page (wizard, keygen_wizard->comment_page,
			  NULL, NULL, NULL, NULL);
			  

  keygen_wizard->passwd_page = gpa_keygen_wizard_password_page ();
  /* Don't use F as the accelerator in "Finish" because Meta-F is
   * already bound in the entry widget */
  gpa_wizard_append_page (wizard, keygen_wizard->passwd_page,
			  NULL, _("F_inish"),
			  gpa_keygen_wizard_password_action, keygen_wizard);

  keygen_wizard->wait_page = gpa_keygen_wizard_wait_page ();
  gpa_wizard_append_page (wizard, keygen_wizard->wait_page,
			  NULL, _("F_inish"), NULL, NULL);

  keygen_wizard->final_page = gpa_keygen_wizard_final_page ();
  gpa_wizard_append_page (wizard, keygen_wizard->final_page,
			  NULL, _("F_inish"), NULL, NULL);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  gtk_main ();

  return keygen_wizard->successful;
}
  

