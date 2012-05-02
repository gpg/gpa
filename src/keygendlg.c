/* keygendlg.c - The GNU Privacy Assistant
   Copyright (C) 2009 g10 Code GmbH

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include "gpa.h"
#include "gpawidgets.h"
#include "gtktools.h"
#include "gpadatebox.h"
#include "keygendlg.h"



/* A table of algorithm combinations we offer to create.  */
static struct
{
  gpa_keygen_algo_t algo;
  const char *name;
} algorithm_table[] =
  {
    { GPA_KEYGEN_ALGO_RSA_RSA,     N_("RSA")},
    { GPA_KEYGEN_ALGO_RSA,         N_("RSA (sign only)")},
    { GPA_KEYGEN_ALGO_DSA_ELGAMAL, N_("DSA")},
    { GPA_KEYGEN_ALGO_DSA,         N_("DSA (sign only)")},
    { 0, NULL}
  };



struct _GpaKeyGenDlg
{
  gboolean forcard;	    /* Specifies if this is a dialog for
			       on-card key generation or not. */
  GtkWidget *dialog;        /* The dialog object.  */

  GtkWidget *entry_algo;    /* Maybe NULL.  */
  GtkWidget *entry_keysize; /* Maybe NULL.  */
  GtkWidget *entry_name;
  GtkWidget *entry_email;
  GtkWidget *entry_comment;
  GtkWidget *entry_expire;
  GtkWidget *entry_backup;  /* Maybe NULL.  */

  GtkWidget *label_userid;
};
typedef struct _GpaKeyGenDlg GpaKeyGenDlg;


static gboolean
validate_name (GpaKeyGenDlg *self)
{
  const char *s;

  s = gpa_validate_gpg_name (gtk_entry_get_text
                             (GTK_ENTRY (self->entry_name)));
  if (s)
    gpa_window_error (s, self->dialog);
  return !s;
}

static gboolean
validate_email (GpaKeyGenDlg *self)
{
  const char *s;

  s = gpa_validate_gpg_email (gtk_entry_get_text
                              (GTK_ENTRY (self->entry_email)));
  if (s)
    gpa_window_error (s, self->dialog);
  return !s;
}

static gboolean
validate_comment (GpaKeyGenDlg *self)
{
  const char *s;

  s = gpa_validate_gpg_comment (gtk_entry_get_text
                                (GTK_ENTRY (self->entry_comment)));
  if (s)
    gpa_window_error (s, self->dialog);
  return !s;
}


/* This callback gets called each time the user clicks on the [OK] or
   [Cancel] buttons.  If the button was [OK], it verifies that the
   input makes sense.  */
static void
response_cb (GtkDialog *dlg, gint response, gpointer user_data)
{
  GpaKeyGenDlg *self = user_data;
  const char *temp;
  int   keysize;

  if (response != GTK_RESPONSE_OK)
    return;

  temp = (self->entry_keysize
          ? gtk_combo_box_get_active_text (GTK_COMBO_BOX
                                           (self->entry_keysize))
          :  NULL);
  keysize = temp? atoi (temp):0;

  if (!validate_name (self)
      || !validate_email (self)
      || !validate_comment (self))
    {
      g_signal_stop_emission_by_name (dlg, "response");
    }
  else if (self->forcard)
    ;
  else if (keysize < 1024)
    {
      gpa_window_error (_("You must enter a key size."), self->dialog);
      g_signal_stop_emission_by_name (dlg, "response");
    }

  /* FIXME: check that the expire date is not in the past.  */
}


static void
update_preview_cb (void *widget, void *user_data)
{
  GpaKeyGenDlg *self = user_data;
  const char *name, *email, *comment;
  char *uid;
  (void)widget;

  name = gtk_entry_get_text (GTK_ENTRY (self->entry_name));
  if (!name)
    name = "";
  email = gtk_entry_get_text (GTK_ENTRY (self->entry_email));
  if (!email)
    email = "";
  comment = gtk_entry_get_text (GTK_ENTRY (self->entry_comment));
  if (!comment)
    comment = "";

  uid = g_strdup_printf ("%s%s%s%s%s%s%s",
                         name,
                         *comment? " (":"", comment, *comment? ")":"",
                         *email? " <":"", email, *email? ">":"");
  gtk_label_set_text (GTK_LABEL (self->label_userid), uid);
  g_free (uid);
}



/* Helper to create the dialog.  PARENT is the parent window.  */
static void
create_dialog (GpaKeyGenDlg *self, GtkWidget *parent, const char *forcard)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *table;

  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *combo;
  GtkWidget *entry;
  GtkWidget *button;

  int rowidx, idx;


  dialog = gtk_dialog_new_with_buttons
    (forcard ? _("Generate card key") : _("Generate key"),
     GTK_WINDOW (parent),
     GTK_DIALOG_MODAL,
     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
     GTK_STOCK_OK, GTK_RESPONSE_OK,
     NULL);
  self->dialog = dialog;
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  vbox = GTK_DIALOG (dialog)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  table = gtk_table_new (9, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  rowidx = 0;

  label = gtk_label_new_with_mnemonic (_("_Algorithm: "));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
                    GTK_FILL, GTK_SHRINK, 0, 0);

  if (forcard)
    {
      label = gtk_label_new (forcard);
      gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 1, 2, rowidx, rowidx+1,
                        GTK_FILL, GTK_SHRINK, 0, 0);
      rowidx++;
    }
  else
    {
      combo = gtk_combo_box_new_text ();
      for (idx=0; algorithm_table[idx].name; idx++)
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
				   algorithm_table[idx].name);
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_table_attach (GTK_TABLE (table), combo, 1, 2, rowidx, rowidx+1,
                        GTK_FILL, GTK_SHRINK, 0, 0);
      self->entry_algo = combo;
      rowidx++;

      label = gtk_label_new_with_mnemonic (_("_Key size (bits): "));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
                        GTK_FILL, GTK_SHRINK, 0, 0);
      combo = gtk_combo_box_new_text ();
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "1024");
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "1536");
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "2048");
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "3072");
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 2);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      gtk_table_attach (GTK_TABLE (table), combo, 1, 2, rowidx, rowidx+1,
                        GTK_FILL, GTK_SHRINK, 0, 0);
      self->entry_keysize = combo;
      rowidx++;
    }


  label = gtk_label_new (NULL);  /* Dummy label.  */
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, rowidx, rowidx+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  rowidx++;


  label = gtk_label_new_with_mnemonic (_("User ID: "));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, rowidx, rowidx+1,
                    GTK_FILL, GTK_SHRINK, 0, 0);
  self->label_userid = label;
  rowidx++;

  label = gtk_label_new (NULL);  /* Dummy label.  */
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, rowidx, rowidx+1,
                    GTK_FILL, GTK_FILL, 0, 0);
  rowidx++;

  label = gtk_label_new_with_mnemonic (_("_Name: "));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, rowidx, rowidx+1,
                    GTK_FILL|GTK_EXPAND, GTK_SHRINK, 0, 0);
  self->entry_name = entry;
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (update_preview_cb), self);
  rowidx++;

  label = gtk_label_new_with_mnemonic (_("_Email: "));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, rowidx, rowidx+1,
                    GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  self->entry_email = entry;
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (update_preview_cb), self);
  rowidx++;

  label = gtk_label_new_with_mnemonic (_("_Comment: "));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
		    GTK_FILL, GTK_SHRINK, 0, 0);
  entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, rowidx, rowidx+1,
                    GTK_FILL|GTK_EXPAND, GTK_SHRINK, 0, 0);
  self->entry_comment = entry;
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (update_preview_cb), self);
  rowidx++;

  label = gtk_label_new_with_mnemonic (_("_Expires: "));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
                    GTK_FILL, GTK_SHRINK, 0, 0);
  button = gpa_date_box_new ();
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, rowidx, rowidx+1,
                    GTK_FILL, GTK_SHRINK, 0, 0);
  self->entry_expire = button;
  rowidx++;

  if (forcard)
    {
      label = gtk_label_new_with_mnemonic (_("Backup: "));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, rowidx, rowidx+1,
                        GTK_FILL, GTK_SHRINK, 0, 0);
      button = gtk_check_button_new ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, rowidx, rowidx+1,
                        GTK_FILL, GTK_SHRINK, 0, 0);
      self->entry_backup = button;
      gpa_add_tooltip (hbox,
                       _("If checked the encryption key will be created "
                         "and stored to a backup file and then loaded into "
                         "the card.  This is recommended so that encrypted "
                         "messages can be decrypted even if the card has a "
                         "malfunction."));
    }
  else
    self->entry_backup = NULL;

}


/* Run the "Generate Key" dialog and if the user presses OK, return
   the values from the dialog in a newly allocated gpa_keygen_para_t
   struct.  If FORCARD is not NULL display a dialog suitable for
   generation of keys on the OpenPGP smartcard; thye string will be
   shown to identify the capabilities of the card.  If the user
   pressed "Cancel", return NULL.  The returned struct has to be freed
   with gpa_keygen_para_free.  */
gpa_keygen_para_t *
gpa_key_gen_run_dialog (GtkWidget *parent, const char *forcard)
{
  GpaKeyGenDlg *self;
  gpa_keygen_para_t *params;

  self = xcalloc (1, sizeof *self);
  self->forcard = !!forcard;
  create_dialog (self, parent, forcard);
  g_signal_connect (G_OBJECT (self->dialog), "response",
                    G_CALLBACK (response_cb), self);


  gtk_widget_show_all (self->dialog);
  if (gtk_dialog_run (GTK_DIALOG (self->dialog)) != GTK_RESPONSE_OK)
    {
      gtk_widget_destroy (self->dialog);
      g_free (self);
      return NULL;
    }

  /* The user pressed OK: Populate gpa_keygen_para_t struct and
     return it.  */
  params = gpa_keygen_para_new ();
  params->name   = g_strdup (gtk_entry_get_text
                             (GTK_ENTRY (self->entry_name)));
  params->email  = g_strdup (gtk_entry_get_text
                             (GTK_ENTRY (self->entry_email)));
  params->comment= g_strdup (gtk_entry_get_text
                             (GTK_ENTRY (self->entry_comment)));

  if (forcard)
    params->algo = GPA_KEYGEN_ALGO_VIA_CARD;
  else
    {
      char *temp;
      int idx;

      idx = gtk_combo_box_get_active (GTK_COMBO_BOX (self->entry_algo));
      if (idx < 0 || idx >= DIM (algorithm_table)
          || !algorithm_table[idx].name)
        {
	  gpa_keygen_para_free (params);
          gtk_widget_destroy (self->dialog);
          g_free (self);
          g_return_val_if_reached (NULL);
        }
      params->algo = algorithm_table[idx].algo;
      temp = gtk_combo_box_get_active_text
        (GTK_COMBO_BOX (self->entry_keysize));
      params->keysize = temp? atoi (temp) : 0;
    }

  gpa_date_box_get_date (GPA_DATE_BOX (self->entry_expire), &params->expire);

  params->backup = self->entry_backup
                   ? gtk_toggle_button_get_active
                        (GTK_TOGGLE_BUTTON (self->entry_backup))
                   : 0;

  gtk_widget_destroy (self->dialog);
  g_free (self);

  return params;
}
