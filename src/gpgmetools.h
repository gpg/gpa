/* gpgmetools.h - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
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

/* A set of auxiliary functions for common tasks related to GPGME */

#ifndef GPGMETOOLS_H
#define GPGMETOOLS_H

#include <gpgme.h>

typedef enum
  {
    GPA_KEYGEN_ALGO_DSA_ELGAMAL,
    GPA_KEYGEN_ALGO_DSA,
    GPA_KEYGEN_ALGO_RSA
  } GPAKeyGenAlgo;

#define GPA_KEYGEN_ALGO_FIRST GPA_KEYGEN_ALGO_DSA_ELGAMAL
#define GPA_KEYGEN_ALGO_LAST GPA_KEYGEN_ALGO_RSA

typedef struct {
  /* user id */
  gchar *userID, *email, *comment;

  /* algorithm */
  GPAKeyGenAlgo algo;

  /* key size. */
  gint keysize;

  /* the password to use */
  gchar * password;

  /* the expiry date. if expiryDate is not NULL it holds the expiry
   * date, otherwise if interval is not zero, it defines the period of
   * time until expiration together with unit (which is one of d, w, m,
   * y), otherwise the user chose "never expire".
   */
  GDate *expiryDate;
  gint interval;
  gchar unit;

  /* if true, generate a revocation certificate */
  gboolean generate_revocation;

  /* if true, send the key to a keyserver */
  gboolean send_to_server;
  
} GPAKeyGenParameters;

/* Report an unexpected error in GPGME and quit the application.
 * Better to use the macro instead of the function
 */
#define gpa_gpgme_error(err) _gpa_gpgme_error (err, __FILE__, __LINE__);
void _gpa_gpgme_error (GpgmeError err, const char *file, int line);

/* Write the contents of the GpgmeData object to the file. Receives a
 * filehandle instead of the filename, so that the caller can make sure the
 * file is accesible before putting anything into data.
 */
void dump_data_to_file (GpgmeData data, FILE *file);

/* Read the contents of the clipboard into the GpgmeData object.
 */
void fill_data_from_clipboard (GpgmeData data, GtkClipboard *clipboard);

/* Write the contents of the GpgmeData into the clipboard
 */
void dump_data_to_clipboard (GpgmeData data, GtkClipboard *clipboard);

/* Generate a key with the given parameters. It prepares the parameters
 * required by Gpgme and returns whatever gpgme_op_genkey returns.
 */
GpgmeError gpa_generate_key (GPAKeyGenParameters *params);

/* Backup a key. It exports both the public and secret keys to a file.
 * Returns TRUE on success and FALSE on error. It displays errors to the
 * user.
 */
gboolean gpa_backup_key (const gchar *fpr, const char *filename);

GPAKeyGenParameters * key_gen_params_new (void);

void gpa_key_gen_free_parameters (GPAKeyGenParameters *params);

const gchar * gpa_algorithm_string (GPAKeyGenAlgo algo);

GPAKeyGenAlgo gpa_algorithm_from_string (const gchar * string);

/* This is the function called by GPGME when it wants a passphrase */
const char * gpa_passphrase_cb (void *opaque, const char *desc, void **r_hd);


/* Convenience functions to access key attributes, which need to be filtered
 * before being displayed to the user. */

gchar *gpa_gpgme_key_get_userid (GpgmeKey key, int idx);

#endif
