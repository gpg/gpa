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

static void keytable_fill (GHashTable *hash, gboolean secret)
{
  GpgmeError err;
  GpgmeKey key;
  gchar *fpr;
  err = gpgme_op_keylist_start( ctx, NULL, secret );
  if( err != GPGME_No_Error )
        gpa_gpgme_error (err);
  while( (err = gpgme_op_keylist_next( ctx, &key )) != GPGME_EOF )
    {
      if( err != GPGME_No_Error )
        gpa_gpgme_error (err);
      fpr = g_strdup (gpgme_key_get_string_attr (key, GPGME_ATTR_FPR, 
                                                NULL, 0 ));
      g_hash_table_insert (hash, fpr, key);
    }
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
