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

#include "gpgmeedit.h"

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


typedef enum
{
  SIGN_START,
  SIGN_COMMAND,
  SIGN_UIDS,
  SIGN_SET_EXPIRE,
  SIGN_SET_CHECK_LEVEL,
  SIGN_CONFIRM,
  SIGN_QUIT,
  SIGN_SAVE,
  SIGN_ERROR
} SignState;

struct sign_parms_s
{
  SignState state;
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
    case SIGN_COMMAND:
      *result = parms->local ? "lsign" : "sign";
      break;
      /* Sign all user ID's */
    case SIGN_UIDS:
      *result = "Y";
      break;
      /* The signature expires at the same time as the key */
    case SIGN_SET_EXPIRE:
      *result = "Y";
      break;
      /* Set the check level on the key */
    case SIGN_SET_CHECK_LEVEL:
      *result = parms->check_level;
      break;
      /* Confirm the signature */
    case SIGN_CONFIRM:
      *result = "Y";
      break;
      /* End the operation */
    case SIGN_QUIT:
      *result = "quit";
      break;
      /* Save */
    case SIGN_SAVE:
      *result = "Y";
      break;
      /* Special state: an error ocurred. Do nothing until we can quit */
    case SIGN_ERROR:
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
    case SIGN_START:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          parms->state = SIGN_COMMAND;
        }
      else
        {
          parms->state = SIGN_ERROR;
          parms->err = GPGME_General_Error;
        }
      break;
    case SIGN_COMMAND:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.sign_all.okay"))
        {
          parms->state = SIGN_UIDS;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.expire"))
        {
          parms->state = SIGN_SET_EXPIRE;
          break;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.class"))
        {
          parms->state = SIGN_SET_CHECK_LEVEL;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          parms->state = SIGN_ERROR;
          parms->err = GPGME_Invalid_Key;
        }
      else
        {
          parms->state = SIGN_ERROR;
          parms->err = GPGME_General_Error;
        }
      break;
    case SIGN_UIDS:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "sign_uid.expire"))
        {
          parms->state = SIGN_SET_EXPIRE;
          break;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.class"))
        {
          parms->state = SIGN_SET_CHECK_LEVEL;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          parms->state = SIGN_ERROR;
          parms->err = GPGME_Invalid_Key;
        }
      else
        {
          parms->state = SIGN_ERROR;
          parms->err = GPGME_General_Error;
        }
      break;
    case SIGN_SET_EXPIRE:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "sign_uid.class"))
        {
          parms->state = SIGN_SET_CHECK_LEVEL;
        }
      else
        {
          parms->state = SIGN_ERROR;
          parms->err = GPGME_General_Error;
        }
      break;
    case SIGN_SET_CHECK_LEVEL:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "sign_uid.okay"))
        {
          parms->state = SIGN_CONFIRM;
        }
      else
        {
          parms->state = SIGN_ERROR;
          parms->err = GPGME_General_Error;
        }
      break;
    case SIGN_CONFIRM:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          parms->state = SIGN_QUIT;
        }
      else
        {
          parms->state = SIGN_ERROR;
          parms->err = GPGME_General_Error;
        }
      break;
    case SIGN_QUIT:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.save.okay"))
        {
          parms->state = SIGN_SAVE;
        }
      else
        {
          parms->state = SIGN_ERROR;
          parms->err = GPGME_General_Error;
        }
      break;
    case SIGN_ERROR:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          parms->state = SIGN_QUIT;
        }
      else
        {
          parms->state = SIGN_ERROR;
        }
      break;
    default:
      parms->state = SIGN_ERROR;
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
      status == GPGME_STATUS_BAD_PASSPHRASE ||
      status == GPGME_STATUS_USERID_HINT ||
      status == GPGME_STATUS_SIGEXPIRED ||
      status == GPGME_STATUS_KEYEXPIRED)
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
      return err;
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
      return err;
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
  struct sign_parms_s parms = {SIGN_START, "0", local, GPGME_No_Error};
  GpgmeKey secret_key = gpa_keytable_secret_lookup (keytable, private_key_fpr);
  GpgmeError err = GPGME_No_Error;
  GpgmeData out;

  err = gpgme_data_new (&out);
  if (err != GPGME_No_Error)
    {
      return err;
    }  
  gpgme_signers_clear (ctx);
  err = gpgme_signers_add (ctx, secret_key);
  if (err != GPGME_No_Error)
    {
      return err;
    }
  err = gpgme_op_edit (ctx, key, edit_sign_fnc, &parms, out);
  gpgme_data_release (out);
  
  return err;
}
