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

/* The edit callback for all the edit operations is edit_fnc(). Each
 * operation is modelled as a sequential machine (a Moore machine, to be
 * precise). Therefore, for each operation you must write two functions.
 *
 * One of them is "action" or output function, that chooses the right value
 * for *result (the next command issued in the edit command line) based on
 * the current state. The other is the "transit" function, which chooses
 * the next state based on the current state and the input (status code and
 * args).
 *
 * See the comments below for details.
 */

/* Prototype of the action function. Returns the error if there is one */
typedef GpgmeError (*EditAction) (int state, void *opaque,
				  const char **result);
/* Prototype of the transit function. Returns the next state. If and error
 * is found changes *err. If there is no error it should NOT touch it */
typedef int (*EditTransit) (int current_state, GpgmeStatusCode status, 
			    const char *args, void *opaque, GpgmeError *err);

/* Data to be passed to the edit callback. Must be filled by the caller of
 * gpgme_op_edit()  */
struct edit_parms_s
{
  /* Current state */
  int state;
  /* Last error: The return code of gpgme_op_edit() is the return value of
   * the last invocation of the callback. But returning an error from the
   * callback does not abort the edit operation, so we must remember any
   * error. In other words, errors from action() or transit() are sticky.
   */
  GpgmeError err;
  /* The action function */
  EditAction action;
  /* The transit function */
  EditTransit transit;
  /* Data to be passed to the previous functions */
  void *opaque;
};

/* The edit callback proper */
static GpgmeError
edit_fnc (void *opaque, GpgmeStatusCode status, const char *args,
	  const char **result)
{
  struct edit_parms_s *parms = opaque;

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
  parms->state = parms->transit (parms->state, status, args, parms->opaque, 
				 &parms->err);
  if (parms->err == GPGME_No_Error)
    {
      GpgmeError err;
      /* Choose the action based on the state */
      err = parms->action (parms->state, parms->opaque, result);
      if (err != GPGME_No_Error)
	{
	  parms->err = err;
	}
    }
  return parms->err;
}


/* Change expiry time */

typedef enum
{
  EXPIRE_START,
  EXPIRE_COMMAND,
  EXPIRE_DATE,
  EXPIRE_QUIT,
  EXPIRE_SAVE,
  EXPIRE_ERROR
} ExpireState;

static GpgmeError
edit_expire_fnc_action (int state, void *opaque, const char **result)
{
  const gchar *date = opaque;
  
  switch (state)
    {
      /* Start the operation */
    case EXPIRE_COMMAND:
      *result = "expire";
      break;
      /* Send the new expire date */
    case EXPIRE_DATE:
      *result = date;
      break;
      /* End the operation */
    case EXPIRE_QUIT:
      *result = "quit";
      break;
      /* Save */
    case EXPIRE_SAVE:
      *result = "Y";
      break;
      /* Special state: an error ocurred. Do nothing until we can quit */
    case EXPIRE_ERROR:
      break;
      /* Can't happen */
    default:
      return GPGME_General_Error;
    }
  return GPGME_No_Error;
}

static int
edit_expire_fnc_transit (int current_state, GpgmeStatusCode status, 
			 const char *args, void *opaque, GpgmeError *err)
{
  int next_state;
 
  switch (current_state)
    {
    case EXPIRE_START:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = EXPIRE_COMMAND;
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case EXPIRE_COMMAND:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keygen.valid"))
        {
          next_state = EXPIRE_DATE;
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case EXPIRE_DATE:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = EXPIRE_QUIT;
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case EXPIRE_QUIT:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = EXPIRE_SAVE;
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case EXPIRE_ERROR:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = EXPIRE_QUIT;
        }
      else
        {
          next_state = EXPIRE_ERROR;
        }
      break;
    default:
      next_state = EXPIRE_ERROR;
      *err = GPGME_General_Error;
    }
  return next_state;
}


/* Change the key ownertrust */

typedef enum
{
  TRUST_START,
  TRUST_COMMAND,
  TRUST_VALUE,
  TRUST_QUIT,
  TRUST_SAVE,
  TRUST_ERROR
} TrustState;

static GpgmeError
edit_trust_fnc_action (int state, void *opaque, const char **result)
{
  const gchar *trust = opaque;
  
  switch (state)
    {
      /* Start the operation */
    case TRUST_COMMAND:
      *result = "trust";
      break;
      /* Send the new trust date */
    case TRUST_VALUE:
      *result = trust;
      break;
      /* End the operation */
    case TRUST_QUIT:
      *result = "quit";
      break;
      /* Save */
    case TRUST_SAVE:
      *result = "Y";
      break;
      /* Special state: an error ocurred. Do nothing until we can quit */
    case TRUST_ERROR:
      break;
      /* Can't happen */
    default:
      return GPGME_General_Error;
    }
  return GPGME_No_Error;
}

static int
edit_trust_fnc_transit (int current_state, GpgmeStatusCode status, 
			const char *args, void *opaque, GpgmeError *err)
{
  int next_state;

  switch (current_state)
    {
    case TRUST_START:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = TRUST_COMMAND;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case TRUST_COMMAND:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "edit_ownertrust.value"))
        {
          next_state = TRUST_VALUE;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case TRUST_VALUE:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = TRUST_QUIT;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case TRUST_QUIT:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = TRUST_SAVE;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case TRUST_ERROR:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = TRUST_QUIT;
        }
      else
        {
          next_state = TRUST_ERROR;
        }
      break;
    default:
      next_state = TRUST_ERROR;
      *err = GPGME_General_Error;
    }

  return next_state;
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
  gchar *check_level;
  gboolean local;
};

static GpgmeError
edit_sign_fnc_action (int state, void *opaque, const char **result)
{
  struct sign_parms_s *parms = opaque;
  
  switch (state)
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
      return GPGME_General_Error;
    }
  return GPGME_No_Error;
}

static int
edit_sign_fnc_transit (int current_state, GpgmeStatusCode status, 
		       const char *args, void *opaque, GpgmeError *err)
{
  int next_state;

  switch (current_state)
    {
    case SIGN_START:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = SIGN_COMMAND;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case SIGN_COMMAND:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.sign_all.okay"))
        {
          next_state = SIGN_UIDS;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.expire"))
        {
          next_state = SIGN_SET_EXPIRE;
          break;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.class"))
        {
          next_state = SIGN_SET_CHECK_LEVEL;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          next_state = SIGN_ERROR;
          *err = GPGME_Invalid_Key;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case SIGN_UIDS:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "sign_uid.expire"))
        {
          next_state = SIGN_SET_EXPIRE;
          break;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "sign_uid.class"))
        {
          next_state = SIGN_SET_CHECK_LEVEL;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          next_state = SIGN_ERROR;
          *err = GPGME_Invalid_Key;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case SIGN_SET_EXPIRE:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "sign_uid.class"))
        {
          next_state = SIGN_SET_CHECK_LEVEL;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case SIGN_SET_CHECK_LEVEL:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "sign_uid.okay"))
        {
          next_state = SIGN_CONFIRM;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case SIGN_CONFIRM:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = SIGN_QUIT;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case SIGN_QUIT:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = SIGN_SAVE;
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = GPGME_General_Error;
        }
      break;
    case SIGN_ERROR:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = SIGN_QUIT;
        }
      else
        {
          next_state = SIGN_ERROR;
        }
      break;
    default:
      next_state = SIGN_ERROR;
      *err = GPGME_General_Error;
    }

  return next_state;
}


/* Change the ownertrust of a key */
GpgmeError gpa_gpgme_edit_trust (GpgmeKey key, GpgmeValidity ownertrust)
{
  const gchar *trust_strings[] = {"1", "1", "2", "3", "4", "5"};
  struct edit_parms_s parms = {TRUST_START, GPGME_No_Error, 
			       edit_trust_fnc_action, edit_trust_fnc_transit,
			       (char*) trust_strings[ownertrust]};
  GpgmeError err;
  GpgmeData out = NULL;

  err = gpgme_data_new (&out);
  if (err != GPGME_No_Error)
    {
      return err;
    }
  err = gpgme_op_edit (ctx, key, edit_fnc, &parms, out);
  gpgme_data_release (out);
  return err;
}

/* Change the expire date of a key */
GpgmeError gpa_gpgme_edit_expire (GpgmeKey key, GDate *date)
{
  gchar buf[12];
  struct edit_parms_s parms = {EXPIRE_START, GPGME_No_Error, 
			       edit_expire_fnc_action, edit_expire_fnc_transit,
			       buf};
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
  err = gpgme_op_edit (ctx, key, edit_fnc, &parms, out);
  gpgme_data_release (out);
  return err;
}

/* Sign this key with the given private key */
GpgmeError gpa_gpgme_edit_sign (GpgmeKey key, gchar *private_key_fpr,
                                gboolean local)
{
  struct sign_parms_s sign_parms = {"0", local};
  struct edit_parms_s parms = {SIGN_START, GPGME_No_Error,
			       edit_sign_fnc_action, edit_sign_fnc_transit,
			       &sign_parms};
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
  err = gpgme_op_edit (ctx, key, edit_fnc, &parms, out);
  gpgme_data_release (out);
  
  return err;
}
