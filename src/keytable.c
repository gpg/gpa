/* keytable.c  -  The GNU Privacy Assistant
 *      Copyright (C) 2002 Miguel Coca
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

#include "gpa.h"
#include "keytable.h"
#include "gtktools.h"

struct _GPAKeyTable
{
  GHashTable *public_hash;
  GHashTable *secret_hash;
};

/* Auxiliary functions */

static void do_keylisting (GpgmeCtx ctx, GHashTable *hash, gboolean secret)
{
  GpgmeError err;
  GpgmeKey key;
  gchar *fpr;
  GtkWidget *window, *table, *label, *progress;
  gchar *text;
  gint i = 0;

  /* Create the progress dialog */
  window = gtk_dialog_new ();
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_title (GTK_WINDOW (window), _("GPA: Loading keyring"));
  table = gtk_table_new (2, 2, FALSE);
  label = gtk_label_new (secret ? 
			 _("Loading secret keys") : _("Loading public keys"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  label = gtk_label_new ("0");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  progress = gtk_progress_bar_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), progress, 0, 2, 1, 2);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox), table);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  gtk_widget_show_all (window);
  /* Load the keys */
  while( (err = gpgme_op_keylist_next( ctx, &key )) != GPGME_EOF )
    {
      const char *s;

      if( err != GPGME_No_Error )
        gpa_gpgme_error (err);
      s = gpgme_key_get_string_attr (key, GPGME_ATTR_FPR, NULL, 0 );
      g_assert (s);
      fpr = g_strdup (s);
      g_hash_table_insert (hash, fpr, key);
      if ( !(++i % 10) )
        {
          text = g_strdup_printf ("%i", i);
          /* Change the progress bar */
          gtk_label_set_text (GTK_LABEL (label), text);
          g_free (text);
          gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress));
          /* Redraw */
          while (gtk_events_pending ())
            {
              gtk_main_iteration ();
            }
        }
    }
  gtk_widget_destroy (window);
}

static void keytable_fill (GHashTable *hash, gboolean secret)
{
  GpgmeError err;
  GpgmeCtx ctx = gpa_gpgme_new ();

  err = gpgme_op_keylist_start( ctx, NULL, secret );
  if( err != GPGME_No_Error )
    gpa_gpgme_error (err);
  do_keylisting (ctx, hash, secret);
  gpgme_release (ctx);
}

static void load_keys (GHashTable *hash, const gchar **keys, gboolean secret)
{
  GpgmeError err;
  GpgmeCtx ctx = gpa_gpgme_new ();

  err = gpgme_op_keylist_ext_start (ctx, keys, secret, 0);
  if( err != GPGME_No_Error )
        gpa_gpgme_error (err);
  do_keylisting (ctx, hash, secret);
  gpgme_release (ctx);
}

static gboolean true (const gchar *fpr, GpgmeKey key, gpointer data)
{
  return TRUE;
}

static void keytable_empty (GPAKeyTable * table)
{
  g_hash_table_foreach_remove (table->public_hash, (GHRFunc) true, NULL);
  g_hash_table_foreach_remove (table->secret_hash, (GHRFunc) true, NULL);
}

static void load_key (GHashTable *hash, const gchar *fpr, gboolean secret)
{
  GpgmeError err;
  GpgmeKey key;
  gchar *key_fpr;
  GpgmeCtx ctx = gpa_gpgme_new ();

  /* List the key with a specific fingerprint */
  err = gpgme_op_keylist_start( ctx, fpr, secret );
  if( err != GPGME_No_Error )
    {
      gpa_gpgme_error (err);
    }
  while( (err = gpgme_op_keylist_next( ctx, &key )) != GPGME_EOF )
    {
      if( err != GPGME_No_Error )
        gpa_gpgme_error (err);
      key_fpr = g_strdup (gpgme_key_get_string_attr (key, GPGME_ATTR_FPR, 
                                                     NULL, 0 ));
      if (g_str_equal (fpr, key_fpr))
        {
          g_hash_table_insert (hash, key_fpr, key);
        }
      else
        {
          g_free (key_fpr);
        }
    }
  gpgme_release (ctx);
}

/* Public functions for the table */

GPAKeyTable *gpa_keytable_new (void)
{
  GPAKeyTable *table;
  table = g_malloc (sizeof(GPAKeyTable));
  table->public_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              (GDestroyNotify) g_free,
                                              (GDestroyNotify) 
                                              gpgme_key_release);
  table->secret_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              (GDestroyNotify) g_free,
                                              (GDestroyNotify)
                                              gpgme_key_release);
  keytable_fill (table->public_hash, FALSE);
  keytable_fill (table->secret_hash, TRUE);
  return table;
}

void gpa_keytable_reload (GPAKeyTable * table)
{
  keytable_empty (table);
  keytable_fill (table->public_hash, FALSE);
  keytable_fill (table->secret_hash, TRUE);
}

void gpa_keytable_load_key (GPAKeyTable * table, const gchar * fpr)
{
  /* Delete the old key */
  gpa_keytable_remove (table, fpr);
  /* Load the key */
  load_key (table->public_hash, fpr, FALSE);
  load_key (table->secret_hash, fpr, TRUE);
}

void gpa_keytable_load_keys (GPAKeyTable * table, const gchar **keys)
{
  gint i;
  
  /* Delete the old keys */
  for (i = 0; keys[i]; i++)
    {
      gpa_keytable_remove (table, keys[i]);
    }
  /* Load the keys */
  load_keys (table->public_hash, keys, FALSE);
  load_keys (table->secret_hash, keys, TRUE);
}

GpgmeKey gpa_keytable_lookup (GPAKeyTable * table, const gchar * fpr)
{
  return (GpgmeKey) g_hash_table_lookup (table->public_hash, fpr);
}

GpgmeKey gpa_keytable_secret_lookup (GPAKeyTable * table, const gchar * fpr)
{
  return (GpgmeKey) g_hash_table_lookup (table->secret_hash, fpr);
}

void gpa_keytable_foreach (GPAKeyTable * table, GPATableFunc func,
			   gpointer data)
{
  g_hash_table_foreach (table->public_hash, (GHFunc) func, data );
}

void gpa_keytable_secret_foreach (GPAKeyTable * table, GPATableFunc func,
                                  gpointer data)
{
  g_hash_table_foreach (table->secret_hash, (GHFunc) func, data );
}

gint gpa_keytable_size (GPAKeyTable * table)
{
  return g_hash_table_size (table->public_hash);
}

gint gpa_keytable_secret_size (GPAKeyTable * table)
{
  return g_hash_table_size (table->secret_hash);
}

void gpa_keytable_remove (GPAKeyTable * table, const gchar * fpr)
{
  g_hash_table_remove (table->secret_hash, fpr);
  g_hash_table_remove (table->public_hash, fpr);
}

void gpa_keytable_destroy (GPAKeyTable * table)
{
  g_hash_table_destroy (table->public_hash);
  g_hash_table_destroy (table->secret_hash);
  g_free (table);
}
