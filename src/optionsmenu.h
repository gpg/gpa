/* optionsmenu.h  -  The GNU Privacy Assistant
 *      Copyright (C) 2000, 2001 G-N-U GmbH.
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

extern void options_keyserver (void);
extern void options_recipients (void);
extern void options_key (void);
extern void options_homedir (void);
extern void options_load (void);
extern void options_save (void);

extern void gpa_homeDirSelect_init (gchar * title);
extern void gpa_loadOptionsSelect_init (gchar * title);
extern void gpa_saveOptionsSelect_init (gchar * title);

void gpa_options_menu_add_to_factory (GtkItemFactory *factory,
				      GtkWidget *window);
