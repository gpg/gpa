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
#include "gpa.h"
#include "gpapastrings.h"
#include "icons.h"
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
      result = xstrdup (filename);
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
  GpapaPublicKey * public_key;
  GpapaSecretKey * secret_key;
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
  return string_strip_dup (gtk_entry_get_text (GTK_ENTRY (entry)));
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

  free (name);
  return result;
}

static GtkWidget *
gpa_keygen_wizard_email_page (GPAKeyGenWizard * keygen_wizard)
{
  return gpa_keygen_wizard_simple_page
    (keygen_wizard,
     _("Please insert your email address."
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
gpa_keygen_wizard_comment_page (GPAKeyGenWizard * keygen_wizard)
{
  return gpa_keygen_wizard_simple_page
    (keygen_wizard,
     _("If you want you can supply a comment that further identifies"
       " the key to other users."
       " The comment is especially useful if you generate several keys"
       " for the same email adress."
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

  label = gtk_label_new (_("Password:"));
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

  label = gtk_label_new (_("Repeat Password:"));
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
  return gtk_entry_get_text (GTK_ENTRY (entry));
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
      gpa_window_error (_("Please insert the same password in "
			  "\"Password\" and \"Repeat Password\"."),
			  keygen_wizard->window);
      result = FALSE;
    }
  else if (strlen (gtk_entry_get_text (GTK_ENTRY (entry_passwd))) == 0)
    {
      gpa_window_error (_("Please insert a password\n"),
			  keygen_wizard->window);
      result = FALSE;
    }

  /* Switch to the backup pixmap. FIXME: There should be a much more
   * generic way to handle the wizard images */
  gtk_pixmap_set (GTK_PIXMAP (keygen_wizard->pixmap_widget),
		  keygen_wizard->backup_pixmap, NULL);

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
    (_("We recommend that you create backup copies of your new key,"
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
      /* Switch back to the genkey pixmap. FIXME: There should be a much
       * more generic way to handle the wizard images */
      gtk_pixmap_set (GTK_PIXMAP (keygen_wizard->pixmap_widget),
		      keygen_wizard->genkey_pixmap, NULL);

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
  dirname = file_dirname (filename);
  gtk_entry_set_text (GTK_ENTRY (entry), dirname);

  free (filename);
  free (dirname);
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
  return string_strip_dup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

static gboolean
gpa_keygen_wizard_backup_dir_action (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  gchar * dir;
  struct stat statbuf;
  gboolean result = FALSE;
  const gchar * buttons[] = {_("_Overwrite"), _("_Cancel"), NULL};
  gchar * message;
  gchar * reply;
  
  dir = gpa_keygen_wizard_backup_get_text (keygen_wizard->backup_dir_page);

  if (isdir (dir))
    {
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
	  free (message);
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
	  free (message);
	}
    }
  else
    {
      gpa_window_error (_("Please enter a valid directory"),
			  keygen_wizard->window);
    }

  if (result)
    {
      /* Switch back to the genkey pixmap. FIXME: There should be a much
       * more generic way to handle the wizard images */
      gtk_pixmap_set (GTK_PIXMAP (keygen_wizard->pixmap_widget),
		      keygen_wizard->genkey_pixmap, NULL);
      keygen_wizard->successful
	= gpa_keygen_wizard_generate_action (keygen_wizard);
    }

  free (dir);
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


/* Generate a key pair for the given name, email, comment and password.
 * Algorithm and keysize are set to default values suitable for new
 * users. The parent parameter is used as the parent widget for error
 * message boxes */
static gboolean
gpa_keygen_generate_key(gchar * name, gchar * email, gchar * comment,
			gchar * password, GPAKeyGenWizard * keygen_wizard)
{
  /* default values for newbie mode */
  GpapaAlgo algo = GPAPA_ALGO_BOTH;
  gint keysize = 1024;

  gpapa_create_key_pair (&(keygen_wizard->public_key),
			 &(keygen_wizard->secret_key), password,
			 algo, keysize, name, email, comment,
			 gpa_callback, keygen_wizard->window);
  return global_lastCallbackResult == GPAPA_ACTION_FINISHED;
}


/* Extract the values the user entered and call gpa_keygen_generate_key.
 * Return TRUE if the key was created successfully.
 */
static gboolean
gpa_keygen_wizard_generate_action (gpointer data)
{
  GPAKeyGenWizard * keygen_wizard = data;
  gchar * name;
  gchar * email;
  gchar * comment;
  gchar * passwd;
  gchar * backup_dir;

  gboolean successful;

  name = gpa_keygen_wizard_simple_get_text (keygen_wizard->name_page);
  email = gpa_keygen_wizard_simple_get_text (keygen_wizard->email_page);
  comment = gpa_keygen_wizard_simple_get_text (keygen_wizard->comment_page);
  passwd = gpa_keygen_wizard_password_get_password(keygen_wizard->passwd_page);
  if (keygen_wizard->create_backups)
    backup_dir = gpa_keygen_wizard_backup_get_text (keygen_wizard
						    ->backup_dir_page);
  else
    backup_dir = NULL;

  /* Switch to the next page showing the "wait" message. */
  gpa_wizard_next_page_no_action (keygen_wizard->wizard);

  /* Handle some events so that the page is correctly displayed */
  while (gtk_events_pending())
    gtk_main_iteration();

  successful = gpa_keygen_generate_key (name, email, comment, passwd,
					keygen_wizard);

  if (successful && keygen_wizard->create_backups)
    {
      gpapa_public_key_export (keygen_wizard->public_key,
			       keygen_wizard->pubkey_filename,
			       GPAPA_ARMOR, gpa_callback,
			       keygen_wizard->window);
      gpapa_secret_key_export (keygen_wizard->secret_key,
			       keygen_wizard->seckey_filename,
			       GPAPA_ARMOR, gpa_callback,
			       keygen_wizard->window);
    }
  
  free (name);
  free (email);
  free (comment);
  free (backup_dir);
  
  keygen_wizard->successful = successful;
  return successful;
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

  free (keygen_wizard->pubkey_filename);
  free (keygen_wizard->seckey_filename);
  gdk_pixmap_unref (keygen_wizard->genkey_pixmap);
  gdk_pixmap_unref (keygen_wizard->backup_pixmap);
  free (keygen_wizard);
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



  keygen_wizard = xmalloc (sizeof (*keygen_wizard));
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

  window = gtk_window_new (GTK_WINDOW_DIALOG);
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
  
  wizard = gpa_wizard_new (accel_group, gpa_keygen_wizard_close,
			   keygen_wizard);
  keygen_wizard->wizard = wizard;
  gtk_box_pack_start (GTK_BOX (hbox), wizard, TRUE, TRUE, 0);

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
  

