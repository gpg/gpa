/* keygendlg.h  -  The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
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

#ifndef KEYGENDLG_H
#define KEYGENDLG_H

#include <gtk/gtk.h>
#include <gpapa.h>

typedef struct {
  /* user id */
  gchar *userID, *email, *comment;

  /* algorithm */
  GpapaAlgo algo;

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

GPAKeyGenParameters * gpa_key_gen_run_dialog (GtkWidget * parent);

void gpa_key_gen_free_parameters(GPAKeyGenParameters *);


#endif /* KEYSIGNDLG_H */
