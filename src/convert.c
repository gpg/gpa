/* convert.c - Conversion functions
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *      Copyright (C) 2009 g10 Code GmbH
 *
 * This file is part of GPA
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

#include <config.h>

#include "gpa.h"
#include "convert.h"


static gchar *unit_expiry_time[4] = 
  {
    N_("days"),
    N_("weeks"),
    N_("months"),
    N_("years")
  };


static gchar unit_time[4] = { 'd', 'w', 'm', 'y' };




/*
 * Helper functions to translate between readable strings and enums.
 */

/* Return a string describing the unit of time.  */
const char *
gpa_unit_expiry_time_string(int idx)
{
  if (idx < 0 || idx >= DIM(unit_expiry_time))
    return "?";
  return _(unit_expiry_time[idx]);
}


/* Return a char as abbreviation for the STRING.  STRING needs to be
   the result of gpa_unit_expiry_time_string.  Returns space on
   error.  */
char
gpa_time_unit_from_string (const char *string)
{
  gchar result = ' ';
  gint i;

  i = 0;
  while (i < DIM(unit_expiry_time)
         && !strcmp (string, unit_expiry_time[i]))
    i++;
  if (i < DIM(unit_expiry_time) && i < DIM(unit_time))
    result = unit_time[i];
  return result;
} 


char *
gpa_expiry_date_string (unsigned long expiry_time)
{
  gchar *result;
  GDate expiry_date;

  if( expiry_time > 0 )
    {
      g_date_set_time_t (&expiry_date, (time_t) expiry_time);
      result = g_strdup_printf ("%04d-%02d-%02d", 
                                g_date_get_year (&expiry_date),
                                g_date_get_month (&expiry_date),
                                g_date_get_day (&expiry_date));
    }
  else
    result = g_strdup (_("never expires"));
  return result;
} 


char *
gpa_creation_date_string (unsigned long creation_time)
{
  gchar *result;
  GDate creation_date;

  if( creation_time > 0 )
    {
      g_date_set_time_t (&creation_date, (time_t) creation_time);
      result = g_strdup_printf ("%04d-%02d-%02d", 
                                g_date_get_year (&creation_date),
                                g_date_get_month (&creation_date),
                                g_date_get_day (&creation_date));
    }
  else
    result = g_strdup (_("unknown"));
  return result;
} 

const char *
gpa_sex_char_to_string (char sex)
{
  if (sex == 'm')
    return _("Mr.");
  else if (sex == 'f')
    return _("Ms.");
  else if (sex == 'u')
    return "";
  else
    return _("(unknown)");
}
