/* helpmenu.h - The GNU Privacy Assistant.
   Copyright (C) 2000, 2001 G-N-U GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef HELPMENU_H__
#define HELPMENU_H__

#include <gtk/gtk.h>

/* Display the about dialog.  */
void gpa_help_about (GtkAction *action, GtkWindow *window);

static const GtkActionEntry gpa_help_menu_action_entries[] =
  {
    { "Help", NULL, N_("_Help"), NULL },
#if 0
    { "HelpContents", GTK_STOCK_HELP, NULL, NULL,
      N_("Open the GPA manual"), G_CALLBACK (gpa_help_contents) },
#endif
    { "HelpAbout", GTK_STOCK_ABOUT, NULL, NULL,
      N_("About this application"), G_CALLBACK (gpa_help_about) }
  };

#endif /* HELPMENU_H__ */
