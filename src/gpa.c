/* pga.c  - The Privacy Guard Assistant
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include "pga.h"


GtkWidget	*box1, *box2,
		*files_box, *crypt_op_box,  *file_op_box, *file_expert_box,
		*keys_box, *key_op_box, *key_expert_box;
char *file_to_crypt=NULL;
char *file_to_import=NULL;
int key_expert_mode=0, file_expert_mode=0;

void cb_main(GtkWidget *widget, gpointer data)
{
	g_print ("button \"%s\" was pressed\n", (char *) data);
	if (!strcmp(data, "files")) {
		gtk_widget_hide(keys_box);
		gtk_widget_show(files_box);
	}
	if (!strcmp(data, "keys")) {
		gtk_widget_hide(files_box);
		gtk_widget_show(keys_box);
	}
	if (!strcmp(data, "select files")) {

		/*file_to_crypt=select_files();*/
		help_set_text(_("source file: "));
		help_set_text(file_to_crypt);
		help_set_text("\n");
	}
}

void cb_file(GtkWidget *widget, gpointer data)
{
	static int file_op_flags=0, files_selected=0;

	g_print ("button \"%s\" was pressed\n", (char *) data);
	if (!strcmp(data, "select files")) {
		/*select_files();*/
	}
	if (!strcmp(data, "options")) {
		if ((file_expert_mode^=1)) {
			gtk_widget_show(file_expert_box);
		} else {
			gtk_widget_hide(file_expert_box);
		}
	}
	if (!strcmp(data, "crypt ok")) {
		if (!file_op_flags && !files_selected) {
			help_set_text(_("Select file(s) and operation(s) to perform."));
		} else if (!file_op_flags) {
			help_set_text(_("Select operation(s) to perform."));
		} else if (!files_selected) {
			help_set_text(_("Select file(s) to process."));
		}
	}
}

void cb_keys(GtkWidget *widget, gpointer data)
{
	g_print ("button \"%s\" was pressed\n", (char *) data);
	if (!strcmp(data, "select files")) {
		/*select_files();*/
	}
	if (!strcmp(data, "options")) {
		if ((key_expert_mode^=1)) {
			gtk_widget_show(key_expert_box);
		} else {
			gtk_widget_hide(key_expert_box);
		}
	}
	if (!strcmp(data, "import")) {
	      #if 0
		file_to_import=select_files();
		if (file_to_import)
			import_keys_from_file(file_to_import);
	      #endif
	}

}





static void
make_windows( void )
{
    GtkWidget *window, *button;
    GtkWidget *separator;
    gint i;

    /* a generic toplevel window */
    window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_signal_connect(GTK_OBJECT(window), "delete_event",
				    GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_container_set_border_width(GTK_CONTAINER(window), 5);

    /* *** */
    box1=gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), box1);
    gtk_widget_show(box1);
    box2=gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), box2, TRUE, TRUE, 0);
    gtk_widget_show(box2);
    separator=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);
    keys_box=gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), keys_box, TRUE, TRUE, 0);
    gtk_widget_show(keys_box);
    files_box=gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), files_box, TRUE, TRUE, 0);
    /*	gtk_widget_show(files_box); */
    separator=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);

    crypt_op_box=gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(files_box), crypt_op_box, TRUE, TRUE, 0);
    gtk_widget_show(crypt_op_box);
    separator=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(files_box), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);
    file_op_box=gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(files_box), file_op_box, TRUE, TRUE, 0);
    gtk_widget_show(file_op_box);
    separator=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(files_box), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);
    file_expert_box=gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(files_box), file_expert_box, TRUE, TRUE, 0);
    if (file_expert_mode)
	gtk_widget_show(file_expert_box);

    key_op_box=gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(keys_box), key_op_box, TRUE, TRUE, 0);
    gtk_widget_show(key_op_box);
    separator=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(keys_box), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);
    key_expert_box=gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(keys_box), key_expert_box, TRUE, TRUE, 0);
    if (key_expert_mode)
	    gtk_widget_show(key_expert_box);

    button=gtk_button_new_with_label("keys");
    gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_main), (gpointer) "keys");
    button=gtk_button_new_with_label("files");
    gtk_box_pack_start(GTK_BOX(box2), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_main), (gpointer) "files");
    button=gtk_radio_button_new_with_label(NULL, "encrypt");
    gtk_box_pack_start(GTK_BOX(crypt_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "encrypt");
    button=gtk_radio_button_new_with_label(NULL, "sign");
    gtk_box_pack_start(GTK_BOX(crypt_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "");
    button=gtk_radio_button_new_with_label(NULL, "decrypt");
    gtk_box_pack_start(GTK_BOX(crypt_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "decrypt");
    button=gtk_radio_button_new_with_label(NULL, "check signature");
    gtk_box_pack_start(GTK_BOX(crypt_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "check");

    button=gtk_button_new_with_label("select files");
    gtk_box_pack_start(GTK_BOX(file_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "select files");
    button=gtk_button_new_with_label("ok");
    gtk_box_pack_start(GTK_BOX(file_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "crypt ok");
    button=gtk_button_new_with_label("options");
    gtk_box_pack_start(GTK_BOX(file_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "options");
    button=gtk_button_new_with_label("enarmour");
    gtk_box_pack_start(GTK_BOX(file_expert_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "enarmour");
    button=gtk_button_new_with_label("dearmour");
    gtk_box_pack_start(GTK_BOX(file_expert_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_file), (gpointer) "dearmour");
  #if 0
    separator=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);
  #endif
    button=gtk_button_new_with_label("import");
    gtk_box_pack_start(GTK_BOX(key_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_keys), (gpointer) "import");
    button=gtk_button_new_with_label("edit");
    gtk_box_pack_start(GTK_BOX(key_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    button=gtk_button_new_with_label("delete");
    gtk_box_pack_start(GTK_BOX(key_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_keys), (gpointer) "delete");
    button=gtk_button_new_with_label("options");
    gtk_box_pack_start(GTK_BOX(key_op_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_keys), (gpointer) "options");
    button=gtk_button_new_with_label("generate");
    gtk_box_pack_start(GTK_BOX(key_expert_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_keys), (gpointer) "generate");
    button=gtk_button_new_with_label("revoke");
    gtk_box_pack_start(GTK_BOX(key_expert_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_keys), (gpointer) "revoke");
    button=gtk_button_new_with_label("sign");
    gtk_box_pack_start(GTK_BOX(key_expert_box), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    GTK_SIGNAL_FUNC(cb_keys), (gpointer) "sign");

   /* key_tree_init(box1);*/
    help_init(box1);
    gtk_widget_show(window);
}





int
main( int argc, char **argv )
{

    gtk_init(&argc, &argv);

    make_windows();

    gtk_main();

    return 0;
}

