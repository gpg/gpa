/* confdialog.c - GPGME based configuration dialog for GPA.
 * Copyright (C) 2007 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <gpgme.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "i18n.h"



/* Return a new dialog widget.  */
static GtkDialog *
create_dialog (void)
{
  FILE *fp;
  char line[1024];
  GtkWidget *dialog;
  GtkWidget *vbox, *hbox, *label;

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog),
                        _("Crypto Backend Configuration"));

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

  hbox = gtk_hbox_new (FALSE, 1);
  label = gtk_label_new ("Left Item");
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
  label = gtk_button_new_with_label ("Right Item");
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  
  fp = popen ("gpgconf --list-options gpgsm" , "r");
  if (!fp)
    {
      perror ("failed to start gpgconf");
      exit (1);
    }

  while (fgets (line, sizeof line, fp))
    {
      int c;
      char *p, *endp;
      int fieldno;
      
      if (!*line || line[strlen(line)-1] != '\n')
        {
          if (*line && feof (fp))
            ; /* Last line not terminated - continue.  */
          else
            {
              fprintf (stderr, _("line too long - skipped\n"));
              while ( (c=fgetc (fp)) != EOF && c != '\n')
                ; /* Skip until end of line. */
              continue;
            }
        }
      /* Skip empty and comment lines.  */
      for (p=line; *p == ' '; p++)
        ;
      if (!*p || *p == '\n' || *p == '#')
        continue;

      /* Parse the colon separated fields. */
      for (fieldno=1, p = line; p; p = endp, fieldno++ )
        {
          endp = strchr (p, ':');
          if (endp)
            *endp++ = 0;
          switch (fieldno)
            {
            case 1:
              label = gtk_label_new (p);
              gtk_box_pack_start (GTK_BOX(vbox), label, TRUE, TRUE, 0);
              break;

            default:
              break;
            }
        }
    } 
  
  if (ferror (fp))
    fprintf (stderr, _("error reading gpgconf's output: %s\n"), 
             strerror (errno));

  pclose (fp);

  gtk_widget_show_all (dialog);

  gtk_main ();

  return GTK_DIALOG(dialog);
}


void
gpa_show_backend_config (void)
{
  create_dialog ();
}


int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  gpa_show_backend_config ();
  return 0;
}
        
