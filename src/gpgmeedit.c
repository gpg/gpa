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

#include <config.h>

#include "gpgmeedit.h"
#include "passwddlg.h"

#include <unistd.h>


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

/* Define this macro to 1 to enable debugging of the FSM. */
#define DEBUG_FSM 0


/* Prototype of the action function. Returns the error if there is one */
typedef gpg_error_t (*EditAction) (int state, void *opaque,
				     char **result);
/* Prototype of the transit function. Returns the next state. If and error
 * is found changes *err. If there is no error it should NOT touch it */
typedef int (*EditTransit) (int current_state, gpgme_status_code_t status, 
			    const char *args, void *opaque, gpg_error_t *err);

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
  gpg_error_t err;
  /* The action function */
  EditAction action;
  /* The transit function */
  EditTransit transit;
  /* The output data object */
  gpgme_data_t out;
  /* Signal attachment id */
  gulong signal_id;
  /* Data to be passed to the previous functions */
  void *opaque;
  /* The passwd dialog needs to see GPGME_STATUS_NEED_PASSPHRASE_SYM.
     To make thinks easier it is only passed to the FSM if this flag
     has been set. */
  int need_status_passphrase_sym;
};

/* The edit callback proper */
static gpg_error_t
edit_fnc (void *opaque, gpgme_status_code_t status,
	  const char *args, int fd)
{
  struct edit_parms_s *parms = opaque;
  char *result = NULL;

  /* Ignore these status lines, as they don't require any response */
  /* FIXME: should GPGME_STATUS_CARDCTRL be included here as well? -mo */
  if (status == GPGME_STATUS_EOF 
      || status == GPGME_STATUS_GOT_IT
      || status == GPGME_STATUS_NEED_PASSPHRASE
      || status == GPGME_STATUS_NEED_PASSPHRASE_SYM
      || status == GPGME_STATUS_GOOD_PASSPHRASE
      || status == GPGME_STATUS_BAD_PASSPHRASE
      || status == GPGME_STATUS_USERID_HINT
      || status == GPGME_STATUS_SIGEXPIRED
      || status == GPGME_STATUS_KEYEXPIRED)
    {
      return parms->err;
    }
  else if (!parms->need_status_passphrase_sym
           && status == GPGME_STATUS_NEED_PASSPHRASE_SYM)
    {
      return parms->err;
    }

#if DEBUG_FSM
  g_debug ("edit_fnc: state=%d input=%d (%s)", parms->state, status, args);
#endif
  /* Choose the next state based on the current one and the input */
  parms->state = parms->transit (parms->state, status, args, parms->opaque, 
				 &parms->err);
  if (gpg_err_code (parms->err) == GPG_ERR_NO_ERROR)
    {
      gpg_error_t err;
      /* Choose the action based on the state */
      err = parms->action (parms->state, parms->opaque, &result);
#if DEBUG_FSM
      g_debug ("edit_fnc: newstate=%d err=%s result=%s",
               parms->state, gpg_strerror (err), result);
#endif
      if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
	{
	  parms->err = err;
	}
      /* Send the command, if any was provided */
      if (result)
	{
          if (*result)
            write (fd, result, strlen (result));
	  write (fd, "\n", 1);
	}
    }
  else
    {
#if DEBUG_FSM
      g_debug ("edit_fnc: newstate=%d err=%s transit failed",
               parms->state, gpg_strerror (parms->err));
#endif
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

static gpg_error_t
edit_expire_fnc_action (int state, void *opaque, char **result)
{
  gchar *date = opaque;
  
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
      return gpg_error (GPG_ERR_GENERAL);
    }
  return gpg_error (GPG_ERR_NO_ERROR);
}

static int
edit_expire_fnc_transit (int current_state, gpgme_status_code_t status, 
			 const char *args, void *opaque, gpg_error_t *err)
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
          *err =  gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case EXPIRE_DATE:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = EXPIRE_QUIT;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
               g_str_equal (args, "keygen.valid"))
        {
          next_state = EXPIRE_ERROR;
          *err = gpg_error (GPG_ERR_INV_TIME);
        }
      else
        {
          next_state = EXPIRE_ERROR;
          *err =  gpg_error (GPG_ERR_GENERAL);
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
      *err = gpg_error (GPG_ERR_GENERAL);
    }
  return next_state;
}


/* Change the key ownertrust */

typedef enum
{
  TRUST_START,
  TRUST_COMMAND,
  TRUST_VALUE,
  TRUST_REALLY_ULTIMATE,
  TRUST_QUIT,
  TRUST_SAVE,
  TRUST_ERROR
} TrustState;

static gpg_error_t
edit_trust_fnc_action (int state, void *opaque, char **result)
{
  gchar *trust = opaque;
  
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
      /* Really set to ultimate trust */
    case TRUST_REALLY_ULTIMATE:
      *result = "Y";
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
      return gpg_error (GPG_ERR_GENERAL);
    }
  return gpg_error (GPG_ERR_NO_ERROR);
}

static int
edit_trust_fnc_transit (int current_state, gpgme_status_code_t status, 
			const char *args, void *opaque, gpg_error_t *err)
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case TRUST_VALUE:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = TRUST_QUIT;
        }
      else if (status == GPGME_STATUS_GET_BOOL &&
               g_str_equal (args, "edit_ownertrust.set_ultimate.okay"))
        {
          next_state = TRUST_REALLY_ULTIMATE;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case TRUST_REALLY_ULTIMATE:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = TRUST_QUIT;
        }
      else
        {
          next_state = TRUST_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
      *err = gpg_error (GPG_ERR_GENERAL);
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

static gpg_error_t
edit_sign_fnc_action (int state, void *opaque, char **result)
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
      return gpg_error (GPG_ERR_GENERAL);
    }
  return gpg_error (GPG_ERR_NO_ERROR);
}

static int
edit_sign_fnc_transit (int current_state, gpgme_status_code_t status, 
		       const char *args, void *opaque, gpg_error_t *err)
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
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case SIGN_COMMAND:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.sign_all.okay"))
        {
          next_state = SIGN_UIDS;
        }
      else if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "sign_uid.okay"))
        {
          next_state = SIGN_CONFIRM;
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
      else if (status == GPGME_STATUS_ALREADY_SIGNED)
        {
          /* The key has already been signed with this key */
          next_state = SIGN_ERROR;
          *err =  gpg_error (GPG_ERR_CONFLICT);
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_UNUSABLE_PUBKEY);
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
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
      else if (status == GPGME_STATUS_GET_BOOL &&
               g_str_equal (args, "sign_uid.okay"))
        {
          next_state = SIGN_CONFIRM;
        }
      else if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Failed sign: expired key */
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_UNUSABLE_PUBKEY);
        }
      else
        {
          next_state = SIGN_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
          *err = gpg_error (GPG_ERR_GENERAL);
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
      *err = gpg_error (GPG_ERR_GENERAL);
    }

  return next_state;
}

/* Change passphrase */

typedef enum
{
  PASSWD_START,
  PASSWD_COMMAND,
  PASSWD_ENTERNEW,
  PASSWD_QUIT,
  PASSWD_SAVE,
  PASSWD_ERROR
} PasswdState;

struct passwd_parms_s 
{
  gpgme_passphrase_cb_t func;
  void *opaque;
};

static gpg_error_t
edit_passwd_fnc_action (int state, void *opaque, char **result)
{
  switch (state)
    {
    case PASSWD_COMMAND:
      /* Start the operation */
      *result = "passwd";
      break;
    case PASSWD_ENTERNEW:
      /* No action required.  This state is only used by the
         passphrase callback.  */
      break;
    case PASSWD_QUIT:
      /* End the operation */
      *result = "quit";
      break;
    case PASSWD_SAVE:
      /* Save */
      *result = "Y";
      break;
    case PASSWD_ERROR:
      /* Special state: an error ocurred. Do nothing until we can quit */
      break;
    default:
      /* Can't happen */
      return gpg_error (GPG_ERR_GENERAL);
    }
  return gpg_error (GPG_ERR_NO_ERROR);
}

static int
edit_passwd_fnc_transit (int current_state, gpgme_status_code_t status, 
			 const char *args, void *opaque, gpg_error_t *err)
{
  int next_state;
 
  switch (current_state)
    {
    case PASSWD_START:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = PASSWD_COMMAND;
        }
      else
        {
          next_state = PASSWD_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case PASSWD_COMMAND:
    case PASSWD_ENTERNEW:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          next_state = PASSWD_QUIT;
        }
      else if (status == GPGME_STATUS_NEED_PASSPHRASE_SYM)
        {
          next_state = PASSWD_ENTERNEW;
        }
      else
        {
          next_state = PASSWD_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case PASSWD_QUIT:
      if (status == GPGME_STATUS_GET_BOOL &&
          g_str_equal (args, "keyedit.save.okay"))
        {
          next_state = PASSWD_SAVE;
        }
      else
        {
          next_state = PASSWD_ERROR;
          *err = gpg_error (GPG_ERR_GENERAL);
        }
      break;
    case PASSWD_ERROR:
      if (status == GPGME_STATUS_GET_LINE &&
          g_str_equal (args, "keyedit.prompt"))
        {
          /* Go to quit operation state */
          next_state = PASSWD_QUIT;
        }
      else
        {
          next_state = PASSWD_ERROR;
        }
      break;
    default:
      next_state = PASSWD_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
    }
  return next_state;
}

/* Release the edit parameters needed for setting owner trust. The prototype
 * is that of a GpaContext's "done" signal handler.
 */
static void
gpa_gpgme_edit_trust_parms_release (GpaContext *ctx, gpg_error_t err,
				   struct edit_parms_s* parms)
{
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    {
      /* Don't run this signal handler again if the context is reused */
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  g_free (parms->opaque);
  g_free (parms);
}

/* Generate the edit parameters needed for setting owner trust.
 */
static struct edit_parms_s*
gpa_gpgme_edit_trust_parms_new (GpaContext *ctx, const char *trust_string,
				gpgme_data_t out)
{
  struct edit_parms_s *edit_parms = g_malloc (sizeof (struct edit_parms_s));

  edit_parms->state = TRUST_START;
  edit_parms->err = gpg_error (GPG_ERR_NO_ERROR);
  edit_parms->action = edit_trust_fnc_action;
  edit_parms->transit = edit_trust_fnc_transit;
  edit_parms->signal_id = 0;
  edit_parms->out = out;
  edit_parms->opaque = g_strdup (trust_string);

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id =
    g_signal_connect (G_OBJECT (ctx), "done", 
		      G_CALLBACK (gpa_gpgme_edit_trust_parms_release),
		      edit_parms);

  return edit_parms;
}

/* Change the ownertrust of a key */
gpg_error_t gpa_gpgme_edit_trust_start (GpaContext *ctx, gpgme_key_t key,
					gpgme_validity_t ownertrust)
{
  const gchar *trust_strings[] = {"1", "1", "2", "3", "4", "5"};
  struct edit_parms_s *parms = NULL;
  gpg_error_t err;
  gpgme_data_t out = NULL;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }
  parms = gpa_gpgme_edit_trust_parms_new (ctx, trust_strings[ownertrust], out);
  err = gpgme_op_edit_start (ctx->ctx, key, edit_fnc, parms, out);
  return err;
}

/* Release the edit parameters needed for changing the expiry date. The 
 * prototype is that of a GpaContext's "done" signal handler.
 */
static void
gpa_gpgme_edit_expire_parms_release (GpaContext *ctx, gpg_error_t err,
				     struct edit_parms_s* parms)
{
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    {
      /* Don't run this signal handler again if the context is reused */
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  g_free (parms->opaque);
  g_free (parms);
}

/* Generate the edit parameters needed for changing the expiry date.
 */
static struct edit_parms_s*
gpa_gpgme_edit_expire_parms_new (GpaContext *ctx, GDate *date,
				 gpgme_data_t out)
{
  struct edit_parms_s *edit_parms = g_malloc (sizeof (struct edit_parms_s));
  int buf_len = 12;
  gchar *buf = g_malloc (buf_len);

  edit_parms->state = EXPIRE_START;
  edit_parms->err = gpg_error (GPG_ERR_NO_ERROR);
  edit_parms->action = edit_expire_fnc_action;
  edit_parms->transit = edit_expire_fnc_transit;
  edit_parms->signal_id = 0;
  edit_parms->out = out;
  edit_parms->opaque = buf;

  /* The new expiration date */
  if (date)
    {
      g_date_strftime (buf, buf_len, "%Y-%m-%d", date);
    }
  else
    {
      strncpy (buf, "0", buf_len);
    }

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id = 
    g_signal_connect (G_OBJECT (ctx), "done", 
		      G_CALLBACK (gpa_gpgme_edit_expire_parms_release),
		      edit_parms);

  return edit_parms;
}

/* Change the expire date of a key */
gpg_error_t gpa_gpgme_edit_expire_start (GpaContext *ctx, gpgme_key_t key,
					 GDate *date)
{
  struct edit_parms_s *parms;
  gpg_error_t err;
  gpgme_data_t out = NULL;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }

  parms = gpa_gpgme_edit_expire_parms_new (ctx, date, out);
  err = gpgme_op_edit_start (ctx->ctx, key, edit_fnc, parms, out);
  return err;
}

/* Release the edit parameters needed for signing. The prototype is that of
 * a GpaContext's "done" signal handler.
 */
static void
gpa_gpgme_edit_sign_parms_release (GpaContext *ctx, gpg_error_t err,
				   struct edit_parms_s* parms)
{
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    {
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  g_free (parms->opaque);
  g_free (parms);
}

/* Generate the edit parameters needed for signing 
 */
static struct edit_parms_s*
gpa_gpgme_edit_sign_parms_new (GpaContext *ctx, char *check_level,
			       gboolean local, gpgme_data_t out)
{
  struct sign_parms_s *sign_parms = g_malloc (sizeof (struct sign_parms_s));
  struct edit_parms_s *edit_parms = g_malloc (sizeof (struct edit_parms_s));

  edit_parms->state = SIGN_START;
  edit_parms->err = gpg_error (GPG_ERR_NO_ERROR);
  edit_parms->action = edit_sign_fnc_action;
  edit_parms->transit = edit_sign_fnc_transit;
  edit_parms->signal_id = 0;
  edit_parms->out = out;
  edit_parms->opaque = sign_parms;
  sign_parms->check_level = check_level;
  sign_parms->local = local;

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id =
    g_signal_connect (G_OBJECT (ctx), "done", 
		      G_CALLBACK (gpa_gpgme_edit_sign_parms_release),
		      edit_parms);

  return edit_parms;
}

/* Sign this key with the given private key */
gpg_error_t gpa_gpgme_edit_sign_start (GpaContext *ctx, gpgme_key_t key,
				       gpgme_key_t secret_key, gboolean local)
{
  struct edit_parms_s *parms = NULL;
  gpg_error_t err = gpg_error (GPG_ERR_NO_ERROR);
  gpgme_data_t out;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }  
  gpgme_signers_clear (ctx->ctx);
  err = gpgme_signers_add (ctx->ctx, secret_key);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }
  parms = gpa_gpgme_edit_sign_parms_new (ctx, "0", local, out);
  err = gpgme_op_edit_start (ctx->ctx, key, edit_fnc, parms, out);

  return err;
}


/*
 * Change the passphrase of the key.
 */

/* Special passphrase callback for use within the passwd command */
gpg_error_t 
passwd_passphrase_cb (void *hook, const char *uid_hint,
                      const char *passphrase_info, 
                      int prev_was_bad, int fd)
{
  struct edit_parms_s *parms = hook;
  
  if (parms->state == PASSWD_ENTERNEW)
    {
      /* Gpg is asking for the new passphrase.  Run the dialog to
         enter and re-enter the new passphrase.  */
      return gpa_change_passphrase_dialog_run (hook, uid_hint, 
					       passphrase_info, 
					       prev_was_bad, fd);
    }
  else
    {
      /* Gpg is asking for the passphrase to unprotect the key.  */
      return gpa_passphrase_cb (hook, uid_hint, passphrase_info, 
				prev_was_bad, fd);
    }
}


/* Release the edit parameters needed for changing the passphrase. The 
 * prototype is that of a GpaContext's "done" signal handler.
 */
static void
gpa_gpgme_edit_passwd_parms_release (GpaContext *ctx, gpg_error_t err,
				     struct edit_parms_s* parms)
{
  struct passwd_parms_s *passwd_parms = parms->opaque;

  gpgme_data_release (parms->out);

  /* Make sure the normal passphrase callback is used */
  if (!cms_hack)
    gpgme_set_passphrase_cb (ctx->ctx,
                             passwd_parms->func, passwd_parms->opaque);

  if (parms->signal_id != 0)
    {
      /* Don't run this signal handler again if the context is reused */
      g_signal_handler_disconnect (ctx, parms->signal_id);
    }
  
  g_free (parms);
  g_free (parms->opaque);
}

/* Generate the edit parameters needed for changing the passphrase.
 */
static struct edit_parms_s*
gpa_gpgme_edit_passwd_parms_new (GpaContext *ctx, gpgme_data_t out)
{
  struct edit_parms_s *edit_parms = g_malloc (sizeof (struct edit_parms_s));
  struct passwd_parms_s *passwd_parms = g_malloc (sizeof 
						  (struct passwd_parms_s));

  edit_parms->state = PASSWD_START;
  edit_parms->err = gpg_error (GPG_ERR_NO_ERROR);
  edit_parms->action = edit_passwd_fnc_action;
  edit_parms->transit = edit_passwd_fnc_transit;
  edit_parms->signal_id = 0;
  edit_parms->out = out;
  edit_parms->opaque = passwd_parms;
  gpgme_get_passphrase_cb (ctx->ctx, &passwd_parms->func,
			   &passwd_parms->opaque);

  /* Make sure the cleanup is run when the edit completes */
  edit_parms->signal_id = 
    g_signal_connect (G_OBJECT (ctx), "done", 
		      G_CALLBACK (gpa_gpgme_edit_passwd_parms_release),
		      edit_parms);

  return edit_parms;
}

gpg_error_t 
gpa_gpgme_edit_passwd_start (GpaContext *ctx, gpgme_key_t key)
{
  struct edit_parms_s *parms;
  gpg_error_t err;
  gpgme_data_t out;

  err = gpgme_data_new (&out);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      return err;
    }

  parms = gpa_gpgme_edit_passwd_parms_new (ctx, out);

  /* Use our own passphrase callback: The data are the actual edit
     parms.  */
  if (!cms_hack)
    gpgme_set_passphrase_cb (ctx->ctx, passwd_passphrase_cb, parms);

  err = gpgme_op_edit_start (ctx->ctx, key, edit_fnc, parms, out);

  return err;
}

/*
 * Card related code.
 */

typedef enum
  {
    CARD_START,
    CARD_COMMAND,
    CARD_ADMIN_COMMAND,
    CARD_QUIT,
    CARD_QUERY_LOGIN,
    CARD_ERROR
  } CardState;

/* Implementation of the card-list operation. */

/* Action function for card-list.  */
static gpg_error_t
card_edit_list_fnc_action (int state, void *opaque, char **result)
{
  switch (state)
    {
    case CARD_COMMAND:
      *result = "list";
      break;
    case CARD_QUIT:
      *result = "quit";
      break;
    default:
      return gpg_error (GPG_ERR_GENERAL);
    }
  return 0;
}

/* Transit function for card-list.  */
static int
card_edit_list_fnc_transit (int current_state, gpgme_status_code_t status,
			    const char *args, void *opaque, gpg_error_t *err)
{
  int next_state;

  switch (current_state)
    {
    case CARD_START:
      if (status == GPGME_STATUS_CARDCTRL)
	/* Consume GPGME_STATUS_CARDCTRL and stay in CARD_START state. */
	next_state = CARD_START;
      else if (status == GPGME_STATUS_GET_LINE)
	next_state = CARD_COMMAND;
      break;
    case CARD_COMMAND:
      next_state = CARD_QUIT;
      break;
    default:
      next_state = CARD_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
    }

  return next_state;
}

static void
gpa_gpgme_card_edit_list_parms_release (GpaContext *ctx, gpg_error_t err,
					struct edit_parms_s *parms)
{
  /* FIXME: is this correct? -mo */

  if (parms->signal_id != 0)
    g_signal_handler_disconnect (ctx, parms->signal_id);
  g_free (parms);
}

/* Triggers card list operation through CTX. OUT is a valid GPGME data
   handle which is filled during the operation with the list
   output. */
gpg_error_t
gpa_gpgme_card_edit_list_start (GpaContext *ctx, gpgme_data_t out)
{
  struct edit_parms_s *parms = g_malloc (sizeof (struct edit_parms_s));
  gpg_error_t err;

  parms->state = CARD_START;
  parms->err = 0;
  parms->action = card_edit_list_fnc_action;
  parms->transit = card_edit_list_fnc_transit;
  parms->out = out;
  parms->opaque = NULL;
  parms->signal_id = g_signal_connect (G_OBJECT (ctx), "done",
				       G_CALLBACK (gpa_gpgme_card_edit_list_parms_release),
				       parms);

  err = gpgme_op_card_edit_start (ctx->ctx, NULL, edit_fnc, parms, out);

  return err;
}

#if 0				/* DISABLED */

/* Modify. */

static gpg_error_t
card_edit_modify_fnc_action (int state, void *opaque, char **result)
{
  switch (state)
    {
    case CARD_START:
      /* Nothing to do here. (?) */
      break;
    case CARD_COMMAND:
      *result = "admin";
      break;
    case CARD_ADMIN_COMMAND:
      *result = "login";
      break;
    case CARD_QUERY_LOGIN:
      *result = (char *) opaque;
      break;
    case CARD_QUIT:
      *result = "quit";
      break;
    default:
      return gpg_error (GPG_ERR_GENERAL);
    }
  return 0;
}

static int
card_edit_modify_fnc_transit (int current_state, gpgme_status_code_t status,
			      const char *args, void *opaque, gpg_error_t *err)
{
  int next_state;

  switch (current_state)
    {
    case CARD_START:
      if (status == GPGME_STATUS_CARDCTRL)
	/* Consume GPGME_STATUS_CARDCTRL and stay in CARD_START state. */
	next_state = CARD_START;
      else if (status == GPGME_STATUS_GET_LINE)
	next_state = CARD_COMMAND;
      break;
    case CARD_COMMAND:
      /* Dummy: */
      next_state = CARD_ADMIN_COMMAND;
      break;
    case CARD_ADMIN_COMMAND:
      fprintf (stderr, "[in CARD_ADMIN_COMMAND, status = %i\n", status);
      /* FIXME! check status.  */
      next_state = CARD_QUERY_LOGIN;
      break;
    case CARD_QUERY_LOGIN:
      next_state = CARD_QUIT;
      break;
    default:
      next_state = CARD_ERROR;
      *err = gpg_error (GPG_ERR_GENERAL);
    }

  return next_state;
}

static void
gpa_gpgme_card_edit_modify_parms_release (GpaContext *ctx, gpg_error_t err,
					  struct edit_parms_s *parms)
{
  /* FIXME: i don't really understand this. -mo */
  gpgme_data_release (parms->out);
  if (parms->signal_id != 0)
    g_signal_handler_disconnect (ctx, parms->signal_id);
  g_free (parms->opaque);
  g_free (parms);
}

gpg_error_t
gpa_gpgme_card_edit_modify_start (GpaContext *ctx, const gchar *login)
{
  struct edit_parms_s *parms = g_malloc (sizeof (struct edit_parms_s));
  gpg_error_t err;
  gpgme_data_t out;
  char *serialno = NULL;

  err = gpgme_data_new (&out);
  if (err)
    return err;

  parms->state = CARD_START;
  parms->err = 0;
  parms->action = card_edit_modify_fnc_action;
  parms->transit = card_edit_modify_fnc_transit;
  parms->out = out;
  parms->opaque = g_strdup (login);
  parms->signal_id = g_signal_connect (G_OBJECT (ctx), "done",
				       G_CALLBACK (gpa_gpgme_card_edit_modify_parms_release),
				       parms);

  err = gpgme_op_card_edit_start (ctx->ctx, NULL, edit_fnc, parms, out);

  return err;
}

#endif	/* DISABLED */
