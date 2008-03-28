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


struct {
  const char *name;
  char **xpm;
} xpms[] = {
  { "gpa_logo",	gpa_logo_xpm},
  { "gpa-encrypt", encrypt_xpm },
  { "gpa-decrypt", decrypt_xpm },
  { "gpa-sign", sign_xpm },
  { "gpa-verify", verify_xpm },
  { "gpa-keyringeditor", keyringeditor_xpm },
  { "gpa-export", export_xpm },
  { "gpa-import", import_xpm },
  { "gpa-brief", brief_xpm },
  { "gpa-detailed", detailed_xpm },
  { "gpa-edit", edit_xpm },
  { "keyring", keyring_xpm },
  { "gpa_blue_key", gpa_blue_key_xpm },
  { "gpa_yellow_key", gpa_yellow_key_xpm },
  { "blue_key",	blue_key_xpm },
  { "blue_yellow_key", blue_yellow_key_xpm },
  { "wizard_genkey", wizard_genkey_xpm},
  { "wizard_backup", wizard_backup_xpm},
  { NULL, NULL }
};



static GdkPixmap *
pixmap_for_icon (GtkWidget *window, const char *name, GdkBitmap **r_mask)
{
    GtkStyle  *style;
    GdkPixmap *pix;
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
    
    style = gtk_widget_get_style (window);
    pix = gdk_pixmap_create_from_xpm_d (window->window, r_mask,
                                        &style->bg[GTK_STATE_NORMAL],
                                        xpm);
    return pix;
}


GtkWidget *
gpa_create_icon_widget (GtkWidget *window, const char *name)
{
    GdkBitmap *mask;
    GtkWidget *icon = NULL;
    GdkPixmap *pix;

    pix = pixmap_for_icon (window, name, &mask);
    if (pix)
      icon = gtk_pixmap_new (pix, mask);
    return icon;
}


GdkPixmap *
gpa_create_icon_pixmap (GtkWidget *window, const char *name, GdkBitmap **mask)
{
  return pixmap_for_icon (window, name, mask);
}


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
	
      gtk_icon_set_add_source (icon_set, icon_source);
      gtk_icon_source_free (icon_source);
      gtk_icon_factory_add (icon_factory, xpms[i].name, icon_set);
      gtk_icon_set_unref (icon_set);
    }

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
      { GPA_STOCK_KEYRING, N_("_Keyring Editor"), 0, 0, PACKAGE },
      { GPA_STOCK_BRIEF, N_("_Brief"), 0, 0, PACKAGE },
      { GPA_STOCK_DETAILED, N_("_Detailed"), 0, 0, PACKAGE }
    };

  register_stock_icons ();

  gtk_stock_add_static (items, G_N_ELEMENTS (items));
}
