/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GPA_FILESEL_H__
#define __GPA_FILESEL_H__


#include <gdk/gdk.h>
#include <gtk/gtkdialog.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GPA_TYPE_FILE_SELECTION            (gpa_file_selection_get_type ())
#define GPA_FILE_SELECTION(obj)            (GTK_CHECK_CAST ((obj), GPA_TYPE_FILE_SELECTION, GpaFileSelection))
#define GPA_FILE_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GPA_TYPE_FILE_SELECTION, GpaFileSelectionClass))
#define GPA_IS_FILE_SELECTION(obj)         (GTK_CHECK_TYPE ((obj), GPA_TYPE_FILE_SELECTION))
#define GPA_IS_FILE_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GPA_TYPE_FILE_SELECTION))
#define GPA_FILE_SELECTION_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GPA_TYPE_FILE_SELECTION, GpaFileSelectionClass))


typedef struct _GpaFileSelection       GpaFileSelection;
typedef struct _GpaFileSelectionClass  GpaFileSelectionClass;

struct _GpaFileSelection
{
  GtkDialog parent_instance;

  GtkTreeModel *dir_list_model;
  GtkWidget *dir_list;
  GtkWidget *file_list;
  GtkWidget *selection_entry;
  GtkWidget *selection_text;
  GtkWidget *main_vbox;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;
  GtkWidget *help_button;
  GtkWidget *history_pulldown;
  GtkWidget *history_menu;
  GList     *history_list;
  GtkWidget *fileop_dialog;
  GtkWidget *fileop_entry;
  gchar     *fileop_file;
  gpointer   cmpl_state;
  
  GtkWidget *fileop_c_dir;
  GtkWidget *fileop_del_file;
  GtkWidget *fileop_ren_file;
  
  GtkWidget *button_area;
  GtkWidget *action_area;
  
};

struct _GpaFileSelectionClass
{
  GtkDialogClass parent_class;
};


GtkType    gpa_file_selection_get_type            (void) G_GNUC_CONST;
GtkWidget* gpa_file_selection_new                 (const gchar      *title);
void       gpa_file_selection_set_filename        (GpaFileSelection *filesel,
						   const gchar      *filename);
/* This function returns the selected filename in the C runtime's
 * multibyte string encoding, which may or may not be the same as that
 * used by GDK (UTF-8). To convert to UTF-8, call g_filename_to_utf8().
 * The returned string points to a statically allocated buffer and
 * should be copied away.
 */
G_CONST_RETURN gchar* gpa_file_selection_get_filename        (GpaFileSelection *filesel);

void	   gpa_file_selection_complete		  (GpaFileSelection *filesel,
						   const gchar	    *pattern);
void       gpa_file_selection_show_fileop_buttons (GpaFileSelection *filesel);
void       gpa_file_selection_hide_fileop_buttons (GpaFileSelection *filesel);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GPA_FILESEL_H__ */
