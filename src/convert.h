/* convert.c - Conversion functions
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

#ifndef CONVERT_H
#define CONVERT_H 1

const char *gpa_unit_expiry_time_string (int idx);
char gpa_time_unit_from_string (const char *string);
char *gpa_date_string (unsigned long t);
char *gpa_expiry_date_string (unsigned long expiry_time);
char *gpa_creation_date_string (unsigned long creation_time);
char *gpa_update_origin_string (unsigned long last_update, unsigned int origin);
const char *gpa_sex_char_to_string (char sex);

#endif /*CONVERT_H*/
