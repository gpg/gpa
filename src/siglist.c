/* siglist.c  -	 The GNU Privacy Assistant
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <config.h>
#include <gpapa.h>
#include "gpa.h"
#include "gpapastrings.h"
#include "siglist.h"

/*
 *  Implement a CList showing signatures
 */


typedef struct {
  /* The main window to use for the gpa_callback */
  GtkWidget * window;
} GPASigList;


/* Create and return a new signature CList. The window is the toplevel
 * window to be passed to the gpa_callback */
GtkWidget *
gpa_siglist_new (GtkWidget *window)
{
  GtkWidget *clist;
  gchar *titles[3] = {
    _("Key ID"), _("Validity"), _("Signature")
  };
  gint i;
  GPASigList * siglist = xmalloc (sizeof (*siglist));

  siglist->window = window;

  clist = gtk_clist_new_with_titles (3, titles);
  for (i = 0; i < 3; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clist), i);
  gtk_object_set_data_full (GTK_OBJECT (clist), "gpa_siglist", siglist, free);

  return clist;
} /* gpa_siglist_new */


/* Update the list of signatures to show the GList signatures. If key_id
 * is given, signatures for that key won't be listed (useful to filter
 * out the self signatures of a key */
void
gpa_siglist_set_signatures (GtkWidget * clist, GList * signatures,
			    gchar * key_id)
{
  GPASigList * siglist = gtk_object_get_data (GTK_OBJECT (clist),
					      "gpa_siglist");
  GpapaSignature *sig;
  GpapaSigValidity validity;
  gchar *contents[3];
  gint i, width;

  gtk_clist_freeze (GTK_CLIST (clist));
  gtk_clist_clear (GTK_CLIST (clist));
  while (signatures)
    {
      sig = (GpapaSignature *)(signatures->data);
      signatures = g_list_next (signatures);

      contents[0] = gpapa_signature_get_identifier (sig, gpa_callback,
						    siglist->window);

      /* if key_id is given and the same as the id of the signature
       * ignore the signature */
      if (key_id && strcmp (contents[0], key_id) == 0)
	continue;

      validity = gpapa_signature_get_validity (sig, gpa_callback,
					       siglist->window);
      contents[1] = gpa_sig_validity_string (validity);
      
      contents[2] = gpapa_signature_get_name (sig, gpa_callback,
					      siglist->window);
      if (!contents[2])
	contents[2] = _("[Unknown user ID]");
	  
      gtk_clist_append (GTK_CLIST (clist), contents);
    }

  gtk_container_check_resize (GTK_CONTAINER (clist));
  for (i = 0; i < 3; i++)
    {
      width = gtk_clist_optimal_column_width (GTK_CLIST (clist), i);
      gtk_clist_set_column_width (GTK_CLIST (clist), i, width);
    }

  gtk_clist_thaw (GTK_CLIST (clist));
} /* gpa_siglist_set_signatures */

