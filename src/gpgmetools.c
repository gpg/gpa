/* gpgmetools.h - additional gpgme support functions for GPA.
   Copyright (C) 2002 Miguel Coca.
   Copyright (C) 2005 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GPA; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA  */


/* A set of auxiliary functions for common tasks related to GPGME */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpgmetools.h"

#include <fcntl.h>
#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#else
#include <io.h>
#endif
#ifdef G_OS_WIN32
#include <windows.h>
#endif


/* Report an unexpected error in GPGME and quit the application.  */
void
_gpa_gpgme_error (gpg_error_t err, const char *file, int line)
{
  gchar *message = g_strdup_printf (_("Fatal Error in GPGME library\n"
                                      "(invoked from file %s, line %i):\n\n"
                                      "\t%s\n\n"
                                      "The application will be terminated"),
                                    file, line, gpgme_strerror (err));
  gpa_window_error (message, NULL);
  g_free (message);
  exit (EXIT_FAILURE);
}


void
gpa_gpgme_warning (gpg_error_t err)
{
  gchar *message = g_strdup_printf (_("The GPGME library returned an unexpected\n"
				      "error. The error was:\n\n"
                                      "\t%s\n\n"
                                      "This is probably a bug in GPA.\n"
				      "GPA will now try to recover from this error."),
                                    gpgme_strerror (err));
  gpa_window_error (message, NULL);
  g_free (message);
}


/* Initialize a gpgme_ctx_t for use with GPA.  */
gpgme_ctx_t
gpa_gpgme_new (void)
{
  gpgme_ctx_t ctx;
  gpg_error_t err;
  
  err = gpgme_new (&ctx);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      gpa_gpgme_error (err);
    }
  gpgme_set_passphrase_cb (ctx, gpa_passphrase_cb, NULL);
  
  return ctx;
}


/* Write the contents of the gpgme_data_t object to the file.
   Receives a filehandle instead of the filename, so that the caller
   can make sure the file is accesible before putting anything into
   data.  */
void
dump_data_to_file (gpgme_data_t data, FILE *file)
{
  char buffer[128];
  int nread;

  nread = gpgme_data_seek (data, 0, SEEK_SET);
  if (nread == -1)
    {
      gpa_window_error (strerror (errno), NULL);
      exit (EXIT_FAILURE);
    }
  while ((nread = gpgme_data_read (data, buffer, sizeof (buffer))) > 0)
    fwrite (buffer, nread, 1, file);
  if (nread == -1)
    {
      gpa_window_error (strerror (errno), NULL);
      exit (EXIT_FAILURE);
    }
  return;
}


static gboolean
check_overwriting (const char *filename, GtkWidget *parent)
{
  /* If the file exists, ask before overwriting.  */
  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      GtkWidget *msgbox = gtk_message_dialog_new 
	(GTK_WINDOW(parent), GTK_DIALOG_MODAL,
	 GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, 
	 _("The file %s already exists.\n"
	   "Do you want to overwrite it?"), filename);
      gtk_dialog_add_buttons (GTK_DIALOG (msgbox),
			      _("_Yes"), GTK_RESPONSE_YES,
			      _("_No"), GTK_RESPONSE_NO, NULL);
      if (gtk_dialog_run (GTK_DIALOG (msgbox)) == GTK_RESPONSE_NO)
	{
	  gtk_widget_destroy (msgbox);
	  return FALSE;
	}
      gtk_widget_destroy (msgbox);
    }
  return TRUE;
}

/* Not really a gpgme function, but needed in most places
   dump_data_to_file is used.  Opens a file for writing, asking the
   user to overwrite if it exists and reporting any errors.  Returns
   NULL on failure, but you can assume the user has been informed of
   the error (or maybe he just didn't want to overwrite!).  */
FILE *
gpa_fopen (const char *filename, GtkWidget *parent)
{
  FILE *target;
  
  if (!check_overwriting (filename, parent))
    return NULL;
  target = fopen (filename, "w");
  if (!target)
    {
      gchar *message;
      message = g_strdup_printf ("%s: %s", filename, strerror(errno));
      gpa_window_error (message, parent);
      g_free (message);
      return NULL;
    }
  return target;
}


int
gpa_open_output (const char *filename, gpgme_data_t *data, GtkWidget *parent)
{
  int target = -1;
  
  if (check_overwriting (filename, parent))
    {
      gpg_error_t err;

      target = creat (filename, 0666);
      if (target == -1)
	{
	  gchar *message;
	  message = g_strdup_printf ("%s: %s", filename, strerror(errno));
	  gpa_window_error (message, parent);
	  g_free (message);
	}
      err = gpgme_data_new_from_fd (data, target);
      if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
	{
	  close (target);
	  target = -1;
	}
    }

  return target;
}


int
gpa_open_input (const char *filename, gpgme_data_t *data, GtkWidget *parent)
{
  gpg_error_t err;
  int target = -1;

  target = open (filename, O_RDONLY);
  if (target == -1)
    {
      gchar *message;
      message = g_strdup_printf ("%s: %s", filename, strerror(errno));
      gpa_window_error (message, parent);
      g_free (message);
    }
  err = gpgme_data_new_from_fd (data, target);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      close (target);
      target = -1;
    }

  return target;
}


/* Do a gpgme_data_new_from_file and report any GPGME_File_Error to
   the user.  */
gpg_error_t
gpa_gpgme_data_new_from_file (gpgme_data_t *data,
			      const char *filename,
			      GtkWidget *parent)
{
  gpg_error_t err;
  err = gpgme_data_new_from_file (data, filename, 1);
  if (gpg_err_code_to_errno (err) != 0)
    {
      gchar *message;
      message = g_strdup_printf ("%s: %s", filename, strerror(errno));
      gpa_window_error (message, NULL);
      g_free (message);
    }
  return err;
}

/* Write the contents of the gpgme_data_t into the clipboard.  */
void
dump_data_to_clipboard (gpgme_data_t data, GtkClipboard *clipboard)
{
  char buffer[128];
  int nread;
  gchar *text = NULL;
  gint len = 0;

  nread = gpgme_data_seek (data, 0, SEEK_SET);
  if (nread == -1)
    {
      gpa_window_error (strerror (errno), NULL);
      exit (EXIT_FAILURE);
    }
  while ((nread = gpgme_data_read (data, buffer, sizeof (buffer))) > 0)
    {
      text = g_realloc (text, len + nread);
      strncpy (text + len, buffer, nread);
      len += nread;
    }
  if (nread == -1)
    {
      gpa_window_error (strerror (errno), NULL);
      exit (EXIT_FAILURE);
    }
  gtk_clipboard_set_text (clipboard, text, len);
  g_free (text);
  return;
}


/* Assemble the parameter string for gpgme_op_genkey for GnuPG.  We
   don't need worry about the user ID being UTF-8 as long as we are
   using GTK+2, because all user input is UTF-8 in it.  */
static gchar *
build_genkey_parms (GPAKeyGenParameters *params)
{
  gchar *string;
  gchar *key_algo;
  gchar *subkeys, *email, *comment, *expire;

  /* Choose which keys and subkeys to generate.  */
  switch (params->algo)
    {
    case GPA_KEYGEN_ALGO_DSA_ELGAMAL:
      key_algo = "DSA";
      subkeys = g_strdup_printf ("SubKey-Type: ELG-E\n"
                                 "Subkey-Length: %i\n", params->keysize);
      break;
    case GPA_KEYGEN_ALGO_DSA:
      key_algo = "DSA";
      subkeys = "";
      break;
    case GPA_KEYGEN_ALGO_RSA:
      key_algo = "RSA";
      subkeys = "";
      break;
    default:
      /* Can't happen */
      return NULL;
    }

  /* Build the extra fields of the user ID if supplied by the user.  */
  if (params->email && params->email[0])
    email = g_strdup_printf ("Name-Email: %s\n", params->email);
  else
    email = "";

  if (params->comment && params->comment[0])
    comment = g_strdup_printf ("Name-Comment: %s\n", params->comment);
  else
    comment = "";

  /* Build the expiration date string if needed */
  if (params->expiryDate)
    {
      expire = g_strdup_printf ("Expire-Date: %i-%02i-%02i\n", 
                                g_date_get_year(params->expiryDate),
                                g_date_get_month(params->expiryDate),
                                g_date_get_day(params->expiryDate));
    }
  else if (params->interval)
    expire = g_strdup_printf ("Expire-Date: %i%c\n", params->interval,
			      params->unit);
  else
    expire = "";
  /* Assemble the final parameter string */
  string = g_strdup_printf ("<GnupgKeyParms format=\"internal\">\n"
                            "Key-Type: %s\n"
                            "Key-Length: %i\n"
                            "%s" /* Subkeys */
                            "Name-Real: %s\n"
                            "%s" /* Email Address */
                            "%s" /* Comment */
                            "%s" /* Expiration date */
                            "Passphrase: %s\n"
                            "</GnupgKeyParms>\n", key_algo, params->keysize,
                            subkeys, params->userID, email, comment, expire,
                            params->password);

  /* Free auxiliary strings if they are not empty */
  if (subkeys[0])
    g_free (subkeys);
  if (email[0])
    g_free (email);
  if (comment[0])
    g_free (comment);
  if (expire[0])
    g_free (expire);

  return string;
}

/* Begin generation of a key with the given parameters.  It prepares
   the parameters required by GPGME and returns whatever
   gpgme_op_genkey_start returns.  */
gpg_error_t
gpa_generate_key_start (gpgme_ctx_t ctx, GPAKeyGenParameters *params)
{
  gchar *parm_string;
  gpg_error_t err;

  parm_string = build_genkey_parms (params);
  err = gpgme_op_genkey_start (ctx, parm_string, NULL, NULL);
  g_free (parm_string);

  return err;
}


/* Retrieve the path to the GnuPG executable.  */
static const gchar *
get_gpg_path (void)
{
  gpgme_engine_info_t engine;
  
  gpgme_get_engine_info (&engine);
  while (engine)
    {
      if (engine->protocol == GPGME_PROTOCOL_OpenPGP)
	return engine->file_name;
      engine = engine->next;
    }
  return NULL;
}


/* Backup a key. It exports both the public and secret keys to a file.
   Returns TRUE on success and FALSE on error. It displays errors to
   the user.  */
gboolean
gpa_backup_key (const gchar *fpr, const char *filename)
{
  gchar *header, *pub_key, *sec_key;
  gchar *err;
  FILE *file;
  gint ret_code;
  gchar *header_argv[] = 
    {
      NULL, "--batch", "--no-tty", "--fingerprint", (gchar*) fpr, NULL
    };
  gchar *pub_argv[] = 
    {
      NULL, "--batch", "--no-tty", "--armor", "--export", (gchar*) fpr, NULL
    };
  gchar *sec_argv[] = 
    {
      NULL, "--batch", "--no-tty", "--armor", "--export-secret-key", 
      (gchar*) fpr, NULL
    };
  const gchar *path;
  mode_t mask;

  /* Get the gpg path */
  path = get_gpg_path ();
  g_assert (path != NULL);
  /* Add the executable to the arg arrays */
  header_argv[0] = (gchar*) path;
  pub_argv[0] = (gchar*) path;
  sec_argv[0] = (gchar*) path;
  /* Open the file */
  mask = umask (0077);
  file = fopen (filename, "w");
  umask (mask);
  if (!file)
    {
      gchar message[256];
      g_snprintf (message, sizeof(message), "%s: %s",
		  filename, strerror(errno));
      gpa_window_error (message, NULL);
      return FALSE;
    }
  /* Get the keys and write them into the file */
  fputs (_("************************************************************************\n"
	   "* WARNING: This file is a backup of your secret key. Please keep it in *\n"
	   "* a safe place.                                                        *\n"
	   "************************************************************************\n\n"),
	 file);
  fputs (_("The key backed up in this file is:\n\n"), file);
  if( !g_spawn_sync (NULL, header_argv, NULL, 0, NULL, NULL, &header,
		     &err, &ret_code, NULL))
    {
      return FALSE;
    }
  fputs (header, file);
  g_free (err);
  g_free (header);
  fputs ("\n", file);
  if( !g_spawn_sync (NULL, pub_argv, NULL, 0, NULL, NULL, &pub_key,
		     &err, &ret_code, NULL))
    {
      return FALSE;
    }
  fputs (pub_key, file);
  g_free (err);
  g_free (pub_key);
  fputs ("\n", file);
  if( !g_spawn_sync (NULL, sec_argv, NULL, 0, NULL, NULL, &sec_key,
		     &err, &ret_code, NULL))
    {
      return FALSE;
    }
  fputs (sec_key, file);
  g_free (err);
  g_free (sec_key);

  fclose (file);
  return TRUE;
}


void
gpa_key_gen_free_parameters(GPAKeyGenParameters * params)
{
  g_free (params->userID);
  g_free (params->email);
  g_free (params->comment);
  if (params->expiryDate)
    g_date_free (params->expiryDate);
  g_free (params);
}


GPAKeyGenParameters *
key_gen_params_new(void)
{
  GPAKeyGenParameters * params = g_malloc (sizeof (*params));
  params->userID = NULL;
  params->email = NULL;
  params->comment = NULL;
  params->expiryDate = NULL;
  params->interval = 0;
  return params;
}


static gchar *algorithm_strings[] = {
  N_("DSA and ElGamal (default)"),
  N_("DSA (sign only)"),
  N_("RSA (sign only)"),
};


const gchar *
gpa_algorithm_string (GPAKeyGenAlgo algo)
{
  return _(algorithm_strings[algo]);
}


GPAKeyGenAlgo
gpa_algorithm_from_string (const gchar * string)
{
  GPAKeyGenAlgo result;

  result = GPA_KEYGEN_ALGO_FIRST;
  while (result <= GPA_KEYGEN_ALGO_LAST &&
	 strcmp (string, _(algorithm_strings[result])) != 0)
    result++;
  return result;
}


/* Ownertrust strings.  */
const gchar *
gpa_key_ownertrust_string (gpgme_key_t key)
{
  switch (key->owner_trust) 
    {
    case GPGME_VALIDITY_UNKNOWN:
    case GPGME_VALIDITY_UNDEFINED:
    default:
      return _("Unknown");
      break;
    case GPGME_VALIDITY_NEVER:
      return _("Never");
      break;
    case GPGME_VALIDITY_MARGINAL:
      return _("Marginal");
      break;
    case GPGME_VALIDITY_FULL:
      return _("Full");
      break;
    case GPGME_VALIDITY_ULTIMATE:
      return _("Ultimate");
      break;
    }
}


/* Key validity strings.  */
const gchar *
gpa_key_validity_string (gpgme_key_t key)
{
  if (!key->uids)
    return _("Unknown");
  switch (key->uids->validity) 
    {
    case GPGME_VALIDITY_UNKNOWN:
    case GPGME_VALIDITY_UNDEFINED:
    case GPGME_VALIDITY_NEVER:
    case GPGME_VALIDITY_MARGINAL:
    default:
      if (key->subkeys->revoked)
	return _("Revoked");
      else if (key->subkeys->expired)
	return _("Expired");
      else if (key->subkeys->disabled)
	return _("Disabled");
      else if (key->subkeys->invalid)
	return _("Incomplete");
      else
	return _("Unknown");
      break;
    case GPGME_VALIDITY_FULL:
    case GPGME_VALIDITY_ULTIMATE:
      return _("Fully Valid");
    }
}


static GtkWidget *
passphrase_question_label (const char *uid_hint,
			   const char *passphrase_info, 
			   int prev_was_bad)
{
  GtkWidget *label;
  gchar *input;
  gchar *text;
  gchar *keyid, *userid;
  gint i;
  input = g_strdup (uid_hint);
  /* The first word in the hint is the key ID */
  keyid = input;
  for (i = 0; input[i] != ' '; i++)
    {
    }
  input[i++] = '\0';
  /* The rest of the hint is the user ID */
  userid = input+i;
  /* Build the label widget */
  if (!prev_was_bad)
    {
      text = g_strdup_printf ("%s\n\n%s %s\n%s %s",
                              _("Please enter the passphrase for"
                                " the following key:"),
                              _("User Name:"), userid,
                              _("Key ID:"), keyid);
    }
  else
    {
      text = g_strdup_printf ("%s\n\n%s %s\n%s %s",
                              _("Wrong passphrase, please try again:"),
                              _("User Name:"), userid,
                              _("Key ID:"), keyid);
    }
  label = gtk_label_new (text);
  g_free (input);
  g_free (text);
  return label;
}


/* This is the function called by GPGME when it wants a passphrase.  */
gpg_error_t
gpa_passphrase_cb (void *hook, const char *uid_hint,
		   const char *passphrase_info, 
		   int prev_was_bad, int fd)
{
  GtkWidget * dialog;
  GtkWidget * hbox;
  GtkWidget * vbox;
  GtkWidget * entry;
  GtkWidget * pixmap;
  GtkResponseType response;
  gchar *passphrase;
  
  dialog = gtk_dialog_new_with_buttons (_("Enter Passphrase"),
					NULL, GTK_DIALOG_MODAL,
					GTK_STOCK_OK,
					GTK_RESPONSE_OK,
                                        _("_Cancel"),
					GTK_RESPONSE_CANCEL,
					NULL);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, 
		      TRUE, FALSE, 10);
  pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
				     GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), pixmap, TRUE, FALSE, 10);
  vbox = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, FALSE, 10);
  /* The default button is OK */
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  /* Set the contents of the dialog */
  gtk_box_pack_start_defaults (GTK_BOX (vbox),
			       passphrase_question_label (uid_hint,
							  passphrase_info,
							  prev_was_bad));
  entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, FALSE, 10);
  gtk_widget_grab_focus (entry);
  /* Run the dialog */
  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  passphrase = g_strdup_printf ("%s\n", 
				gtk_entry_get_text (GTK_ENTRY (entry)));
  gtk_widget_destroy (dialog);
  if (response == GTK_RESPONSE_OK)
    {
      int passphrase_len = strlen (passphrase);
#ifdef G_OS_WIN32
      DWORD res;

      if (WriteFile ((HANDLE) _get_osfhandle (fd), passphrase,
		     passphrase_len, &res, NULL) == 0
	  || res < passphrase_len)
	{	
	  g_free (passphrase);
	  return gpg_error (gpg_err_code_from_errno (EIO));
	}
      else
	return gpg_error (GPG_ERR_NO_ERROR);
#else
      int res;
      res = write (fd, passphrase, passphrase_len);
      g_free (passphrase);
      if (res == -1)
	return gpg_error (gpg_err_code_from_errno (errno));
      else if (res < passphrase_len)
	return gpg_error (gpg_err_code_from_errno (EIO));
      else
	return gpg_error (GPG_ERR_NO_ERROR);
#endif
    }
  else
    {
      g_free (passphrase);
      return gpg_error (GPG_ERR_CANCELED);
    }
}


/* Convenience functions to access key attributes, which need to be
   filtered before being displayed to the user.  */

/* Return the user ID, making sure it is properly UTF-8 encoded.
   Allocates a new string, which must be freed with g_free ().  */
static gchar *
string_to_utf8 (const gchar *string)
{
  const gchar *s;

  if (!string)
    {
      return NULL;
    }
  /* Make sure the encoding is UTF-8.  Test structure suggested by
     Werner Koch.  */
  for (s = string; *s && !(*s & 0x80); s++)
    ;
  if (*s && !strchr (string, 0xc3))
    {
      /* The string is Latin-1.  */
      return  g_convert (string, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
    }
  else
    {
      /* The string is already in UTF-8.  */
      return g_strdup (string);
    }
}


gchar *
gpa_gpgme_key_get_userid (gpgme_user_id_t uid)
{
  gchar *uid_utf8;

  if (!uid)
    return g_strdup (_("[None]"));

  uid_utf8 = string_to_utf8 (uid->uid);

  /* Tag revoked UID's*/
  if (uid->revoked)
    {
      gchar *tmp = g_strdup_printf ("[%s] %s", _("Revoked"), uid_utf8);
      g_free (uid_utf8);
      uid_utf8 = tmp;
    }
  return uid_utf8;
}


/* Return the key fingerprint, properly formatted according to the key
   version.  Allocates a new string, which must be freed with
   g_free().  This is based on code from GPAPA's
   extract_fingerprint.  */
gchar *
gpa_gpgme_key_format_fingerprint (const char *fpraw)
{
  if (strlen (fpraw) == 32 )  /* v3 */
    {
      char *fp = g_malloc (strlen (fpraw) + 16 + 1);
      const char *r = fpraw;
      char *q = fp;
      gint c = 0;
      while (*r)
	{
	  *q++ = *r++;
	  c++;
	  if (c < 32)
	    {
	      if (c % 2 == 0)
		*q++ = ' ';
	      if (c % 16 == 0)
		*q++ = ' ';
	    }
	}
      *q = 0;
      return fp;
    }
  else
    {
      char *fp = g_malloc (strlen (fpraw) + 10 + 1);
      const char *r = fpraw;
      char *q = fp;
      gint c = 0;
      while (*r)
	{
	  *q++ = *r++;
	  c++;
	  if (c < 40)
	    {
	      if (c % 4 == 0)
		*q++ = ' ';
	      if (c % 20 == 0)
		*q++ = ' ';
	    }
	}
      *q = 0;
      return fp;
    }
}


/* Return the short key ID of the indicated key.  The returned string
   is valid as long as the key is valid.  */
const gchar *
gpa_gpgme_key_get_short_keyid (gpgme_key_t key)
{
  const char *keyid = key->subkeys->keyid;
  if (!keyid)
    return NULL;
  else
    return keyid + 8;
}


/* Convenience function to access key signature attibutes, much like
   the previous ones.  */

/* Return the user ID, making sure it is properly UTF-8 encoded.
   Allocates a new string, which must be freed with g_free().  */
gchar *
gpa_gpgme_key_sig_get_userid (gpgme_key_sig_t sig)
{
  if (!sig->uid || !*sig->uid)
    /* Duplicate it to make sure it can be g_free'd.  */
    return g_strdup (_("[Unknown user ID]"));
  else
    return string_to_utf8 (sig->uid);
}


/* Return the short key ID of the indicated key.  The returned string
   is valid as long as the key is valid.  */
const gchar *
gpa_gpgme_key_sig_get_short_keyid (gpgme_key_sig_t sig)
{
  return sig->keyid + 8;
}

/* Return a string with the status of the key signature.  */
const gchar *
gpa_gpgme_key_sig_get_sig_status (gpgme_key_sig_t sig,
				  GHashTable *revoked)
{
  const gchar *status;
  switch (sig->status)
    {
    case GPGME_SIG_STAT_GOOD:
      status = _("Valid");
      break;
    case GPGME_SIG_STAT_BAD:
      status = _("Bad");
    default:
      status = _("Unknown");
    }
  if (sig->expired)
    {
      status = _("Expired");
    }
  else if (g_hash_table_lookup (revoked, sig->keyid))
    {
      status = _("Revoked");
    }
  return status;
}


/* Return a string with the level of the key signature.  */
const gchar *
gpa_gpgme_key_sig_get_level (gpgme_key_sig_t sig)
{
  switch (sig->sig_class)
    {
    case 0x10:
      return _("Generic");
      break;
    case 0x11:
      return _("Persona");
      break;
    case 0x12:
      return _("Casual");
      break;
    case 0x13:
      return _("Positive");
      break;
    default:
      return _("Unknown");
      break;
    }
}


/* Return a string listing the capabilities of a key.  */
const gchar *
gpa_get_key_capabilities_text (gpgme_key_t key)
{
  if (key->can_certify)
    {
      if (key->can_sign)
	{
	  if (key->can_encrypt)
	    return _("The key can be used for certification, signing "
		     "and encryption.");
	  else
	    return _("The key can be used for certification and "
		     "signing, but not for encryption.");
	}
      else
	{
	  if (key->can_encrypt)
	    return _("The key can be used for certification and "
		     "encryption.");
	  else
	    return _("The key can be used only for certification.");
	}
    }
  else
    {
      if (key->can_sign)
	{
	  if (key->can_encrypt)
	    return _("The key can be used only for signing and "
		     "encryption, but not for certification.");
	  else
	    return _("The key can be used only for signing.");
	}
      else
	{
	  if (key->can_encrypt)
	    return _("The key can be used only for encryption.");
	  else
	    /* Can't happen, even for subkeys.  */
	      return _("This key is useless.");
	}
    }
}
