/* gpawidgets.c  -  The GNU Privacy Assistant
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/*
 *	Functions to construct a number of commonly used but GPA
 *	specific widgets
 */

#include <config.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <gpapa.h>

#include "gpapastrings.h"
#include "gpa.h"
#include "gtktools.h"


/*
 *	A table showing some basic information about the key, such as
 *	the key id and the user name
 */ 

GtkWidget *
gpa_key_info_new (GpapaKey * key, GtkWidget * window)
{
  GtkWidget * table;
  GtkWidget * label;

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 10);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 0);

  /* User Name */
  label = gtk_label_new (_("User Name:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  label = gtk_label_new (gpapa_key_get_name (key, gpa_callback, window));
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  /* User Name */
  label = gtk_label_new (_("Key ID:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  label = gtk_label_new (gpapa_key_get_identifier (key, gpa_callback, window));
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  return table;
}

/*
 *	A CList for choosing from secret keys
 */

GtkWidget *
gpa_secret_key_list_new (GtkWidget *window)
{
  GtkWidget *clist;
  gint num_keys;
  GpapaSecretKey *key;
  gint default_key_index = 0;
  gchar *titles[2] = {_("Key ID"), _("User Identity/Role")};
  gchar *contents[2];
  gint row;

  clist = gtk_clist_new_with_titles (2, titles);

  /* FIXME: widths shouldn't be hard-coded: */
  gtk_clist_set_column_width (GTK_CLIST (clist), 0, 120);
  gtk_clist_set_column_width (GTK_CLIST (clist), 1, 200);
  num_keys = gpapa_get_secret_key_count (gpa_callback, window);
  while (num_keys)
    {
      num_keys--;
      key = gpapa_get_secret_key_by_index (num_keys, gpa_callback, window);
      contents[0] = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
					      window);
      contents[1] = gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, window);
      row = gtk_clist_prepend (GTK_CLIST (clist), contents);
      gtk_clist_set_row_data_full (GTK_CLIST (clist), row,
				   contents[0]? xstrdup (contents[0]):NULL,
                                   free);
      if (gpa_default_key () && strcmp (gpa_default_key (), contents[0]) == 0)
	{
	  default_key_index = num_keys;
	}
    } /* while */
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_SINGLE);
  gtk_clist_column_title_passive (GTK_CLIST (clist), 0);
  gtk_clist_column_title_passive (GTK_CLIST (clist), 1);
  gtk_clist_select_row (GTK_CLIST (clist), default_key_index, 0);

  return clist;
}

GtkWidget *
gpa_public_key_list_new (GtkWidget *window)
{
  GtkWidget *clist;
  gint num_keys;
  GpapaPublicKey *key;
  gchar *titles[2] = {_("Key ID"), _("User Identity/Role")};
  gchar *contents[2];
  gint row;

  clist = gtk_clist_new_with_titles (2, titles);

  /* FIXME: widths shouldn't be hard-coded: */
  gtk_clist_set_column_width (GTK_CLIST (clist), 0, 120);
  gtk_clist_set_column_width (GTK_CLIST (clist), 1, 200);
  num_keys = gpapa_get_public_key_count (gpa_callback, window);
  while (num_keys)
    {
      num_keys--;
      key = gpapa_get_public_key_by_index (num_keys, gpa_callback, window);
      contents[0] = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
					      window);
      contents[1] = gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, window);
      row = gtk_clist_prepend (GTK_CLIST (clist), contents);
      gtk_clist_set_row_data_full (GTK_CLIST (clist), row,
				   contents[0]?xstrdup (contents[0]):NULL,
                                   free);
    } /* while */
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_SINGLE);
  gtk_clist_column_title_passive (GTK_CLIST (clist), 0);
  gtk_clist_column_title_passive (GTK_CLIST (clist), 1);

  return clist;
}

GtkWidget *
gpa_key_list_new_from_glist (GtkWidget *window, GList *list)
{
  GtkWidget *clist;
  gint num_keys;
  GpapaKey *key;
  gchar *titles[2] = {_("Key ID"), _("User Identity/Role")};
  gchar *contents[2];
  gint row;

  clist = gtk_clist_new_with_titles (2, titles);

  /* FIXME: widths shouldn't be hard-coded: */
  gtk_clist_set_column_width (GTK_CLIST (clist), 0, 120);
  gtk_clist_set_column_width (GTK_CLIST (clist), 1, 200);
  num_keys = g_list_length (list);
  while (num_keys)
    {
      num_keys--;
      key = (GpapaKey *) g_list_nth_data (list, num_keys);
      contents[0] = gpapa_key_get_identifier (key, gpa_callback,
					      window);
      contents[1] = gpapa_key_get_name (key, gpa_callback, window);
      row = gtk_clist_prepend (GTK_CLIST (clist), contents);
      gtk_clist_set_row_data_full (GTK_CLIST (clist), row,
				   contents[0]?xstrdup (contents[0]):NULL,
                                   free);
    } /* while */
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_SINGLE);
  gtk_clist_column_title_passive (GTK_CLIST (clist), 0);
  gtk_clist_column_title_passive (GTK_CLIST (clist), 1);

  return clist;
}

gint
gpa_key_list_selection_length (GtkWidget *clist)
{
  return g_list_length (GTK_CLIST (clist)->selection);
}

GList *
gpa_key_list_selected_ids (GtkWidget *clist)
{
  GList * ids = NULL;
  GList * selection = GTK_CLIST (clist)->selection;
  gint row;

  while (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      ids = g_list_prepend (ids, gtk_clist_get_row_data (GTK_CLIST (clist),
							 row));
      selection = g_list_next (selection);
    }
  return ids;
}

gchar *
gpa_key_list_selected_id (GtkWidget *clist)
{
  gint row;

  if (GTK_CLIST (clist)->selection)
    {
      row = GPOINTER_TO_INT (GTK_CLIST (clist)->selection->data);
      return gtk_clist_get_row_data (GTK_CLIST (clist), row);
    }

  return NULL;
}
      

/*
 *	A Frame to select an expiry date
 */

typedef struct {
  GtkWidget *frame;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *entryAt;
  GtkWidget *radioDont;
  GtkWidget *radioAfter;
  GtkWidget *radioAt;
  GDate *expiryDate;
} GPAExpiryFrame;

static void
gpa_expiry_frame_free (gpointer param)
{
  GPAExpiryFrame * frame = param;
  if (frame->expiryDate)
    g_date_free (frame->expiryDate);
  free (frame);
}

static void
gpa_expiry_frame_dont (GtkToggleButton * radioDont, gpointer param)
{
  GPAExpiryFrame * frame = (GPAExpiryFrame*)param;

  if (!gtk_toggle_button_get_active (radioDont))
    return;
  gtk_entry_set_text (GTK_ENTRY (frame->entryAfter), "");
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (frame->comboAfter)->entry),
		      "days");
  gtk_entry_set_text (GTK_ENTRY (frame->entryAt), "");
} /* gpa_expiry_frame_dont */

static void
gpa_expiry_frame_after (GtkToggleButton * radioAfter, gpointer param)
{
  GPAExpiryFrame * frame = (GPAExpiryFrame*)param;

  if (!gtk_toggle_button_get_active (radioAfter))
    return;
  if (frame->expiryDate)
    {
      gtk_entry_set_text (GTK_ENTRY (frame->entryAfter), "1"); /*!!! */
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (frame->comboAfter)->entry),
			  "days");
    } /* if */
  else
    gtk_entry_set_text (GTK_ENTRY (frame->entryAfter), "1");
  gtk_entry_set_text (GTK_ENTRY (frame->entryAt), "");
  gtk_widget_grab_focus (frame->entryAfter);
} /* gpa_expiry_frame_after */


static void
gpa_expiry_frame_at (GtkToggleButton * radioAt, gpointer param)
{
  GPAExpiryFrame * frame = (GPAExpiryFrame*)param;
  gchar *dateBuffer;

  if (!gtk_toggle_button_get_active (radioAt))
    return;
  gtk_entry_set_text (GTK_ENTRY (frame->entryAfter), "");
  if (frame->expiryDate)
    {
      dateBuffer = gpa_expiry_date_string (frame->expiryDate);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (frame->comboAfter)->entry),
			  "days");
      gtk_entry_set_text (GTK_ENTRY (frame->entryAt), dateBuffer);
      free (dateBuffer);
    } /* if */
  else
    gtk_entry_set_text (GTK_ENTRY (frame->entryAt), _("01.01.2000")); /*!!! */
  gtk_widget_grab_focus (frame->entryAt);
} /* gpa_expiry_frame_at */

GtkWidget *
gpa_expiry_frame_new (GtkAccelGroup * accelGroup, GDate * expiryDate)
{
  GList *contentsAfter = NULL;
  gint i;
  gchar *dateBuffer;

  GtkWidget *expiry_frame;
  GtkWidget *vboxExpire;
  GtkWidget *radioDont;
  GtkWidget *hboxAfter;
  GtkWidget *radioAfter;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *hboxAt;
  GtkWidget *radioAt;
  GtkWidget *entryAt;

  GPAExpiryFrame * frame;

  frame = xmalloc(sizeof(*frame));
  frame->expiryDate = expiryDate;

  expiry_frame = gtk_frame_new (_("Expiration"));
  frame->frame = expiry_frame;

  vboxExpire = gtk_vbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (expiry_frame), vboxExpire);
  gtk_container_set_border_width (GTK_CONTAINER (vboxExpire), 5);

  radioDont = gpa_radio_button_new (accelGroup, _("_indefinitely valid"));
  frame->radioDont = radioDont;
  gtk_box_pack_start (GTK_BOX (vboxExpire), radioDont, FALSE, FALSE, 0);

  hboxAfter = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxExpire), hboxAfter, FALSE, FALSE, 0);

  radioAfter = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radioDont),
						 accelGroup,
						 _("expire _after"));
  frame->radioAfter = radioAfter;
  gtk_box_pack_start (GTK_BOX (hboxAfter), radioAfter, FALSE, FALSE, 0);
  entryAfter = gtk_entry_new ();
  frame->entryAfter = entryAfter;
  gtk_widget_set_usize (entryAfter,
    gdk_string_width (gtk_style_get_font (entryAfter->style), " 00000 "), 0);
  gtk_box_pack_start (GTK_BOX (hboxAfter), entryAfter, FALSE, FALSE, 0);

  comboAfter = gtk_combo_new ();
  frame->comboAfter = comboAfter;
  gtk_combo_set_value_in_list (GTK_COMBO (comboAfter), TRUE, FALSE);
  for (i = 0; i < 4; i++)
    contentsAfter = g_list_append (contentsAfter,
				   gpa_unit_expiry_time_string(i));
  gtk_combo_set_popdown_strings (GTK_COMBO (comboAfter), contentsAfter);
  gtk_box_pack_start (GTK_BOX (hboxAfter), comboAfter, FALSE, FALSE, 0);

  hboxAt = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxExpire), hboxAt, FALSE, FALSE, 0);

  radioAt = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radioDont),
					      accelGroup, _("expire a_t"));
  frame->radioAt = radioAt;
  gtk_box_pack_start (GTK_BOX (hboxAt), radioAt, FALSE, FALSE, 0);
  entryAt = gtk_entry_new ();
  frame->entryAt = entryAt;
  if (expiryDate)
    {
      dateBuffer = gpa_expiry_date_string (expiryDate);
      gtk_entry_set_text (GTK_ENTRY (entryAt), dateBuffer);
      free (dateBuffer);
    } /* if */
  gtk_box_pack_start (GTK_BOX (hboxAt), entryAt, FALSE, FALSE, 0);

  if (expiryDate)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radioAt), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radioDont), TRUE);
  gtk_signal_connect (GTK_OBJECT (radioDont), "toggled",
		      GTK_SIGNAL_FUNC (gpa_expiry_frame_dont),
		      (gpointer) frame);
  gtk_signal_connect (GTK_OBJECT (radioAfter), "toggled",
		      GTK_SIGNAL_FUNC (gpa_expiry_frame_after),
		      (gpointer) frame);
  gtk_signal_connect (GTK_OBJECT (radioAt), "toggled",
		      GTK_SIGNAL_FUNC (gpa_expiry_frame_at), (gpointer) frame);

  gtk_object_set_data_full (GTK_OBJECT (expiry_frame), "user_data",
			    (gpointer) frame, gpa_expiry_frame_free);
  return expiry_frame;
} /* gpa_expiry_frame_new */


gboolean
gpa_expiry_frame_get_expiration(GtkWidget * expiry_frame, GDate ** date,
				int * interval, gchar * unit)
{
  GPAExpiryFrame * frame = gtk_object_get_data (GTK_OBJECT (expiry_frame),
						"user_data");
  gchar *temp;
  gboolean result = FALSE;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (frame->radioDont)))
    {
      *interval = 0;
      *date = NULL;
      result = TRUE;
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(frame->radioAfter)))
    {
      *interval = atoi (gtk_entry_get_text (GTK_ENTRY(frame->entryAfter)));
      temp = (gchar *) gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(frame->comboAfter)->entry));
      *unit = gpa_time_unit_from_string (temp);
      *date = NULL;
      result = TRUE;
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (frame->radioAt)))
    {
      *date = g_date_new ();
      g_date_set_parse (*date,
			gtk_entry_get_text (GTK_ENTRY (frame->entryAt)));
      *interval = 0;
      result = TRUE;
    } 
  else
    {
      /* this should never happen */
      gpa_window_error (_("!FATAL ERROR!\n"
			  "Invalid insert mode for expiry date."),
			expiry_frame);
      *interval = 0;
      *date = NULL;
      result = FALSE;
    }
  return result;
} /* gpa_expiry_frame_get_expiration */


/* Return NULL if the values are correct and an error message if not.
 * The error message is suitable for a message box. Currently only the
 * date is validated if the "expire at" radio button is active.
 */
gchar *
gpa_expiry_frame_validate(GtkWidget * expiry_frame)
{
  GPAExpiryFrame * frame = gtk_object_get_data (GTK_OBJECT (expiry_frame),
						"user_data");
  gchar * result = NULL;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (frame->radioDont)))
    {
      /* This case is always correct */
      result = NULL;
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(frame->radioAfter)))
    {
      /* This case should probably check whether the interval value is > 0 */
      result = NULL;
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (frame->radioAt)))
    {
      GDate *date = g_date_new ();
      g_date_set_parse (date,
			gtk_entry_get_text (GTK_ENTRY (frame->entryAt)));
      if (!g_date_valid (date))
	{
	  result = _("Please provide a correct date.");
	}
      else
	{
	  result = NULL;
	}
      g_date_free (date);
    } 
  return result;
} /* gpa_expiry_frame_validate */
    
