/* gpgmetools.c - The GNU Privacy Assistant
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

#include "gpa.h"
#include "gpgmetools.h"

/* Report an unexpected error in GPGME and quit the application */
void gpa_gpgme_error (GpgmeError err)
{
  gchar *message = g_strdup_printf (_("Fatal Error in GPGME library:"
                                      "\n\n\t%s\n\n"
                                      "The application will be terminated"),
                                    gpgme_strerror (err));
  GtkWidget *label = gtk_label_new (message);
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Error"),
                                                   NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_STOCK_OK,
                                                   GTK_RESPONSE_NONE,
                                                   NULL);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG(dialog));
  exit (EXIT_FAILURE);
}


/* Write the contents of the GpgmeData object to the file. Receives a
 * filehandle instead of the filename, so that the caller can make sure the
 * file is accesible before putting anything into data.
 */
void dump_data_to_file (GpgmeData data, FILE *file)
{
  char buffer[128];
  int nread;
  GpgmeError err;

  err = gpgme_data_rewind (data);
  if (err != GPGME_No_Error)
    gpa_gpgme_error (err);
  while ( !(err = gpgme_data_read (data, buffer, sizeof(buffer), &nread)) ) 
    {
      fwrite ( buffer, nread, 1, file );
    }
  if (err != GPGME_EOF)
    gpa_gpgme_error (err);
  return;
}
