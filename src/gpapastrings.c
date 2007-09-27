/* gpapastrings.c  -	 The GNU Privacy Assistant
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

#include <config.h>

#include "gpa.h"
#include <config.h>
#include "gpapastrings.h"

/*
 *	Some helper functions to translate between readable strings and
 *	enums
 */

static gchar *unit_expiry_time[4] = {
  N_("days"),
  N_("weeks"),
  N_("months"),
  N_("years")
};


static gchar unit_time[4] = { 'd', 'w', 'm', 'y' };

gchar *
gpa_unit_expiry_time_string(int idx)
{
  return _(unit_expiry_time[idx]);
}

gchar
gpa_time_unit_from_string (gchar * string)
{
  gchar result = ' ';
  gint i;

  i = 0;
  while (i < 4 && strcmp (string, unit_expiry_time[i]) != 0)
    i++;
  if (i < 4)
    result = unit_time[i];
  return result;
} /* gpa_time_unit_from_string */

gchar *
gpa_expiry_date_string (unsigned long expiry_time)
{
  gchar date_buffer[256];
  gchar *result;
  GDate expiry_date;

  if( expiry_time > 0 )
    {
      g_date_set_time (&expiry_date, expiry_time);
      g_date_strftime (date_buffer, 256, "%x", &expiry_date);
      result = g_strdup (date_buffer);
    }
  else
    result = g_strdup (_("never expires"));
  return result;
} /* gpa_expiry_data_string */

gchar *
gpa_creation_date_string (unsigned long creation_time)
{
  gchar date_buffer[256];
  gchar *result;
  GDate creation_date;

  if( creation_time > 0 )
    {
      g_date_set_time (&creation_date, creation_time);
      g_date_strftime (date_buffer, 256, "%x", &creation_date);
      result = g_strdup (date_buffer);
    }
  else
    result = g_strdup (_("unknown"));
  return result;
} /* gpa_creation_data_string */

