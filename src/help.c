/* help.c  - Help facility
 * Copyright (C) 2000 OpenIT GmbH
 * Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#ifdef HAVE_LOCALE_H
  #include <locale.h>
#endif

#include "gpa.h"
#include "stringhelp.h"

GtkWidget *text;
GtkWidget *global_textTip;

#if 0
void
help_set_text (char *string)
{
  gtk_text_freeze (GTK_TEXT (text));
  gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL,
		   string, -1);
  gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL,
		   "\n", -1);
  gtk_text_thaw (GTK_TEXT (text));
}

void
help_init (GtkWidget * box1)
{
  GtkWidget *table, *vscrollbar;

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (box1), table, TRUE, TRUE, 0);
  gtk_widget_show (table);
  text = gtk_text_new (NULL, NULL);
  gtk_text_set_line_wrap (GTK_TEXT (text), FALSE);
  gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (text);

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
  gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vscrollbar);
  help_set_text (_("Welcome to the GNU privacy guard assistant."));
}
#endif


/* Guess value of current locale from value of the environment variables.  */
/* Taken from GNU gettext */
static const char *
get_language ( void )
{
  const char *retval;

  /* The highest priority value is the `LANGUAGE' environment
     variable.	This is a GNU extension.  */
  retval = getenv ("LANGUAGE");
  if (retval != NULL && retval[0] != '\0')
    goto leave;

  /* `LANGUAGE' is not set.  So we have to proceed with the POSIX
     methods of looking to `LC_ALL', `LC_xxx', and `LANG'.  On some
     systems this can be done by the `setlocale' function itself.  */
#if defined HAVE_SETLOCALE && defined HAVE_LC_MESSAGES && defined HAVE_LOCALE_NULL
  retval = setlocale (LC_MESSAGES, NULL);
  goto leave;
#else
  /* Setting of LC_ALL overwrites all other.  */
  retval = getenv ("LC_ALL");
  if (retval != NULL && retval[0] != '\0')
    goto leave;

  /* Next comes the name of the desired category.  */
  retval = getenv ("LC_MESSAGES");
  if (retval != NULL && retval[0] != '\0')
    goto leave;

  /* Last possibility is the LANG environment variable.  */
  retval = getenv ("LANG");
  if (retval != NULL && retval[0] != '\0')
    goto leave;
  retval = "en"; /* default is English */
#endif
 leave:
  if ( !retval || !*retval
       || !strcmp( retval, "C" ) || !strcmp (retval, "POSIX" ) )
    retval = "en";
  return retval;
}

void
gpa_windowTip_show ( const char *key )
{
    char line[512];
    FILE *fp;
    int empty_lines = 0;

    if ( global_noTips )
	return;

    gtk_editable_delete_text (GTK_EDITABLE (global_textTip), 0, -1);

    {	char *fname;
	const char *datadir = GPA_DATADIR;
	const char *lang = get_language ();

	fname = xmalloc ( 20 + strlen(datadir) + strlen(lang) );
	strcpy ( stpcpy ( stpcpy ( fname, datadir ), "/gpa_tips."), lang );
	fp = fopen ( fname, "r" );
	free ( fname );
    }
    if ( !fp ) {
	gtk_text_insert (GTK_TEXT (global_textTip), NULL,
			 &global_textTip->style->black, NULL,
			 _("Installation problem: tip file not found"), -1 );
	goto leave;
    }

    /* Read until we find the key */
    while ( fgets ( line, DIM(line), fp ) ) {
	trim_spaces ( line );
	if ( *line == '\\' && line[1] == ':' ) {
	    trim_spaces (line+2);
	    if ( !strcmp ( line+2, key ) )
		break;
	}
    }
    if ( ferror (fp) || feof (fp) ) {
	gtk_text_insert (GTK_TEXT (global_textTip), NULL,
			 &global_textTip->style->black, NULL,
			 key, -1 );
	goto leave;
    }
    /* Continue with read, do the formatting and stop at the next key */
    while ( fgets ( line, DIM(line), fp ) ) {
	trim_spaces ( line );
	if ( *line == '#' )
	    continue;
	if ( *line == '\\' && line[1] == ':' )
	    break; /* next key - ready */
	if ( !*line ) {
	    empty_lines++;
	    continue;
	}
	if (empty_lines) { /* make a paragraph */
	    gtk_text_insert (GTK_TEXT (global_textTip), NULL,
			 &global_textTip->style->black, NULL, "\n\n", -1);
	    empty_lines = 0;
	}
	/* fixme: handle control sequences */
	gtk_text_insert (GTK_TEXT (global_textTip), NULL,
			 &global_textTip->style->black, NULL, line, -1);
    }
 leave:
    if ( fp )
	fclose ( fp );
    gtk_widget_show_all (global_windowTip);
}


