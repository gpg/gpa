/* icons.h  -  Icons for GPA
   Copyright (C) 2000 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  */

#ifndef ICONS_H
#define ICONS_H

GdkPixbuf *gpa_create_icon_pixbuf (const char *name);

void gpa_register_stock_items (void);

#define GPA_STOCK_SIGN "gpa-sign"
#define GPA_STOCK_VERIFY "gpa-verify"
#define GPA_STOCK_ENCRYPT "gpa-encrypt"
#define GPA_STOCK_DECRYPT "gpa-decrypt"

/* Windows items.  */
#define GPA_STOCK_KEYMAN        "gpa-keyring"
#define GPA_STOCK_KEYMAN_SIMPLE "gpa-keyringeditor"
#define GPA_STOCK_FILEMAN       "gpa-fileman"
#define GPA_STOCK_CLIPBOARD     "gpa-clipboard"
#define GPA_STOCK_CARDMAN       "gpa-cardman"

/* Toolbar in key manager.  */
#define GPA_STOCK_BRIEF    "gpa-brief"
#define GPA_STOCK_DETAILED "gpa-detailed"
#define GPA_STOCK_EDIT     "gpa-edit"
#define GPA_STOCK_IMPORT   "gpa-import"
#define GPA_STOCK_EXPORT   "gpa-export"
#define GPA_STOCK_BACKUP   "gpa-backup"

#define GPA_STOCK_SECRET_CARDKEY "gpa-secret-cardkey"
#define GPA_STOCK_SECRET_KEY "gpa-secret-key"
#define GPA_STOCK_PUBLIC_KEY "gpa-public-key"

#endif /*ICONS_H*/
