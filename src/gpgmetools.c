/* gpgmetools.c - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
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

/* A set of auxiliary functions for common tasks related to GPGME */

#include <errno.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpgmetools.h"

/* Report an unexpected error in GPGME and quit the application */
void _gpa_gpgme_error (GpgmeError err, const char *file, int line)
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


/* Write the contents of the GpgmeData object to the file. Receives a
 * filehandle instead of the filename, so that the caller can make sure the
 * file is accesible before putting anything into data.
 */
void dump_data_to_file (GpgmeData data, FILE *file)
{
  char buffer[128];
  int nread;
  GpgmeError err;

  err = gpgme_data_rewind (data);
  if (err != GPGME_No_Error)
    gpa_gpgme_error (err);
  while ( !(err = gpgme_data_read (data, buffer, sizeof(buffer), &nread)) ) 
    {
      fwrite ( buffer, nread, 1, file );
    }
  if (err != GPGME_EOF)
    gpa_gpgme_error (err);
  return;
}

/* Read the contents of the clipboard into the GpgmeData object.
 */
void fill_data_from_clipboard (GpgmeData data, GtkClipboard *clipboard)
{
  GpgmeError err;
  gchar *text = gtk_clipboard_wait_for_text (clipboard);
  if (text)
    {
      err = gpgme_data_write (data, text, strlen (text));
      if (err != GPGME_No_Error)
        {
          gpa_gpgme_error (err);
        }
    }
  g_free (text);
}

/* Write the contents of the GpgmeData into the clipboard
 */
void dump_data_to_clipboard (GpgmeData data, GtkClipboard *clipboard)
{
  char buffer[128];
  int nread;
  GpgmeError err;
  gchar *text = NULL;
  gint len = 0;

  err = gpgme_data_rewind (data);
  if (err != GPGME_No_Error)
    gpa_gpgme_error (err);
  while ( !(err = gpgme_data_read (data, buffer, sizeof(buffer), &nread)) ) 
    {
      text = g_realloc (text, len+nread);
      strncpy (text+len, buffer, nread);
      len += nread;
    }
  if (err != GPGME_EOF)
    gpa_gpgme_error (err);
  gtk_clipboard_set_text (clipboard, text, len);
  g_free (text);
  return;
}

/* Assemble the parameter string for gpgme_op_genkey for GnuPG. We don't need
 * worry about the user ID being UTF-8 as long as we are using GTK+2, because
 * all user input is UTF-8 in it.
 */
static gchar * build_genkey_parms (GPAKeyGenParameters *params)
{
  gchar *string;
  gchar *key_algo;
  gchar *subkeys, *email, *comment, *expire;

  /* Choose which keys and subkeys to generate */
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
  /* Build the extra fields of the user ID if supplied by the user */
  if (params->email && params->email[0])
    {
      email = g_strdup_printf ("Name-Email: %s\n", params->email);
    }
  else
    {
      email = "";
    }
  if (params->comment && params->comment[0])
    {
      comment = g_strdup_printf ("Name-Comment: %s\n", params->comment);
    }
  else
    {
      comment = "";
    }
  /* Build the expiration date string if needed */
  if (params->expiryDate)
    {
      expire = g_strdup_printf ("Expire-Date: %i-%i-%i\n", 
                                g_date_get_year(params->expiryDate),
                                g_date_get_month(params->expiryDate),
                                g_date_get_day(params->expiryDate));
    }
  else if (params->interval)
    {
      expire = g_strdup_printf ("Expire-Date: %i%c\n", params->interval,
                                params->unit);
    }
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
    {
      g_free (subkeys);
    }
  if (email[0])
    {
      g_free (email);
    }
  if (comment[0])
    {
      g_free (comment);
    }
  if (expire[0])
    {
      g_free (expire);
    }
  
  return string;
}

/* Generate a key with the given parameters. It prepares the parameters
 * required by Gpgme and returns whatever gpgme_op_genkey returns.
 */
GpgmeError gpa_generate_key (GPAKeyGenParameters *params)
{
  gchar *parm_string;
  GpgmeError err;

  parm_string = build_genkey_parms (params);
  err = gpgme_op_genkey (ctx, parm_string, NULL, NULL);
  g_free (parm_string);

  return err;
}

struct parse_engine_info_s
{
  GQueue *tag_stack;
  gboolean in_openpgp;
  gchar *path;
};

void parse_engine_info_start (GMarkupParseContext *context,
			      const gchar         *element_name,
			      const gchar        **attribute_names,
			      const gchar        **attribute_values,
			      gpointer             user_data,
			      GError             **error)
{
  struct parse_engine_info_s *data = user_data;
  g_queue_push_head (data->tag_stack, (gpointer) element_name);
}

void parse_engine_info_end (GMarkupParseContext *context,
			    const gchar         *element_name,
			    gpointer             user_data,
			    GError             **error)
{
  struct parse_engine_info_s *data = user_data;
  g_queue_pop_head (data->tag_stack);
}

void parse_engine_info_text (GMarkupParseContext *context,
			     const gchar         *text,
			     gsize                text_len,  
			     gpointer             user_data,
			     GError             **error)
{
  struct parse_engine_info_s *data = user_data;
  if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack), "protocol"))
    {
      if (g_str_equal (text, "OpenPGP"))
	{
	  data->in_openpgp = TRUE;
	}
      else
	{
	  data->in_openpgp = FALSE;
	}
    }
  else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack), "path") 
	   && data->in_openpgp)
    {
      data->path = g_strdup (text);
    }
}

/* Find the path to the gpg executable from the gpgme engine information. */
static gchar *
find_gpg_executable (void)
{
  GMarkupParser parser = 
    {
      parse_engine_info_start,
      parse_engine_info_end,
      parse_engine_info_text,
      NULL, NULL
    };
  struct parse_engine_info_s data = {g_queue_new(), FALSE, NULL};
  const gchar *engine_info = gpgme_get_engine_info ();
  GMarkupParseContext* context = g_markup_parse_context_new (&parser, 0,
							     &data, NULL);
  g_markup_parse_context_parse (context, engine_info, strlen (engine_info),
				NULL);
  g_markup_parse_context_free (context);
  return data.path;
}

/* Backup a key. It exports both the public and secret keys to a file.
 * Returns TRUE on success and FALSE on error. It displays errors to the
 * user.
 */
gboolean gpa_backup_key (const gchar *fpr, const char *filename)
{
  gchar *header, *pub_key, *sec_key;
  gchar *err;
  FILE *file;
  gint ret_code;
  gchar *gpg = find_gpg_executable();
  gchar *header_argv[] = 
    {
      gpg, "--batch", "--no-tty", "--fingerprint", fpr, NULL
    };
  gchar *pub_argv[] = 
    {
      gpg, "--batch", "--no-tty", "--armor", "--export", fpr, NULL
    };
  gchar *sec_argv[] = 
    {
      gpg, "--batch", "--no-tty", "--armor", "--export-secret-key", fpr, NULL
    };

  /* Open the file */
  file = fopen (filename, "w");
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
  free (params->userID);
  free (params->email);
  free (params->comment);
  if (params->expiryDate)
    g_date_free (params->expiryDate);
  free (params);
}

GPAKeyGenParameters *
key_gen_params_new(void)
{
  GPAKeyGenParameters * params = xmalloc (sizeof (*params));
  params->userID = NULL;
  params->email = NULL;
  params->comment = NULL;
  params->expiryDate = NULL;
  params->interval = 0;
  params->generate_revocation = FALSE;
  params->send_to_server = FALSE;
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

static GtkWidget *passphrase_question_label (const gchar *desc)
{
  GtkWidget *label;
  gchar *input;
  gchar *text;
  gchar *keyid, *userid, *action;
  gint i;
  input = g_strdup (desc);
  /* The first line tells us whether this is the first time we ask for a
   * passphrase or not. */
  action = input;
  for (i = 0; input[i] != '\n'; i++)
    {
    }
  input[i++] = '\0';
  /* The first word in the second line is the key ID */
  keyid = input+i;
  for (i = 0; input[i] != ' '; i++)
    {
    }
  input[i++] = '\0';
  /* The rest of the line is the user ID */
  userid = input+i;
  for (i = 0; input[i] != '\n'; i++)
    {
    }
  input[i++] = '\0';
  /* Build the label widget */
  if (g_str_equal (action, "ENTER"))
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

/* This is the function called by GPGME when it wants a passphrase */
const char * gpa_passphrase_cb (void *opaque, const char *desc, void **r_hd)
{
  GtkWidget * dialog;
  GtkWidget * hbox;
  GtkWidget * vbox;
  GtkWidget * entry;
  GtkWidget * pixmap;
  GtkResponseType response;

  if (desc)
    {
      dialog = gtk_dialog_new_with_buttons (_("Enter Passphrase"),
                                            NULL, GTK_DIALOG_MODAL,
                                            GTK_STOCK_OK,
                                            GTK_RESPONSE_OK,
                                            GTK_STOCK_CANCEL,
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
                                   passphrase_question_label (desc));
      entry = gtk_entry_new ();
      gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, FALSE, 10);
      gtk_widget_grab_focus (entry);
      /* Run the dialog */
      gtk_widget_show_all (dialog);
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_hide (dialog);
      if (response == GTK_RESPONSE_OK)
        {
          return gtk_entry_get_text (GTK_ENTRY (entry));
        }
      else
        {
          gpgme_cancel (ctx);
          return "";
        }
    }
  else
    {
      /* Free the dialog */
      dialog = *r_hd;
      gtk_widget_destroy (dialog);
      return NULL;
    }
}


/*
 * Edit functions. These are wrappers around the experimental gpgme key edit
 * interface.
 */

/* These are the edit callbacks. Each of them can be modelled as a sequential
 * machine: it has a state, an input (the status and it's args) and an output
 * (the result argument). In each state, we check the input and then issue a
 * command and/or change state for the next invocation. */

/* Change expiry time */

struct expiry_parms_s
{
  int state;
  const char *date;
};

static GpgmeError
edit_expiry_fnc (void *opaque, GpgmeStatusCode status, const char *args,
                 const char **result)
{
  struct expiry_parms_s *parms = opaque;

  /* Ignore these status lines */
  if (status == GPGME_STATUS_EOF ||
      status == GPGME_STATUS_GOT_IT ||
      status == GPGME_STATUS_NEED_PASSPHRASE ||
      status == GPGME_STATUS_GOOD_PASSPHRASE)
    {
      return GPGME_No_Error;
    }

  switch (parms->state)
    {
      /* State 0: start the operation */
    case 0:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          *result = "expire";
          parms->state = 1;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
      /* State 1: send the new expiration date */
    case 1:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keygen.valid"))
        {
          *result = parms->date;
          parms->state = 2;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
      /* State 2: gpg tells us the user ID we are about to edit */
    case 2:
      if (status == GPGME_STATUS_USERID_HINT)
        {
          parms->state = 3;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
      /* State 3: end the edit operation */
    case 3:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          *result = "quit";
          parms->state = 4;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
      /* State 4: Save */
    case 4:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.save.okay"))
        {
          *result = "Y";
          parms->state = 5;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
    default:
      /* Can't happen */
      return GPGME_General_Error;
      break;
    }
  return GPGME_No_Error;
}

/* Change the key ownertrust */

struct ownertrust_parms_s
{
  int state;
  const char *trust;
};

static GpgmeError
edit_ownertrust_fnc (void *opaque, GpgmeStatusCode status, const char *args,
                 const char **result)
{
  struct ownertrust_parms_s *parms = opaque;

  /* Ignore these status lines */
  if (status == GPGME_STATUS_EOF ||
      status == GPGME_STATUS_GOT_IT ||
      status == GPGME_STATUS_NEED_PASSPHRASE ||
      status == GPGME_STATUS_GOOD_PASSPHRASE)
    {
      return GPGME_No_Error;
    }

  switch (parms->state)
    {
      /* State 0: start the operation */
    case 0:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          *result = "trust";
          parms->state = 1;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
      /* State 1: send the new ownertrust value */
    case 1:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "edit_ownertrust.value"))
        {
          *result = parms->trust;
          parms->state = 2;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
      /* State 2: end the edit operation */
    case 2:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          *result = "quit";
          parms->state = 3;
        }
      else
        {
          return GPGME_General_Error;
        }
      break;
    default:
      /* Can't happen */
      return GPGME_General_Error;
      break;
    }
  return GPGME_No_Error;
}

/* Sign a key */

struct sign_parms_s
{
  int state;
  gchar *check_level;
  gboolean local;
  GpgmeError err;
};

static GpgmeError
edit_sign_fnc_action (struct sign_parms_s *parms, const char **result)
{
  switch (parms->state)
    {
      /* Start the operation */
    case 1:
      *result = parms->local ? "lsign" : "sign";
      break;
      /* Sign all user ID's */
    case 2:
      *result = "Y";
      break;
      /* The signature expires at the same time as the key */
    case 3:
      *result = "Y";
      break;
      /* Set the check level on the key */
    case 4:
      *result = parms->check_level;
      break;
      /* Confirm the signature */
    case 5:
      *result = "Y";
      break;
      /* End the operation */
    case 6:
      *result = "quit";
      break;
      /* Save */
    case 7:
      *result = "Y";
      break;
      /* Special state: an error ocurred. Do nothing until we can quit */
    case 8:
      break;
      /* Can't happen */
    default:
      parms->err = GPGME_General_Error;
    }
  return parms->err;
}

static void
edit_sign_fnc_transit (struct sign_parms_s *parms, GpgmeStatusCode status, 
                       const char *args)
{
  switch (parms->state)
    {
    case 0:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          parms->state = 1;
        }
      else if (status == GPGME_STATUS_KEYEXPIRED)
        {
          /* Skip every "signature by an expired key" status line */
          parms->state = 0;
        }
      else
        {
          parms->state = 8;
          parms->err = GPGME_General_Error;
        }
      break;
    case 1:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.sign_all.okay"))
        {
          parms->state = 2;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.expire"))
        {
          parms->state = 3;
          break;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.class"))
        {
          parms->state = 4;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          parms->state = 8;
          parms->err = GPGME_Invalid_Key;
        }
      else
        {
          parms->state = 8;
          parms->err = GPGME_General_Error;
        }
      break;
    case 2:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "sign_uid.expire"))
        {
          parms->state = 3;
          break;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.class"))
        {
          parms->state = 4;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          parms->state = 8;
          parms->err = GPGME_Invalid_Key;
        }
      else
        {
          parms->state = 8;
          parms->err = GPGME_General_Error;
        }
      break;
    case 3:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "sign_uid.class"))
        {
          parms->state = 4;
        }
      else
        {
          parms->state = 8;
          parms->err = GPGME_General_Error;
        }
      break;
    case 4:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "sign_uid.okay"))
        {
          parms->state = 5;
        }
      else
        {
          parms->state = 8;
          parms->err = GPGME_General_Error;
        }
      break;
    case 5:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          parms->state = 6;
        }
      else
        {
          parms->state = 8;
          parms->err = GPGME_General_Error;
        }
      break;
    case 6:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.save.okay"))
        {
          parms->state = 7;
        }
      else
        {
          parms->state = 8;
          parms->err = GPGME_General_Error;
        }
      break;
    case 8:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          parms->state = 6;
        }
      else
        {
          parms->state = 8;
        }
      break;
    default:
      parms->state = 8;
      parms->err = GPGME_General_Error;
    }
}

static GpgmeError
edit_sign_fnc (void *opaque, GpgmeStatusCode status, const char *args,
               const char **result)
{
  struct sign_parms_s *parms = opaque;

  /* Ignore these status lines, as they don't require any response */
  if (status == GPGME_STATUS_EOF ||
      status == GPGME_STATUS_GOT_IT ||
      status == GPGME_STATUS_NEED_PASSPHRASE ||
      status == GPGME_STATUS_GOOD_PASSPHRASE ||
      status == GPGME_STATUS_USERID_HINT ||
      status == GPGME_STATUS_SIGEXPIRED)
    {
      return parms->err;
    }

  /* Choose the next state based on the current one and the input */
  edit_sign_fnc_transit (parms, status, args);

  /* Choose the action based on the state */
  return edit_sign_fnc_action (parms, result);
}

/* Change the ownertrust of a key */
GpgmeError gpa_gpgme_edit_ownertrust (GpgmeKey key, GpgmeValidity ownertrust)
{
  const gchar *trust_strings[] = {"1", "1", "2", "3", "4", "5"};
  struct expiry_parms_s parms = {0, trust_strings[ownertrust]};
  GpgmeError err;
  GpgmeData out = NULL;

  err = gpgme_data_new (&out);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  err = gpgme_op_edit (ctx, key, edit_ownertrust_fnc, &parms, out);
  gpgme_data_release (out);
  return err;
}

/* Change the expiry date of a key */
GpgmeError gpa_gpgme_edit_expiry (GpgmeKey key, GDate *date)
{
  gchar buf[12];
  struct expiry_parms_s parms = {0, buf};
  GpgmeError err;
  GpgmeData out = NULL;

  err = gpgme_data_new (&out);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  /* The new expiration date */
  if (date)
    {
      g_date_strftime (buf, sizeof(buf), "%F", date);
    }
  else
    {
      strncpy (buf, "0", sizeof (buf));
    }
  err = gpgme_op_edit (ctx, key, edit_expiry_fnc, &parms, out);
  gpgme_data_release (out);
  return err;
}

/* Sign this key with the given private key */
GpgmeError gpa_gpgme_edit_sign (GpgmeKey key, gchar *private_key_fpr,
                                gboolean local)
{
  struct sign_parms_s parms = {0, "0", local, GPGME_No_Error};
  GpgmeKey secret_key = gpa_keytable_secret_lookup (keytable, private_key_fpr);
  GpgmeError err = GPGME_No_Error;
  GpgmeData out;

  err = gpgme_data_new (&out);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }  
  gpgme_signers_clear (ctx);
  gpgme_key_ref (secret_key);
  err = gpgme_signers_add (ctx, secret_key);
  if (err != GPGME_No_Error)
    {
      return err;
    }
  err = gpgme_op_edit (ctx, key, edit_sign_fnc, &parms, out);
  gpgme_data_release (out);
  
  return err;
}
