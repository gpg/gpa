/* gpapastrings.h  -	 The GNU Privacy Assistant
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

/*
 *	Some helper functions to translate between readable strings and
 *	enums
 */

#ifndef GPAPASTRINGS_H
#define GPAPASTRINGS_H

#include <gpapa.h>

gchar * gpa_keytrust_string (GpapaKeytrust keytrust);

gchar * gpa_ownertrust_string (GpapaOwnertrust ownertrust);
GpapaOwnertrust gpa_ownertrust_from_string (gchar * string);
const char * gpa_ownertrust_icon_name (GpapaOwnertrust ownertrust);

gchar * gpa_algorithm_string (GpapaAlgo algo);
GpapaAlgo gpa_algorithm_from_string (gchar * string);

gchar * gpa_unit_expiry_time_string(int index);
gchar gpa_time_unit_from_string (gchar * string);
gchar * gpa_expiry_date_string (GDate * expiry_date);
gchar * gpa_creation_date_string (GDate * creation_date);

gchar * gpa_file_status_string (GpapaFileStatus status);

gchar * gpa_sig_validity_string (GpapaSigValidity validity);

#endif /* GPAPASTRINGS_H */
