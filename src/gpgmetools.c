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

#include "gpa.h"
#include "gpgmetools.h"

/* Report an unexpected error in GPGME and quit the application */
void _gpa_gpgme_error (GpgmeError err, const char *file, int line)
{
  gchar *message = g_strdup_printf (_("Fatal Error in GPGME library\n"
                                      "(invoked from file %s, line %i):\n\n"
                                      "\t%s\n\n"
                                      "The application will be terminated"),
                                    file, line, gpgme_strerror (err));
  GtkWidget *label = gtk_label_new (message);
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Error"),
                                                   NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_STOCK_OK,
                                                   GTK_RESPONSE_NONE,
                                                   NULL);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG(dialog));
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
      text = g_strdup_printf ("%s\n%s %s\n%s %s",
                              _("Please enter the passphrase for"
                                " the following key:"),
                              _("User Name:"), userid,
                              _("Key ID:"), keyid);
    }
  else
    {
      text = g_strdup_printf ("%s\n%s %s\n%s %s",
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
  GtkWidget * vbox;
  GtkWidget * entry;
  GtkWidget * action_area;
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
      vbox = GTK_DIALOG (dialog)->vbox;
      action_area = GTK_DIALOG (dialog)->action_area;
      /* The default button is OK */
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      /* Set the contents of the dialog */
      gtk_box_pack_start_defaults (GTK_BOX (vbox),
                                   passphrase_question_label (desc));
      entry = gtk_entry_new ();
      gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
      gtk_box_pack_start_defaults (GTK_BOX (vbox), entry);
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

