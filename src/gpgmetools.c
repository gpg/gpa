/* gpgmetools.h - Additional gpgme support functions for GPA.
   Copyright (C) 2002 Miguel Coca.
   Copyright (C) 2005, 2008, 2009, 2012, 2014, 2015 g10 Code GmbH.

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
   along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


/* A set of auxiliary functions for common tasks related to GPGME */

#include <config.h>

#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

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

#include <glib/gstdio.h>

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY	_O_BINARY
#else
#define O_BINARY	0
#endif
#endif

/* Helper to strip the path from a source file name.  This helps to
   avoid showing long filenames in case of VPATH builds.  */
static const char *
strip_path (const char *file)
{
  const char *s = strrchr (file, '/');
  return s? s+1:file;
}



/* Report an unexpected error in GPGME and quit the application.  */
void
_gpa_gpgme_error (gpg_error_t err, const char *file, int line)
{
  gchar *message = g_strdup_printf (_("Fatal Error in GPGME library\n"
                                      "(invoked from file %s, line %i):\n\n"
                                      "\t%s\n\n"
                                      "The application will be terminated"),
                                    strip_path (file), line,
                                    gpgme_strerror (err));
  gpa_window_error (message, NULL);
  g_free (message);
  exit (EXIT_FAILURE);
}


/* (Please use the gpa_gpgme_warn macros).  */
void
_gpa_gpgme_warn (gpg_error_t err, const char *desc, GpaContext *ctx,
                 const char *file, int line)
{
  char *argbuf = NULL;
  const char *arg;

  if (desc && (!err || gpg_err_code (err) == GPG_ERR_GENERAL))
    arg = desc;
  else if (desc)
    {
      argbuf = g_strdup_printf ("%s (%s)", gpgme_strerror (err), desc);
      arg = argbuf;
    }
  else
    arg = gpgme_strerror (err);

  gpa_show_warn (NULL, ctx,
                 _("The GPGME library returned an unexpected\n"
                   "error at %s:%d. The error was:\n\n"
                   "\t%s\n\n"
                   "This is either an installation problem or a bug in %s.\n"
                   "%s will now try to recover from this error."),
                 strip_path (file), line, arg, GPA_NAME, GPA_NAME);
  g_free (argbuf);
}


/* Initialize a gpgme_ctx_t for use with GPA.  */
gpgme_ctx_t
gpa_gpgme_new (void)
{
  gpgme_ctx_t ctx;
  gpg_error_t err;

  /* g_assert (!"using gpa_gpgme_new"); */
  err = gpgme_new (&ctx);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    gpa_gpgme_error (err);

  if (! cms_hack)
    gpgme_set_passphrase_cb (ctx, gpa_passphrase_cb, NULL);

  return ctx;
}


/* Write the contents of the gpgme_data_t object to the file.
   Receives a filehandle instead of the filename, so that the caller
   can make sure the file is accesible before putting anything into
   data.  This is only used for a TMP file, thus it is okay to
   terminate the application on error. */
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


static char *
check_overwriting (const char *filename, GtkWidget *parent)
{
  GtkWidget *dialog;
  int response;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
  char *filename_used = xstrdup (filename);

  while (1)
    {
      /* If the file exists, ask before overwriting.  */
      if (! g_file_test (filename_used, G_FILE_TEST_EXISTS))
        return filename_used;

      dialog = gtk_message_dialog_new
        (GTK_WINDOW (parent), GTK_DIALOG_MODAL,
         GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
         _("The file %s already exists.\n"
           "Do you want to overwrite it?"), filename_used);
      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                              _("_Yes"), GTK_RESPONSE_YES,
                              _("_No"), GTK_RESPONSE_NO,
                              _("_Use a different filename"), 1,
                              NULL);

      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      if (response == GTK_RESPONSE_YES)
        return filename_used;
      if (response == GTK_RESPONSE_NO)
        {
          xfree (filename_used);
          return NULL;
        }

      /* Use a different filename.  */
      dialog = gtk_file_chooser_dialog_new
        ("Open File", GTK_WINDOW (parent), action,
         _("_Cancel"), GTK_RESPONSE_CANCEL,
         _("_Open"), GTK_RESPONSE_ACCEPT,
         NULL);
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      if (response == GTK_RESPONSE_ACCEPT)
        {
          GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
          filename_used = gtk_file_chooser_get_filename (chooser);
        }

      gtk_widget_destroy (dialog);
    }
}

/* Not really a gpgme function, but needed in most places
   dump_data_to_file is used.  Opens a file for writing, asking the
   user to overwrite if it exists and reporting any errors.  Returns
   NULL on failure, but you can assume the user has been informed of
   the error (or maybe he just didn't want to overwrite!).  */
FILE *
gpa_fopen (const char *filename, GtkWidget *parent, char **filename_used)
{
  FILE *target;

  *filename_used = check_overwriting (filename, parent);
  if (! *filename_used)
    return NULL;
  target = g_fopen (*filename_used, "w");
  if (!target)
    {
      gchar *message;
      message = g_strdup_printf ("%s: %s", *filename_used, strerror(errno));
      gpa_window_error (message, parent);
      g_free (message);
    }
  return target;
}


int
gpa_open_output_direct (const char *filename, gpgme_data_t *data,
			GtkWidget *parent)
{
  int target = -1;
  gpg_error_t err;

  target = g_open (filename,
		   O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
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


int
gpa_open_output (const char *filename, gpgme_data_t *data, GtkWidget *parent,
                 char **filename_used)
{
  int res;

  *filename_used = check_overwriting (filename, parent);
  if (! *filename_used)
    return -1;

  res = gpa_open_output_direct (*filename_used, data, parent);
  return res;
}


int
gpa_open_input (const char *filename, gpgme_data_t *data, GtkWidget *parent)
{
  gpg_error_t err;
  int target = -1;

  target = g_open (filename, O_RDONLY | O_BINARY, 0);
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


/* Write the contents of the gpgme_data_t into the clipboard.  Assumes
   that the data is ASCII.  Return 0 on success.  */
int
dump_data_to_clipboard (gpgme_data_t data, GtkClipboard *clipboard)
{
  char buffer[512];
  int nread;
  gchar *text = NULL;
  size_t len = 0;

  nread = gpgme_data_seek (data, 0, SEEK_SET);
  if (nread == -1)
    {
      gpa_window_error (strerror (errno), NULL);
      return -1;
    }
  while ((nread = gpgme_data_read (data, buffer, sizeof (buffer))) > 0)
    {
      text = g_realloc (text, len + nread + 1);
      strncpy (text + len, buffer, nread);
      len += nread;
    }
  if (nread == -1)
    {
      gpa_window_error (strerror (errno), NULL);
      return -1;
    }

  gtk_clipboard_set_text (clipboard, text, (int)len);
  g_free (text);
  return 0;
}


/* Assemble the parameter string for gpgme_op_genkey for GnuPG.  We
   don't need worry about the user ID being UTF-8 as long as we are
   using GTK+2, because all user input is UTF-8 in it.  */
static gchar *
build_genkey_parms (gpa_keygen_para_t *params)
{
  gchar *string;
  const char *key_algo;
  gchar *subkeys = NULL;
  gchar *name = NULL;
  gchar *email = NULL;
  gchar *comment = NULL;
  gchar *expire = NULL;

  /* Choose which keys and subkeys to generate.  */
  switch (params->algo)
    {
    case GPA_KEYGEN_ALGO_RSA_RSA:
      key_algo = "RSA";
      subkeys = g_strdup_printf ("Subkey-Type: RSA\n"
                                 "Subkey-Length: %d\n"
                                 "Subkey-Usage: encrypt\n", params->keysize);
      break;
    case GPA_KEYGEN_ALGO_RSA_ELGAMAL:
      key_algo = "RSA";
      subkeys = g_strdup_printf ("Subkey-Type: ELG-E\n"
                                 "Subkey-Length: %d\n"
                                 "Subkey-Usage: encrypt\n", params->keysize);
      break;
    case GPA_KEYGEN_ALGO_RSA:
      key_algo = "RSA";
      break;
    case GPA_KEYGEN_ALGO_DSA_ELGAMAL:
      key_algo = "DSA";
      subkeys = g_strdup_printf ("Subkey-Type: ELG-E\n"
                                 "Subkey-Length: %i\n"
                                 "Subkey-Usage: encrypt\n", params->keysize);
      break;
    case GPA_KEYGEN_ALGO_DSA:
      key_algo = "DSA";
      break;
    default:
      /* Can't happen */
      return NULL;
    }

  /* Construct the user ID.  */
  if (params->name && params->name[0])
    name = g_strdup_printf ("Name-Real: %s\n", params->name);
  if (params->email && params->email[0])
    email = g_strdup_printf ("Name-Email: %s\n", params->email);
  if (params->comment && params->comment[0])
    comment = g_strdup_printf ("Name-Comment: %s\n", params->comment);

  /* Build the expiration date string if needed */
  if (g_date_valid (&params->expire))
    expire = g_strdup_printf ("Expire-Date: %04d-%02d-%02d\n",
                              g_date_get_year (&params->expire),
                              g_date_get_month (&params->expire),
                              g_date_get_day (&params->expire));

  /* Assemble the final parameter string */
  string = g_strdup_printf ("<GnupgKeyParms format=\"internal\">\n"
                            "Key-Type: %s\n"
                            "Key-Length: %i\n"
                            "Key-Usage: sign\n"
                            "%s" /* Subkeys */
                            "%s" /* Name */
                            "%s" /* Email */
                            "%s" /* Comment */
                            "%s" /* Expiration date */
                            "%%ask-passphrase\n"
                            "</GnupgKeyParms>\n",
                            key_algo,
                            params->keysize,
                            subkeys? subkeys : "",
                            name? name:"",
                            email? email : "",
                            comment? comment : "",
                            expire? expire : "");

  /* Free auxiliary strings if they are not empty */
  g_free (subkeys);
  g_free (name);
  g_free (email);
  g_free (comment);
  g_free (expire);

  return string;
}

/* Begin generation of a key with the given parameters.  It prepares
   the parameters required by GPGME and returns whatever
   gpgme_op_genkey_start returns.  */
gpg_error_t
gpa_generate_key_start (gpgme_ctx_t ctx, gpa_keygen_para_t *params)
{
  gchar *parm_string;
  gpg_error_t err;

  parm_string = build_genkey_parms (params);
  err = gpgme_op_genkey_start (ctx, parm_string, NULL, NULL);
  g_free (parm_string);

  return err;
}


/* Retrieve the path to the GPG executable.  */
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


/* Retrieve the path to the GPGSM executable.  */
static const gchar *
get_gpgsm_path (void)
{
  gpgme_engine_info_t engine;

  gpgme_get_engine_info (&engine);
  while (engine)
    {
      if (engine->protocol == GPGME_PROTOCOL_CMS)
	return engine->file_name;
      engine = engine->next;
    }
  return NULL;
}


/* Retrieve the path to the GPGCONF executable.  */
static const gchar *
get_gpgconf_path (void)
{
  gpgme_engine_info_t engine;

  gpgme_get_engine_info (&engine);
  while (engine)
    {
      if (engine->protocol == GPGME_PROTOCOL_GPGCONF)
	return engine->file_name;
      engine = engine->next;
    }
  return NULL;
}


/* Retrieve the path to the GPG-CONNECT-AGENT executable.  Note that
   the caller must free the returned string.  */
static char *
get_gpg_connect_agent_path (void)
{
  const char *gpgconf;
  char *fname, *p;

  gpgconf = get_gpgconf_path ();
  if (!gpgconf)
    return NULL;

#ifdef G_OS_WIN32
# define NEWNAME "gpg-connect-agent.exe"
#else
# define NEWNAME "gpg-connect-agent"
#endif

  fname = g_malloc (strlen (gpgconf) + strlen (NEWNAME) + 1);
  strcpy (fname, gpgconf);
#ifdef G_OS_WIN32
  for (p=fname; *p; p++)
    if (*p == '\\')
      *p = '/';
#endif /*G_OS_WIN32*/
  p = strrchr (fname, '/');
  if (p)
    p++;
  else
    p = fname;
  strcpy (p, NEWNAME);

#undef NEWNAME
  return fname;
}


/* Backup a key. It exports both the public and secret keys to a file.
   IS_X509 tells the function that the fingerprint is from an X.509
   key.  Returns TRUE on success and FALSE on error. It displays
   errors to the user.  */
gboolean
gpa_backup_key (const gchar *fpr, const char *filename, int is_x509)
{
  const char *header_argv[] =
    {
      "", "--batch", "--no-tty", "--fingerprint",
      (char*) fpr, NULL
    };
  const char *pub_argv[] =
    {
      "", "--batch", "--no-tty", "--armor", "--export",
      (char*) fpr, NULL
    };
  const char *sec_argv[] =
    {
      "", "--batch", "--no-tty", "--armor", "--export-secret-key",
      (char*) fpr, NULL
    };
  const char *seccms_argv[] =
    {
      "", "--batch", "--no-tty", "--armor", "--export-secret-key-p12",
      (char*) fpr, NULL
    };
  gpg_error_t err;
  FILE *fp;
  gpgme_data_t dfp = NULL;
  const char *pgm;
  gpgme_ctx_t ctx = NULL;
  int result = FALSE;

  /* Get the gpg path.  */
  if (is_x509)
    pgm = get_gpgsm_path ();
  else
    pgm = get_gpg_path ();
  g_return_val_if_fail (pgm && *pgm, FALSE);

  /* Open the file */
  {
    mode_t mask = umask (0077);
    fp = g_fopen (filename, "w");
    umask (mask);
  }
  if (!fp)
    {
      gchar message[256];
      g_snprintf (message, sizeof(message), "%s: %s",
		  filename, strerror(errno));
      gpa_window_error (message, NULL);
      return FALSE;
    }

  fputs (_(
    "************************************************************************\n"
    "* WARNING: This file is a backup of your secret key. Please keep it in *\n"
    "* a safe place.                                                        *\n"
    "************************************************************************\n"
    "\n"), fp);

  fputs (_("The key backed up in this file is:\n\n"), fp);
  fflush (fp);

  err = gpgme_data_new_from_stream (&dfp, fp);
  if (err)
    {
      g_message ("error creating data object '%s': %s",
                 filename, gpg_strerror (err));
      goto leave;
    }

  ctx = gpa_gpgme_new ();
  gpgme_set_protocol (ctx, GPGME_PROTOCOL_SPAWN);

  err = gpgme_op_spawn (ctx, pgm, header_argv, NULL, dfp, NULL,
                        GPGME_SPAWN_DETACHED|GPGME_SPAWN_ALLOW_SET_FG);
  if (err)
    {
      g_message ("error running '%s' (1): %s", pgm, gpg_strerror (err));
      goto leave;
    }
  gpgme_data_write (dfp, "\n", 1);

  err = gpgme_op_spawn (ctx, pgm, pub_argv, NULL, dfp, NULL,
                        GPGME_SPAWN_DETACHED|GPGME_SPAWN_ALLOW_SET_FG);
  if (err)
    {
      g_message ("error running '%s' (2): %s", pgm, gpg_strerror (err));
      goto leave;
    }
  gpgme_data_write (dfp, "\n", 1);

  err = gpgme_op_spawn (ctx, pgm, is_x509? seccms_argv : sec_argv,
                        NULL, dfp, NULL,
                        GPGME_SPAWN_DETACHED|GPGME_SPAWN_ALLOW_SET_FG);
  if (err)
    {
      g_message ("error running '%s' (3): %s", pgm, gpg_strerror (err));
      goto leave;
    }

  result = TRUE;

 leave:
  gpgme_release (ctx);
  gpgme_data_release (dfp);
  fclose (fp);
  return result;
}


void
gpa_keygen_para_free (gpa_keygen_para_t *params)
{
  if (params)
    {
      g_free (params->name);
      g_free (params->email);
      g_free (params->comment);
      g_free (params->r_error_desc);
      g_free (params);
    }
}


gpa_keygen_para_t *
gpa_keygen_para_new (void)
{
  gpa_keygen_para_t *params = xcalloc (1, sizeof *params);
  g_date_clear (&params->expire, 1);
  return params;
}



/* Ownertrust strings.  */
const gchar *
gpa_key_ownertrust_string (gpgme_key_t key)
{
  if (key->protocol == GPGME_PROTOCOL_CMS)
    return "";

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


/* UID validity strings.  */
const gchar *
gpa_uid_validity_string (gpgme_user_id_t uid)
{
  if (uid->revoked)
    return _("Revoked");
  else if (uid->invalid)
    return _("Invalid");

  switch (uid->validity)
    {
    case GPGME_VALIDITY_UNKNOWN:
    case GPGME_VALIDITY_UNDEFINED:return _("Unknown");
    case GPGME_VALIDITY_NEVER:    return _("Faked");
    case GPGME_VALIDITY_MARGINAL: return _("Marginal");
    case GPGME_VALIDITY_FULL:     return _("Fully");
    case GPGME_VALIDITY_ULTIMATE: return _("Ultimate");
    default: return "[?]";
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

  /* Just in case this is called without a user id hint we return a
     simple text to avoid a crash.  */
  if (!uid_hint)
    return gtk_label_new ("Passphrase?");

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
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

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
  const char *s;

  if (!string)
    return NULL;

  /* Due to a bug in old and not so old PGP versions user IDs have
     been copied verbatim into the key.  Thus many users with Umlauts
     et al. in their name will see their names garbled.  Although this
     is not an issue for me (;-)), I have a couple of friends with
     Umlauts in their name, so let's try to make their life easier by
     detecting invalid encodings and convert that to Latin-1.  We use
     this even for X.509 because it may make things even better given
     all the invalid encodings often found in X.509 certificates.  */
  for (s = string; *s && !(*s & 0x80); s++)
    ;
  if (*s && ((s[1] & 0xc0) == 0x80) && ( ((*s & 0xe0) == 0xc0)
                                         || ((*s & 0xf0) == 0xe0)
                                         || ((*s & 0xf8) == 0xf0)
                                         || ((*s & 0xfc) == 0xf8)
                                         || ((*s & 0xfe) == 0xfc)) )
    {
      /* Possible utf-8 character followed by continuation byte.
         Although this might still be Latin-1 we better assume that it
         is valid utf-8. */
      return g_strdup (string);
     }
  else if (*s && !strchr (string, 0xc3))
    {
      /* No 0xC3 character in the string; assume that it is Latin-1.  */
      return g_convert (string, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
    }
  else
    {
      /* Everything else is assumed to be UTF-8.  We do this even that
         we know the encoding is not valid.  However as we only test
         the first non-ascii character, valid encodings might
         follow.  */
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
      /* FIXME: I think, this code should be simply disabled. Even if
	 the uid U is revoked, it is "U", not "[Revoked] U". Side
	 note: adding this prefix obviously breaks sorting by
	 uid. -moritz */
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


/* Return a human readable string with the status of the signature
   SIG.  If R_KEYDESC is not NULL, the description of the key
   (e.g.. the user ID) will be stored as a malloced string at that
   address; if no key is known, NULL will be stored.  If R_KEY is not
   NULL, a key object will be stored at that address; NULL if no key
   is known.  CTX is used as helper to figure out the key
   description.  */
char *
gpa_gpgme_get_signature_desc (gpgme_ctx_t ctx, gpgme_signature_t sig,
                              char **r_keydesc, gpgme_key_t *r_key)
{
  gpgme_key_t key = NULL;
  char *keydesc = NULL;
  char *sigdesc;
  const char *sigstatus;

  sigstatus = sig->status? gpg_strerror (sig->status) : "";

  if (sig->fpr && ctx)
    {
      gpgme_get_key (ctx, sig->fpr, &key, 0);
      if (key)
        keydesc = gpa_gpgme_key_get_userid (key->uids);
    }

  if (sig->summary & GPGME_SIGSUM_RED)
    {
      if (keydesc && *sigstatus)
        sigdesc = g_strdup_printf (_("Bad signature by %s: %s"),
                                   keydesc, sigstatus);
      else if (keydesc)
        sigdesc = g_strdup_printf (_("Bad signature by %s"),
                                   keydesc);
      else if (sig->fpr && *sigstatus)
        sigdesc = g_strdup_printf (_("Bad signature by unknown key "
                                     "%s: %s"), sig->fpr, sigstatus);
      else if (sig->fpr)
        sigdesc = g_strdup_printf (_("Bad signature by unknown key "
                                     "%s"), sig->fpr);
      else if (*sigstatus)
        sigdesc = g_strdup_printf (_("Bad signature by unknown key: "
                                     "%s"), sigstatus);
      else
        sigdesc = g_strdup_printf (_("Bad signature by unknown key"));
    }
  else if (sig->summary & GPGME_SIGSUM_VALID)
    {
      if (keydesc && *sigstatus)
        sigdesc = g_strdup_printf (_("Good signature by %s: %s"),
                                   keydesc, sigstatus);
      else if (keydesc)
        sigdesc = g_strdup_printf (_("Good signature by %s"),
                                   keydesc);
      else if (sig->fpr && *sigstatus)
        sigdesc = g_strdup_printf (_("Good signature by unknown key "
                                     "%s: %s"), sig->fpr, sigstatus);
      else if (sig->fpr)
        sigdesc = g_strdup_printf (_("Good signature by unknown key "
                                     "%s"), sig->fpr);
      else if (*sigstatus)
        sigdesc = g_strdup_printf (_("Good signature by unknown key: "
                                     "%s"), sigstatus);
      else
        sigdesc = g_strdup_printf (_("Good signature by unknown key"));
    }
  else
    {
      if (keydesc && *sigstatus)
        sigdesc = g_strdup_printf (_("Uncertain signature by %s: %s"),
                                   keydesc, sigstatus);
      else if (keydesc)
        sigdesc = g_strdup_printf (_("Uncertain signature by %s"),
                                   keydesc);
      else if (sig->fpr && *sigstatus)
        sigdesc = g_strdup_printf (_("Uncertain signature by unknown key "
                                     "%s: %s"), sig->fpr, sigstatus);
      else if (sig->fpr)
        sigdesc = g_strdup_printf (_("Uncertain signature by unknown key "
                                     "%s"), sig->fpr);
      else if (*sigstatus)
        sigdesc = g_strdup_printf (_("Uncertain signature by unknown "
                                     "key: %s"), sigstatus);
      else
        sigdesc = g_strdup_printf (_("Uncertain signature by unknown "
                                     "key"));
    }


  if (r_keydesc)
    *r_keydesc = keydesc;
  else
    g_free (keydesc);

  if (r_key)
    *r_key = key;
  else
    gpgme_key_unref (key);

  return sigdesc;
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


/* Update the result structure RESULT using the gpgme result INFO and
   the FILES and BAD_FILES counter.  */
void
gpa_gpgme_update_import_results (gpa_import_result_t result,
                                 unsigned int files, unsigned int bad_files,
                                 gpgme_import_result_t info)
{
  result->files     += files;
  result->bad_files += bad_files;
  if (info)
    {
      result->considered       += info->considered;
      result->imported         += info->imported;
      result->unchanged        += info->unchanged;
      result->secret_read      += info->secret_read;
      result->secret_imported  += info->secret_imported;
      result->secret_unchanged += info->secret_unchanged;
    }
}


void
gpa_gpgme_show_import_results (GtkWidget *parent, gpa_import_result_t result)
{
  char *buf1, *buf2;

  if (result->files)
    buf2 = g_strdup_printf (_("%u file(s) read\n"
                              "%u file(s) with errors"),
                            result->files,
                            result->bad_files);
  else
    buf2 = NULL;


  if (!result->considered)
    gpa_show_warn (parent, NULL, "%s%s%s",
                      _("No keys were found."),
                      buf2? "\n":"",
                      buf2? buf2:"");
  else
    {
      buf1 = g_strdup_printf (_("%i public keys read\n"
                                "%i public keys imported\n"
                                "%i public keys unchanged\n"
                                "%i secret keys read\n"
                                "%i secret keys imported\n"
                                "%i secret keys unchanged"),
                              result->considered,
                              result->imported,
                              result->unchanged,
                              result->secret_read,
                              result->secret_imported,
                              result->secret_unchanged);

      gpa_show_info (parent,
                     "%s%s%s",
                     buf1,
                     buf2? "\n":"",
                     buf2? buf2:"");
      g_free (buf1);
    }

  g_free (buf2);
}


/* Return a copy of the key array.  */
gpgme_key_t *
gpa_gpgme_copy_keyarray (gpgme_key_t *keys)
{
  gpgme_key_t *newarray;
  int idx;

  if (!keys)
    return NULL;

  for (idx=0; keys[idx]; idx++)
    ;
  idx++;
  newarray = g_new (gpgme_key_t, idx);
  for (idx=0; keys[idx]; idx++)
    {
      gpgme_key_ref (keys[idx]);
      newarray[idx] = keys[idx];
    }
  newarray[idx] = NULL;

  return newarray;
}


/* Release all keys in the array KEYS as well as ARRAY itself.  */
void
gpa_gpgme_release_keyarray (gpgme_key_t *keys)
{
  if (keys)
    {
      int idx;

      for (idx=0; keys[idx]; idx++)
        gpgme_key_unref (keys[idx]);
      g_free (keys);
    }
}





/* Read the next number in the version string STR and return it in
   *NUMBER.  Return a pointer to the tail of STR after parsing, or
   *NULL if the version string was invalid.  */
static const char *
parse_version_number (const char *str, int *number)
{
#define MAXVAL ((INT_MAX - 10) / 10)
  int val = 0;

  /* Leading zeros are not allowed.  */
  if (*str == '0' && isascii (str[1]) && isdigit (str[1]))
    return NULL;

  while (isascii (*str) && isdigit (*str) && val <= MAXVAL)
    {
      val *= 10;
      val += *(str++) - '0';
    }
  *number = val;
  return val > MAXVAL ? NULL : str;
#undef MAXVAL
}


/* Parse the version string STR in the format MAJOR.MINOR.MICRO (for
   example, 9.3.2) and return the components in MAJOR, MINOR and MICRO
   as integers.  The function returns the tail of the string that
   follows the version number.  This might be te empty string if there
   is nothing following the version number, or a patchlevel.  The
   function returns NULL if the version string is not valid.  */
static const char *
parse_version_string (const char *str, int *major, int *minor, int *micro)
{
  str = parse_version_number (str, major);
  if (!str || *str != '.')
    return NULL;
  str++;

  str = parse_version_number (str, minor);
  if (!str || *str != '.')
    return NULL;
  str++;

  str = parse_version_number (str, micro);
  if (!str)
    return NULL;

  /* A patchlevel might follow.  */
  return str;
}


/* Return true if MY_VERSION is at least REQ_VERSION, and false
   otherwise.  */
static int
compare_version_strings (const char *my_version,
			 const char *rq_version)
{
  int my_major, my_minor, my_micro;
  int rq_major, rq_minor, rq_micro;
  const char *my_plvl, *rq_plvl;

  if (!rq_version)
    return 1;
  if (!my_version)
    return 0;

  my_plvl = parse_version_string (my_version, &my_major, &my_minor, &my_micro);
  if (!my_plvl)
    return 0;

  rq_plvl = parse_version_string (rq_version, &rq_major, &rq_minor, &rq_micro);
  if (!rq_plvl)
    return 0;

  if (my_major > rq_major
      || (my_major == rq_major && my_minor > rq_minor)
      || (my_major == rq_major && my_minor == rq_minor
	  && my_micro > rq_micro)
      || (my_major == rq_major && my_minor == rq_minor
	  && my_micro == rq_micro && strcmp (my_plvl, rq_plvl) >= 0))
    return 1;

  return 0;
}


/* Try switching to the gpg2 backend or the one given by filename.  */
/* Return 1 if the gpg engine has at least version NEED_VERSION,
   otherwise 0.  */
int
is_gpg_version_at_least (const char *need_version)
{
  gpgme_engine_info_t engine;

  gpgme_get_engine_info (&engine);
  while (engine)
    {
      if (engine->protocol == GPGME_PROTOCOL_OpenPGP)
        return !!compare_version_strings (engine->version, need_version);
      engine = engine->next;
    }
  return 0; /* No gpg-engine available. */
}


/* Structure used to communicate with gpg_simple_stdio_cb.  */
struct gpg_simple_stdio_parm_s
{
  gboolean (*cb)(void *opaque, char *line);
  void *cb_arg;
  GString *string;
  int only_status_lines;
};

/* Helper for gpa_start_simple_gpg_command.  */
static gboolean
gpg_simple_stdio_cb (GIOChannel *channel, GIOCondition condition,
                     void *user_data)
{
  struct gpg_simple_stdio_parm_s *parm = user_data;
  GIOStatus status;
  char *line, *p;

  if ((condition & G_IO_IN))
    {
      /* We don't use a while but an if because that allows to update
         progress bars nicely.  A bit slower, but no real problem.  */
      if ((status = g_io_channel_read_line_string
           (channel, parm->string, NULL, NULL)) == G_IO_STATUS_NORMAL)
        {
          line = parm->string->str;

          /* Strip line terminator.  */
          p = strchr (line, '\n');
          if (p)
            {
              if (p > line && p[-1] == '\r')
                p[-1] = 0;
              else
                *p = 0;
            }

          /* We care only about status lines.  */
          if (parm->only_status_lines && !strncmp (line, "[GNUPG:] ", 9))
            {
              line += 9;

              /* Call user callback.  */
              if (parm->cb && !parm->cb (parm->cb_arg, line))
                {
                  /* User requested EOF.  */
                  goto cleanup;
                }
              /* Return direct so that we do not run into the G_IO_HUP
                 check.  This is required to read all buffered input.  */
              return TRUE;  /* Keep on watching this channel. */
            }
          else if (!parm->only_status_lines)
            {
              /* Call user callback.  */
              if (parm->cb && !parm->cb (parm->cb_arg, line))
                {
                  /* User requested EOF.  */
                  goto cleanup;
                }

              return TRUE;  /* Keep on watching this channel. */
            }
        }
      if (status != G_IO_STATUS_NORMAL && status != G_IO_STATUS_AGAIN )
        {
          /* Error or EOF.  */
          goto cleanup;
        }
    }
  if ((condition & G_IO_HUP))
    {
      goto cleanup;
    }

  return TRUE;  /* Keep on watching this channel. */

 cleanup:
  if (parm->cb)
    parm->cb (parm->cb_arg, NULL);  /* Tell user about EOF.  */
  g_string_free (parm->string, TRUE);
  xfree (parm);
  g_io_channel_unref (channel);  /* Close channel.  */
  return FALSE;  /* Remove us from the watch list.  */
}


/* Run gpg asynchronously with the given arguments and return a gpg
   error code on error.  The list of arguments are all of type (const
   char*) and end with a NULL argument (FIRST_ARG may already be NULL,
   but that does not make any sense).  STDIN and STDOUT are connected
   to /dev/null.  No more than 20 arguments may be given.

   Because the protocol GPGME_PROTOCOL_ASSUAN makes no sense here, it
   is used to call gpg-connect-agent.

   If the function returns success the provided callback CB is called
   for each line received on stdout (respective stderr if USE_STADERR
   is true).  EOF is send to this callback by passing a LINE as NULL.
   The callback may use this for cleanup.  If the callback returns
   FALSE, an EOF is forced so that the callback is called once more
   with LINE set to NULL.

   This function is used to run

    gpgsm --learn-card
    gpg-connect-agent NOP /bye

   The problem is that under Windows g_spawn does not allow to specify
   flags for the underlying CreateProcess.  Thus it is not possible to
   create a detached process (i.e. without a console); the result is
   that a console window pops up.  I can see two solutions: (1) Use a
   wrapper process to start them detached, or (2) move the required
   function into GPGME and use that new API.
  */
gpg_error_t
gpa_start_simple_gpg_command (gboolean (*cb)(void *opaque, char *line),
                              void *cb_arg, gpgme_protocol_t protocol,
                              int use_stderr,
                              const char *first_arg, ...)
{
  char *argv[24];
  int argc;
  int fd_stdio;
  GIOChannel *channel;
  struct gpg_simple_stdio_parm_s *parm = NULL;
  char *freeme = NULL;

  if (protocol == GPGME_PROTOCOL_OpenPGP)
    argv[0] = (char*)get_gpg_path ();
  else if (protocol == GPGME_PROTOCOL_CMS)
    argv[0] = (char*)get_gpgsm_path ();
  else if (protocol == GPGME_PROTOCOL_GPGCONF)
    argv[0] = (char*)get_gpgconf_path ();
  else if (protocol == GPGME_PROTOCOL_ASSUAN)
    argv[0] = freeme = get_gpg_connect_agent_path ();
  else
    argv[0] = NULL;

  if (!argv[0])
    {
      gpa_window_error (_("A required engine component is not installed."),
                        NULL);
      return gpg_error (GPG_ERR_INV_ARG);
    }

  argc = 1;
  if (protocol != GPGME_PROTOCOL_GPGCONF
      && protocol != GPGME_PROTOCOL_ASSUAN)
    {
      argv[argc++] = (char*)"--status-fd";
      argv[argc++] = (char*)"2";
    }
  argv[argc++] = (char*)first_arg;
  if (first_arg)
    {
      va_list arg_ptr;
      const char *s;

      va_start (arg_ptr, first_arg);
      while (argc < DIM (argv)-1 && (s=va_arg (arg_ptr, const char *)))
        argv[argc++] = (char*)s;
      va_end (arg_ptr);
      argv[argc] = NULL;
      g_return_val_if_fail (argc < DIM (argv), gpg_error (GPG_ERR_INV_ARG));
    }

  parm = g_try_malloc (sizeof *parm);
  if (!parm)
    return gpg_error_from_syserror ();
  parm->cb = cb;
  parm->cb_arg = cb_arg;
  parm->string = g_string_sized_new (200);
  parm->only_status_lines = use_stderr;

  if (!g_spawn_async_with_pipes (NULL, argv, NULL,
                                 (use_stderr
                                  ? G_SPAWN_STDOUT_TO_DEV_NULL
                                  : G_SPAWN_STDERR_TO_DEV_NULL),
                                 NULL, NULL, NULL,
                                 NULL,
                                 use_stderr? NULL : &fd_stdio,
                                 use_stderr? &fd_stdio : NULL,
                                 NULL))
    {
      gpa_window_error (_("Calling the crypto engine program failed."), NULL);
      xfree (parm);
      g_free (freeme);
      return gpg_error (GPG_ERR_GENERAL);
    }
  g_free (freeme);
#ifdef G_OS_WIN32
  channel = g_io_channel_win32_new_fd (fd_stdio);
#else
  channel = g_io_channel_unix_new (fd_stdio);
#endif
  g_io_channel_set_encoding (channel, NULL, NULL);
  /* Note that we need a buffered channel, so that we can use the read
     line function.  */
  g_io_channel_set_close_on_unref (channel, TRUE);

  /* Create a watch for the channel.  */
  if (!g_io_add_watch (channel, (G_IO_IN|G_IO_HUP),
                       gpg_simple_stdio_cb, parm))
    {
      g_debug ("error creating watch for gpg command");
      g_io_channel_unref (channel);
      xfree (parm);
      return gpg_error (GPG_ERR_GENERAL);
    }

  return 0;
}


/* Try to start the gpg-agent if it has not yet been started.
   Starting the agent works in the background.  Thus if the function
   returns, it is not sure that the agent is now running.  */
void
gpa_start_agent (void)
{
  gpg_error_t err;
  gpgme_ctx_t ctx;
  char *pgm;
  const char *argv[3];

  pgm = get_gpg_connect_agent_path ();
  if (!pgm)
    {
      g_message ("tool to start the agent is not available");
      return;
    }

  ctx = gpa_gpgme_new ();
  gpgme_set_protocol (ctx, GPGME_PROTOCOL_SPAWN);
  argv[0] = "";   /* Auto-insert the basename.  */
  argv[1] = "NOP";
  argv[2] = NULL;
  err = gpgme_op_spawn (ctx, pgm, argv, NULL, NULL, NULL, GPGME_SPAWN_DETACHED);
  if (err)
    g_message ("error running '%s': %s", pgm, gpg_strerror (err));
  g_free (pgm);
  gpgme_release (ctx);

}



/* Fucntions matching the user id verification isn gpg's key generation.  */

const char *
gpa_validate_gpg_name (const char *name)
{
  const char *result = NULL;

  if (!name || !*name)
    result = _("You must enter a name.");
  else if (strpbrk (name, "<>"))
    result = _("Invalid character in name.");
  else if (g_ascii_isdigit (*name))
    result = _("Name may not start with a digit.");
  else if (g_utf8_strlen (name, -1) < 5)
    result = _("Name is too short.");

  return result;
}


/* Check whether the string has characters not valid in an RFC-822
   address.  To cope with OpenPGP we allow non-ascii characters
   so that for example umlauts are legal in an email address.  An
   OpenPGP user ID must be utf-8 encoded but there is no strict
   requirement for RFC-822.  Thus to avoid IDNA encoding we put the
   address verbatim as utf-8 into the user ID under the assumption
   that mail programs handle IDNA at a lower level and take OpenPGP
   user IDs as utf-8.  */
static int
has_invalid_email_chars (const char *s)
{
  int at_seen = 0;
  const char *valid_chars=
    "01234567890_-.abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  for ( ; *s; s++ )
    {
      if ((*s & 0x80))
        continue; /* We only care about ASCII.  */
      if (*s == '@')
        at_seen=1;
      else if (!at_seen && !( !!strchr( valid_chars, *s ) || *s == '+' ))
        return 1;
      else if (at_seen && !strchr (valid_chars, *s))
        return 1;
    }
  return 0;
}


static int
string_count_chr (const char *string, int c)
{
  int count;

  for (count=0; *string; string++ )
    if ( *string == c )
      count++;
  return count;
}


/* Check whether NAME represents a valid mailbox according to RFC822
   except for non-ascii utf-8 characters. Returns true if so. */
static int
is_valid_mailbox (const char *name)
{
  return !( !name
            || !*name
            || has_invalid_email_chars (name)
            || string_count_chr (name,'@') != 1
            || *name == '@'
            || name[strlen(name)-1] == '@'
            || name[strlen(name)-1] == '.'
            || strstr (name, "..") );
}


const char *
gpa_validate_gpg_email (const char *email)
{
  const char *result = NULL;

  if (!email || !*email)
    ;
  else if (!is_valid_mailbox (email))
    result = _("Email address is not valid.");

  return result;
}


const char *
gpa_validate_gpg_comment (const char *comment)
{
  const char *result = NULL;

  if (!comment || !*comment)
    ;
  else if (strpbrk (comment, "()"))
    result = _("Invalid character in comments.");

  return result;
}

