/* keylist.c  -	 The GNU Privacy Assistant
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
#include <gpgme.h>
#include "icons.h"
#include "gpa.h"
#include "gpapastrings.h"
#include "gtktools.h"
#include "keylist.h"


struct _GPAKeyList;

/* A column definition. Consists of a title and a function that returns
 * the values for that column given a public key */

typedef void (*ColumnValueFunc)(GpgmeKey key,
				struct _GPAKeyList * keylist,
				gchar ** label,
				gboolean * free_label,
				GdkPixmap ** pixmap,
				GdkBitmap ** mask);

typedef struct {
  /* column title. May be NULL to indicate no title. Empty strings won't
   * work properly because they're passed through _(). */
  gchar * title;

  /* function to return the value */
  ColumnValueFunc value_func;
} ColumnDef;


struct _GPAKeyList {
  /* the number and definitins of the currently visible columns */
  gint ncolumns;
  ColumnDef ** column_defs;

  /* flag whether the column_defs have changed since the last update */
  gboolean column_defs_changed;

  /* maximum number of columns */
  gint max_columns;
  
  /* The CList widget */
  GtkWidget * clist;

  /* the main window to use as the the gpa_callback parameter */
  GtkWidget * window;

};
typedef struct _GPAKeyList GPAKeyList;


/* internal API */

/* Auxiliary struct required because we need to keep two state elements when
 * adding keys to the list: the list itself and the hash table of previously
 * selected keys. */
struct keylist_fill_data 
{
  GPAKeyList *keylist;
  GHashTable *selected_keys;
};

static void keylist_free (gpointer param);
static void keylist_fill_list (GPAKeyList * keylist);
static void keylist_fill_column_titles (GPAKeyList * keylist);
static void keylist_fill_row (const gchar * fpr, GpgmeKey key,
                              struct keylist_fill_data *data);

/*
 * ColumnDef functions
 */

static void
get_name_value (GpgmeKey key, GPAKeyList * keylist,
		gchar ** label, gboolean * free_label, GdkPixmap ** pixmap,
		GdkBitmap ** mask)
{
  *label = gpa_gpgme_key_get_userid (key, 0);
  *free_label = TRUE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_trust_value (GpgmeKey key, GPAKeyList * keylist,
		 gchar ** label, gboolean * free_label, GdkPixmap ** pixmap,
		 GdkBitmap ** mask)
{
  GpgmeValidity trust;

  trust = gpgme_key_get_ulong_attr (key, GPGME_ATTR_VALIDITY, NULL, 0);
  *label = gpa_trust_string (trust);
  *free_label = FALSE;
  *pixmap = NULL;
  *mask = NULL;
}
  
static void
get_ownertrust_value (GpgmeKey key, GPAKeyList * keylist,
		      gchar ** label, gboolean * free_label,
		      GdkPixmap ** pixmap, GdkBitmap ** mask)
{
  GpgmeValidity trust;

  trust = gpgme_key_get_ulong_attr (key, GPGME_ATTR_OTRUST, NULL, 0);
  *label = gpa_trust_string (trust);
  *free_label = FALSE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_expirydate_value (GpgmeKey key, GPAKeyList * keylist,
		      gchar ** label, gboolean * free_label,
		      GdkPixmap ** pixmap, GdkBitmap ** mask)
{
  *label = gpa_expiry_date_string 
          (gpgme_key_get_ulong_attr( key, GPGME_ATTR_EXPIRE, NULL, 0 ));
  *free_label = TRUE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_identifier_value (GpgmeKey key, GPAKeyList * keylist,
		      gchar ** label, gboolean * free_label,
		      GdkPixmap ** pixmap, GdkBitmap ** mask)
{
  *label = g_strdup (gpa_gpgme_key_get_short_keyid (key, 0));
  *free_label = TRUE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_key_type_pixmap_value (GpgmeKey key, GPAKeyList *keylist,
			   gchar **label, gboolean *free_label,
			   GdkPixmap **pixmap, GdkBitmap **mask)
{
  static gboolean pixmaps_created = FALSE;
  static GdkPixmap *secret_pixmap = NULL;
  static GdkBitmap *secret_mask = NULL;
  static GdkPixmap *public_pixmap = NULL;
  static GdkBitmap *public_mask = NULL;

  if (!pixmaps_created)
    {
      secret_pixmap = gpa_create_icon_pixmap (keylist->window,
					      "blue_yellow_key",
					      &secret_mask);
      public_pixmap = gpa_create_icon_pixmap (keylist->window,
					      "blue_key",
					      &public_mask);
      pixmaps_created = TRUE;
    }

  *label = NULL;
  *free_label = FALSE;
  if (gpa_keytable_secret_lookup 
      (keytable, gpgme_key_get_string_attr (key, GPGME_ATTR_FPR, NULL, 0)))
    {
      *pixmap = secret_pixmap;
      *mask = secret_mask;
    }
  else
    {
      *pixmap = public_pixmap;
      *mask = public_mask;
    }
}

/* The order of items in column_defs must match the one in the
 * GPAKeyListColumn enum because the enum values are used as indices
 * into column_defs.
 */
static ColumnDef column_defs[] = {
  {N_("User Name"), get_name_value},
  {N_("Key ID"), get_identifier_value},
  {N_("Expiry Date"), get_expirydate_value},
  {N_("Key Trust"), get_trust_value},
  {N_("Owner Trust"), get_ownertrust_value},
  {NULL, get_key_type_pixmap_value},
};


/* Create and return a new keylist widget */
GtkWidget *
gpa_keylist_new (gint ncolumns, GPAKeyListColumn *columns, gint max_columns,
		 GtkWidget *window)
{
  GtkWidget *clist;
  GPAKeyList *keylist;
  int i;

  keylist = g_malloc (sizeof (*keylist));

  keylist->window = window;

  keylist->column_defs_changed = TRUE;
  keylist->max_columns = max_columns;
  keylist->ncolumns = ncolumns;
  keylist->column_defs = g_malloc (ncolumns * sizeof (*keylist->column_defs));
  for (i = 0; i < ncolumns; i++)
    {
      keylist->column_defs[i] = &(column_defs[columns[i]]);
    }

  clist = gtk_clist_new (max_columns);
  keylist->clist = clist;
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_EXTENDED);
  gtk_clist_column_titles_show (GTK_CLIST (clist));

  gtk_object_set_data_full (GTK_OBJECT (clist), "gpa_keylist",
			    keylist, keylist_free);
  gtk_clist_set_sort_column( GTK_CLIST (clist), ncolumns-1 );
  gtk_clist_set_sort_type(GTK_CLIST (clist), GTK_SORT_ASCENDING);

  gpa_keylist_update_list (keylist->clist);

  return clist;
}


/* Free the GPAKeyList struct and its contents */
static void
keylist_free (gpointer param)
{
  GPAKeyList * keylist = param;

  g_free (keylist->column_defs);
  g_free (keylist);
}

/* Free the key of a g_hash_table. Usable as a GHFunc. */
static void
free_hash_key (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
}


/* fill the list with the keys. This is also used to update the list
 * when the key list in the keytable may have changed.
 */
static void
keylist_fill_list (GPAKeyList *keylist)
{
  const gchar *fpr;
  GHashTable *sel_hash = g_hash_table_new (g_str_hash, g_str_equal);
  struct keylist_fill_data fill_data = { keylist, sel_hash };
  GpgmeError err;
  GpgmeKey key;

  /* Remember the current selection. Use the
   * hash table itself as the value just because its a non-NULL pointer
   * and we're interested in finding out whether a given fpr is used
   * as a key in the hash later */
  GList *selection = GTK_CLIST (keylist->clist)->selection;
  while (selection)
    {
      fpr = gtk_clist_get_row_data (GTK_CLIST (keylist->clist),
                                       GPOINTER_TO_INT (selection->data));
      if (fpr)
        {
          g_hash_table_insert (sel_hash, g_strdup (fpr), sel_hash);
          selection = g_list_next (selection);
        }
    }
  
  /* Clear the list and fill it again from our key table's key list */
  gtk_clist_freeze (GTK_CLIST (keylist->clist));
  gtk_clist_clear (GTK_CLIST (keylist->clist));
  gpa_keytable_foreach (keytable, (GPATableFunc) keylist_fill_row, &fill_data);
  /* Make sure the list is sorted before unfreezing it */
  gtk_clist_sort (GTK_CLIST (keylist->clist));
  gtk_clist_thaw (GTK_CLIST (keylist->clist));

  /* Iterate over the secret keyring and verify that every key has a
   * corresponding public key. */
  /* FIXME: Do this with a gpa_keytable_secret_foreach() */
  err = gpgme_op_keylist_start (ctx, NULL, 1);
  if( err != GPGME_No_Error )
    gpa_gpgme_error (err);
  while( (err = gpgme_op_keylist_next( ctx, &key )) != GPGME_EOF )
    {
      if( err != GPGME_No_Error )
        gpa_gpgme_error (err);
      fpr = gpgme_key_get_string_attr (key, GPGME_ATTR_FPR, NULL, 0);
      gpgme_key_release (key);
      if( !gpa_keytable_lookup (keytable, fpr) )
        {
          const gchar * buttons[] = {_("_OK"),
                                     NULL};
          gpa_message_box_run (keylist->window, _("Missing public key"),
                               _("You have a secret key without the\n"
                                 "corresponding public key in your\n"
                                 "key ring. In order to use this key\n"
                                 "you will need to import the public\n"
                                 "key, too."),
                               buttons);
        }
    }
  g_hash_table_foreach (sel_hash, free_hash_key, NULL);
  g_hash_table_destroy (sel_hash);
} /* keylist_fill_list */


/* Fill the clist row row with the labels and/or pixmaps of the given
 * key. This is a callback called for each key in the keyring.
 */
static void
keylist_fill_row (const gchar *fpr, GpgmeKey key,
                  struct keylist_fill_data *data)
{
  gint row, col;
  gboolean free_label;
  gchar * label;
  GdkPixmap * pixmap;
  GdkBitmap * mask;
  gchar **labels = g_malloc (data->keylist->max_columns * sizeof (gchar*));
  /* Fill a dummy list of labels. */
  /* FIXME: There must be a cleaner way to do this */
  for (col = 0; col < data->keylist->max_columns; col++)
    labels[col] = "";
  /* Get a new empty row */
  row = gtk_clist_append (GTK_CLIST (data->keylist->clist), labels);
  /* Set the right value for each column */
  for (col = 0; col < data->keylist->ncolumns; col++)
    {
      data->keylist->column_defs[col]->value_func(key, data->keylist, 
                                                  &label, &free_label, 
                                                  &pixmap, &mask);
      if (pixmap)
        {
          gtk_clist_set_pixmap (GTK_CLIST (data->keylist->clist), row, col,
                                pixmap, mask);
        }
      else if (label)
        {
          gtk_clist_set_text (GTK_CLIST (data->keylist->clist), 
                              row, col, label);
        }
      if (free_label)
        g_free (label);
    }
  
  /* The labels are no longer needed */
  g_free (labels);
  /* Each row remembers the associated fingerprint */
  gtk_clist_set_row_data_full (GTK_CLIST (data->keylist->clist), row,
                               g_strdup (fpr), free);

  if (g_hash_table_lookup (data->selected_keys, fpr))
    {
      /* The key had been selected previously, so select it again */
      gtk_clist_select_row (GTK_CLIST (data->keylist->clist), row, 0);
    }
}


/* Set the titles of the visible columns and make the rest invisible */
static void
keylist_fill_column_titles (GPAKeyList *keylist)
{
  gint i;

  for (i = 0; i < keylist->ncolumns; i++)
    {
      gchar *title = keylist->column_defs[i]->title;
      if (title)
	title = _(title);
      else
	title = "";
      gtk_clist_set_column_title (GTK_CLIST (keylist->clist), i, title);
      gtk_clist_set_column_visibility (GTK_CLIST (keylist->clist), i, TRUE);
      /*      gtk_clist_column_title_passive (GTK_CLIST (keylist->clist), i);*/
    }
  for (i = keylist->ncolumns; i < keylist->max_columns; i++)
    {
      gtk_clist_set_column_title (GTK_CLIST (keylist->clist), i, "");
      gtk_clist_set_column_visibility (GTK_CLIST (keylist->clist), i, FALSE);
    }
} /* keylist_fill_column_titles */


/* Change the column definitions. */
void
gpa_keylist_set_column_defs (GtkWidget * clist, gint ncolumns,
			     GPAKeyListColumn *columns)
{
  GPAKeyList * keylist = gtk_object_get_data (GTK_OBJECT (clist),
					      "gpa_keylist");
  gint i;

  g_free (keylist->column_defs);

  keylist->ncolumns = ncolumns;
  keylist->column_defs = g_malloc (ncolumns * sizeof (*keylist->column_defs));
  for (i = 0; i < ncolumns; i++)
    {
      keylist->column_defs[i] = &(column_defs[columns[i]]);
    }
  gtk_clist_set_sort_column( GTK_CLIST (clist), ncolumns-1 );
  gtk_clist_set_sort_type(GTK_CLIST (clist), GTK_SORT_ASCENDING);

  keylist->column_defs_changed = TRUE;
}


/* Update the list to reflect the current state of the keytable and apply
 * changes made to the column definitions.
 */
void
gpa_keylist_update_list (GtkWidget * clist)
{
  gint i;
  gint width;
  gboolean update_widths;
  GPAKeyList * keylist = gtk_object_get_data (GTK_OBJECT (clist),
					      "gpa_keylist");

  update_widths = keylist->column_defs_changed || GTK_CLIST (clist)->rows == 0;

  if (keylist->column_defs_changed)
    {
      keylist_fill_column_titles (keylist);
    }

  keylist_fill_list (keylist);

  /* set the width of all columns to the optimal width, but only when
   * the column defintions changed or the list was empty before so as
   * not to annoy the user by changing user defined widths every time
   * the list is updated */
  if (update_widths)
    {
      /* First, make sure that the size changes in the title buttons are
       * correctly accounted for */
      gtk_container_check_resize (GTK_CONTAINER (keylist->clist));

      for (i = 0; i < keylist->ncolumns; i++)
	{
	  width = gtk_clist_optimal_column_width (GTK_CLIST (keylist->clist),
						  i);
	  gtk_clist_set_column_width (GTK_CLIST (keylist->clist), i, width);
	}
    }

  keylist->column_defs_changed = FALSE;
}


/* Return TRUE if the list widget has at least one selected item.
 */
gboolean
gpa_keylist_has_selection (GtkWidget * clist)
{
  return (GTK_CLIST (clist)->selection != NULL);
}

/* Return TRUE if the list widget has exactly one selected item. */
gboolean
gpa_keylist_has_single_selection (GtkWidget * clist)
{
  return (g_list_length (GTK_CLIST (clist)->selection) == 1);
}

/* Return the id of the currently selected key. NULL if no key is
 * selected */
gchar *
gpa_keylist_current_key_id (GtkWidget * clist)
{
  int row;
  gchar * key_id = NULL;

  if (GTK_CLIST (clist)->selection)
    {
      row = GPOINTER_TO_INT (GTK_CLIST (clist)->selection->data);
      key_id = gtk_clist_get_row_data (GTK_CLIST (clist), row);
    }

  return key_id;
}


/* Return the currently selected key. NULL if no key is selected */
GpgmeKey
gpa_keylist_current_key (GtkWidget * clist)
{
  int row;
  gchar * fpr;
  GpgmeKey key = NULL;

  if (GTK_CLIST (clist)->selection)
    {
      row = GPOINTER_TO_INT (GTK_CLIST (clist)->selection->data);
      fpr = gtk_clist_get_row_data (GTK_CLIST (clist), row);

      key = gpa_keytable_lookup (keytable, fpr);
    }

  return key;
}


/* Return the current selection of the CList */
GList *
gpa_keylist_selection (GtkWidget * keylist)
{
  return GTK_CLIST (keylist)->selection;
}

/* return the number of selected keys */
gint
gpa_keylist_selection_length (GtkWidget * keylist)
{
  return g_list_length (GTK_CLIST (keylist)->selection);
}
