/* icons.c  - GPA icons
 * Copyright (C) 2000, 2001 OpenIT GmbH
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i18n.h"

#include "icons.h"

#include "org.gnupg.gpa.src.h"

#include "gpa.h"

const char *icons_string =
  "<interface>"
    // blue_key.xpm
    "<object class='GtkImage' id='blue_key'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>blue_key.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // blue_yellow_cardkey.xpm
    "<object class='GtkImage' id='blue_yellow_cardkey'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>blue_yellow_cardkey.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // blue_yellow_key.xpm
    "<object class='GtkImage' id='blue_yellow_key'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>blue_yellow_key.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // brief.xpm
    "<object class='GtkImage' id='brief'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>brief.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // clipboard
    "<object class='GtkImage' id='clipboard'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='icon-name'>task-due</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // decrypt.xpm
    "<object class='GtkImage' id='decrypt'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>decrypt.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // delete.xpm
    "<object class='GtkImage' id='delete'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>delete.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // detailed.xpm
    "<object class='GtkImage' id='detailed'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>detailed.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // edit.xpm
    "<object class='GtkImage' id='edit'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>edit.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
    // encrypt.xpm
    "<object class='GtkImage' id='encrypt'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>encrypt.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
    // export.xpm
    "<object class='GtkImage' id='export'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>export.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // floppy.xpm
    // folder.xpm
    // gpa_blue_key.xpm
    "<object class='GtkImage' id='gpa_blue_key'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>gpa_blue_key.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // gpa_logo.xpm
    "<object class='GtkImage' id='gpa_logo'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>gpa_logo.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // gpa_yellow_key.xpm
    "<object class='GtkImage' id='gpa_yellow_key'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>gpa_yellow_key.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // harddisk.xpm
    // help.xpm
    // import.xpm
    "<object class='GtkImage' id='import'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>import.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // keyring.xpm
    "<object class='GtkImage' id='keyring'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>keyring.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
    // keyringeditor.xpm
    "<object class='GtkImage' id='keyringeditor'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>True</property>"
        "<property name='resource'>keyringeditor.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
    // open_folder.xpm
    // openfile.xpm
    // sign.xpm
    "<object class='GtkImage' id='sign'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>sign.xpm</property>"
        "<property name='icon_size'>1</property>"
      "</object>"
    // smartcard.xpm
    "<object class='GtkImage' id='smartcard'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>/org/gnupg/gpa/smartcard.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
      // verify
    "<object class='GtkImage' id='verify'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>/org/gnupg/gpa/verify.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
      // wizard_backup
    "<object class='GtkImage' id='wizard_backup'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>/org/gnupg/gpa/wizard_backup.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
      // wizard_genkey
    "<object class='GtkImage' id='wizard_genkey'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='resource'>/org/gnupg/gpa/wizard_genkey.xpm</property>"
        "<property name='icon_size'>1</property>"
    "</object>"
  "</interface>";

static void
register_stock_icons (void)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkScreen *screen = gdk_display_get_default_screen (display);
  GtkIconTheme *icon_theme;
  GResource *resource;

  g_application_set_resource_base_path (G_APPLICATION (get_gpa_application ()), "/org/gnupg/gpa");

  /*icon_theme = gtk_icon_theme_new ();*/
  icon_theme = gtk_icon_theme_get_for_screen (screen);

  resource = org_get_resource ();

  g_resources_register (resource);
  gtk_icon_theme_add_resource_path (icon_theme, "/org/gnupg/gpa");

}


void
gpa_register_stock_items (void)
{
  register_stock_icons ();

}
