/* helpmenu.c  -  The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *	Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "mischelp.h"
#include "stringhelp.h"
#include "gpa.h"
#include "gpawindowkeeper.h"
#include "gtktools.h"


static char *scroll_text[] =
{
  "Markus Gerwinski",
  "Peter Gerwinski",
  "Peter Neuhaus",
  "Werner Koch",
  "Jan-Oliver Wagner",
  "Beate Esser"
};
static int shuffle_array[ DIM(scroll_text) ];
static int do_scrolling = 0;
static int scroll_state = 0;
static int scroll_offset = 0;
static int *scroll_text_widths;
static GtkWidget *about_dialog = NULL;
static GtkWidget *scroll_area = NULL;
static GdkPixmap *scroll_pixmap = NULL;
static GtkWidget *logo_area = NULL;
static GdkPixmap *logo_pixmap = NULL;
static int logo_width = 0;
static int logo_height = 0;
static int cur_scroll_text = 0;
static int cur_scroll_index = 0;
static int timer = 0;



static void
about_dialog_unmap (void)
{
  if (timer)
    {
      gtk_timeout_remove (timer);
      timer = 0;
    }
}

static void
about_dialog_destroy (void)
{
  about_dialog = NULL;
  about_dialog_unmap ();
}


static int
about_dialog_button (GtkWidget *widget, GdkEventButton *event)
{
  if (timer)
    gtk_timeout_remove (timer);
  timer = 0;
  gtk_widget_hide (about_dialog);
  return FALSE;
}


static int
about_dialog_timer (gpointer data)
{
  gint return_val = TRUE;
  
  if (do_scrolling)
    {
      if (!scroll_pixmap)
	{
	  scroll_pixmap = gdk_pixmap_new (scroll_area->window,
					  scroll_area->allocation.width,
					  scroll_area->allocation.height,
					  -1);
	}

      switch (scroll_state)
	{
	case 1:
	  scroll_state = 2;
	  timer = gtk_timeout_add (700, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	case 2:
	  scroll_state = 3;
	  timer = gtk_timeout_add (75, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	}

      if (scroll_offset > (scroll_text_widths[cur_scroll_text]
			   + scroll_area->allocation.width))
	{
	  scroll_state = 0;
	  if (++cur_scroll_index == DIM(scroll_text))
	    cur_scroll_index = 0;
	  cur_scroll_text = shuffle_array[cur_scroll_index];
	  scroll_offset = 0;
	}

      gdk_draw_rectangle (scroll_pixmap,
			  scroll_area->style->white_gc,
			  TRUE, 0, 0,
			  scroll_area->allocation.width,
			  scroll_area->allocation.height);
      gdk_draw_string (scroll_pixmap,
		       scroll_area->style->font,
		       scroll_area->style->black_gc,
		       scroll_area->allocation.width - scroll_offset,
		       scroll_area->style->font->ascent,
		       scroll_text[cur_scroll_text]);
      gdk_draw_pixmap (scroll_area->window,
		       scroll_area->style->black_gc,
		       scroll_pixmap, 0, 0, 0, 0,
		       scroll_area->allocation.width,
		       scroll_area->allocation.height);

      scroll_offset += 15;
      if (!scroll_state
	  && scroll_offset > ((scroll_area->allocation.width
			       + scroll_text_widths[cur_scroll_text])/2))
	{
	  scroll_state = 1;
	  scroll_offset = (scroll_area->allocation.width
			   + scroll_text_widths[cur_scroll_text])/2;
	}
    }

  return return_val;
}


static int
about_dialog_logo_expose (GtkWidget *widget, GdkEventExpose *event)
{
  /* If we draw beyond the boundaries of the pixmap, then X
     will generate an expose area for those areas, starting
     an infinite cycle. We now set allow_grow = FALSE, so
     the drawing area can never be bigger than the preview.
     Otherwise, it would be necessary to intersect event->area
     with the pixmap boundary rectangle. */

  gdk_draw_pixmap (widget->window,
		   widget->style->black_gc,
		   logo_pixmap,
		   event->area.x, event->area.y,
		   event->area.x, event->area.y,
		   event->area.width, event->area.height);
  
  return FALSE;
}

static int
about_dialog_load_logo (GtkWidget *window)
{
  GtkWidget *preview;
  GdkGC *gc;
  char buf[1024];
  unsigned char *pixelrow;
  FILE *fp;
  int count;
  int i;

  {
    char *fname;
    const char *datadir = "." /*GPA_DATADIR */;

    fname = xmalloc (strlen(datadir) + 20);
    strcpy (stpcpy (fname, datadir), "/gpa_logo.ppm");
    fp = fopen (fname, "rb");
    free (fname);
  }

  if (!fp)
    return -1;

  fgets (buf, DIM(buf), fp);
  if (strcmp (buf, "P6\n"))
    {
      fclose (fp);
      return -1;
    }

  fgets (buf, DIM(buf), fp);
  fgets (buf, DIM(buf), fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fgets (buf, DIM(buf), fp);
  if (strcmp (buf, "255\n"))
    {
      fclose (fp);
      return -1;
    }

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), logo_width, logo_height);
  pixelrow = g_new (guchar, logo_width * 3);

  for (i = 0; i < logo_height; i++)
    {
      count = fread (pixelrow, 1, logo_width * 3, fp);
      if (count != (logo_width * 3))
	{
	  gtk_widget_destroy (preview);
	  g_free (pixelrow);
	  fclose (fp);
	  return -1;
	}
      gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0,
			    i, logo_width);
    }

  gtk_widget_realize (window);
  logo_pixmap = gdk_pixmap_new (window->window, logo_width, logo_height,
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_destroy (gc);

  gtk_widget_unref (preview);
  g_free (pixelrow);

  fclose (fp);

  return 0;
}

/****************
 * Scroll and logo stuff taken from GIMP 1.0
 */
void
help_about (void)
{

  if (!about_dialog)
    {
      GtkWidget *vbox;
      GtkWidget *frame;
      GtkWidget *label;
      GtkWidget *alignment;
      int max_width;
      int i;

      about_dialog = gtk_dialog_new ();
      gtk_window_set_title (GTK_WINDOW (about_dialog), _("About GPA"));
      gtk_window_set_policy (GTK_WINDOW (about_dialog), FALSE, FALSE, FALSE);
      gtk_window_set_position (GTK_WINDOW (about_dialog), GTK_WIN_POS_CENTER);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "destroy",
			  GTK_SIGNAL_FUNC(about_dialog_destroy), NULL);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "unmap_event",
			  GTK_SIGNAL_FUNC(about_dialog_unmap), NULL);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "button_press_event",
			  GTK_SIGNAL_FUNC(about_dialog_button), NULL);
      gtk_widget_set_events (about_dialog, GDK_BUTTON_PRESS_MASK);

      if (!logo_pixmap && about_dialog_load_logo (about_dialog))
	{
	  /* problem reading the logo image */
	  logo_width = logo_height = 0;
	}

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
      gtk_container_add (GTK_CONTAINER(GTK_DIALOG(about_dialog)->vbox), vbox);
      gtk_widget_show (vbox);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
      gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      if (logo_width)
	{
	  logo_area = gtk_drawing_area_new ();
	  gtk_signal_connect (GTK_OBJECT (logo_area), "expose_event",
			      (GtkSignalFunc) about_dialog_logo_expose, NULL);
	  gtk_drawing_area_size (GTK_DRAWING_AREA (logo_area),
				 logo_width, logo_height);
	  gtk_widget_set_events (logo_area, GDK_EXPOSURE_MASK);
	  gtk_container_add (GTK_CONTAINER (frame), logo_area);
	  gtk_widget_show (logo_area);

	  gtk_widget_realize (logo_area);
	  gdk_window_set_background (logo_area->window,
				     &logo_area->style->black);
	}


      label = gtk_label_new ("GNU Privacy Assistant v" VERSION);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("brought to you by"));
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, TRUE, 0);
      gtk_widget_show (alignment);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_border_width (GTK_CONTAINER (frame), 0);
      gtk_container_add (GTK_CONTAINER (alignment), frame);
      gtk_widget_show (frame);

      scroll_text_widths = xcalloc (DIM(scroll_text),
				    sizeof *scroll_text_widths);
      max_width = 0;
      for (i = 0; i < DIM(scroll_text); i++)
	{
	  scroll_text_widths[i] = gdk_string_width (frame->style->font,
						    scroll_text[i]);
	  if (scroll_text_widths[i] > max_width)
	    max_width = scroll_text_widths[i];
	}

      scroll_area = gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (scroll_area),
			     max_width + 10,
			     frame->style->font->ascent
			     + frame->style->font->descent );
      gtk_widget_set_events (scroll_area, GDK_BUTTON_PRESS_MASK);
      gtk_container_add (GTK_CONTAINER (frame), scroll_area);
      gtk_widget_show (scroll_area);

      label = gtk_label_new (_("See http://www.gnupg.org for news"));
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      gtk_widget_realize (scroll_area);
      gdk_window_set_background (scroll_area->window,
				 &scroll_area->style->white);

    }

  if (!GTK_WIDGET_VISIBLE (about_dialog))
    {
      int i;

      gtk_widget_show (about_dialog);

      do_scrolling = 0;
      scroll_state = 0;

      for (i = 0; i < DIM(scroll_text); i++)
	{
	  shuffle_array[i] = i;
	}

      for (i = 0; i < DIM(scroll_text); i++)
	{
	  int j, k;
	  j = rand() % DIM(scroll_text); /* Hmmm: Not really portable */
	  k = rand() % DIM(scroll_text);
	  if (j != k)
	    {
	      int t;
	      t = shuffle_array[j];
	      shuffle_array[j] = shuffle_array[k];
	      shuffle_array[k] = t;
	    }
	}
      do_scrolling = 1;
      about_dialog_timer (about_dialog);
      timer = gtk_timeout_add (75, about_dialog_timer, NULL);
    }
  else
    {
      gdk_window_raise(about_dialog->window);
    }
}

static void
help_license_destroy (GtkWidget * widget, gpointer param)
{
  gtk_main_quit ();
}


void
help_license (gpointer param)
{
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gpointer *paramClose;
  GtkWidget *parent = param;

  GtkWidget *windowLicense;
  GtkWidget *vboxLicense;
  GtkWidget *vboxGPL;
  GtkWidget *labelJfdGPL;
  GtkWidget *labelGPL;
  GtkWidget *hboxGPL;
  GtkWidget *textGPL;
  GtkWidget *vscrollbarGPL;
  GtkWidget *hButtonBoxLicense;
  GtkWidget *buttonClose;

  keeper = gpa_windowKeeper_new ();
  windowLicense = gtk_window_new (GTK_WINDOW_DIALOG);
  gpa_windowKeeper_set_window (keeper, windowLicense);
  gtk_window_set_title (GTK_WINDOW (windowLicense),
			_("GNU general public license"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowLicense), accelGroup);
  gtk_signal_connect (GTK_OBJECT (windowLicense), "destroy",
		      GTK_SIGNAL_FUNC (help_license_destroy), NULL);
  
  vboxLicense = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxLicense), 5);
  vboxGPL = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxGPL), 5);
  labelGPL = gtk_label_new ("");
  labelJfdGPL = gpa_widget_hjustified_new (labelGPL, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxGPL), labelJfdGPL, FALSE, FALSE, 0);
  hboxGPL = gtk_hbox_new (FALSE, 0);
  textGPL = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (textGPL), FALSE);
  gtk_text_insert (GTK_TEXT (textGPL), NULL, &textGPL->style->black, NULL,
		   "GNU General public license", -1);
  gtk_widget_set_usize (textGPL, 280, 400);
  gpa_connect_by_accelerator (GTK_LABEL (labelGPL), textGPL, accelGroup,
			      _("_GNU general public license"));
  gtk_box_pack_start (GTK_BOX (hboxGPL), textGPL, TRUE, TRUE, 0);
  vscrollbarGPL = gtk_vscrollbar_new (GTK_TEXT (textGPL)->vadj);
  gtk_box_pack_start (GTK_BOX (hboxGPL), vscrollbarGPL, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxGPL), hboxGPL, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxLicense), vboxGPL, TRUE, TRUE, 0);
  hButtonBoxLicense = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxLicense),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxLicense), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxLicense), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  buttonClose = gpa_buttonCancel_new (accelGroup, _("_Close"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxLicense), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxLicense), hButtonBoxLicense, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowLicense), vboxLicense);

  gtk_window_set_modal (GTK_WINDOW (windowLicense), TRUE);
  gpa_window_show_centered (windowLicense, parent);
  gtk_main ();
}

void
help_warranty (void)
{
  g_print (_("Show Warranty Information\n"));   /*!!! */
}				/* help_warranty */

void
help_help (void)
{
  g_print (_("Show Help Text\n"));      /*!!! */
}				/* help_help */


void
gpa_help_menu_add_to_factory (GtkItemFactory *factory, GtkWidget * window)
{
  GtkItemFactoryEntry menu[] = {
    {_("/_Help"), NULL, NULL, 0, "<Branch>"},
    {_("/Help/_About"), NULL, help_about, 0, NULL},
    {_("/Help/_License"), NULL, help_license, 0, NULL},
    {_("/Help/_Warranty"), NULL, help_warranty, 0, NULL},
    {_("/Help/_Help"), "F1", help_help, 0, NULL}
  };

  gtk_item_factory_create_items (factory, sizeof (menu) / sizeof (menu[0]),
				 menu, window);
}
