/* certchain.h  - Definitions of functions to show a certification chain.
 * Copyright (C) 2009 g10 Code GmbH.
 *
 * This file is part of GPA.
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

#ifndef CERTCHAIN_H
#define CERTCHAIN_H

GtkWidget *gpa_certchain_new (void);
void gpa_certchain_update (GtkWidget *list, gpgme_key_t key);



#endif /*CERTCHAIN_H*/
