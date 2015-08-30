/* gpawidgets.c  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2005, 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GPA; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA  */

/* Functions to construct a number of commonly used but GPA specific
   widgets.  */

#include <config.h>

#include "gpa.h"
#include <time.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gtktools.h"
#include "convert.h"

/* A table showing some basic information about the key, such as the
   key id and the user name.  */
GtkWidget *
gpa_key_info_new (gpgme_key_t key)
{
  GtkWidget * table;
  GtkWidget * label;
  gchar *string;
  gpgme_user_id_t uid;

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 10);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 0);

  /* One user ID on each line.  */
  string = gpa_gpgme_key_get_userid (key->uids);
  uid = key->uids->next;
  while (uid)
    {
      if (!uid->revoked)
	{
	  gchar *uid_string = gpa_gpgme_key_get_userid (uid);
          gchar *tmp = string;
          string = g_strconcat (string, "\n", uid_string, NULL);
          g_free (tmp);
	  g_free (uid_string);
        }
      uid = uid->next;
    }
  label = gtk_label_new (string);
  gpa_add_tooltip (label, string);
  g_free (string);
  gtk_label_set_max_width_chars (GTK_LABEL (label), GPA_MAX_UID_WIDTH);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);

  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);

  /* User Name */
  label = gtk_label_new (key->uids->next == NULL
			 ? _("User Name:") : _("User Names:") );
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
                    0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);

  /* Key ID */
  label = gtk_label_new (_("Key ID:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  label = gtk_label_new (gpa_gpgme_key_get_short_keyid (key));
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  /* Fingerprint */
  label = gtk_label_new (_("Fingerprint:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  string = gpa_gpgme_key_format_fingerprint (key->subkeys->fpr);
  label = gtk_label_new (string);
  g_free (string);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  return table;
}

/* A Frame to select an expiry date.  */

typedef struct
{
  GtkWidget *frame;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *calendar;
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
  g_free (frame);
}

static void
gpa_expiry_frame_dont (GtkToggleButton * radioDont, gpointer param)
{
  GPAExpiryFrame * frame = (GPAExpiryFrame*)param;

  if (! gtk_toggle_button_get_active (radioDont))
    return;

  gtk_widget_set_sensitive (frame->entryAfter, FALSE);
  gtk_widget_set_sensitive (frame->comboAfter, FALSE);
}

static void
gpa_expiry_frame_after (GtkToggleButton * radioAfter, gpointer param)
{
  GPAExpiryFrame * frame = (GPAExpiryFrame*)param;

  if (!gtk_toggle_button_get_active (radioAfter))
    return;

  gtk_widget_set_sensitive (frame->entryAfter, TRUE);
  gtk_widget_set_sensitive (frame->comboAfter, TRUE);

  gtk_widget_grab_focus (frame->entryAfter);
}


static void
gpa_expiry_frame_at (GtkToggleButton * radioAt, gpointer param)
{
  GPAExpiryFrame * frame = (GPAExpiryFrame*)param;

  if (! gtk_toggle_button_get_active (radioAt))
    return;

  gtk_widget_set_sensitive (frame->entryAfter, FALSE);
  gtk_widget_set_sensitive (frame->comboAfter, FALSE);
}

static void
expire_date_toggled_cb (GtkToggleButton *togglebutton, gpointer user_data)
{
  GtkWidget *calendar = user_data;

  gtk_widget_set_sensitive (calendar,
                            gtk_toggle_button_get_active (togglebutton));
}


GtkWidget *
gpa_expiry_frame_new (GDate * expiryDate)
{
  gint i;
  GtkWidget *expiry_frame;
  GtkWidget *vboxExpire;
  GtkWidget *radioDont;
  GtkWidget *hboxAfter;
  GtkWidget *radioAfter;
  GtkWidget *entryAfter;
  GtkWidget *comboAfter;
  GtkWidget *radioAt;
  GtkWidget *calendar;

  GPAExpiryFrame * frame;

  frame = g_malloc(sizeof(*frame));
  frame->expiryDate = expiryDate;

  expiry_frame = gtk_frame_new (_("Expiration"));
  frame->frame = expiry_frame;

  vboxExpire = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (expiry_frame), vboxExpire);
  gtk_container_set_border_width (GTK_CONTAINER (vboxExpire), 5);

  radioDont = gtk_radio_button_new_with_mnemonic (NULL, _("_indefinitely valid"));
  frame->radioDont = radioDont;
  gtk_box_pack_start (GTK_BOX (vboxExpire), radioDont, FALSE, FALSE, 0);

  hboxAfter = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxExpire), hboxAfter, FALSE, FALSE, 0);

  radioAfter = gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radioDont), _("expire _after"));
  frame->radioAfter = radioAfter;
  gtk_box_pack_start (GTK_BOX (hboxAfter), radioAfter, FALSE, FALSE, 0);
  entryAfter = gtk_entry_new ();
  frame->entryAfter = entryAfter;
  gtk_entry_set_width_chars (GTK_ENTRY (entryAfter), strlen (" 00000 "));
  gtk_box_pack_start (GTK_BOX (hboxAfter), entryAfter, FALSE, FALSE, 0);

  comboAfter = gtk_combo_box_new_text ();
  frame->comboAfter = comboAfter;
  for (i = 3; i >= 0; i--)
    gtk_combo_box_prepend_text (GTK_COMBO_BOX (comboAfter),
				gpa_unit_expiry_time_string (i));
  gtk_combo_box_set_active (GTK_COMBO_BOX (comboAfter), 0);
  gtk_box_pack_start (GTK_BOX (hboxAfter), comboAfter, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (entryAfter, FALSE);
  gtk_widget_set_sensitive (comboAfter, FALSE);
  /* FIXME: Set according to expiry date.  */

  radioAt = gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radioDont), _("expire o_n:"));
  frame->radioAt = radioAt;
  gtk_box_pack_start (GTK_BOX (vboxExpire), radioAt, FALSE, FALSE, 0);
  calendar = gtk_calendar_new ();
  frame->calendar = calendar;
  gtk_widget_set_sensitive (calendar, FALSE);
  gtk_box_pack_start (GTK_BOX (vboxExpire), calendar, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (frame->radioAt), "toggled",
                    G_CALLBACK (expire_date_toggled_cb), calendar);
  if (expiryDate)
    {
      gtk_calendar_select_month (GTK_CALENDAR (calendar),
                                 g_date_get_month (expiryDate) - 1,
                                 g_date_get_year (expiryDate));
      gtk_calendar_select_day (GTK_CALENDAR (calendar),
                               g_date_get_day (expiryDate));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (frame->radioAt),
				    TRUE);
    }

  if (expiryDate)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radioAt), TRUE);
      gtk_widget_set_sensitive (calendar, TRUE);
    }
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radioDont), TRUE);
  g_signal_connect (G_OBJECT (radioDont), "toggled",
		    G_CALLBACK (gpa_expiry_frame_dont), frame);
  g_signal_connect (G_OBJECT (radioAfter), "toggled",
		    G_CALLBACK (gpa_expiry_frame_after), frame);
  g_signal_connect (G_OBJECT (radioAt), "toggled",
		    G_CALLBACK (gpa_expiry_frame_at), frame);

  g_object_set_data_full (G_OBJECT (expiry_frame), "user_data",
			  frame, gpa_expiry_frame_free);
  return expiry_frame;
}


gboolean
gpa_expiry_frame_get_expiration(GtkWidget * expiry_frame, GDate ** date,
				int * interval, gchar * unit)
{
  GPAExpiryFrame * frame = g_object_get_data (G_OBJECT (expiry_frame),
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
      temp = gtk_combo_box_get_active_text (GTK_COMBO_BOX (frame->comboAfter));
      *unit = gpa_time_unit_from_string (temp);
      *date = NULL;
      result = TRUE;
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (frame->radioAt)))
    {
      guint day, month, year;
      gtk_calendar_get_date (GTK_CALENDAR (frame->calendar),
                             &year, &month, &day);
      *date = g_date_new_dmy (day, month+1, year);
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
}


/* Return NULL if the values are correct and an error message if not.
   The error message is suitable for a message box.  Currently only
   the date is validated if the "expire at" radio button is
   active.  */
gchar *
gpa_expiry_frame_validate(GtkWidget * expiry_frame)
{
  GPAExpiryFrame * frame = g_object_get_data (G_OBJECT (expiry_frame),
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
      /* This case is always correct.  */
      result = NULL;
    }
  return result;
}

