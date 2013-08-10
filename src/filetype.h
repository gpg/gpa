/* filetype.h -  Identify file types
 * Copyright (C) 2012 g10 Code GmbH
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILETYPE_H
#define FILETYPE_H

int is_cms_file (const char *fname);
int is_cms_data (const char *data, size_t datalen);
int is_cms_data_ext (gpgme_data_t dh);


#endif /*FILETYPE_H*/
