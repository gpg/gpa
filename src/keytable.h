/* keytable.h  -  The GNU Privacy Assistant
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

/*
 * Hash table of all the keys in the keyring.
 */

#ifndef KEYTABLE_H
#define KEYTABLE_H

#include <glib.h>
#include <gpgme.h>

typedef struct _GPAKeyTable GPAKeyTable;

typedef void (*GPATableFunc) (const gchar * fpr, gpgme_key_t key, gpointer data);

/* Creates a new keytable */
GPAKeyTable *gpa_keytable_new (void);

/* Reloads the table from gpg */
void gpa_keytable_reload (GPAKeyTable * table);

/* Load a single key from gpg */
void gpa_keytable_load_key (GPAKeyTable * table, const gchar * fpr);

/* Load several keys from gpg */
void gpa_keytable_load_keys (GPAKeyTable * table, const gchar ** keys);

/* Return the key with a given fingerprint. It does not provide a reference 
 * for the user */
gpgme_key_t gpa_keytable_lookup (GPAKeyTable * table, const gchar * fpr);
gpgme_key_t gpa_keytable_secret_lookup (GPAKeyTable * table, const gchar * fpr);

/* Call the function "func" for each key and value in the table */
void gpa_keytable_foreach (GPAKeyTable * table, GPATableFunc func,
			   gpointer data);
void gpa_keytable_secret_foreach (GPAKeyTable * table, GPATableFunc func,
                                  gpointer data);

/* How many keys are there in the keyring? */
gint gpa_keytable_size (GPAKeyTable * table);
gint gpa_keytable_secret_size (GPAKeyTable * table);

/* Remove a key from the hash table */
void gpa_keytable_remove (GPAKeyTable * table, const gchar * fpr);

/* Delete the hash table and all it's elements */
void gpa_keytable_destroy (GPAKeyTable * table);

#endif /* KEYTABLE_H */
