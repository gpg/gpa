/* fileverifydlg.c - Dialog for verifying files.
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2005, 2012 g10 Code GmbH.

   This file is part of GPA

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

#include <config.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#include <errno.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "verifydlg.h"

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
};

static GObjectClass *parent_class = NULL;

static void
gpa_file_verify_dialog_get_property (GObject     *object,
				      guint        prop_id,
				      GValue      *value,
				      GParamSpec  *pspec)
{
  GpaFileVerifyDialog *dialog = GPA_FILE_VERIFY_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value,
			  gtk_window_get_transient_for (GTK_WINDOW (dialog)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_file_verify_dialog_set_property (GObject     *object,
				   guint        prop_id,
				   const GValue      *value,
				   GParamSpec  *pspec)
{
  GpaFileVerifyDialog *dialog = GPA_FILE_VERIFY_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      gtk_window_set_transient_for (GTK_WINDOW (dialog),
				    g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_file_verify_dialog_finalize (GObject *object)
{
  GpaFileVerifyDialog *dialog = GPA_FILE_VERIFY_DIALOG (object);

  g_object_unref (dialog->ctx);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_file_verify_dialog_init (GpaFileVerifyDialog *dialog)
{
}

static GObject*
gpa_file_verify_dialog_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileVerifyDialog *dialog;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = GPA_FILE_VERIFY_DIALOG (object);
  /* Initialize */

  dialog->ctx = gpa_context_new ();
  /* Set up the dialog */
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  _("_Close"), GTK_RESPONSE_CLOSE, NULL);
  gpa_window_set_title (GTK_WINDOW (dialog), _("Verify documents"));
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  dialog->notebook = gtk_notebook_new ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			       dialog->notebook);
  /* Hide on response */
  g_signal_connect (G_OBJECT (dialog), "response",
		    G_CALLBACK (gtk_widget_hide), NULL);


  return object;
}

static void
gpa_file_verify_dialog_class_init (GpaFileVerifyDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_file_verify_dialog_constructor;
  object_class->finalize = gpa_file_verify_dialog_finalize;
  object_class->set_property = gpa_file_verify_dialog_set_property;
  object_class->get_property = gpa_file_verify_dialog_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_WINDOW,
				   g_param_spec_object
				   ("window", "Parent window",
				    "Parent window", GTK_TYPE_WIDGET,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_file_verify_dialog_get_type (void)
{
  static GType verify_dialog_type = 0;

  if (!verify_dialog_type)
    {
      static const GTypeInfo verify_dialog_info =
	{
	  sizeof (GpaFileVerifyDialogClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_file_verify_dialog_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GpaFileVerifyDialog),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gpa_file_verify_dialog_init,
	};

      verify_dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
						    "GpaFileVerifyDialog",
						    &verify_dialog_info, 0);
    }

  return verify_dialog_type;
}

/* Internal */


typedef struct
{
  gchar *fpr;
  gpgme_key_t key;
  gpgme_validity_t validity;
  unsigned long summary;
  time_t created;
  time_t expire;
  char *sigdesc;
  char *keydesc;
} SignatureData;

typedef enum
{
  SIG_KEYID_COLUMN,
  SIG_STATUS_COLUMN,
  SIG_USERID_COLUMN,
  SIG_DESC_COLUMN,
  SIG_N_COLUMNS
} SignatureListColumn;

/* Return the text of the "status" column in pango markup language.
 */
static gchar*
signature_status_label (SignatureData *data)
{
  gchar *text = NULL;
  gchar *color = NULL;
  gchar *label = NULL;

  if (data->summary & GPGME_SIGSUM_VALID)
    {
      text = _("Valid");
      color = "green";
    }
  else if (data->summary & GPGME_SIGSUM_RED)
    {
      text = _("Bad");
      color = "red";
    }
  else if (data->summary & GPGME_SIGSUM_KEY_MISSING)
    {
      text = _("Unknown Key");
      color = "red";
    }
  else if (data->summary & GPGME_SIGSUM_KEY_REVOKED)
    {
      text = _("Revoked Key");
      color = "red";
    }
  else if (data->summary & GPGME_SIGSUM_KEY_EXPIRED)
    {
      text = _("Expired Key");
      color = "orange";
    }
  else
    {
      /* If we arrived here we know the key is available, the signature is
       * not bad, but it's not completely valid. So, the signature is good
       * but the key is not valid. */
      text = _("Key NOT valid");
      color = "orange";
    }

  label = g_strdup_printf ("<span foreground=\"%s\" weight=\"bold\">%s</span>",
			   color, text);
  return label;
}

/* Add a signature to the list */
static void
add_signature_to_model (GtkListStore *store, SignatureData *data)
{
  GtkTreeIter iter;
  const gchar *keyid;
  gchar *userid;
  gchar *status;

  if (data->key)
    {
      keyid = gpa_gpgme_key_get_short_keyid (data->key);
    }
  else if (data->fpr && strlen (data->fpr) > 8)
    {
      /* We use the last 8 bytes of fingerprint for the keyID. */
      keyid = data->fpr + strlen (data->fpr) - 8;
    }
  else
    {
      keyid = "";
    }

  status = signature_status_label (data);

  userid = data->keydesc;
  if (!userid)
    userid = _("[Unknown user ID]");

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      SIG_KEYID_COLUMN, keyid,
		      SIG_STATUS_COLUMN, status,
		      SIG_USERID_COLUMN, userid,
                      SIG_DESC_COLUMN, data->sigdesc,
                      -1);

  g_free (status);

  gpgme_key_unref (data->key);
  g_free (data->sigdesc);
  g_free (data->keydesc);
  g_free (data);
}

/* Fill the list of signatures with the data from the verification */
static void
fill_sig_model (GtkListStore *store, gpgme_signature_t sigs,
		gpgme_ctx_t ctx)
{
  SignatureData *data;
  gpgme_signature_t sig;

  for (sig = sigs; sig; sig = sig->next)
    {
      data = g_malloc (sizeof (SignatureData));
      data->fpr = sig->fpr? g_strdup (sig->fpr) : NULL;
      data->validity = sig->validity;
      data->summary = sig->summary;
      data->created = sig->timestamp;
      data->expire = sig->exp_timestamp;
      data->sigdesc = gpa_gpgme_get_signature_desc (ctx, sig,
                                                    &data->keydesc, &data->key);
      add_signature_to_model (store, data);
    }
}


/* Create the list of signatures */
static GtkWidget *
signature_list (gpgme_signature_t sigs, gpgme_ctx_t ctx)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkWidget *list;

  store = gtk_list_store_new (SIG_N_COLUMNS, G_TYPE_STRING,
			      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
  gtk_widget_set_size_request (list, 400, 100);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Key ID"), renderer,
						     "text", SIG_KEYID_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Status"), renderer,
						     "markup",
						     SIG_STATUS_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("User Name"), renderer,
						     "text", SIG_USERID_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Description"), renderer,
						     "text", SIG_DESC_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  fill_sig_model (store, sigs, ctx);

  return list;
}

static GtkWidget *
verify_file_page (gpgme_signature_t sigs, const gchar *signed_file,
		  const gchar *signature_file, gpgme_ctx_t ctx)
{
  GtkWidget *vbox;
  GtkWidget *list;
  GtkWidget *label;
  GtkWidget *scrolled;

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  if (signed_file)
    {
      gchar *text = g_strdup_printf (_("Verified data in file: %s"),
				     signed_file);
      label = gtk_label_new (text);
      gtk_box_pack_start_defaults (GTK_BOX (vbox), label);
      g_free (text);
      text = g_strdup_printf (_("Signature: %s"), signature_file);
      label = gtk_label_new (text);
      gtk_box_pack_start_defaults (GTK_BOX (vbox), label);
      g_free (text);
    }

  label = gtk_label_new (_("Signatures:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label);

  list = signature_list (sigs, ctx);
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled), list);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), scrolled);

  return vbox;
}

/* API */

GtkWidget *gpa_file_verify_dialog_new (GtkWidget *parent)
{
  GpaFileVerifyDialog *dialog;

  dialog = g_object_new (GPA_FILE_VERIFY_DIALOG_TYPE,
			 "window", parent,
			 NULL);

  return GTK_WIDGET(dialog);
}

void
gpa_file_verify_dialog_set_title (GpaFileVerifyDialog *dialog,
                                  const char *title)
{
  if (dialog && title && *title)
    gpa_window_set_title (GTK_WINDOW (dialog), title);
}


void gpa_file_verify_dialog_add_file (GpaFileVerifyDialog *dialog,
				      const gchar *filename,
				      const gchar *signed_file,
				      const gchar *signature_file,
				      gpgme_signature_t sigs)
{
  GtkWidget *page;

  page = verify_file_page (sigs, signed_file, signature_file,
			   dialog->ctx->ctx);

  gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), page,
			    gtk_label_new (filename));
}
