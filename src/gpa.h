/* pga.h  -  main header
 *	Copyright (C) 2000 Free Software Foundation, Inc.
 *
 * This file is part of PGA
 *
 * PGA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * PGA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef PGA_H
#define PGA_H

#include <gtk/gtk.h>

#define _(a)	(a)
#define N_(a)	(a)



/*-- help.c --*/
void help_init(GtkWidget *box1);
void help_set_text(char *string);



#endif/*PGA_H*/
