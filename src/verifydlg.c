/* verifydlg.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2002 Miguel Coca.
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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "verifydlg.h"

typedef struct
{
  gchar *fpr;
  GpgmeKey key;
  GpgmeValidity validity;
  unsigned long summary;
  time_t created;
  time_t expire;
} SignatureData;

typedef enum
{
  SIG_KEYID_COLUMN,
  SIG_STATUS_COLUMN,
  SIG_USERID_COLUMN,
  SIG_N_COLUMNS
} SignatureListColumn;

/* Check whether the file is a detached signature and deduce the name of the
 * original file. Since we only have access to the filename, this is not
 * very solid.
 */
static gboolean
is_detached_sig (const gchar *filename, gchar **signed_file)
{
  gchar *extension;
  *signed_file = g_strdup (filename);
  extension = g_strrstr (*signed_file, ".");
  *extension++ = '\0';
  if (g_str_equal (extension, "sig") ||
      g_str_equal (extension, "sign"))
    {
      return TRUE;
    }
  else
    {
      g_free (*signed_file);
      *signed_file = NULL;
      return FALSE;
    }
}

/* Run gpgme_op_verify on the file.
 */
static gboolean
verify_file (const gchar *filename, GtkWidget *parent)
{
  GpgmeError err;
  GpgmeData input, plain;
  gchar *signed_file = NULL;
  GpgmeSigStat status;

  if (is_detached_sig (filename, &signed_file))
    {
      err = gpa_gpgme_data_new_from_file (&input, filename, parent);
      if (err == GPGME_File_Error)
	{
	  return FALSE;
	}
      else if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
      err = gpa_gpgme_data_new_from_file (&plain, signed_file, parent);
      if (err == GPGME_File_Error)
	{
	  gpgme_data_release (input);
	  return FALSE;
	}
      else if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
    }
  else
    {
      err = gpa_gpgme_data_new_from_file (&input, filename, parent);
      if (err == GPGME_File_Error)
	{
	  return FALSE;
	}
      else if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
      err = gpgme_data_new (&plain);
      if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
    }

  err = gpgme_op_verify (ctx, input, plain, &status);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  gpgme_data_release (input);
  gpgme_data_release (plain);
  if (status == GPGME_SIG_STAT_NOSIG)
    {
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

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
  else
    {
      /* If we arrived here we know the key is available, the signature is
       * not bad, but it's not completely valid. So, the signature is good
       * but the key is not valid. */
      text = _("Untrusted Key");
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
      keyid = gpa_gpgme_key_get_short_keyid (data->key, 0);
      userid = gpa_gpgme_key_get_userid (data->key, 0);
      status = signature_status_label (data);
    }
  else
    {
      /* This is really the keyID */
      keyid = data->fpr + 8;
      /* Dup the static string, so that we are always safe free'ing it*/
      userid = g_strdup (_("[Unknown user ID]"));
      status = signature_status_label (data);
    }
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      SIG_KEYID_COLUMN, keyid, 
		      SIG_STATUS_COLUMN, status,
		      SIG_USERID_COLUMN, userid, -1);

  g_free (userid);
  g_free (status);
  /* Someday we might want to keep the SignatureData in the table */
  gpgme_key_unref (data->key);
  g_free (data);
}

/* Fill the list of signatures with the data from the verification */
static void
fill_sig_model (GtkListStore *store, const gchar *filename, GtkWidget *parent)
{
  SignatureData *data;
  const gchar *fpr;
  int i;
  
  if (!verify_file (filename, parent))
    {
      return;
    }
  for (i = 0; (fpr = gpgme_get_sig_string_attr (ctx, i, GPGME_ATTR_FPR, 0));
       i++)
    {
      data = g_malloc (sizeof (SignatureData));
      data->fpr = g_strdup (fpr);
      data->key = NULL;
      gpgme_get_sig_key (ctx, i, &data->key);
      data->validity = gpgme_get_sig_ulong_attr (ctx, i, GPGME_ATTR_VALIDITY, 
						 0);
      data->summary = gpgme_get_sig_ulong_attr (ctx, i, GPGME_ATTR_SIG_SUMMARY,
						0);
      data->created = gpgme_get_sig_ulong_attr (ctx, i, GPGME_ATTR_CREATED, 0);
      data->expire = gpgme_get_sig_ulong_attr (ctx, i, GPGME_ATTR_EXPIRE, 0);
      add_signature_to_model (store, data);
    }
}

/* Create the list of signatures */
static GtkWidget *
signature_list (const gchar *filename, GtkWidget *parent)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkWidget *list;

  store = gtk_list_store_new (SIG_N_COLUMNS, G_TYPE_STRING,
			      G_TYPE_STRING, G_TYPE_STRING);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
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

  fill_sig_model (store, filename, parent);

  return list;
}

static GtkWidget *
verify_file_page (GtkWidget *parent, const gchar *filename)
{
  GtkWidget *vbox;
  GtkWidget *list;
  GtkWidget *label;
  
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  label = gtk_label_new (_("Signatures:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), label);

  list = signature_list (filename, parent);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), list);

  return vbox;
}

void gpa_file_verify_dialog_run (GtkWidget *parent, GList *files)
{
  GtkWidget *dialog;
  GtkWidget *notebook;
  GList *cur;
  
  dialog = gtk_dialog_new_with_buttons (_("Verify files"), GTK_WINDOW(parent),
					GTK_DIALOG_MODAL, 
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  notebook = gtk_notebook_new ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook);
  for (cur = files; cur; cur = g_list_next (cur))
    {
      GtkWidget *contents;
      contents = verify_file_page (parent, cur->data);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), contents,
				gtk_label_new (cur->data));
    }

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
