/* cm-openpgp.c  -  OpenPGP card part for the card manager.
 * Copyright (C) 2008, 2009 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute and/or modify this part
 * of GPA under the terms of either
 *
 *   - the GNU Lesser General Public License as published by the Free
 *     Software Foundation; either version 3 of the License, or (at
 *     your option) any later version.
 *
 * or
 *
 *   - the GNU General Public License as published by the Free
 *     Software Foundation; either version 2 of the License, or (at
 *     your option) any later version.
 *
 * or both in parallel, as here.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gpa.h"
#include "gtktools.h"
#include "convert.h"
#include "gpa-key-details.h"

#include "cm-object.h"
#include "cm-openpgp.h"




/* Identifiers for the entry fields.  */
enum
  {
    ENTRY_SERIALNO,
    ENTRY_VERSION,
    ENTRY_MANUFACTURER,
    ENTRY_LOGIN,
    ENTRY_LANGUAGE,
    ENTRY_PUBKEY_URL,
    ENTRY_FIRST_NAME,
    ENTRY_LAST_NAME,
    ENTRY_SEX,
    ENTRY_KEY_SIG,
    ENTRY_KEY_ENC,
    ENTRY_KEY_AUTH,
    ENTRY_SIG_COUNTER,
    ENTRY_PIN_RETRYCOUNTER,
    ENTRY_PUK_RETRYCOUNTER,
    ENTRY_ADMIN_PIN_RETRYCOUNTER,
    ENTRY_SIG_FORCE_PIN,

    ENTRY_LAST
  };


/* Object's class definition.  */
struct _GpaCMOpenpgpClass
{
  GpaCMObjectClass parent_class;
};


/* Object definition.  */
struct _GpaCMOpenpgp
{
  GpaCMObject  parent_instance;

  GtkWidget *general_frame;
  GtkWidget *personal_frame;
  GtkWidget *keys_frame;
  GtkWidget *pin_frame;

  GtkWidget *entries[ENTRY_LAST];
  gboolean  changed[ENTRY_LAST];

  /* An array of 3 widgets to hold the key_details widget.  */
  GtkWidget *key_details[3];

  /* The key attributes.  */
  struct {
    int algo;
    int nbits;
  } key_attr[3];

  /* A malloced string with the key attributes in a human readable
     format.  */
  char *key_attributes;

  GtkLabel  *puk_label;  /* The label of the PUK field.  */

  /* An array with the current value of the retry counters.  */
  int retry_counter[3];

  /* An array for the buttons to change the 3 PINs.  */
  GtkWidget *change_pin_btn[3];

  /* True is this is a version 2 card. */
  int is_v2;

  /* This flag is set while we are reloading data.  */
  int  reloading;
};

/* The parent class.  */
static GObjectClass *parent_class;

/* Local prototypes */
static void gpa_cm_openpgp_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

/* NOTE:  This code has been copied from GnuPG/g10/card-util.c.  */
static const char *
get_manufacturer (unsigned int no)
{
  /* Note:  Make sure that there is no colon or linefeed in the string. */
  switch (no)
    {
    case 0x0001: return "PPC Card Systems";
    case 0x0002: return "Prism";
    case 0x0003: return "OpenFortress";
    case 0x0004: return "Wewid";
    case 0x0005: return "ZeitControl";
    case 0x0006: return "Yubico";
    case 0x0007: return "OpenKMS";
    case 0x0008: return "LogoEmail";

    case 0x002A: return "Magrathea";

    case 0x1337: return "Warsaw Hackerspace";

    case 0xF517: return "FSIJ";

      /* 0x0000 and 0xFFFF are defined as test cards per spec,
         0xFF00 to 0xFFFE are assigned for use with randomly created
         serial numbers.  */
    case 0x0000:
    case 0xffff: return "test card";
    default: return (no & 0xff00) == 0xff00? "unmanaged S/N range":"unknown";
    }
}


/* If not yet done show a warning to tell the user about the Admin
   PIN.  Returns true if the operation shall continue.  */
static int
show_admin_pin_notice (GpaCMOpenpgp *card)
{
  static int shown;
  GtkWidget *dialog;
  const char *string;
  int okay;

  if (shown)
    return 1;

  string = _("<b>Admin-PIN Required</b>\n"
             "\n"
             "Depending on the previous operations you may now "
             "be asked for the Admin-PIN.  Entering a wrong value "
             "for the Admin-PIN decrements the corresponding retry counter. "
             "If the retry counter is down to zero, the Admin-PIN can't "
             "be restored anymore and thus the data on the card can't be "
             "modified.\n"
             "\n"
             "Unless changed, a fresh standard card has set the Admin-PIN "
             "to the value <i>12345678</i>.  "
             "However, the issuer of your card might "
             "have initialized the card with a different Admin-PIN and "
             "that Admin-PIN might only be known to the issuer.  "
             "Please check the instructions of your issuer.\n"
             "\n"
             "This notice will be shown only once per session.");

  /* FIXME:  How do we figure out our GtkWindow?  */
  dialog = gtk_message_dialog_new_with_markup (NULL /*GTK_WINDOW (card)*/,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK_CANCEL,
                                               NULL);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), string);
  okay = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (okay)
    shown = 1;
  return okay;
}


/* Clears the info contained in the card widget. */
static void
clear_card_data (GpaCMOpenpgp *card)
{
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    if (idx == ENTRY_SEX)
      gtk_combo_box_set_active (GTK_COMBO_BOX (card->entries[idx]), 2);
    else if (idx == ENTRY_SIG_FORCE_PIN)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (card->entries[idx]), 0);
    else if (GTK_IS_LABEL (card->entries[idx]))
      gtk_label_set_text (GTK_LABEL (card->entries[idx]), "");
    else
      gtk_entry_set_text (GTK_ENTRY (card->entries[idx]), "");
}


static void
clear_changed_flags (GpaCMOpenpgp *card)
{
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    card->changed[idx] = 0;
}


/* Show the error string TEXT in the status bar.  With TEXT of NULL
   clear the edit error message.  */
static void
show_edit_error (GpaCMOpenpgp *card, const char *text)
{
  gpa_cm_object_update_status (GPA_CM_OBJECT(card), text);
}


/* Update the the serialno field.  This also updates the version and
   the manufacturer field.  */
static void
update_entry_serialno (GpaCMOpenpgp *card, int entry_id, const char *string)
{
  char version_buffer[6];
  char serialno_buffer[8+1];
  char *p;
  const char *serialno = "";
  const char *vendor = "";
  const char *version = "";

  (void)entry_id; /* Not used.  */

  if (strncmp (string, "D27600012401", 12) || strlen (string) != 32 )
    {
      /* Not a proper OpenPGP card serialnumber.  Display the full
         serialnumber. */
      serialno = string;
    }
  else
    {
      /* A proper OpenPGP AID.  */
      strncpy (serialno_buffer, string + 20, 8);
      serialno_buffer[8] = 0;
      serialno = serialno_buffer;

      /* Reformat the version number to be better human readable.  */
      p = version_buffer;
      if (string[12] != '0')
        *p++ = string[12];
      *p++ = string[13];
      *p++ = '.';
      if (string[14] != '0')
        *p++ = string[14];
      *p++ = string[15];
      *p++ = '\0';
      version = version_buffer;

      /* Get the manufactorer.  */
      vendor = get_manufacturer (xtoi_2(string+16)*256 + xtoi_2 (string+18));

    }
  gtk_label_set_text (GTK_LABEL (card->entries[ENTRY_SERIALNO]), serialno);
  gtk_label_set_text (GTK_LABEL (card->entries[ENTRY_MANUFACTURER]), vendor);
  gtk_label_set_text (GTK_LABEL (card->entries[ENTRY_VERSION]), version);

  card->is_v2 = !((*version == '1' || *version == '0') && version[1] == '.');
  gtk_label_set_text (card->puk_label, (card->is_v2
                                        ? _("PUK retry counter:")
                                        : _("CHV2 retry counter: ")));
}


static void
update_entry_name (GpaCMOpenpgp *card, int entry_id, const char *string)
{
  const char *first = "";
  const char *last = "";
  char *buffer = NULL;

  (void)entry_id; /* Not used.  */

  if (*string)
    {
      char *p, *given;

      buffer = xstrdup (string);
      given = strstr (buffer, "<<");
      for (p=buffer; *p; p++)
        if (*p == '<')
          *p = ' ';
      if (given && given[2])
        {
          *given = 0;
          given += 2;
          first = given;
        }
      last = buffer;
    }

  gtk_entry_set_text (GTK_ENTRY (card->entries[ENTRY_FIRST_NAME]), first);
  gtk_entry_set_text (GTK_ENTRY (card->entries[ENTRY_LAST_NAME]), last);
  xfree (buffer);
}


static void
update_entry_sex (GpaCMOpenpgp *card, int entry_id, const char *string)
{
  int idx;

  if (*string == '1')
    idx = 0;  /* 'm' */
  else if (*string == '2')
    idx = 1;  /* 'f' */
  else
    idx = 2;  /* 'u' is the default.  */

  gtk_combo_box_set_active (GTK_COMBO_BOX (card->entries[entry_id]), idx);
}


static void
set_integer (GtkWidget *entry, int value)
{
  char numbuf[35];

  snprintf (numbuf, sizeof numbuf, "%d", value);
  gtk_label_set_text (GTK_LABEL (entry), numbuf);
}


/* Update the CVH (PIN) status fields.  STRING is the space separated
   list of the decimal encoded values from the DO 0xC4.  */
static void
update_entry_chv_status (GpaCMOpenpgp *card, int entry_id, const char *string)
{
  int force_pw1;
  int pw_maxlen[3];
  int pw_retry[3];
  int i;

  (void)entry_id; /* Not used.  */

  while (spacep (string))
    string++;
  force_pw1 = !atoi (string);
  while (*string && !spacep (string))
    string++;
  while (spacep (string))
    string++;
  for (i=0; *string && i < DIM (pw_maxlen); i++)
    {
      pw_maxlen[i] = atoi (string);
      while (*string && !spacep (string))
        string++;
      while (spacep (string))
        string++;
    }
  for (i=0; *string && i < DIM (pw_retry); i++)
    {
      pw_retry[i] = atoi (string);
      while (*string && !spacep (string))
        string++;
      while (spacep (string))
        string++;
    }

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (card->entries[ENTRY_SIG_FORCE_PIN]), !!force_pw1);
  set_integer (card->entries[ENTRY_PIN_RETRYCOUNTER], pw_retry[0]);
  set_integer (card->entries[ENTRY_PUK_RETRYCOUNTER], pw_retry[1]);
  set_integer (card->entries[ENTRY_ADMIN_PIN_RETRYCOUNTER], pw_retry[2]);
  for (i=0; i < 3; i++)
    card->retry_counter[i] = pw_retry[i];

  /* Set the Change button label to reset or change depending on the
     current retry count.  Make the button insensitive if the
     Admin PIN is blocked.  */
  gtk_button_set_label (GTK_BUTTON (card->change_pin_btn[0]),
                        (!pw_retry[0] || (!pw_retry[1] && !card->is_v2)
                         ? _("Reset PIN") : _("Change PIN")));
  gtk_widget_set_sensitive (card->change_pin_btn[0], !!pw_retry[2]);

  /* For version 1 cards, PIN2 is usually synchronized with PIN1 thus
     we don't need the Change PIN button.  Version 2 cards use a PUK
     (actually a Reset Code) with entirely different semantics, thus a
     button is required.  */
  if (card->is_v2)
    {
      gtk_button_set_label (GTK_BUTTON (card->change_pin_btn[1]),
                            (pw_retry[1]? _("Change PUK"):_("Reset PUK")));
      gtk_widget_set_sensitive (card->change_pin_btn[1], !!pw_retry[2]);
      gtk_widget_show_all (card->change_pin_btn[1]);
    }
  else
    gtk_widget_hide_all (card->change_pin_btn[1]);
  gtk_widget_set_no_show_all (card->change_pin_btn[1], !card->is_v2);

  /* The Admin PIN has always the same label.  If the Admin PIN is
     blocked the button is made insensitive.  Fixme: In the case of a
     blocked Admin PIN we should have a button to allow a factory
     reset of v2 cards.  */
  gtk_button_set_label (GTK_BUTTON (card->change_pin_btn[2]), _("Change PIN"));
  gtk_widget_set_sensitive (card->change_pin_btn[2], !!pw_retry[2]);

}


/* Format the raw hex fingerprint STRING and set it into ENTRY_ID.  */
static void
update_entry_fpr (GpaCMOpenpgp *card, int entry_id, const char *string)
{
  char *buf;

  buf = gpa_gpgme_key_format_fingerprint (string);
  if (GTK_IS_EXPANDER (card->entries[entry_id]))
    gtk_expander_set_label (GTK_EXPANDER (card->entries[entry_id]), buf);
  else
    gtk_label_set_text (GTK_LABEL (card->entries[entry_id]), buf);
  xfree (buf);
}


/* Update the key attributes.  We don't have a regular entry field but
   append it to the version number field.  We are called for each key
   in turn and thus we only collect the information here and print
   them if we care called with STRING set to NULL.  */
static void
update_entry_key_attr (GpaCMOpenpgp *card, int entry_id, const char *string)
{
  int keyno, algo, nbits;
  char *buf, *tmpbuf;
  const char *s;
  int items;

  (void)entry_id; /* Not used.  */

  if (string)
    {
      sscanf (string, "%d %d %d", &keyno, &algo, &nbits);
      keyno--;
      if (keyno >= 0 && keyno < DIM (card->key_attr))
        {
          card->key_attr[keyno].algo = algo;
          card->key_attr[keyno].nbits = nbits;
        }
    }
  else
    {
      for (keyno=1; keyno < DIM (card->key_attr); keyno++)
        if (card->key_attr[0].algo != card->key_attr[keyno].algo
            || card->key_attr[0].nbits != card->key_attr[keyno].nbits)
          break;
      if (keyno < DIM (card->key_attr))
        items = DIM (card->key_attr); /* Attributes are mixed.  */
      else
        items = 1; /* All the same attributes.  */

      buf = NULL;
      for (keyno=0; keyno < items; keyno++)
        {
          tmpbuf = g_strdup_printf ("%s%s%s-%d",
                                    buf? buf : "",
                                    buf? ", ":"",
                                    card->key_attr[keyno].algo ==  1?"RSA":
                                    card->key_attr[keyno].algo == 17?"DSA":"?",
                                    card->key_attr[keyno].nbits);
          g_free (buf);
          buf = tmpbuf;
        }

      g_free (card->key_attributes);
      card->key_attributes = buf;

      s = gtk_label_get_text (GTK_LABEL (card->entries[ENTRY_VERSION]));
      if (!s)
        s = "";
      buf = g_strdup_printf ("%s  (%s)", s, card->key_attributes);
      gtk_label_set_text (GTK_LABEL (card->entries[ENTRY_VERSION]), buf);
      g_free (buf);
    }
}


struct scd_getattr_parm
{
  GpaCMOpenpgp *card;  /* The object.  */
  const char *name;     /* Name of expected attribute.  */
  int entry_id;        /* The identifier for the entry.  */
  void (*updfnc) (GpaCMOpenpgp *card, int entry_id, const char *string);
};


static gpg_error_t
scd_getattr_cb (void *opaque, const char *status, const char *args)
{
  struct scd_getattr_parm *parm = opaque;
  int entry_id;

/*   g_debug ("STATUS_CB: status=`%s'  args=`%s'", status, args); */

  if (!strcmp (status, parm->name) )
    {
      entry_id = parm->entry_id;
      if (entry_id == ENTRY_LAST && !strcmp (status, "KEY-FPR"))
        {
          /* Special entry ID for the fingerprints: We need to figure
             out what entry is actually to be used.  */
          if (*args == '1')
            entry_id = ENTRY_KEY_SIG;
          else if (*args == '2')
            entry_id = ENTRY_KEY_ENC;
          else if (*args == '3')
            entry_id = ENTRY_KEY_AUTH;
          else
            {
              /* Ooops.  */
            }
          if (*args)
            {
              for (args++; spacep (args); args++)
                ;
            }
        }

      if (entry_id < ENTRY_LAST)
        {
          char *tmp = xstrdup (args);

          percent_unescape (tmp, 1);
          if (parm->updfnc)
            parm->updfnc (parm->card, entry_id, tmp);
          else if (GTK_IS_LABEL (parm->card->entries[entry_id]))
            gtk_label_set_text
              (GTK_LABEL (parm->card->entries[entry_id]), tmp);
          else
            gtk_entry_set_text
              (GTK_ENTRY (parm->card->entries[entry_id]), tmp);
          xfree (tmp);
        }
      else if (entry_id == ENTRY_LAST && parm->updfnc)
        {
          char *tmp = xstrdup (args);

          percent_unescape (tmp, 1);
          parm->updfnc (parm->card, entry_id, tmp);
          xfree (tmp);
        }
    }

  return 0;
}



/* Use the assuan machinery to load the bulk of the OpenPGP card data.  */
static void
reload_data (GpaCMOpenpgp *card)
{
  static struct {
    const char *name;
    int entry_id;
    void (*updfnc) (GpaCMOpenpgp *card, int entry_id,  const char *string);
  } attrtbl[] = {
    { "SERIALNO",   ENTRY_SERIALNO, update_entry_serialno },
    { "DISP-NAME",  ENTRY_LAST_NAME, update_entry_name },
    { "DISP-LANG",  ENTRY_LANGUAGE },
    { "DISP-SEX",   ENTRY_SEX, update_entry_sex },
    { "PUBKEY-URL", ENTRY_PUBKEY_URL },
    { "LOGIN-DATA", ENTRY_LOGIN },
    { "SIG-COUNTER",ENTRY_SIG_COUNTER },
    { "CHV-STATUS", ENTRY_PIN_RETRYCOUNTER,  update_entry_chv_status },
    { "KEY-FPR",    ENTRY_LAST, update_entry_fpr },
/*     { "CA-FPR", }, */
    { "KEY-ATTR",   ENTRY_LAST, update_entry_key_attr },
    { NULL }
  };
  int attridx;
  gpg_error_t err, operr;
  char command[100];
  struct scd_getattr_parm parm;
  gpgme_ctx_t gpgagent;

  show_edit_error (card, NULL);

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);

  card->reloading++;
  parm.card = card;
  for (attridx=0; attrtbl[attridx].name; attridx++)
    {
      parm.name     = attrtbl[attridx].name;
      parm.entry_id = attrtbl[attridx].entry_id;
      parm.updfnc   = attrtbl[attridx].updfnc;
      snprintf (command, sizeof command, "SCD GETATTR %s", parm.name);

      err = gpgme_op_assuan_transact_ext (gpgagent,
                                          command,
                                          NULL, NULL,
                                          NULL, NULL,
                                          scd_getattr_cb, &parm, &operr);
      if (!err)
        err = operr;

      if (err)
        {
          if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT)
            ; /* Lost the card.  */
          else
            {
              g_debug ("assuan command `%s' failed: %s <%s>\n",
                       command, gpg_strerror (err), gpg_strsource (err));
            }
          clear_card_data (card);
          break;
        }

    }
  update_entry_key_attr (card, 0, NULL);  /* Append ky attributes.  */
  clear_changed_flags (card);
  card->reloading--;
}


static gpg_error_t
save_attr (GpaCMOpenpgp *card, const char *name,
           const char *value, int is_escaped)
{
  gpg_error_t err, operr;
  char *command;
  gpgme_ctx_t gpgagent;

  g_return_val_if_fail (*name && value, gpg_error (GPG_ERR_BUG));

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_val_if_fail (gpgagent,gpg_error (GPG_ERR_BUG));

  if (!show_admin_pin_notice (card))
    return gpg_error (GPG_ERR_CANCELED);

  if (is_escaped)
    command = g_strdup_printf ("SCD SETATTR %s %s", name, value);
  else
    {
      char *p;

      p = percent_escape (value, NULL, 1);
      command = g_strdup_printf ("SCD SETATTR %s %s", name, p);
      xfree (p);
    }
  err = gpgme_op_assuan_transact_ext (gpgagent,
                                      command,
                                      NULL, NULL,
                                      NULL, NULL,
                                      NULL, NULL, &operr);
  if (!err)
    err = operr;

  if (err && !(gpg_err_code (err) == GPG_ERR_CANCELED
               && gpg_err_source (err) == GPG_ERR_SOURCE_PINENTRY))
    {
      char *message = g_strdup_printf
        (_("Error saving the changed values.\n"
           "(%s <%s>)"), gpg_strerror (err), gpg_strsource (err));
      gpa_cm_object_alert_dialog (GPA_CM_OBJECT (card), message);
      xfree (message);
    }
  xfree (command);
  return err;
}


/* Check the constraints for NAME and return a string with an error
   description or NULL if the name is fine.  */
static const char *
check_one_name (const char *name)
{
  int i;

  for (i=0; name[i] && name[i] >= ' ' && name[i] <= 126; i++)
    ;

  /* The name must be in Latin-1 and not UTF-8 - lacking the code to
     ensure this we restrict it to ASCII. */
  if (name[i])
    return _("Only plain ASCII is currently allowed.");
  else if (strchr (name, '<'))
    return _("The \"<\" character may not be used.");
  else if (strstr (name, "  "))
    return _("Double spaces are not allowed.");
  else
    return NULL;
}


/* Save the first and last name fields.  Returns true on error.  */
static gboolean
save_entry_name (GpaCMOpenpgp *card)
{
  const char *first, *last;
  const char *errstr;
  int failed = ENTRY_FIRST_NAME;

  first = gtk_entry_get_text (GTK_ENTRY (card->entries[ENTRY_FIRST_NAME]));
  last = gtk_entry_get_text (GTK_ENTRY (card->entries[ENTRY_LAST_NAME]));
  errstr = check_one_name (first);
  if (!errstr)
    {
      errstr = check_one_name (last);
      if (errstr)
        failed = ENTRY_LAST_NAME;
    }
  if (!errstr)
    {
      char *buffer, *p;

      buffer = g_strdup_printf ("%s<<%s", last, first);
      for (p=buffer; *p; p++)
        if (*p == ' ')
          *p = '<';
      if (strlen (buffer) > 39)
        errstr = _("Total length of first and last name "
                   "may not be longer than 39 characters.");
      else if (save_attr (card, "DISP-NAME", buffer, 0))
        errstr = _("Saving the field failed.");
      g_free (buffer);
    }

  if (errstr)
    {
      show_edit_error (card, errstr);
      gtk_widget_grab_focus (GTK_WIDGET (card->entries[failed]));
    }

  return !!errstr;
}


/* Save the language field.  Returns true on error.  */
static gboolean
save_entry_language (GpaCMOpenpgp *card)
{
  const char *errstr = NULL;
  const char *value, *s;

  value = gtk_entry_get_text (GTK_ENTRY (card->entries[ENTRY_LANGUAGE]));
  if (strlen (value) > 8 || (strlen (value) & 1))
    {
      errstr = _("Invalid length of the language preference.");
      goto leave;
    }

  for (s=value; *s && *s >= 'a' && *s <= 'z'; s++)
    ;
  if (*s)
    {
      errstr = _("The language preference may only contain "
                 "the letters 'a' through 'z'.");
      goto leave;
    }

  if (save_attr (card, "DISP-LANG", value, 0))
    errstr = _("Saving the field failed.");

leave:
  if (errstr)
    show_edit_error (card, errstr);

  return !!errstr;
}


/* Save the salutation field.  Returns true on error.  */
static gpg_error_t
save_entry_sex (GpaCMOpenpgp *card)
{
  int idx;
  const char *string;
  const char *errstr = NULL;

  idx = gtk_combo_box_get_active (GTK_COMBO_BOX (card->entries[ENTRY_SEX]));
  if (!idx)
    string = "1";
  else if (idx == 1)
    string = "2";
  else
    string = "9";

  if (save_attr (card, "DISP-SEX", string, 0))
    errstr = _("Saving the field failed.");

  if (errstr)
    show_edit_error (card, errstr);

  return !!errstr;
}


/* Save the pubkey url field.  Returns true on error.  */
static gboolean
save_entry_pubkey_url (GpaCMOpenpgp *card)
{
  const char *errstr = NULL;
  const char *value;

  value = gtk_entry_get_text (GTK_ENTRY (card->entries[ENTRY_PUBKEY_URL]));
  if (strlen (value) > 254)
    {
      errstr = _("The field may not be longer than 254 characters.");
      goto leave;
    }

  if (save_attr (card, "PUBKEY-URL", value, 0))
    errstr = _("Saving the field failed.");

leave:
  if (errstr)
    show_edit_error (card, errstr);

  return !!errstr;
}


/* Save the login field.  Returns true on error.  */
static gboolean
save_entry_login (GpaCMOpenpgp *card)
{
  const char *errstr = NULL;
  const char *value;

  value = gtk_entry_get_text (GTK_ENTRY (card->entries[ENTRY_LOGIN]));
  if (strlen (value) > 254)
    {
      errstr = _("The field may not be longer than 254 characters.");
      goto leave;
    }

  if (save_attr (card, "LOGIN-DATA", value, 0))
    errstr = _("Saving the field failed.");

leave:
  if (errstr)
    show_edit_error (card, errstr);

  return !!errstr;
}


/* Save the force signature PIN field.  */
static gpg_error_t
save_entry_sig_force_pin (GpaCMOpenpgp *card)
{
  int value;
  const char *errstr = NULL;

  value = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON (card->entries[ENTRY_SIG_FORCE_PIN]));

  if (save_attr (card, "CHV-STATUS-1", value? "%00":"%01", 1))
    errstr = _("Saving the field failed.");

  if (errstr)
    show_edit_error (card, errstr);

  return !!errstr;
}



/* Callback for the "changed" signal connected to entry fields.  We
   figure out the entry field for which this signal has been emitted
   and set a flag to know wether we have unsaved data.  */
static void
edit_changed_cb (GtkEditable *editable, void *opaque)
{
  GpaCMOpenpgp *card = opaque;
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    if (GTK_IS_EDITABLE (card->entries[idx])
        && GTK_EDITABLE (card->entries[idx]) == editable)
      break;
  if (!(idx < ENTRY_LAST))
    return;

  if (!card->changed[idx])
    {
      card->changed[idx] = TRUE;
/*       g_debug ("changed signal for entry %d", idx); */
    }
}


/* Callback for the "changed" signal connected to combo boxes.  We
   figure out the entry field for which this signal has been emitted
   and set a flag to know ehether we have unsaved data.  */
static void
edit_changed_combo_box_cb (GtkComboBox *cbox, void *opaque)
{
  GpaCMOpenpgp *card = opaque;
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    if (GTK_IS_COMBO_BOX (card->entries[idx])
        && GTK_COMBO_BOX (card->entries[idx]) == cbox)
      break;
  if (!(idx < ENTRY_LAST))
    return;

  if (!card->changed[idx])
    {
      int result;

      card->changed[idx] = TRUE;
/*       g_debug ("changed signal for combo box %d", idx); */
      if (!card->reloading)
        {
          result = save_entry_sex (card);
          /* Fixme: How should we handle errors? Trying it this way:
                 Always clear the changed flag, so that we will receive one
                  again and grab the focus on error.
             fails.  We may need to make use of the keynav-changed signal. */
          card->changed[idx] = 0;
          if (!result)
            show_edit_error (card, NULL);
          else
            gtk_widget_grab_focus (GTK_WIDGET (cbox));
        }
    }
}

/* Callback for the "changed" signal connected to entry fields.  We
   figure out the entry field for which this signal has been emitted
   and set a flag to know ehether we have unsaved data.  */
static void
edit_toggled_cb (GtkToggleButton *toggle, void *opaque)
{
  GpaCMOpenpgp *card = opaque;
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    if (GTK_IS_TOGGLE_BUTTON (card->entries[idx])
        && GTK_TOGGLE_BUTTON (card->entries[idx]) == toggle)
      break;
  if (!(idx < ENTRY_LAST))
    return;

  if (!card->changed[idx])
    {
      card->changed[idx] = TRUE;
/*       g_debug ("toggled signal for entry %d", idx); */
    }
}



/* Callback for the "focus" signal connected to entry fields.  We
   figure out the entry field for which this signal has been emitted
   and whether we are receiving or losing focus.  If a the change flag
   has been set and we are losing focus we call the save function to
   store the data on the card.  If the change function returns true,
   an error occurred and we keep the focus at the current field.  */
static gboolean
edit_focus_cb (GtkWidget *widget, GtkDirectionType direction, void *opaque)
{
  GpaCMOpenpgp *card = opaque;
  int idx;
  int result = FALSE; /* Continue calling other handlers.  */

  for (idx=0; idx < ENTRY_LAST; idx++)
    if (card->entries[idx] == widget)
      break;
  if (!(idx < ENTRY_LAST))
    return FALSE;

  /* FIXME: The next test does not work for combo boxes.  Any hints
     on how to solve that?.  */
  if (gtk_widget_is_focus (widget))
    {
      /* Entry IDX is about to lose the focus.  */
/*       g_debug ("entry %d is about to lose the focus", idx); */
      if (card->changed[idx])
        {
          switch (idx)
            {
            case ENTRY_FIRST_NAME:
            case ENTRY_LAST_NAME:
              result = save_entry_name (card);
              break;
            case ENTRY_LANGUAGE:
              result = save_entry_language (card);
              break;
            case ENTRY_SEX:
              /* We never get to here due to the ComboBox.  Instead we
                 use the changed signal directly.  */
              /* result = save_entry_sex (card);*/
              break;
            case ENTRY_PUBKEY_URL:
              result = save_entry_pubkey_url (card);
              break;
            case ENTRY_LOGIN:
              result = save_entry_login (card);
              break;
            case ENTRY_SIG_FORCE_PIN:
              result = save_entry_sig_force_pin (card);
              break;
            }

          if (!result)
            {
              card->changed[idx] = 0;
              show_edit_error (card, NULL);
            }
        }
    }
  else
    {
      /* Entry IDX is about to receive the focus.  */
/*       g_debug ("entry %d is about to receive the focus", idx); */
      show_edit_error (card, NULL);
    }
  return result;
}



/* The button to change the PIN with number PINNO has been clicked.  */
static void
change_pin (GpaCMOpenpgp *card, int pinno)
{
  gpg_error_t err, operr;
  GtkWidget *dialog;
  gpgme_ctx_t gpgagent;
  int reset_mode = 0;
  int unblock_pin = 0;
  const char *string;
  int okay;


  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);
  g_return_if_fail (pinno >= 0 && pinno < DIM (card->change_pin_btn));

  if (!card->is_v2 && pinno == 1)
    return;  /* ooops, we should never get to here.  */

  if (pinno == 0 && !card->is_v2)
    reset_mode = (!card->retry_counter[0] || !card->retry_counter[1]);
  else if (pinno == 0 && !card->retry_counter[0] && card->retry_counter[1])
    {
      unblock_pin = 1;
      pinno = 1;
    }
  else if (!card->retry_counter[pinno])
    reset_mode = 1;

/*   g_debug ("%s pin for PIN %d", reset_mode? "reset":"change", pinno); */

  if (unblock_pin)
    string = _("<b>Unblocking the PIN</b>\n"
               "\n"
               "The retry counter of the PIN is down to zero "
               "but a Reset Code has been set.\n"
               "\n"
               "The Reset Code is similar to a PUK (PIN Unblocking Code)"
               "and used to unblock a PIN without the need to know the "
               "Admin-PIN.\n"
               "\n"
               "If you proceed you will be asked to enter the current "
               "value of the <b>Reset Code</b> and then to enter a new "
               "value for the PIN and repeat that new value at another "
               "prompt.");
  else if (!reset_mode && pinno == 0)
    string = _("<b>Changing the PIN</b>\n"
               "\n"
               "If you proceed you will be asked to enter "
               "the current value of the PIN and then to enter a new "
               "value and repeat that value at another prompt.\n"
               "\n"
               "Entering a wrong value for the PIN "
               "decrements the retry counter.  If the retry counters "
               "of the PIN and of the Reset Code are both down "
               "to zero, the PIN can still be reset by using the "
               "Admin-PIN.\n"
               "\n"
               "A fresh standard card has set the PIN to the value "
               "<i>123456</i>.  However, the issuer of your card might "
               "have initialized the card with a different PIN.  "
               "Please check the instructions of your issuer.");
  else if (!reset_mode && pinno == 1)
    string = _("<b>Changing the Reset Code</b>\n"
               "\n"
               "The Reset Code is similar to a PUK (PIN Unblocking Code) "
               "and used to unblock a PIN without the need to know the "
               "Admin-PIN.\n"
               "\n"
               "If you proceed you will be asked to enter the current "
               "value of the PIN and then to enter a new value for the "
               "Reset Code and repeat that new value at another prompt.");
  else if (reset_mode && pinno < 2)
    string = _("<b>Resetting the PIN or the Reset Code</b>\n"
               "\n"
               "If the retry counters of the PIN and of the Reset Code are "
               "both down to zero, it is only possible to reset them if you "
               "have access to the Admin-PIN.\n"
               "\n"
               "A fresh standard card has set the Admin-PIN to the value "
               "<i>12345678</i>.  However, the issuer of your card might "
               "have initialized the card with a different Admin-PIN and "
               "that Admin-PIN might only be known to the issuer.  "
               "Please check the instructions of your issuer.\n"
               "\n"
               "If you proceed you will be asked to enter the current "
               "value of the <b>Admin-PIN</b> and then to enter a new "
               "value for the PIN or the Reset Code and repeat that new "
               "value at another prompt.");
  else if (pinno == 2)
    string = _("<b>Changing the Admin-PIN</b>\n"
               "\n"
               "If you know the Admin-PIN you may change the Admin-PIN.\n"
               "\n"
               "The Admin-PIN is required to create keys on the card and to "
               "change other data.  You may or may not know the Admin-PIN.  "
               "A fresh standard card has set the Admin-PIN to the value "
               "<i>12345678</i>.  However, the issuer of your card might "
               "have initialized the card with a different Admin-PIN and "
               "that Admin-PIN might only be known to the issuer.  "
               "Please check the instructions of your issuer.\n"
               "\n"
               "If you proceed you will be asked to enter the current "
               "value of the <b>Admin-PIN</b> and then to enter a new "
               "value for that Admin-PIN and repeat that new "
               "value at another prompt.");
  else
    string = "oops";


  /* FIXME:  How do we figure out our GtkWindow?  */
  dialog = gtk_message_dialog_new_with_markup (NULL /*GTK_WINDOW (card)*/,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK_CANCEL,
                                               NULL);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), string);
  okay = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);
  if (okay)
    {
      char command[100];

      snprintf (command, sizeof command, "SCD PASSWD%s %d",
                reset_mode? " --reset":"", pinno+1);
      err = gpgme_op_assuan_transact_ext (gpgagent, command,
                                          NULL, NULL, NULL, NULL, NULL, NULL,
                                          &operr);
      if (!err)
        err = operr;

      if (gpg_err_code (err) == GPG_ERR_CANCELED)
        okay = 0; /* No need to reload the data.  */
      else if (err)
        {
          char *message = g_strdup_printf
            (_("Error changing or resetting the PIN/PUK.\n"
               "(%s <%s>)"), gpg_strerror (err), gpg_strsource (err));
          gpa_window_error (message, NULL);
          xfree (message);
        }
    }
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (okay)
    reload_data (card);
}


/* Handler for the "clicked" signal connected to the Change PIN
   buttons.  */
static void
change_pin_clicked_cb (void *widget, void *user_data)
{
  GpaCMOpenpgp *card = user_data;
  int idx;

  for (idx=0; idx < 3; idx++)
    if (widget == card->change_pin_btn[idx])
      {
        change_pin (card, idx);
        break;
      }
}



static void
entry_fpr_expanded_cb (GObject *object, GParamSpec *dummy, void *user_data)
{
  GtkExpander *expander = GTK_EXPANDER (object);
  GpaCMOpenpgp *card = user_data;
  int keyidx;

  (void)dummy;

  if (GTK_WIDGET (expander) == card->entries[ENTRY_KEY_SIG])
    keyidx = 0;
  else if (GTK_WIDGET (expander) == card->entries[ENTRY_KEY_ENC])
    keyidx = 1;
  else if (GTK_WIDGET (expander) == card->entries[ENTRY_KEY_AUTH])
    keyidx = 2;
  else
    g_return_if_fail (!"invalid key entry");

  if (gtk_expander_get_expanded (expander))
    {
      const char *fpr, *src;
      char *rawfpr, *dst;

      if (!card->key_details[keyidx])
        {
          card->key_details[keyidx] = gpa_key_details_new ();
          gtk_container_add (GTK_CONTAINER (expander),
                             card->key_details[keyidx]);
        }

      fpr = gtk_expander_get_label (expander);
      if (fpr)
        {
          rawfpr = dst = xmalloc (2 + strlen (fpr) + 1);
          *dst++ = '0';
          *dst++ = 'x';
          for (src=fpr; *src; src++)
            if (*src != ' ' && *src != ':')
              *dst++ = *src;
          *dst = 0;
        }
      else
        rawfpr = NULL;
      if (rawfpr)
        gpa_key_details_find (card->key_details[keyidx], rawfpr);
      else
        gpa_key_details_update (card->key_details[keyidx], NULL, 0);
      xfree (rawfpr);
    }
  else
    {
      if (card->key_details[keyidx])
        gtk_widget_destroy (card->key_details[keyidx]);
      card->key_details[keyidx] = NULL;
    }
}


/* Menu function to fetch the key from the URL.  */
static void
pubkey_url_menu_fetch_key (GtkMenuItem *menuitem, void *opaque)
{
  GpaCMOpenpgp *card = opaque;
  const char *url;
  gpg_error_t err;
  gpgme_data_t urllist;
  gpgme_ctx_t ctx;

  (void)menuitem;

  url = gtk_entry_get_text (GTK_ENTRY (card->entries[ENTRY_PUBKEY_URL]));
  if (!url || !*url)
    return;

  err = gpgme_data_new_from_mem (&urllist, url, strlen (url), 1);
  if (err)
    gpa_gpgme_error (err);
  gpgme_data_set_encoding (urllist, GPGME_DATA_ENCODING_URL0);

  ctx = gpa_gpgme_new ();
  err = gpgme_op_import (ctx, urllist);
  if (err)
    {
      char *message = g_strdup_printf
        (_("Error fetching the key.\n"
           "(%s <%s>)"), gpg_strerror (err), gpg_strsource (err));
      gpa_cm_object_alert_dialog (GPA_CM_OBJECT (card), message);
      xfree (message);
    }
  else
    {
      gpgme_import_result_t impres;

      impres = gpgme_op_import_result (ctx);
      if (impres)
        {
          char *message = g_strdup_printf
            (_("Keys found: %d, imported: %d, unchanged: %d"),
             impres->considered, impres->imported, impres->unchanged);
          gpa_cm_object_update_status (GPA_CM_OBJECT(card), message);
          xfree (message);
        }
    }

  gpgme_release (ctx);
  gpgme_data_release (urllist);
}


/* Add a context menu item to the pubkey URL box.  */
static void
pubkey_url_populate_popup_cb (GtkEntry *entry, GtkMenu *menu, void *opaque)
{
  GtkWidget *menuitem;

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Fetch Key"));
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (menuitem, "activate",
                    G_CALLBACK (pubkey_url_menu_fetch_key), opaque);
}



/* Helper for construct_data_widget.  Returns the label widget. */
static GtkLabel *
add_table_row (GtkWidget *table, int *rowidx,
               const char *labelstr, GtkWidget *widget, GtkWidget *widget2,
               int readonly, int compact)
{
  GtkWidget *label;
  int is_label = GTK_IS_LABEL (widget);

  label = gtk_label_new (labelstr);
  gtk_label_set_width_chars  (GTK_LABEL (label), 22);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_FILL, 0, 0);

  if (is_label)
    gtk_misc_set_alignment (GTK_MISC (widget), 0, 0.5);

  if (readonly)
    {
      if (!is_label && GTK_IS_ENTRY (widget))
        {
          gtk_entry_set_has_frame (GTK_ENTRY (widget), FALSE);
          gtk_entry_set_editable (GTK_ENTRY (widget), FALSE);
        }
    }
  else
    {
      if (is_label)
        gtk_label_set_selectable (GTK_LABEL (widget), TRUE);
    }

  gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
		    *rowidx, *rowidx + 1,
		    (compact ? GTK_FILL : (GTK_FILL | GTK_EXPAND)),
		    GTK_SHRINK, 0, 0);
  if (widget2)
    gtk_table_attach (GTK_TABLE (table), widget2, 2, 3,
		      *rowidx, *rowidx + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
  ++*rowidx;

  return GTK_LABEL (label);
}


/* This function constructs the container holding all widgets making
   up this data widget.  It is called during instance creation.  */
static void
construct_data_widget (GpaCMOpenpgp *card)
{
  GtkWidget *label;
  GtkWidget *general_frame;
  GtkWidget *general_table;
  GtkWidget *personal_frame;
  GtkWidget *personal_table;
  GtkWidget *keys_frame;
  GtkWidget *keys_table;
  GtkWidget *pin_frame;
  GtkWidget *pin_table;
  GtkWidget *button;
  int rowidx;
  int idx;


  /* General frame.  */
  general_frame = card->general_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (general_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>General</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (general_frame), label);

  general_table = gtk_table_new (4, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (general_table), 10);

  rowidx = 0;

  card->entries[ENTRY_SERIALNO] = gtk_label_new (NULL);
  add_table_row (general_table, &rowidx, _("Serial number:"),
                 card->entries[ENTRY_SERIALNO], NULL, 0, 1);

  card->entries[ENTRY_VERSION] = gtk_label_new (NULL);
  add_table_row (general_table, &rowidx, _("Card version:"),
                 card->entries[ENTRY_VERSION], NULL, 0, 1);

  card->entries[ENTRY_MANUFACTURER] = gtk_label_new (NULL);
  add_table_row (general_table, &rowidx, _("Manufacturer:"),
                 card->entries[ENTRY_MANUFACTURER], NULL, 0, 1);

  gtk_container_add (GTK_CONTAINER (general_frame), general_table);


  /* Personal frame.  */
  personal_frame = card->personal_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (personal_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>Personal</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (personal_frame), label);

  personal_table = gtk_table_new (6, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (personal_table), 10);

  rowidx = 0;

  card->entries[ENTRY_SEX] = gtk_combo_box_new_text ();
  for (idx=0; "mfu"[idx]; idx++)
    gtk_combo_box_append_text (GTK_COMBO_BOX (card->entries[ENTRY_SEX]),
                               gpa_sex_char_to_string ("mfu"[idx]));
  add_table_row (personal_table, &rowidx,
                 _("Salutation:"), card->entries[ENTRY_SEX], NULL, 0, 0);

  card->entries[ENTRY_FIRST_NAME] = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (card->entries[ENTRY_FIRST_NAME]), 35);
  add_table_row (personal_table, &rowidx,
                 _("First name:"), card->entries[ENTRY_FIRST_NAME], NULL, 0, 0);

  card->entries[ENTRY_LAST_NAME] = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (card->entries[ENTRY_LAST_NAME]), 35);
  add_table_row (personal_table, &rowidx,
                 _("Last name:"),  card->entries[ENTRY_LAST_NAME], NULL, 0, 0);

  card->entries[ENTRY_LANGUAGE] = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (card->entries[ENTRY_LANGUAGE]), 8);
  add_table_row (personal_table, &rowidx,
                 _("Language:"), card->entries[ENTRY_LANGUAGE], NULL, 0, 0);

  card->entries[ENTRY_LOGIN] = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (card->entries[ENTRY_LOGIN]), 254);
  add_table_row (personal_table, &rowidx,
                 _("Login data:"), card->entries[ENTRY_LOGIN], NULL, 0, 0);

  card->entries[ENTRY_PUBKEY_URL] = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (card->entries[ENTRY_PUBKEY_URL]), 254);
  add_table_row (personal_table, &rowidx,
                 _("Public key URL:"),
                 card->entries[ENTRY_PUBKEY_URL], NULL, 0, 0);

  gtk_container_add (GTK_CONTAINER (personal_frame), personal_table);


  /* Keys frame.  */
  keys_frame = card->keys_frame = gtk_expander_new (NULL);
  label = gtk_label_new (_("<b>Keys</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_expander_set_label_widget (GTK_EXPANDER (keys_frame), label);
  gtk_expander_set_expanded (GTK_EXPANDER (keys_frame), TRUE);

  keys_table = gtk_table_new (4, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (keys_table), 10);

  rowidx = 0;

  /* Fixme: A problem using an expander instead of a plain label is
     that the label is not selectable.  Any ideas on how to fix that?
     Is that really required given that we can use the details page
     for selection.  We might always want to switch between expander
     and label depending on whether we have a key for the fingerprint.
     Check whether gtk_expander_set_label_wdiget works for us. */
  card->entries[ENTRY_KEY_SIG] = gtk_expander_new (NULL);
  add_table_row (keys_table, &rowidx, _("Signature key:"),
                 card->entries[ENTRY_KEY_SIG], NULL, 0, 0);

  card->entries[ENTRY_KEY_ENC] = gtk_expander_new (NULL);
  add_table_row (keys_table, &rowidx, _("Encryption key:"),
                 card->entries[ENTRY_KEY_ENC], NULL, 0, 0);

  card->entries[ENTRY_KEY_AUTH] = gtk_expander_new (NULL);
  add_table_row (keys_table, &rowidx, _("Authentication key:"),
                 card->entries[ENTRY_KEY_AUTH], NULL, 0, 0);

  card->entries[ENTRY_SIG_COUNTER] = gtk_label_new (NULL);
  add_table_row (keys_table, &rowidx, _("Signature counter:"),
                 card->entries[ENTRY_SIG_COUNTER], NULL, 0, 0);

  gtk_container_add (GTK_CONTAINER (keys_frame), keys_table);


  /* PIN frame.  */
  pin_frame = card->pin_frame = gtk_expander_new (NULL);
  label = gtk_label_new (_("<b>PIN</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_expander_set_label_widget (GTK_EXPANDER (pin_frame), label);
  gtk_expander_set_expanded (GTK_EXPANDER (pin_frame), TRUE);

  pin_table = gtk_table_new (4, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (pin_table), 10);

  rowidx = 0;

  card->entries[ENTRY_SIG_FORCE_PIN] = gtk_check_button_new ();
  add_table_row (pin_table, &rowidx,
                 _("Force signature PIN:"),
                 card->entries[ENTRY_SIG_FORCE_PIN], NULL, 0, 1);
  card->entries[ENTRY_PIN_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new ();
  add_table_row (pin_table, &rowidx, _("PIN retry counter:"),
                 card->entries[ENTRY_PIN_RETRYCOUNTER], button, 1, 1);
  card->change_pin_btn[0] = button;

  card->entries[ENTRY_PUK_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new ();
  card->puk_label =
    add_table_row (pin_table, &rowidx,
                   "", /* The label depends on the card version.  */
                   card->entries[ENTRY_PUK_RETRYCOUNTER], button, 1, 1);
  card->change_pin_btn[1] = button;

  card->entries[ENTRY_ADMIN_PIN_RETRYCOUNTER] = gtk_label_new (NULL);
  button = gtk_button_new ();
  add_table_row (pin_table, &rowidx, _("Admin-PIN retry counter:"),
                 card->entries[ENTRY_ADMIN_PIN_RETRYCOUNTER], button, 1, 1);
  card->change_pin_btn[2] = button;

  gtk_container_add (GTK_CONTAINER (pin_frame), pin_table);


  /* Put all frames together.  */
  gtk_box_pack_start (GTK_BOX (card), general_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (card), personal_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (card), keys_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (card), pin_frame, FALSE, TRUE, 0);

  /* Setup signals */
  for (idx=0; idx < ENTRY_LAST; idx++)
    {
      if (!card->entries[idx])
        continue;
      switch (idx)
        {
        case ENTRY_SEX:
          g_signal_connect (G_OBJECT (card->entries[idx]), "changed",
                            G_CALLBACK (edit_changed_combo_box_cb), card);
          g_signal_connect (G_OBJECT (card->entries[idx]), "focus",
                            G_CALLBACK (edit_focus_cb), card);
          break;

        case ENTRY_SIG_FORCE_PIN:
          g_signal_connect (G_OBJECT (card->entries[idx]), "toggled",
                            G_CALLBACK (edit_toggled_cb), card);
          g_signal_connect (G_OBJECT (card->entries[idx]), "focus",
                            G_CALLBACK (edit_focus_cb), card);
          break;

        case ENTRY_KEY_SIG:
        case ENTRY_KEY_ENC:
        case ENTRY_KEY_AUTH:
          g_signal_connect (G_OBJECT (card->entries[idx]), "notify::expanded",
                            G_CALLBACK (entry_fpr_expanded_cb), card);
          break;

        default:
          if (!GTK_IS_LABEL (card->entries[idx]))
            {
              g_signal_connect (G_OBJECT (card->entries[idx]), "changed",
                                G_CALLBACK (edit_changed_cb), card);
              g_signal_connect (G_OBJECT (card->entries[idx]), "focus",
                                G_CALLBACK (edit_focus_cb), card);
            }
          break;
        }
    }

  for (idx=0; idx < 3; idx++)
    g_signal_connect (G_OBJECT (card->change_pin_btn[idx]), "clicked",
                      G_CALLBACK (change_pin_clicked_cb), card);

  g_signal_connect (G_OBJECT (card->entries[ENTRY_PUBKEY_URL]),
                    "populate-popup",
                    G_CALLBACK (pubkey_url_populate_popup_cb), card);


}



/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_cm_openpgp_class_init (void *class_ptr, void *class_data)
{
  GpaCMOpenpgpClass *klass = class_ptr;

  parent_class = g_type_class_peek_parent (klass);

  G_OBJECT_CLASS (klass)->finalize = gpa_cm_openpgp_finalize;
}


static void
gpa_cm_openpgp_init (GTypeInstance *instance, void *class_ptr)
{
  GpaCMOpenpgp *card = GPA_CM_OPENPGP (instance);

  construct_data_widget (card);

}


static void
gpa_cm_openpgp_finalize (GObject *object)
{
  GpaCMOpenpgp *card = GPA_CM_OPENPGP (object);

  xfree (card->key_attributes);
  card->key_attributes = NULL;

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_cm_openpgp_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCMOpenpgpClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_cm_openpgp_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaCMOpenpgp),
	  0,    /* n_preallocs */
	  gpa_cm_openpgp_init
	};

      this_type = g_type_register_static (GPA_CM_OBJECT_TYPE,
                                          "GpaCMOpenpgp",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_cm_openpgp_new ()
{
  return GTK_WIDGET (g_object_new (GPA_CM_OPENPGP_TYPE, NULL));
}


/* If WIDGET is of type GpaCMOpenpgp do a data reload through the
   Assuan connection identified by GPGAGENT.  This will keep a
   reference to GPGAGENT for later processing.  Passing NULL for
   GPGAGENT removes this reference. */
void
gpa_cm_openpgp_reload (GtkWidget *widget, gpgme_ctx_t gpgagent)
{
  if (GPA_IS_CM_OPENPGP (widget))
    {
      GPA_CM_OBJECT (widget)->agent_ctx = gpgagent;
      if (gpgagent)
        reload_data (GPA_CM_OPENPGP (widget));
    }
}

char *
gpa_cm_openpgp_get_key_attributes (GtkWidget *widget)
{
  g_return_val_if_fail (GPA_IS_CM_OPENPGP (widget), NULL);
  return g_strdup (GPA_CM_OPENPGP (widget)->key_attributes);
}
