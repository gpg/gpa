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

#include "gpa.h"
#include <time.h>
#include <config.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gpapastrings.h"
#include "gtktools.h"


/*
 *	A table showing some basic information about the key, such as
 *	the key id and the user name
 */ 

GtkWidget *
gpa_key_info_new (gpgme_key_t key)
{
  GtkWidget * table;
  GtkWidget * label;
  gchar *string, *uid;
  int i;

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 10);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 0);

  /* One user ID on each line */
  string = gpa_gpgme_key_get_userid (key, 0);
  for (i = 1; (uid = gpa_gpgme_key_get_userid (key, i)) != NULL; i++)
    {
      /* Don't display revoked UID's */
      if (!gpgme_key_get_ulong_attr (key, GPGME_ATTR_UID_REVOKED, NULL, i))
        {
          gchar *tmp = string;
          string = g_strconcat (string, "\n", uid, NULL);
          g_free (tmp);
	  g_free (uid);
        }
    }
  label = gtk_label_new (string);
  g_free (string);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);

  /* User Name */
  label = gtk_label_new (i == 1 ? _("User Name:") : _("User Names:") );
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 
                    0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);

  /* Key ID */
  label = gtk_label_new (_("Key ID:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  label = gtk_label_new (gpa_gpgme_key_get_short_keyid (key, 0));
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  return table;
}      

/*
 *	A Frame to select an expiry date
 */

typedef struct {
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

  if (!gtk_toggle_button_get_active (radioDont))
    return;
  gtk_entry_set_text (GTK_ENTRY (frame->entryAfter), "");
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (frame->comboAfter)->entry),
		      "days");
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
      struct tm tm;
      g_date_to_struct_tm (frame->expiryDate, &tm);
      dateBuffer = gpa_expiry_date_string (mktime (&tm));
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (frame->comboAfter)->entry),
			  "days");
      g_free (dateBuffer);
    } /* if */
} /* gpa_expiry_frame_at */

static void
expire_date_toggled_cb (GtkToggleButton *togglebutton, gpointer user_data)
{
  GtkWidget *calendar = user_data;
  
  gtk_widget_set_sensitive (calendar,
                            gtk_toggle_button_get_active (togglebutton));
}

GtkWidget *
gpa_expiry_frame_new (GtkAccelGroup * accelGroup, GDate * expiryDate)
{
  GList *contentsAfter = NULL;
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

  radioAt = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radioDont),
					      accelGroup, _("expire o_n:"));
  frame->radioAt = radioAt;
  gtk_box_pack_start (GTK_BOX (vboxExpire), radioAt, FALSE, FALSE, 0);
  calendar = gtk_calendar_new ();
  frame->calendar = calendar;
  gtk_widget_set_sensitive (calendar, FALSE);
  gtk_box_pack_start (GTK_BOX (vboxExpire), calendar, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (frame->radioAt), "toggled",
                    (GCallback) expire_date_toggled_cb, calendar);
  if (expiryDate)
    {
      gtk_calendar_select_month (GTK_CALENDAR (calendar),
                                 g_date_get_month (expiryDate)-1,
                                 g_date_get_year (expiryDate));
      gtk_calendar_select_day (GTK_CALENDAR (calendar),
                               g_date_get_day (expiryDate));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (frame->radioAt),
				    TRUE);
    } /* if */

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
      /* This case is always correct */
      result = NULL;
    } 
  return result;
} /* gpa_expiry_frame_validate */
    
