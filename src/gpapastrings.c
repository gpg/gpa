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
#include <gpapa.h>
#include "gpa.h"
#include "gpapastrings.h"

/*
 *	Some helper functions to translate between readable strings and
 *	enums
 */

static gchar *keytrust_strings[4] = {
  N_("unknown"),
  N_("don't trust"),
  N_("trust marginally"),
  N_("trust fully")
};

gchar *
gpa_keytrust_string (GpapaKeytrust keytrust)
{
  return _(keytrust_strings[keytrust]);
} /* gpa_keytrust_string */

static gchar *ownertrust_strings[4] = {
  N_("unknown"),
  N_("don't trust"),
  N_("trust marginally"),
  N_("trust fully")
};

gchar *
gpa_ownertrust_string (GpapaOwnertrust ownertrust)
{
  return _(ownertrust_strings[ownertrust]);
} /* gpa_ownertrust_string */


GpapaOwnertrust
gpa_ownertrust_from_string (gchar * string)
{
  GpapaOwnertrust result;

  result = GPAPA_OWNERTRUST_FIRST;
  while (result <= GPAPA_OWNERTRUST_LAST &&
	 strcmp (string, _(ownertrust_strings[result])) != 0)
    result++;
  return result;
} /* gpa_ownertrust_from_string */

const char *
gpa_ownertrust_icon_name (GpapaOwnertrust ownertrust)
{
  switch ( ownertrust )
    {
    case 0: return "gpa_trust_unknown";
    case 1: return "gpa_dont_trust";
    case 2: return "gpa_trust_marginally";
    case 3: return "gpa_trust_fully";
    }
  return "oops";
}


static gchar *unit_expiry_time[4] = {
  N_("days"),
  N_("weeks"),
  N_("months"),
  N_("years")
};


static gchar unit_time[4] = { 'd', 'w', 'm', 'y' };

gchar *
gpa_unit_expiry_time_string(int index)
{
  return _(unit_expiry_time[index]);
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

static gchar *algorithm_strings[4] = {
  N_("DSA and ElGamal"),
  N_("DSA (sign only)"),
  N_("ElGamal (sign and encrypt)"),
  N_("ElGamal (encrypt only)")
};

gchar *
gpa_algorithm_string (GpapaAlgo algo)
{
  return _(algorithm_strings[algo]);
}

GpapaAlgo
gpa_algorithm_from_string (gchar * string)
{
  GpapaAlgo result;

  result = GPAPA_ALGO_FIRST;
  while (result <= GPAPA_ALGO_LAST &&
	 strcmp (string, algorithm_strings[result]) != 0)
    result++;
  return result;
} /* gpa_algorithm_from_string */

gchar *
gpa_expiry_date_string (GDate * expiry_date)
{
  gchar date_buffer[256];
  gchar *result;

  if (expiry_date != NULL)
    {
      g_date_strftime (date_buffer, 256, "%x", expiry_date);
      result = xstrdup (date_buffer);
    } /* if */
  else
    result = xstrdup (_("never expires"));
  return result;
} /* gpa_expiry_data_string */


static gchar *file_status_strings[] = {
  N_("unknown"),
  N_("clear"),
  N_("encrypted"),
  N_("protected"),
  N_("signed"),
  N_("clearsigned"),
  N_("detach-signed")
};

gchar *
gpa_file_status_string (GpapaFileStatus status)
{
  return _(file_status_strings[status]);
} /* gpa_file_status_string */


static gchar *sig_validity_strings[3] = {
  N_("unknown"),
  N_("!INVALID!"),
  N_("valid")
};

gchar *
gpa_sig_validity_string (GpapaSigValidity validity)
{
  return _(sig_validity_strings[validity]);
} /* gpa_sig_validity_string */

