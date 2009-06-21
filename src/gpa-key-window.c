/* gpa-key-window.c - per-key window
 * Copyright (C) 2009 g10 Code GmbH.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpa.h"   
//#include "convert.h"
//#include "siglist.h"
#include "gpa-key-window.h"




/* Local prototypes */
//static void gpa_key_details_finalize (GObject *object);



/************************************************************ 
 *******************   Implementation   *********************
 ************************************************************/


static void
gpa_key_window_class_init (void *class_ptr, void *class_data)
{
}

static void
close_button_cb (GtkButton *button, gpointer user_data)
{
  gtk_widget_destroy (GTK_WIDGET (user_data));
}

static void
gpa_key_window_init (GTypeInstance *instance, gpointer class_ptr)
{
  GpaKeyWindow *key_window = GPA_KEY_WINDOW (instance);
  GtkWidget *vbox;
  GtkWidget *button_box;
  GtkWidget *close_button;
  GtkWidget *halign;

  gtk_window_set_title (GTK_WINDOW (key_window), "Key details");
  gtk_window_set_default_size (GTK_WINDOW (key_window), 600, 480);
  key_window->key_details = gpa_key_details_new ();

  vbox = gtk_vbox_new (FALSE, 18);
  button_box = gtk_hbox_new (FALSE, 6);
  close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);

  g_signal_connect (G_OBJECT (close_button), "clicked",
		    G_CALLBACK (close_button_cb), key_window);

  halign = gtk_alignment_new (1, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (halign), close_button);
  gtk_container_add (GTK_CONTAINER (button_box), halign);

  gtk_container_set_border_width (GTK_CONTAINER (key_window), 12);

  gtk_box_pack_start (GTK_BOX (vbox), key_window->key_details,
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 0);
  

  gtk_container_add (GTK_CONTAINER (key_window), vbox);
  gtk_widget_show_all (vbox);

  
}


#if 0
static void
gpa_key_details_finalize (GObject *object)
{  
  GpaKeyDetails *kdt = GPA_KEY_DETAILS (object);

  if (kdt->current_key)
    {
      gpgme_key_unref (kdt->current_key);
      kdt->current_key = NULL;
    }
  if (kdt->signatures_list)
    {
      g_object_unref (kdt->signatures_list);
      kdt->signatures_list = NULL;
    }
  if (kdt->certchain_list)
    {
      g_object_unref (kdt->certchain_list);
      kdt->certchain_list = NULL;
    }
  if (kdt->subkeys_list)
    {
      g_object_unref (kdt->subkeys_list);
      kdt->subkeys_list = NULL;
    }

  parent_class->finalize (object);
}
#endif

/* Construct the class.  */
GType
gpa_key_window_get_type (void)
{
  static GType this_type = 0;
  
  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaKeyWindowClass),
	  NULL,
	  NULL,
	  (GClassInitFunc) gpa_key_window_class_init,
	  NULL,
	  NULL, /* class_data */
	  sizeof (GpaKeyWindow),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) gpa_key_window_init
	};
      
      this_type = g_type_register_static (GTK_TYPE_WINDOW,
                                          "GpaKeyWindow",
                                          &this_info, 0);
    }
  
  return this_type;
}



GtkWidget *
gpa_key_window_new ()
{
  return GTK_WIDGET (g_object_new (GPA_KEY_WINDOW_TYPE, NULL));
}


/* Update the key details widget KEYDETAILS with KEY.  The caller also
   needs to provide the number of keys, so that the widget may show a
   key count instead of a key.  The actual key details are only shown
   if KEY is not NULL and KEYCOUNT is 1.  */
void
gpa_key_window_update (GtkWidget *widget, gpgme_key_t key)
{
  GpaKeyWindow *key_window;

  g_return_if_fail (IS_GPA_KEY_WINDOW (widget));
  key_window = GPA_KEY_WINDOW (widget);

  gpa_key_details_update (key_window->key_details, key, 1);
}
