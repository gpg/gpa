/* gtkhacks.h - kludge around incompatibilities between GTK+ version 1 and 2
 *
 * Copyright (C) 2002 G-N-U GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software. You can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY - without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GPA; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef GTKHACKS_H
#define GTKHACKS_H

#include <glib.h>
#include <gtk/gtk.h>

#if (GTK_CHECK_VERSION (1, 3, 12)) /* new GTK */

#define __NEW_GTK__
#define my_gtk_style_get_xthickness(foo) (foo->xthickness)

#else /* old GTK */

#define gtk_label_new_with_mnemonic gtk_label_new
#define gtk_button_new_with_mnemonic gtk_button_new_with_label
#define gtk_menu_item_new_with_mnemonic gtk_menu_item_new_with_label

#define gtk_style_get_font(foo) (foo->font)
#define my_gtk_style_get_xthickness(foo) (foo->klass->xthickness)

#endif /* old GTK */

#endif /* GTKHACKS_H */

