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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <config.h>
#include <gpapa.h>
#include "gpa.h"
#include "gpapastrings.h"
#include "siglist.h"


typedef struct {
  /* The main window to use for the gpa_callback */
  GtkWidget * window;

  /* Whether the widths of the columns were set to their optimal values */
  gboolean widths_set;
} GPASigList;


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
  siglist->widths_set = FALSE;

  clist = gtk_clist_new_with_titles (3, titles);
  for (i = 0; i < 3; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clist), i);
  gtk_object_set_data_full (GTK_OBJECT (clist), "gpa_siglist", siglist, free);

  return clist;
}

void
gpa_siglist_set_signatures (GtkWidget * clist, GList * signatures)
{
  GPASigList * siglist = gtk_object_get_data (GTK_OBJECT (clist),
					      "gpa_siglist");
  GpapaSignature *sig;
  GpapaSigValidity validity;
  gchar *contents[3];

  gtk_clist_freeze (GTK_CLIST (clist));
  gtk_clist_clear (GTK_CLIST (clist));
  while (signatures)
    {
      sig = (GpapaSignature *)(signatures->data);

      contents[0] = gpapa_signature_get_identifier (sig, gpa_callback,
						    siglist->window);

      validity = gpapa_signature_get_validity (sig, gpa_callback,
					       siglist->window);
      contents[1] = gpa_sig_validity_string (validity);

      contents[2] = gpapa_signature_get_name (sig, gpa_callback,
					      siglist->window);
      if (!contents[2])
	contents[2] = _("[Unknown user ID]");

      gtk_clist_append (GTK_CLIST (clist), contents);

      signatures = g_list_next (signatures);
    }

  if (!siglist->widths_set)
    {
      gint i, width;
      for (i = 0; i < 3; i++)
	{
	  width = gtk_clist_optimal_column_width (GTK_CLIST (clist), i);
	  gtk_clist_set_column_width (GTK_CLIST (clist), i, width);
	}
    }
  siglist->widths_set = TRUE;

  gtk_clist_thaw (GTK_CLIST (clist));
}

