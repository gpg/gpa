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
#include <gpapa.h>
#include "icons.h"
#include "gpa.h"
#include "gpapastrings.h"
#include "keylist.h"


struct _GPAKeyList;

/* A column definition. Consists of a title and a function that returns
 * the values for that column given a public key */

typedef void (*ColumnValueFunc)(GpapaPublicKey * key,
				struct _GPAKeyList * keylist,
				gchar ** label,
				gboolean * free_label,
				GdkPixmap ** pixmap,
				GdkBitmap ** mask);

typedef struct {
  /* column title */
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

static void keylist_free (gpointer param);
static void keylist_fill_list (GPAKeyList * keylist, gboolean keep_selection);
static void keylist_fill_column_titles (GPAKeyList * keylist);
static void keylist_fill_row (GPAKeyList * keylist, GpapaPublicKey * key,
			      gint row);


/*
 * ColumnDef functions
 */

static void
get_name_value (GpapaPublicKey * key, GPAKeyList * keylist,
		gchar ** label, gboolean * free_label, GdkPixmap ** pixmap,
		GdkBitmap ** mask)
{
  *label = gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, keylist->window);
  *free_label = FALSE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_trust_value (GpapaPublicKey * key, GPAKeyList * keylist,
		 gchar ** label, gboolean * free_label, GdkPixmap ** pixmap,
		 GdkBitmap ** mask)
{
  GpapaKeytrust trust;

  trust = gpapa_public_key_get_keytrust (key, gpa_callback, keylist->window);
  *label = gpa_keytrust_string (trust);
  *free_label = FALSE;
  *pixmap = NULL;
  *mask = NULL;
}
  
static void
get_ownertrust_value (GpapaPublicKey * key, GPAKeyList * keylist,
		      gchar ** label, gboolean * free_label,
		      GdkPixmap ** pixmap, GdkBitmap ** mask)
{
  GpapaOwnertrust trust;

  trust = gpapa_public_key_get_ownertrust (key, gpa_callback, keylist->window);
  *label = gpa_ownertrust_string (trust);
  *free_label = FALSE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_expirydate_value (GpapaPublicKey * key, GPAKeyList * keylist,
		      gchar ** label, gboolean * free_label,
		      GdkPixmap ** pixmap, GdkBitmap ** mask)
{
  GDate * date;

  date = gpapa_key_get_expiry_date (GPAPA_KEY (key), gpa_callback,
				    keylist->window);
  *label = gpa_expiry_date_string (date);
  *free_label = TRUE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_identifier_value (GpapaPublicKey * key, GPAKeyList * keylist,
		      gchar ** label, gboolean * free_label,
		      GdkPixmap ** pixmap, GdkBitmap ** mask)
{
  *label = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
				     keylist->window);
  *free_label = FALSE;
  *pixmap = NULL;
  *mask = NULL;
}

static void
get_key_type_pixmap_value (GpapaPublicKey * key, GPAKeyList * keylist,
			   gchar ** label, gboolean * free_label,
			   GdkPixmap ** pixmap, GdkBitmap ** mask)
{
  gchar * key_id;
  GpapaSecretKey * secret_key;
  static gboolean pixmaps_created = FALSE;
  static GdkPixmap * secret_pixmap = NULL;
  static GdkBitmap * secret_mask = NULL;
  static GdkPixmap * public_pixmap = NULL;
  static GdkBitmap * public_mask = NULL;

  if (!pixmaps_created)
    {
      secret_pixmap = gpa_create_icon_pixmap (keylist->window,
					      "gpa_yellow_key",
					      &secret_mask);
      public_pixmap = gpa_create_icon_pixmap (keylist->window,
					      "gpa_blue_key",
					      &public_mask);
      pixmaps_created = TRUE;
    }

  key_id = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
				     keylist->window);
  secret_key = gpapa_get_secret_key_by_ID (key_id, gpa_callback,
					   keylist->window);
  *label = NULL;
  *free_label = FALSE;
  if (secret_key)
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
  {"", get_key_type_pixmap_value},
};


/* Create and return a new keylist widget */
GtkWidget *
gpa_keylist_new (gint ncolumns, GPAKeyListColumn * columns, gint max_columns,
		 GtkWidget * window)
{
  GtkWidget * clist;
  GPAKeyList * keylist;
  int i;

  keylist = xmalloc (sizeof (*keylist));

  keylist->window = window;

  keylist->column_defs_changed = TRUE;
  keylist->max_columns = max_columns;
  keylist->ncolumns = ncolumns;
  keylist->column_defs = xmalloc (ncolumns * sizeof (*keylist->column_defs));
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

  gpa_keylist_update_list (keylist->clist);

  return clist;
}


/* Free the GPAKeyList struct and its contents */
static void
keylist_free (gpointer param)
{
  GPAKeyList * keylist = param;

  free (keylist->column_defs);
  free (keylist);
}

/* Free the key of a g_hash_table. Usable as a GHFunc. */
static void
free_hash_key (gpointer key, gpointer value, gpointer data)
{
  free (key);
}


/* fill the list with the keys. This is also used to update the list
 * when the key list in gpapa may have changed. If keep_selection is
 * true, try to keep the current selection.
 */
static void
keylist_fill_list (GPAKeyList * keylist, gboolean keep_selection)
{
  gint num_keys;
  gint key_index;
  gint row, col;
  GpapaPublicKey * key;
  gchar ** labels;
  gchar * key_id;
  GHashTable * sel_hash = g_hash_table_new (g_str_hash, g_str_equal);

  /* if keep_selection is true, remember the current selection. Use the
   * hast table itself as the value just because its a non-NULL pointer
   * and we're interested in finding out whether a given key_id is used
   * as a key in the hash later */
  if (keep_selection)
    {
      GList * selection = GTK_CLIST (keylist->clist)->selection;
      while (selection)
	{
	  key_id = gtk_clist_get_row_data (GTK_CLIST (keylist->clist),
					   GPOINTER_TO_INT (selection->data));
	  g_hash_table_insert (sel_hash, xstrdup (key_id), sel_hash);
	  selection = g_list_next (selection);
	}
    }

  /* Fill a dummy list of labels. */
  labels = xmalloc (keylist->max_columns * sizeof (gchar*));
  for (col = 0; col < keylist->max_columns; col++)
    labels[col] = "";

  /* Clear the list and fill it again from gpapa's public key list */
  gtk_clist_freeze (GTK_CLIST (keylist->clist));
  gtk_clist_clear (GTK_CLIST (keylist->clist));
  num_keys = gpapa_get_public_key_count(gpa_callback, keylist->window);
  for (key_index = 0; key_index < num_keys; key_index++)
    {
      key = gpapa_get_public_key_by_index (key_index, gpa_callback,
					   keylist->window);

      row = gtk_clist_append (GTK_CLIST (keylist->clist), labels);
      keylist_fill_row (keylist, key, row);
      key_id = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
					 keylist->window);
      gtk_clist_set_row_data_full (GTK_CLIST (keylist->clist), row,
				   xstrdup (key_id), free);
      if (keep_selection && g_hash_table_lookup (sel_hash, key_id))
	{
	  /* The key had been selected previously, so select it again */
	  gtk_clist_select_row (GTK_CLIST (keylist->clist), row, 0);
	}
    } /* for */

  gtk_clist_thaw (GTK_CLIST (keylist->clist));

  free (labels);
  
  g_hash_table_foreach (sel_hash, free_hash_key, NULL);
  g_hash_table_destroy (sel_hash);
} /* keylist_fill_list */


/* Fill the clist row row with the labels and/or pixmaps of the given
 * key.
 */
static void
keylist_fill_row (GPAKeyList * keylist, GpapaPublicKey * key, gint row)
{
  gint col;
  gchar * label;
  gboolean free_label;
  GdkPixmap * pixmap;
  GdkBitmap * mask;
  
  for (col = 0; col < keylist->ncolumns; col++)
    {
      keylist->column_defs[col]->value_func(key, keylist, &label, &free_label,
					    &pixmap, &mask);
      if (pixmap)
	{
	  gtk_clist_set_pixmap (GTK_CLIST (keylist->clist), row, col,
				pixmap, mask);
	}
      else if (label)
	{
	  gtk_clist_set_text (GTK_CLIST (keylist->clist), row, col, label);
	}
      if (free_label)
	free (label);
    }
}


/* Set the titles of the visible columns and make the rest invisible */
static void
keylist_fill_column_titles (GPAKeyList * keylist)
{
  gint i;

  for (i = 0; i < keylist->ncolumns; i++)
    {
      gtk_clist_set_column_title (GTK_CLIST (keylist->clist), i,
				  _(keylist->column_defs[i]->title));
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

  free (keylist->column_defs);

  keylist->ncolumns = ncolumns;
  keylist->column_defs = xmalloc (ncolumns * sizeof (*keylist->column_defs));
  for (i = 0; i < ncolumns; i++)
    {
      keylist->column_defs[i] = &(column_defs[columns[i]]);
    }

  keylist->column_defs_changed = TRUE;
}


/* Update the list to reflect the current state of gpapa and apply
 * changes made to the column definitions.
 */
void
gpa_keylist_update_list (GtkWidget * clist)
{
  gint i;
  gint width;
  GPAKeyList * keylist = gtk_object_get_data (GTK_OBJECT (clist),
					      "gpa_keylist");

  if (keylist->column_defs_changed)
    {
      keylist_fill_column_titles (keylist);
    }

  keylist_fill_list (keylist, TRUE);

  /* set the width of all columns to the optimal width, but only when
   * the column defintions change so as not to annoy the user by
   * changing user defined widths every time the list is updated */
  if (keylist->column_defs_changed)
    {
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
GpapaPublicKey *
gpa_keylist_current_key (GtkWidget * clist)
{
  int row;
  gchar * key_id;
  GpapaPublicKey * key = NULL;
  GPAKeyList * keylist = gtk_object_get_data (GTK_OBJECT (clist),
					      "gpa_keylist");

  if (GTK_CLIST (clist)->selection)
    {
      row = GPOINTER_TO_INT (GTK_CLIST (clist)->selection->data);
      key_id = gtk_clist_get_row_data (GTK_CLIST (clist), row);

      key = gpapa_get_public_key_by_ID (key_id, gpa_callback, keylist->window);
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
