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

#include "verify.xpm"
#include "blue_key.xpm"
#include "blue_yellow_key.xpm"
#include "blue_yellow_cardkey.xpm"
#include "brief.xpm"
#include "decrypt.xpm"
#include "detailed.xpm"
#include "edit.xpm"
#include "encrypt.xpm"
#include "export.xpm"
#include "gpa_blue_key.xpm"
#include "gpa_logo.xpm"
#include "gpa_yellow_key.xpm"
#include "import.xpm"
#include "keyring.xpm"
#include "keyringeditor.xpm"
#include "sign.xpm"
#include "wizard_backup.xpm"
#include "wizard_genkey.xpm"
#include "smartcard.xpm"


struct {
  const char *name;
  char **xpm;
} xpms[] = {
  { "gpa_logo",	gpa_logo_xpm},
  { GPA_STOCK_ENCRYPT, encrypt_xpm },
  { GPA_STOCK_DECRYPT, decrypt_xpm },
  { GPA_STOCK_SIGN, sign_xpm },
  { GPA_STOCK_VERIFY, verify_xpm },
  { GPA_STOCK_KEYMAN_SIMPLE, keyringeditor_xpm },
  { GPA_STOCK_EXPORT, export_xpm },
  { GPA_STOCK_IMPORT, import_xpm },
  { GPA_STOCK_BRIEF, brief_xpm },
  { GPA_STOCK_DETAILED, detailed_xpm },
  { GPA_STOCK_EDIT, edit_xpm },
  { GPA_STOCK_KEYMAN, keyring_xpm },
  { "gpa_blue_key", gpa_blue_key_xpm },
  { "gpa_yellow_key", gpa_yellow_key_xpm },
  { GPA_STOCK_PUBLIC_KEY, blue_key_xpm },
  { GPA_STOCK_SECRET_KEY, blue_yellow_key_xpm },
  { GPA_STOCK_SECRET_CARDKEY, blue_yellow_cardkey_xpm },
  { "wizard_genkey", wizard_genkey_xpm},
  { "wizard_backup", wizard_backup_xpm},
  { GPA_STOCK_CARDMAN, smartcard_xpm },
  { NULL, NULL }
};



GdkPixbuf *
gpa_create_icon_pixbuf (const char *name)
{
  char **xpm = NULL;
  int i;

  for (i = 0; xpms[i].name; i++)
    if (! strcmp (xpms[i].name, name))
      {
	xpm = xpms[i].xpm;
	break;
      }

  if (! xpm)
    {
      fprintf (stderr, "Icon `%s' not found\n", name);
      fflush (stderr);
      return NULL;
    }

  return gdk_pixbuf_new_from_xpm_data ((const char**) xpm);
}


static void
register_stock_icons (void)
{
  GdkPixbuf *pixbuf;
  GtkIconFactory *icon_factory;
  GtkIconSet *icon_set;
  GtkIconSource *icon_source;
  gint i;

  icon_factory = gtk_icon_factory_new ();

  for (i = 0; xpms[i].name; i++)
    {
      icon_set = gtk_icon_set_new ();
      icon_source = gtk_icon_source_new ();

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) xpms[i].xpm);
      gtk_icon_source_set_pixbuf (icon_source, pixbuf);
      gtk_icon_source_set_direction_wildcarded (icon_source, TRUE);
      gtk_icon_source_set_state_wildcarded (icon_source, TRUE);
      if (! strcmp (xpms[i].name, GPA_STOCK_PUBLIC_KEY)
	  || ! strcmp (xpms[i].name, GPA_STOCK_SECRET_KEY))
	{
	  /* FIXME: For the keylist icons, we disable scaling for now
	     for best visual results.  */
	  gtk_icon_source_set_size_wildcarded (icon_source, FALSE);
	  gtk_icon_source_set_size (icon_source, GTK_ICON_SIZE_LARGE_TOOLBAR);
	}

      gtk_icon_set_add_source (icon_set, icon_source);
      gtk_icon_source_free (icon_source);
      gtk_icon_factory_add (icon_factory, xpms[i].name, icon_set);
      gtk_icon_set_unref (icon_set);
    }

  /* Add a fake stock icon for the clipboard window.  */
  icon_set = gtk_icon_factory_lookup_default (GTK_STOCK_PASTE);
  icon_set = gtk_icon_set_copy (icon_set);
  gtk_icon_factory_add (icon_factory, GPA_STOCK_CLIPBOARD, icon_set);

  /* Add a fake stock icon for the file manager window.  */
  icon_set = gtk_icon_factory_lookup_default (GTK_STOCK_DIRECTORY);
  icon_set = gtk_icon_set_copy (icon_set);
  gtk_icon_factory_add (icon_factory, GPA_STOCK_FILEMAN, icon_set);

  gtk_icon_factory_add_default (icon_factory);

  g_object_unref (icon_factory);
}


void
gpa_register_stock_items (void)
{
  static const GtkStockItem items[] =
    {
      { GPA_STOCK_SIGN, N_("_Sign"), 0, 0, PACKAGE },
      { GPA_STOCK_VERIFY, N_("_Verify"), 0, 0, PACKAGE },
      { GPA_STOCK_ENCRYPT, N_("_Encrypt"), 0, 0, PACKAGE },
      { GPA_STOCK_DECRYPT, N_("_Decrypt"), 0, 0, PACKAGE },
      { GPA_STOCK_BRIEF, N_("_Brief"), 0, 0, PACKAGE },
      { GPA_STOCK_DETAILED, N_("_Detailed"), 0, 0, PACKAGE },
      { GPA_STOCK_KEYMAN,   N_("_Keyring Manager"), 0, 0, PACKAGE },
      { GPA_STOCK_CLIPBOARD, N_("_Clipboard"), 0, 0, PACKAGE },
      { GPA_STOCK_FILEMAN, N_("_File Manager"), 0, 0, PACKAGE },
      { GPA_STOCK_CARDMAN, N_("_Card Manager"), 0, 0, PACKAGE }
    };

  register_stock_icons ();

  gtk_stock_add_static (items, G_N_ELEMENTS (items));
}
