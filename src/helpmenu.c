/* helpmenu.c  -  The GNU Privacy Assistant
   Copyright (C) 1995 Spencer Kimball and Peter Mattis
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include "gpa.h"
#include "icons.h"
#include "gpl-text.h"
#include "helpmenu.h"


/* Display the about dialog.  */
void
gpa_help_about (GtkAction *action, GtkWindow *window)
{
  static const gchar *authors[] =
    {
      "Andy Ruddock",
      "Andreas Rönnquist",
      "Beate Esser",
      "Benedikt Wildenhain",
      "Bernhard Herzog",
      "Bernhard Reiter",
      "Can Berk Güder",
      "Daniel Leidert",
      "Daniel Nylander",
      "Emilian Nowak",
      "Jan-Oliver Wagner",
      "Josué Burgos",
      "Ling Li",
      "Marcus Brinkmann",
      "Markus Gerwinski",
      "Maxim Britov",
      "Michael Anckaert",
      "Michael Fischer v. Mollard",
      "Michael Mauch",
      "Michael Petzold",
      "Mick Ohrberg",
      "Miguel Coca",
      "Moritz Schulte",
      "Peter Gerwinski",
      "Peter Hanecak",
      "Peter Neuhaus",
      "Renato Martini",
      "Shell Hung",
      "Werner Koch",
      "Yasunari Imado",
      "Zdenek Hatas",
      NULL
    };
  static const gchar copyright[] =
    "Copyright \xc2\xa9 2000-2002 G-N-U GmbH\n"
    "Copyright \xc2\xa9 2002-2003 Miguel Coca\n"
    "Copyright \xc2\xa9 2005-2016 g10 Code GmbH";
  static const gchar website[] = "https://gnupg.org/related_software/gpa/";
  static const gchar website_label[] = "www.gnupg.org";
  char *comment;
  GdkPixbuf *logo;
  gpgme_engine_info_t engine;

  gpgme_get_engine_info (&engine);
  for (; engine; engine = engine->next)
    if (engine->protocol == GPGME_PROTOCOL_OpenPGP)
      break;
  comment = g_strdup_printf ("[%s]\n\n(GPGME %s)\n(GnuPG %s)\n\n%s",
                             BUILD_REVISION,
                             gpgme_check_version (NULL),
                             engine? engine->version : "?",
                             _("GPA is the GNU Privacy Assistant."));
  logo = gpa_create_icon_pixbuf ("gpa_logo");
  gtk_show_about_dialog (window,
			 "program-name", "GPA",
			 "version", VERSION,
			 "title", _("About GPA"),
			 /* Only clickable if
			    gtk_about_dialog_set_url_hook() is
			    used.  */
                         "website-label", website_label,
			 "website", website,
			 "copyright", copyright,
			 "comments", comment,
			 "authors", authors,
			 "license", get_gpl_text (),
			 "logo", logo,
			 /* TRANSLATORS: The translation of this string should
			  be your name and mail */
			 "translator-credits", _("translator-credits"),
			 NULL);
  if (logo)
    g_object_unref (logo);
  g_free (comment);
}
