/* gpgmetools.h - additional gpgme support functions for GPA.
   Copyright (C) 2002, Miguel Coca.
   Copyright (C) 2005, 2008 g10 Code GmbH.

   This file is part of GPA

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* A set of auxiliary functions for common tasks related to GPGME.  */

#ifndef GPGMETOOLS_H
#define GPGMETOOLS_H

#include <gtk/gtk.h>
#include <gpgme.h>

typedef enum
  {
    GPA_KEYGEN_ALGO_DSA_ELGAMAL,
    GPA_KEYGEN_ALGO_DSA,
    GPA_KEYGEN_ALGO_RSA
  } GPAKeyGenAlgo;

#define GPA_KEYGEN_ALGO_FIRST GPA_KEYGEN_ALGO_DSA_ELGAMAL
#define GPA_KEYGEN_ALGO_LAST GPA_KEYGEN_ALGO_RSA

typedef struct
 {
  /* User ID.  */
  gchar *userID;
  gchar *email;
  gchar *comment;

  /* Algorithm.  */
  GPAKeyGenAlgo algo;

  /* Key size.  */
  gint keysize;

  /* The password to use.  */
  gchar *password;

  /* The expiry date.  If expiryDate is not NULL it holds the expiry
     date, otherwise if interval is not zero, it defines the period of
     time until expiration together with unit (which is one of d, w,
     m, y), otherwise the user chose "never expire".  */
  GDate *expiryDate;
  gint interval;
  gchar unit;
} GPAKeyGenParameters;

/* Report an unexpected error in GPGME and quit the application.
   Better to use the macro instead of the function.  */
#define gpa_gpgme_error(err) _gpa_gpgme_error (err, __FILE__, __LINE__);
void _gpa_gpgme_error (gpg_error_t err, const char *file, int line);

/* The same as gpa_gpgme_error, without quitting.  */
void gpa_gpgme_warning (gpg_error_t err);

/* Initialize a gpgme_ctx_t for use with GPA.  */
gpgme_ctx_t gpa_gpgme_new (void);

/* Write the contents of the gpgme_data_t object to the file. Receives
   a filehandle instead of the filename, so that the caller can make
   sure the file is accesible before putting anything into data.  */
void dump_data_to_file (gpgme_data_t data, FILE *file);

/* Not really a gpgme function, but needed in most places
   dump_data_to_file is used.  Opens a file for writing, asking the
   user to overwrite if it exists and reporting any errors.  Returns
   NULL on failure, but you can assume the user has been informed of
   the error (or maybe he just didn't want to overwrite!).  */
FILE *gpa_fopen (const char *filename, GtkWidget *parent);

/* Do a gpgme_data_new_from_file and report any GPG_ERR_File_Error to
   the user.  */
gpg_error_t gpa_gpgme_data_new_from_file (gpgme_data_t *data,
					 const char *filename,
					 GtkWidget *parent);

/* Create a new gpgme_data_t from a file for writing, and return the
   file descriptor for the file.  Always reports all errors to the
   user.  */
int gpa_open_output (const char *filename, gpgme_data_t *data,
		     GtkWidget *parent);

/* Create a new gpgme_data_t from a file for reading, and return the
   file descriptor for the file.  Always reports all errors to the user.  */
int gpa_open_input (const char *filename, gpgme_data_t *data,
		    GtkWidget *parent);

/* Write the contents of the gpgme_data_t into the clipboard.  */
void dump_data_to_clipboard (gpgme_data_t data, GtkClipboard *clipboard);

/* Begin generation of a key with the given parameters.  It prepares
   the parameters required by Gpgme and returns whatever
   gpgme_op_genkey_start returns.  */
gpg_error_t gpa_generate_key_start (gpgme_ctx_t ctx, 
				    GPAKeyGenParameters *params);

/* Backup a key.  It exports both the public and secret keys to a
   file.  Returns TRUE on success and FALSE on error.  It displays
   errors to the user.  */
gboolean gpa_backup_key (const gchar *fpr, const char *filename);

GPAKeyGenParameters *key_gen_params_new (void);

void gpa_key_gen_free_parameters (GPAKeyGenParameters *params);

const gchar * gpa_algorithm_string (GPAKeyGenAlgo algo);

GPAKeyGenAlgo gpa_algorithm_from_string (const gchar * string);

/* Ownertrust strings.  */
const gchar *gpa_key_ownertrust_string (gpgme_key_t key);

/* Key validity strings.  */
const gchar *gpa_key_validity_string (gpgme_key_t key);

/* This is the function called by GPGME when it wants a
   passphrase.  */
gpg_error_t gpa_passphrase_cb (void *hook, const char *uid_hint,
			       const char *passphrase_info, 
			       int prev_was_bad, int fd);


/* Convenience functions to access key attributes, which need to be
   filtered before being displayed to the user. */

/* Return the user ID string, making sure it is properly UTF-8
   encoded.  Allocates a new string, which must be freed with
   g_free().  */
gchar *gpa_gpgme_key_get_userid (gpgme_user_id_t key);

/* Return the key fingerprint, properly formatted according to the key
   version.  Allocates a new string, which must be freed with
   g_free().  This is based on code from GPAPA's extract_fingerprint.  */
gchar *gpa_gpgme_key_format_fingerprint (const char *fpraw);

/* Return the short key ID of the indicated key. The returned string
   is valid as long as the key is valid.  */
const gchar *gpa_gpgme_key_get_short_keyid (gpgme_key_t key);

/* Convenience function to access key signature attibutes, much like
   the previous ones.  */

/* Return the user ID, making sure it is properly UTF-8 encoded.
   Allocates a new string, which must be freed with g_free().  */
gchar *gpa_gpgme_key_sig_get_userid (gpgme_key_sig_t sig);

/* Return the short key ID of the indicated key. The returned string
   is valid as long as the key is valid.  */
const gchar *gpa_gpgme_key_sig_get_short_keyid (gpgme_key_sig_t sig);

/* Return a string with the status of the key signature.  */
const gchar *gpa_gpgme_key_sig_get_sig_status (gpgme_key_sig_t sig,
					       GHashTable *revoked);

/* Return a string with the level of the key signature.  */
const gchar *gpa_gpgme_key_sig_get_level (gpgme_key_sig_t sig);

/* Return a string listing the capabilities of a key.  */
const gchar *gpa_get_key_capabilities_text (gpgme_key_t key);

/* Return a copy of the key array.  */
gpgme_key_t *gpa_gpgme_copy_keyarray (gpgme_key_t *keys);

/* Release all keys in the array KEYS as weel as ARRY itself.  */
void gpa_gpgme_release_keyarray (gpgme_key_t *keys);


#endif
