/* certchain.c  - Functions to show a certification chain.
 * Copyright (C) 2009 g10 Code GmbH.
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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gpa.h"
#include "gtktools.h"
#include "format-dn.h"
#include "certchain.h"

enum
  {
    CERTCHAIN_COLUMN_KEYID,
    CERTCHAIN_COLUMN_SERIALNO,
    CERTCHAIN_COLUMN_ISSUER,
    CERTCHAIN_N_COLUMNS
  };


static void
certchain_construct (GtkTreeView *tview)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes 
    (NULL, renderer, "text", CERTCHAIN_COLUMN_KEYID, NULL);
  /* FIXME  Use a function to get the title and tooltip.  */
  gpa_set_column_title 
    (column, _("Key ID"),
     _("The key ID is a short number to identify a certificate."));
  gtk_tree_view_append_column (tview, column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_("Serialno."), renderer, "text", CERTCHAIN_COLUMN_SERIALNO, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tview), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (_("Issuer"), renderer, "text", CERTCHAIN_COLUMN_ISSUER, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tview), column);

}


static void
append_row (GtkListStore *store, GtkTreeIter *iter,
            gpgme_key_t key, const char *errormsg)
{
  char *serialno;
  char *issuer;

  gtk_list_store_append (store, iter);
  if (errormsg || !key)
    {
      gtk_list_store_set (store, iter, CERTCHAIN_COLUMN_ISSUER, errormsg, -1);
      return;
    }

  if (key->issuer_serial && strlen (key->issuer_serial) <= 8)
    {
      unsigned long value = g_ascii_strtoull (key->issuer_serial, NULL, 16);
      serialno = g_strdup_printf ("%lu", value);
    }
  else if (key->issuer_serial)
    serialno = g_strdup_printf ("0x%s", key->issuer_serial);
  else
    serialno = NULL;

  issuer = gpa_format_dn (key->issuer_name); 

  gtk_list_store_set 
    (store, iter,
     CERTCHAIN_COLUMN_KEYID, gpa_gpgme_key_get_short_keyid (key),
     CERTCHAIN_COLUMN_SERIALNO, serialno,
     CERTCHAIN_COLUMN_ISSUER, issuer,
     -1);
  g_free (issuer);
  g_free (serialno);
}


static void
certchain_update (GtkWidget *tview, gpgme_key_t key)
{
  gpgme_error_t err;
  gpgme_ctx_t listctx;
  GtkListStore *store;
  GtkTreeIter iter;
  int maxdepth = 20;

  err = gpgme_new (&listctx);
  if (err)
    gpa_gpgme_error (err);
  gpgme_set_protocol (listctx, key->protocol);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (tview)));
  gtk_list_store_clear (store);
  
  append_row (store, &iter, key, NULL);

  gpgme_key_ref (key);
  while (key && key->chain_id
         && key->subkeys && strcmp (key->chain_id, key->subkeys->fpr))
    {
      if (!--maxdepth)
        {
          append_row (store, &iter, NULL, _("[chain too long]"));
          break;
        }
      err = gpgme_op_keylist_start (listctx, key->chain_id, 0);
      gpgme_key_unref (key);
      key = NULL;
      if (!err)
	err = gpgme_op_keylist_next (listctx, &key);
      gpgme_op_keylist_end (listctx);
      if (err)
        append_row (store, &iter, NULL, _("[issuer not found]"));
      else
        append_row (store, &iter, key, NULL);
    }
  gpgme_key_unref (key);
  gpgme_release (listctx);
}


/* Create the widget to show a certificate chain.  */
GtkWidget *
gpa_certchain_new (void)
{
  GtkListStore *store;
  GtkWidget *tview;
  
  store = gtk_list_store_new (CERTCHAIN_N_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
			      G_TYPE_STRING);
  tview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  certchain_construct (GTK_TREE_VIEW (tview));

  return tview;
}


/* Update the certificate chain list.  */
void
gpa_certchain_update (GtkWidget *list, gpgme_key_t key)
{
  GtkListStore *store;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));
  if (key && key->protocol == GPGME_PROTOCOL_CMS)
    certchain_update (list, key);
  else
    gtk_list_store_clear (store);
}
