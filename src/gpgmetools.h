/* gpgmetools.h - additional gpgme support functions for GPA.
   Copyright (C) 2002, Miguel Coca.
   Copyright (C) 2005, 2008 g10 Code GmbH.
   Copyright (C) 2015 g10 Code GmbH.

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

#include "gpacontext.h"

/* Internal algorithm identifiers, describing which keys to
   create.  */
typedef enum
  {
    GPA_KEYGEN_ALGO_RSA_RSA,
    GPA_KEYGEN_ALGO_RSA_ELGAMAL,
    GPA_KEYGEN_ALGO_RSA,
    GPA_KEYGEN_ALGO_DSA_ELGAMAL,
    GPA_KEYGEN_ALGO_DSA,
    GPA_KEYGEN_ALGO_VIA_CARD
  } gpa_keygen_algo_t;



typedef struct
{
  /* User ID.  */
  gchar *name;
  gchar *email;
  gchar *comment;

  /* Algorithm.  */
  gpa_keygen_algo_t algo;

  /* Key size.  */
  gint keysize;

  /* The password to use.  */
  gchar *password;

  /* Epiration date.  It is only used if it is valid.  */
  GDate expire;

  /* True if the encryption key is created with an backup file.  */
  gboolean backup;

  /* Used to return an error description.  */
  char *r_error_desc;

} gpa_keygen_para_t;


/* An object to collect information about key imports.  */
struct gpa_import_result_s
{
  unsigned int files;     /* # of files imported.  */
  unsigned int bad_files; /* # of files with errors.  */

  /* To avoid breaking translated strings the variables below are int
     and not unsigned int as they should be for counters.  */
  int considered;
  int imported;
  int unchanged;
  int secret_read;
  int secret_imported;
  int secret_unchanged;
};
typedef struct gpa_import_result_s *gpa_import_result_t;



/* Report an unexpected error in GPGME and quit the application.
   Better to use the macro instead of the function.  */
#define gpa_gpgme_error(err) \
         do { _gpa_gpgme_error (err, __FILE__, __LINE__); } while (0)
void _gpa_gpgme_error (gpg_error_t err,
                       const char *file, int line) G_GNUC_NORETURN;

/* The same as gpa_gpgme_error, without quitting.  */
#define gpa_gpgme_warn(err,desc,ctx)                                       \
  do { _gpa_gpgme_warn (err, desc, ctx, __FILE__, __LINE__); } while (0)
#define gpa_gpgme_warning_ext(err,desc) \
  do { _gpa_gpgme_warn (err, desc, NULL, __FILE__, __LINE__); } while (0)
#define gpa_gpgme_warning(err) \
  do { _gpa_gpgme_warn (err, NULL, NULL, __FILE__, __LINE__); } while (0)
void _gpa_gpgme_warn (gpg_error_t err, const char *desc, GpaContext *ctx,
                      const char *file, int line);

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
   the error (or maybe he just didn't want to overwrite!).  The
   filename of the file that is actually open (if FILENAME already
   exists, then the user can choose a different file) is saved in
   *FILENAME_USED.  It must be xfreed.  This is set even if this
   function returns NULL!  */
FILE *gpa_fopen (const char *filename, GtkWidget *parent,
                 char **filename_used);

/* Do a gpgme_data_new_from_file and report any GPG_ERR_File_Error to
   the user.  */
gpg_error_t gpa_gpgme_data_new_from_file (gpgme_data_t *data,
					 const char *filename,
					 GtkWidget *parent);

/* Create a new gpgme_data_t from a file for writing, and return the
   file descriptor for the file.  Always reports all errors to the
   user.  The _direct variant does not check for overwriting.  The
   filename of the file that is actually open (if FILENAME already
   exists, then the user can choose a different file) is saved in
   *FILENAME_USED.  It must be xfreed.  This is set even if this
   function returns NULL!  */
int gpa_open_output_direct (const char *filename, gpgme_data_t *data,
			    GtkWidget *parent);
int gpa_open_output (const char *filename, gpgme_data_t *data,
		     GtkWidget *parent, char **filename_used);

/* Create a new gpgme_data_t from a file for reading, and return the
   file descriptor for the file.  Always reports all errors to the user.  */
int gpa_open_input (const char *filename, gpgme_data_t *data,
		    GtkWidget *parent);

/* Write the contents of the gpgme_data_t into the clipboard.  */
int dump_data_to_clipboard (gpgme_data_t data, GtkClipboard *clipboard);

/* Begin generation of a key with the given parameters.  It prepares
   the parameters required by Gpgme and returns whatever
   gpgme_op_genkey_start returns.  */
gpg_error_t gpa_generate_key_start (gpgme_ctx_t ctx,
				    gpa_keygen_para_t *params);

/* Backup a key.  It exports both the public and secret keys to a
   file.  IS_X509 tells the function that the fingerprint is from an
   X.509 key.  Returns TRUE on success and FALSE on error.  It
   displays errors to the user.  */
gboolean gpa_backup_key (const gchar *fpr, const char *filename, int is_x509);

gpa_keygen_para_t *gpa_keygen_para_new (void);

void gpa_keygen_para_free (gpa_keygen_para_t *params);

/* Ownertrust strings.  */
const gchar *gpa_key_ownertrust_string (gpgme_key_t key);

/* Key validity strings.  */
const gchar *gpa_key_validity_string (gpgme_key_t key);

/* UID validity strings.  */
const gchar *gpa_uid_validity_string (gpgme_user_id_t uid);

/* Function to manage import results.  */
void gpa_gpgme_update_import_results (gpa_import_result_t result,
                                      unsigned int files,
                                      unsigned int bad_files,
                                      gpgme_import_result_t info);
void gpa_gpgme_show_import_results (GtkWidget *parent,
                                    gpa_import_result_t result);



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

/* Return a human readable string with the status of the signature
   SIG.  */
char *gpa_gpgme_get_signature_desc (gpgme_ctx_t ctx, gpgme_signature_t sig,
                                    char **r_keydesc, gpgme_key_t *r_key);


/* Return a string listing the capabilities of a key.  */
const gchar *gpa_get_key_capabilities_text (gpgme_key_t key);

/* Return a copy of the key array.  */
gpgme_key_t *gpa_gpgme_copy_keyarray (gpgme_key_t *keys);

/* Release all keys in the array KEYS as weel as ARRY itself.  */
void gpa_gpgme_release_keyarray (gpgme_key_t *keys);

/* Try switching to the gpg2 backend.  */
void gpa_switch_to_gpg2 (const char *gpg_binary, const char *gpgsm_binary);

/* Return true if the gpg engine has at least version NEED_VERSION.  */
int is_gpg_version_at_least (const char *need_version);

/* Run a simple gpg command.  */
gpg_error_t gpa_start_simple_gpg_command (gboolean (*cb)
                                          (void *opaque, char *line),
                                          void *cb_arg,
                                          gpgme_protocol_t protocol,
                                          int use_stderr,
                                          const char *first_arg, ...
                                          ) G_GNUC_NULL_TERMINATED;

void gpa_start_agent (void);

/* Funtions to check user inputs in the same way gpg does.  */
const char *gpa_validate_gpg_name (const char *name);
const char *gpa_validate_gpg_email (const char *email);
const char *gpa_validate_gpg_comment (const char *comment);


#endif /*GPGMETOOLS_H*/
