/* gpa.h  -  main header
 *      Copyright (C) 2000 Free Software Foundation, Inc.
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
                   
#ifndef GPA_H
#define GPA_H

#include <gtk/gtk.h>

#define _(a)	(a)
#define N_(a)	(a)

GtkWidget *windowMain;
gboolean noTips;

extern void gpa_switch_tips ( void );
extern void gpa_dialog_tip ( char *tip );

#endif /*GPA_H*/
