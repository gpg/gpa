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

#include <sys/stat.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <gpgme.h>
#include "gpa.h"
#include "gpapastrings.h"
#include "icons.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "gpawizard.h"
#include "qdchkpwd.h"
#include "gpgmetools.h"


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
  return g_strstrip (g_strdup (string));
}

/* Return TRUE if filename is a directory */
gboolean
isdir (const gchar * filename)
{
  struct stat statbuf;

  return (stat (filename, &statbuf) == 0
	  && S_ISDIR (statbuf.st_mode));
}

/* Return a copy of filename if it's a directory, its (malloced)
 * basename otherwise */
gchar *
file_dirname (const gchar * filename)
{
  gchar * result;
  if (!isdir (filename))
    {
      result = g_dirname (filename);
    }
  else
    {
      result = g_strdup (filename);
    }

  return result;
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
  GtkWidget * backup_page;
  GtkWidget * backup_dir_page;
  GtkWidget * pixmap_widget;
  GtkAccelGroup * accel_group;
  GdkPixmap * genkey_pixmap;
  GdkPixmap * backup_pixmap;
  gboolean create_backups;
  gchar * pubkey_filename;
  gchar * seckey_filename;
  gboolean successful;
} GPAKeyGenWizard;


/* Handler for the activate signals of several entry fields in the
 * wizard. Switch the wizard to the next page. */
static void
switch_to_next_page (GtkEditable *editable, gpointer user_data)
{
  gpa_wizard_next_page (((GPAKeyGenWizard*)user_data)->wizard);
}

/* internal API */
static gboolean gpa_keygen_wizard_generate_action (gpointer data);

/*
 * The user ID pages
 */

static GtkWidget *
gpa_keygen_wizard_simple_page (GPAKeyGenWizard * keygen_wizard,
			       const gchar * description_text,
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
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (switch_to_next_page), keygen_wizard);

  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_entry", entry);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_wizard_focus_child", entry);
  return vbox;
}

static gchar *
gpa_keygen_wizard_simple_get_text (GtkWidget * vbox)
{
  GtkWidget * entry;

  entry = gtk_object_get_data (GTK_OBJECT (vbox), "gpa_keygen_entry");
  return string_strip_dup ((gchar *) gtk_entry_get_text (GTK_ENTRY (entry)));
}


static GtkWidget *
gpa_keygen_wizard_name_page (GPAKeyGenWizard * keygen_wizard)
{
  return gpa_keygen_wizard_simple_page
    (keygen_wizard,
     _("Please insert your full name."
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

  g_free (name);
  return result;
}

static GtkWidget *
gpa_keygen_wizard_email_page (GPAKeyGenWizard * keygen_wizard)
{
  return gpa_keygen_wizard_simple_page
    (keygen_wizard,
     _("Please insert your email address."
       "\n\n"
       " Your email address will be part of the"
       " new key to make it easier for others to"
       " identify keys."
       " If you have several email addresses, you can add further email"
       " adresses later."),
     _("Your Email Address:"));
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

  g_free (email);
  return result;
}


static GtkWidget *
gpa_keygen_wizard_comment_page (GPAKeyGenWizard * keygen_wizard)
{
  return gpa_keygen_wizard_simple_page
    (keygen_wizard,
     _("If you want you can supply a comment that further identifies"
       " the key to other users."
       " The comment is especially useful if you generate several keys"
       " for the same email address."
       " The comment is completely optional."
       " Leave it empty if you don't have a use for it."),
     _("Comment:"));
}


/* Handler for the activate signal of the passphrase entry. Focus the
 * repeat passhrase entry. */
static void
focus_repeat_passphrase (GtkEditable *editable, gpointer user_data)
{
  gtk_widget_grab_focus ((GtkWidget*)user_data);
}



static GtkWidget *
gpa_keygen_wizard_password_page (GPAKeyGenWizard * keygen_wizard)
{
  GtkWidget * vbox;
  GtkWidget * description;
  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * entry, *passwd_entry;

  vbox = gtk_vbox_new (FALSE, 0);

  description = gtk_label_new
    (_("Please choose a passphrase for the new key."));
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 5);

  label = gtk_label_new (_("Passphrase: "));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = passwd_entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_passwd",
		       (gpointer)entry);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_wizard_focus_child",
		       (gpointer)entry);

  label = gtk_label_new (_("Repeat Passphrase: "));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_passwd_repeat",
		       (gpointer)entry);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (switch_to_next_page), keygen_wizard);
  gtk_signal_connect (GTK_OBJECT (passwd_entry), "activate",
		      GTK_SIGNAL_FUNC (focus_repeat_passphrase), entry);

  return vbox;
}

static gchar *
gpa_keygen_wizard_password_get_password (GtkWidget * vbox)
{
  GtkWidget * entry = gtk_object_get_data (GTK_OBJECT (vbox),
					   "gpa_keygen_passwd");
  return (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
}

/* Validate the password in both entries and return TRUE if they're OK,
 * otherwise show a suitable message to the user and return FALSE.
 */
/* FIXME: We should add some checks for whitespace because leading and
 * trailing whitespace is stripped somewhere in gpapa/gpg and whitespace
 * only passwords are not allowed */
static gboolean
gpa_keygen_wizard_password_validate (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  gboolean result = TRUE;
  GtkWidget * vbox = keygen_wizard->passwd_page;
  GtkWidget * entry_passwd = gtk_object_get_data (GTK_OBJECT (vbox),
						  "gpa_keygen_passwd");
  GtkWidget * entry_repeat = gtk_object_get_data (GTK_OBJECT (vbox),
						  "gpa_keygen_passwd_repeat");

  if (strcmp (gtk_entry_get_text (GTK_ENTRY (entry_passwd)),
	      gtk_entry_get_text (GTK_ENTRY (entry_repeat))) != 0)
    {
      gpa_window_error (_("In \"Passphrase\" and \"Repeat passphrase\",\n"
                          "you must enter the same passphrase."),
			  keygen_wizard->window);
      result = FALSE;
    }
  else if (strlen (gtk_entry_get_text (GTK_ENTRY (entry_passwd))) == 0)
    {
      gpa_window_error (_("You did not enter a passphrase.\n"
                          "It is needed to protect your private key."),
			  keygen_wizard->window);
      result = FALSE;
    }
  else if (strlen (gtk_entry_get_text (GTK_ENTRY (entry_passwd))) < 10
           || qdchkpwd ((char*)gtk_entry_get_text (GTK_ENTRY (entry_passwd))) < 0.6)
    {
      const gchar * buttons[] = {_("_Enter new passphrase"), _("Take this one _anyway"),
                                 NULL};
      gchar * dlg_result;
      dlg_result = gpa_message_box_run (keygen_wizard->window, _("Weak passphrase"),
                                        _("Warning: You have entered a passphrase\n"
                                          "that is obviously not secure.\n\n"
                                          "Please enter a new passphrase."),
                                        buttons);
      if (dlg_result && strcmp (dlg_result, _("_Enter new passphrase")) == 0)
        result = FALSE;
    }

  return result;
}


/* Backup copies and revocation certificate */
static GtkWidget *
gpa_keygen_wizard_backup_page (GPAKeyGenWizard * keygen_wizard)
{
  GtkWidget * vbox;
  GtkWidget * description;
  GtkWidget * radio;
  
  vbox = gtk_vbox_new (FALSE, 0);

  description = gtk_label_new
    (_("It is recommended that you create a backup copy of your new key,"
       " once it has been generated.\n\n"
       "Do you want to create a backup copy?"));
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  radio = gpa_radio_button_new (keygen_wizard->accel_group,
				_("Create _backup copy"));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, TRUE, 5);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_backup",
		       (gpointer)radio);
  
  radio = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio),
					    keygen_wizard->accel_group,
					    _("Do it _later"));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, TRUE, 5);

  return vbox;
}


static gboolean
gpa_keygen_wizard_backup_action (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  GtkWidget * radio;
  gboolean do_backup;

  radio = gtk_object_get_data (GTK_OBJECT (keygen_wizard->backup_page),
			       "gpa_keygen_backup");
  do_backup = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio));

  keygen_wizard->create_backups = do_backup;

  if (!do_backup)
    {
      gpa_wizard_next_page_no_action (keygen_wizard->wizard);
      keygen_wizard->successful
	= gpa_keygen_wizard_generate_action (keygen_wizard);
    }
  return TRUE;
}


static void
gpa_keygen_wizard_backup_dir_browse (GtkWidget * button, gpointer param)
{
  GtkWidget * entry = param;
  gchar * filename;
  gchar * dirname;

  filename = gpa_get_load_file_name (gtk_widget_get_toplevel (button),
				     _("Backup Directory"), NULL);
  if (filename)
    {
      dirname = file_dirname (filename);
      gtk_entry_set_text (GTK_ENTRY (entry), dirname);
      g_free (filename);
      g_free (dirname);
    }
}
  
  

static GtkWidget *
gpa_keygen_wizard_backup_dir_page (GPAKeyGenWizard * keygen_wizard)
{
  GtkWidget * vbox;
  GtkWidget * description;
  GtkWidget * hbox;
  GtkWidget * entry;
  GtkWidget * label;
  GtkWidget * button;
  
  vbox = gtk_vbox_new (FALSE, 0);

  description = gtk_label_new (_("Please enter a directory where your"
				 " backup keys should be saved.\n\n"
				 "GPA will create two files in that directory:"
				 " pub_key.asc and sec_key.asc"));
  gtk_box_pack_start (GTK_BOX (vbox), description, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (description), 0.0, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (description), TRUE);
  gtk_label_set_justify (GTK_LABEL (description), GTK_JUSTIFY_LEFT);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);

  label = gtk_label_new (_("Directory:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (switch_to_next_page), keygen_wizard);

  button = gpa_button_new (keygen_wizard->accel_group, _("B_rowse..."));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gpa_keygen_wizard_backup_dir_browse),
		      entry);

  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_keygen_entry", entry);
  gtk_object_set_data (GTK_OBJECT (vbox), "gpa_wizard_focus_child", entry);

  return vbox;
}

static gchar *
gpa_keygen_wizard_backup_get_text (GtkWidget * vbox)
{
  GtkWidget * entry;

  entry = gtk_object_get_data (GTK_OBJECT (vbox), "gpa_keygen_entry");
  return string_strip_dup ((gchar *) gtk_entry_get_text (GTK_ENTRY (entry)));
}

static gboolean
gpa_keygen_wizard_backup_dir_action (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  gchar * dir;
  struct stat statbuf;
  gboolean result = FALSE;
/*  const gchar * buttons[] = {_("_Overwrite"), _("_Cancel"), NULL};
  gchar * message;
  gchar * reply;
  */
  dir = gpa_keygen_wizard_backup_get_text (keygen_wizard->backup_dir_page);

  if (!isdir (dir))
    {
      const gchar *buttons[] = {_("C_reate"), _("_Cancel"), NULL};
      gchar *message = g_strdup_printf (_("Directory %s does not exist.\n"
					  "Do you want to create it now?"),
					dir);
      gchar *reply = gpa_message_box_run (keygen_wizard->window, _("Directory does not exist"),
				          message, buttons);
      if (!reply || strcmp (reply, _("C_reate")) != 0)
	result = FALSE;
      else
        {
	  g_free (message);
	  if (mkdir (dir, 0755) < 0)
	    {
	      const gchar *buttons_1[] = {_("_OK"), NULL};
	      gchar *message1 = g_strdup_printf (_("Error creating directory \"%s\": %s\n"),
						dir, g_strerror (errno));
	      gpa_message_box_run (keygen_wizard->window, _("Error creating directory"),
				   message, buttons_1);
	      g_free (message1);
	      result = FALSE;
	    }
	}
    }
  if (isdir (dir))
    {
      const gchar * buttons[] = {_("_Overwrite"), _("_Cancel"), NULL};
      gchar * message;
      gchar * reply;
      /* FIXME: we should also test for permissions */
      keygen_wizard->pubkey_filename = g_strconcat (dir, "/pub_key.asc", NULL);
      keygen_wizard->seckey_filename = g_strconcat (dir, "/sec_key.asc", NULL);
      result = TRUE;
      if (stat (keygen_wizard->pubkey_filename, &statbuf) == 0)
	{
	  message = g_strdup_printf (_("The file %s already exists.\n"
				       "Do you want to overwrite it?"),
				     keygen_wizard->pubkey_filename);
	  reply = gpa_message_box_run (keygen_wizard->window, _("File exists"),
				       message, buttons);
	  if (!reply || strcmp (reply, _("_Overwrite")) != 0)
	    result = FALSE;
	  g_free (message);
	}

      if (result && stat (keygen_wizard->pubkey_filename, &statbuf) == 0)
	{
	  message = g_strdup_printf (_("The file %s already exists.\n"
				       "Do you want to overwrite it?"),
				     keygen_wizard->seckey_filename);
	  reply = gpa_message_box_run (keygen_wizard->window, _("File exists"),
				       message, buttons);
	  if (!reply || strcmp (reply, _("_Overwrite")) != 0)
	    result = FALSE;
	  g_free (message);
	}
    }
  else
    {
      gpa_window_error (_("Please enter a valid directory"),
			  keygen_wizard->window);
    }

  if (result)
    {
      keygen_wizard->successful
	= gpa_keygen_wizard_generate_action (keygen_wizard);
    }

  g_free (dir);
  return result;
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
    (_("Your key is being generated.\n\n"
       "Even on fast computers this may take a while. Please be patient."));
}


static GtkWidget *
gpa_keygen_wizard_final_page (void)
{
  return gpa_keygen_wizard_message_page
    (_("Congratulations!\n\n"
       "You have successfully generated a key."
       " The key is indefinitely valid and has a length of 1024 bits."));
}


/* Extract the values the user entered and call gpa_generate_key.
 * Return TRUE if the key was created successfully.
 */
static gboolean
gpa_keygen_wizard_generate_action (gpointer data)
{
  GPAKeyGenWizard *keygen_wizard = data;
  GPAKeyGenParameters params;
  GpgmeError err;

  /* The User ID */
  params.userID = gpa_keygen_wizard_simple_get_text (keygen_wizard->name_page);
  params.email = gpa_keygen_wizard_simple_get_text (keygen_wizard->email_page);
  params.comment = gpa_keygen_wizard_simple_get_text (keygen_wizard
                                                      ->comment_page);
  params.password = gpa_keygen_wizard_password_get_password (keygen_wizard
                                                             ->passwd_page);

  /* default values for newbie mode */
  params.algo = GPA_KEYGEN_ALGO_DSA_ELGAMAL;
  params.keysize = 1024;
  params.expiryDate = NULL;
  params.interval = 0;
  params.generate_revocation = FALSE;
  params.send_to_server = FALSE;

  /* Switch to the next page showing the "wait" message. */
  gpa_wizard_next_page_no_action (keygen_wizard->wizard);

  /* Handle some events so that the page is correctly displayed */
  while (gtk_events_pending())
    gtk_main_iteration();

  err = gpa_generate_key (&params);

  keygen_wizard->successful = (err == GPGME_No_Error);

  g_free (params.userID);
  g_free (params.email);
  g_free (params.comment);

  return keygen_wizard->successful;
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


static void
free_keygen_wizard (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;

  g_free (keygen_wizard->pubkey_filename);
  g_free (keygen_wizard->seckey_filename);
  gdk_pixmap_unref (keygen_wizard->genkey_pixmap);
  gdk_pixmap_unref (keygen_wizard->backup_pixmap);
  g_free (keygen_wizard);
}


static void
page_switched (GtkWidget * page, gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  GdkPixmap * pixmap;

  if (page == keygen_wizard->backup_page
      || page == keygen_wizard->backup_dir_page)
    {
      pixmap = keygen_wizard->backup_pixmap;
    }
  else
    {
      pixmap = keygen_wizard->genkey_pixmap;
    }

  gtk_pixmap_set (GTK_PIXMAP (keygen_wizard->pixmap_widget), pixmap, NULL);
}


gboolean
gpa_keygen_wizard_run (GtkWidget * parent)
{
  GtkWidget * window;
  GtkWidget * wizard;
  GtkWidget * hbox;
  GtkWidget * pixmap_widget;
  GtkAccelGroup * accel_group;
  GPAKeyGenWizard * keygen_wizard;



  keygen_wizard = g_malloc (sizeof (*keygen_wizard));
  keygen_wizard->successful = FALSE;
  keygen_wizard->pubkey_filename = NULL;
  keygen_wizard->seckey_filename = NULL;
  keygen_wizard->genkey_pixmap = gpa_create_icon_pixmap (parent,
							 "wizard_genkey",
							 NULL);
  keygen_wizard->backup_pixmap = gpa_create_icon_pixmap (parent,
							 "wizard_backup",
							 NULL);

  accel_group = gtk_accel_group_new ();
  keygen_wizard->accel_group = accel_group;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  keygen_wizard->window = window;
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_window_set_title (GTK_WINDOW (window), _("Generate key"));
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  gtk_object_set_data_full (GTK_OBJECT (window), "user_data",
			    (gpointer) keygen_wizard, free_keygen_wizard);
  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
			     GTK_SIGNAL_FUNC (gpa_keygen_wizard_destroy),
			     (gpointer)keygen_wizard);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (window), hbox);

  pixmap_widget = gtk_pixmap_new (keygen_wizard->genkey_pixmap, NULL);
  keygen_wizard->pixmap_widget = pixmap_widget;
  gtk_box_pack_start (GTK_BOX (hbox), pixmap_widget, FALSE, TRUE, 0);
  
  wizard = gpa_wizard_new (accel_group,
                           (GtkSignalFunc) gpa_keygen_wizard_close,
			   (GtkSignalFunc) keygen_wizard);
  keygen_wizard->wizard = wizard;
  gtk_box_pack_start (GTK_BOX (hbox), wizard, TRUE, TRUE, 0);
  gpa_wizard_set_page_switched (wizard, page_switched, keygen_wizard);

  keygen_wizard->name_page = gpa_keygen_wizard_name_page (keygen_wizard);
  gpa_wizard_append_page (wizard, keygen_wizard->name_page,
			  NULL, NULL,
			  gpa_keygen_wizard_name_validate, keygen_wizard);

  keygen_wizard->email_page = gpa_keygen_wizard_email_page (keygen_wizard);
  gpa_wizard_append_page (wizard, keygen_wizard->email_page,
			  NULL, NULL,
			  gpa_keygen_wizard_email_validate, keygen_wizard);

  keygen_wizard->comment_page = gpa_keygen_wizard_comment_page (keygen_wizard);
  gpa_wizard_append_page (wizard, keygen_wizard->comment_page,
			  NULL, NULL, NULL, NULL);
			  
  keygen_wizard->passwd_page = gpa_keygen_wizard_password_page (keygen_wizard);
  gpa_wizard_append_page (wizard, keygen_wizard->passwd_page,
			  NULL, NULL, gpa_keygen_wizard_password_validate,
			  keygen_wizard);

  keygen_wizard->backup_page = gpa_keygen_wizard_backup_page (keygen_wizard);
  gpa_wizard_append_page (wizard, keygen_wizard->backup_page,
			  NULL, NULL,
			  gpa_keygen_wizard_backup_action, keygen_wizard);

  keygen_wizard->backup_dir_page
    = gpa_keygen_wizard_backup_dir_page (keygen_wizard);
  gpa_wizard_append_page (wizard, keygen_wizard->backup_dir_page,
			  NULL, _("F_inish"),
			  gpa_keygen_wizard_backup_dir_action, keygen_wizard);
  /* Don't use F as the accelerator in "Finish" because Meta-F is
   * already bound in the entry widget */

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
  

