/* cm-openpgp.c  -  OpenPGP card part for the card manager.
 * Copyright (C) 2008, 2009 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
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
#include "convert.h"

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
    ENTRY_PIN_RETRYCOUNTER,
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
    case 0x0004: return "Wewid AB";

      /* 0x00000 and 0xFFFF are defined as test cards per spec,
         0xFFF00 to 0xFFFE are assigned for use with randomly created
         serial numbers.  */
    case 0x0000:
    case 0xffff: return "test card";
    default: return (no & 0xff00) == 0xff00? "unmanaged S/N range":"unknown";
    }
}


/* Clears the info contained in the card widget. */
static void
clear_card_data (GpaCMOpenpgp *card)
{
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    gtk_entry_set_text (GTK_ENTRY (card->entries[idx]), "");
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
  gtk_entry_set_text (GTK_ENTRY (card->entries[ENTRY_SERIALNO]), serialno);
  gtk_entry_set_text (GTK_ENTRY (card->entries[ENTRY_MANUFACTURER]), vendor);
  gtk_entry_set_text (GTK_ENTRY (card->entries[ENTRY_VERSION]), version);
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
  if (*string == '1')
    string = _("male");
  else if (*string == '2')
    string = _("female");
  else if (*string == '9')
    string = _("unspecified");

  gtk_entry_set_text (GTK_ENTRY (card->entries[entry_id]), string);
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
            ; /* Ooops.  */
          if (*args)
            {
              for (args++; spacep (args); args++)
                ;
            }
        }

      if (entry_id < ENTRY_LAST)
        {
          if (parm->updfnc)
            parm->updfnc (parm->card, entry_id, args);
          else
            gtk_entry_set_text 
              (GTK_ENTRY (parm->card->entries[entry_id]), args);
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
/*     { "SIG-COUNTER",ENTRY_SIG_COUNTER }, */
    { "CHV-STATUS", ENTRY_PIN_RETRYCOUNTER,  NULL/*fixme*/ },
    { "KEY-FPR",    ENTRY_LAST },
/*     { "CA-FPR", }, */
    { NULL }
  };
  int attridx;
  gpg_error_t err;
  char command[100];
  struct scd_getattr_parm parm;
  gpgme_ctx_t gpgagent;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);


  parm.card = card;
  for (attridx=0; attrtbl[attridx].name; attridx++)
    {
      parm.name     = attrtbl[attridx].name;
      parm.entry_id = attrtbl[attridx].entry_id;
      parm.updfnc   = attrtbl[attridx].updfnc;
      snprintf (command, sizeof command, "SCD GETATTR %s", parm.name);

      err = gpgme_op_assuan_transact (gpgagent,
                                      command,
                                      NULL, NULL,
                                      NULL, NULL,
                                      scd_getattr_cb, &parm);
      if (!err)
        err = gpgme_op_assuan_result (gpgagent);

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
}


static GtkDialog *
create_edit_dialog (GpaCMOpenpgp *card, const char *title)
{
  GtkDialog *dialog;

  dialog = GTK_DIALOG (gtk_dialog_new_with_buttons
                       (title,
                        GTK_WINDOW (card),
                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                        NULL));
  gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  return dialog;
}


/* Run the dialog and return true if the user affirmed the change.  */
static int
run_edit_dialog (GtkDialog *dialog)
{
  gtk_widget_show_all (GTK_WIDGET (dialog));
  return (gtk_dialog_run (dialog) == GTK_RESPONSE_OK);
}


static void
destroy_edit_dialog (GtkDialog *dialog)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}


/* Action for the Edit Name button.  Display a new dialog through
   which the user can change the name (firstname + lastname) stored on
   the card.*/
static void
edit_name_cb (GtkWidget *widget, void *opaque)
{
  GpaCMOpenpgp *card = opaque;
  GtkDialog *dialog;
  GtkWidget *first_name, *last_name;
  GtkWidget *content_area;
  GtkWidget *table;
  
  dialog = create_edit_dialog (card, _("Change Name"));

  content_area = GTK_DIALOG (dialog)->vbox;

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);

  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("Current Value:"), 
                    0, 1, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("New Value:"), 
                    0, 1,  2, 3, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("First Name"),
                    1,  2, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new ("Last Name"),
                    2,  3, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new 
                    (gtk_entry_get_text 
                     (GTK_ENTRY (card->entries[ENTRY_FIRST_NAME]))),
		    1, 2, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table),
		    gtk_label_new 
                    (gtk_entry_get_text 
                     (GTK_ENTRY (card->entries[ENTRY_LAST_NAME]))),
		    2, 3, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);

  first_name = gtk_entry_new ();
  last_name = gtk_entry_new ();

  gtk_table_attach (GTK_TABLE (table), first_name,
                    1, 2, 2, 3, GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach (GTK_TABLE (table), last_name,
                    2, 3, 2, 3, GTK_FILL, GTK_SHRINK, 0, 0);
  
  gtk_container_add (GTK_CONTAINER (content_area), table);


  if (run_edit_dialog (dialog))
    {



    }

  destroy_edit_dialog (dialog);
}


static void
edit_sex_cb (GtkWidget *widget, void *opaque)
{
}


static void
edit_language_cb (GtkWidget *widget, void *opaque)
{
}


static void
edit_login_cb (GtkWidget *widget, void *opaque)
{
}


static void
edit_url_cb (GtkWidget *widget, void *opaque)
{
}



/* Helper for construct_data_widget.  */
static void
add_table_row (GtkWidget *table, int *rowidx,
               const char *labelstr, GtkWidget *widget, GtkWidget *widget2)
{
  GtkWidget *label;

  label = gtk_label_new (labelstr);
  gtk_label_set_width_chars  (GTK_LABEL (label), 22);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,	       
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0); 

  gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);
  gtk_entry_set_has_frame (GTK_ENTRY (widget), FALSE);

  gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);
  if (widget2)
    gtk_table_attach (GTK_TABLE (table), widget2, 2, 3,
		      *rowidx, *rowidx + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
  ++*rowidx;
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

  /* Create frames and tables. */

  general_frame = card->general_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (general_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>General</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (general_frame), label);

  personal_frame = card->personal_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (personal_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>Personal</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (personal_frame), label);

  keys_frame = card->keys_frame = gtk_expander_new (NULL);
  label = gtk_label_new (_("<b>Keys</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_expander_set_label_widget (GTK_EXPANDER (keys_frame), label);
  gtk_expander_set_expanded (GTK_EXPANDER (keys_frame), TRUE);

  pin_frame = card->pin_frame = gtk_expander_new (NULL);
  label = gtk_label_new (_("<b>PIN</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_expander_set_label_widget (GTK_EXPANDER (pin_frame), label);
  gtk_expander_set_expanded (GTK_EXPANDER (pin_frame), TRUE);

  general_table = gtk_table_new (4, 3, FALSE);
  personal_table = gtk_table_new (6, 3, FALSE);
  keys_table = gtk_table_new (3, 3, FALSE);
  pin_table = gtk_table_new (3, 3, FALSE);

  gtk_container_set_border_width (GTK_CONTAINER (general_table), 10);
  gtk_container_set_border_width (GTK_CONTAINER (personal_table), 10);
  gtk_container_set_border_width (GTK_CONTAINER (keys_table), 10);
  gtk_container_set_border_width (GTK_CONTAINER (pin_table), 10);
  
  /* General frame.  */
  rowidx = 0;

  card->entries[ENTRY_SERIALNO] = gtk_entry_new ();
  add_table_row (general_table, &rowidx, 
                 "Serial Number: ", card->entries[ENTRY_SERIALNO], NULL);

  card->entries[ENTRY_VERSION] = gtk_entry_new ();
  add_table_row (general_table, &rowidx,
                 "Card Version: ", card->entries[ENTRY_VERSION], NULL);

  card->entries[ENTRY_MANUFACTURER] = gtk_entry_new ();
  add_table_row (general_table, &rowidx,
                 "Manufacturer: ", card->entries[ENTRY_MANUFACTURER], NULL);

  gtk_container_add (GTK_CONTAINER (general_frame), general_table);

  /* Personal frame.  */
  rowidx = 0;

  card->entries[ENTRY_FIRST_NAME] = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (card->entries[ENTRY_FIRST_NAME]), 48);
  add_table_row (personal_table, &rowidx, 
                 "First Name:", card->entries[ENTRY_FIRST_NAME], NULL);

  card->entries[ENTRY_LAST_NAME] = gtk_entry_new ();
  add_table_row (personal_table, &rowidx,
                 "Last Name:",
                 card->entries[ENTRY_LAST_NAME], NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
  gtk_table_attach (GTK_TABLE (personal_table), button,
                    2, 3, rowidx-2, rowidx, 
                    GTK_SHRINK, GTK_FILL, 0, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (edit_name_cb), card);


  card->entries[ENTRY_SEX] = gtk_entry_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
  add_table_row (personal_table, &rowidx,
                 "Sex:", card->entries[ENTRY_SEX], button);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (edit_sex_cb), card);

  card->entries[ENTRY_LANGUAGE] = gtk_entry_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
  add_table_row (personal_table, &rowidx,
                 "Language: ", card->entries[ENTRY_LANGUAGE], button);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (edit_language_cb), card);

  card->entries[ENTRY_LOGIN] = gtk_entry_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
  add_table_row (personal_table, &rowidx,
                 "Login Data: ", card->entries[ENTRY_LOGIN], button);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (edit_login_cb), card);

  card->entries[ENTRY_PUBKEY_URL] = gtk_entry_new ();
  button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
  add_table_row (personal_table, &rowidx,
                 "Public key URL: ", card->entries[ENTRY_PUBKEY_URL], button);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (edit_url_cb), card);

  gtk_container_add (GTK_CONTAINER (personal_frame), personal_table);

  /* Keys frame.  */
  rowidx = 0;

  card->entries[ENTRY_KEY_SIG] = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (card->entries[ENTRY_KEY_SIG]), 48);
  add_table_row (keys_table, &rowidx, 
                 "Signature Key: ", card->entries[ENTRY_KEY_SIG], NULL);

  card->entries[ENTRY_KEY_ENC] = gtk_entry_new ();
  add_table_row (keys_table, &rowidx,
                 "Encryption Key: ", card->entries[ENTRY_KEY_ENC], NULL);

  card->entries[ENTRY_KEY_AUTH] = gtk_entry_new ();
  add_table_row (keys_table, &rowidx,
                 "Authentication Key: ", card->entries[ENTRY_KEY_AUTH], NULL);

  gtk_container_add (GTK_CONTAINER (keys_frame), keys_table);


  /* PIN frame.  */
  rowidx = 0;

  card->entries[ENTRY_PIN_RETRYCOUNTER] = gtk_entry_new ();
  gtk_entry_set_width_chars
    (GTK_ENTRY (card->entries[ENTRY_PIN_RETRYCOUNTER]), 24);
  add_table_row (pin_table, &rowidx, 
                 "PIN Retry Counter: ", 
                 card->entries[ENTRY_PIN_RETRYCOUNTER], NULL);

  card->entries[ENTRY_ADMIN_PIN_RETRYCOUNTER] = gtk_entry_new ();
  gtk_entry_set_width_chars 
    (GTK_ENTRY (card->entries[ENTRY_ADMIN_PIN_RETRYCOUNTER]), 24);
  add_table_row (pin_table, &rowidx, 
                 "Admin PIN Retry Counter: ",
                 card->entries[ENTRY_ADMIN_PIN_RETRYCOUNTER], NULL);

  card->entries[ENTRY_SIG_FORCE_PIN] = gtk_entry_new ();
  gtk_entry_set_width_chars 
    (GTK_ENTRY (card->entries[ENTRY_SIG_FORCE_PIN]), 24);
  add_table_row (pin_table, &rowidx, 
                 "Force Signature PIN: ",
                 card->entries[ENTRY_SIG_FORCE_PIN], NULL);

  gtk_container_add (GTK_CONTAINER (pin_frame), pin_table);

  /* Put all frames together.  */
  gtk_box_pack_start (GTK_BOX (card), general_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (card), personal_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (card), keys_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (card), pin_frame, FALSE, TRUE, 0);

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
/*   GpaCMOpenpgp *card = GPA_CM_OPENPGP (object); */

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
