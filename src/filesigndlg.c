/* filesigndlg.c  -  The GNU Privacy Assistant
 * Copyright (C) 2000, 2001 G-N-U GmbH.
 * Copyright (C) 2008 g10 Code GmbH.
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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#include <errno.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "filesigndlg.h"
#include "gpakeyselector.h"

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_FORCE_ARMOR
};

static GObjectClass *parent_class = NULL;

static void
gpa_file_sign_dialog_get_property (GObject     *object,
				   guint        prop_id,
				   GValue      *value,
				   GParamSpec  *pspec)
{
  GpaFileSignDialog *dialog = GPA_FILE_SIGN_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value,
			  gtk_window_get_transient_for (GTK_WINDOW (dialog)));
      break;
    case PROP_FORCE_ARMOR:
      g_value_set_boolean (value, dialog->force_armor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_file_sign_dialog_set_property (GObject     *object,
				   guint        prop_id,
				   const GValue      *value,
				   GParamSpec  *pspec)
{
  GpaFileSignDialog *dialog = GPA_FILE_SIGN_DIALOG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      gtk_window_set_transient_for (GTK_WINDOW (dialog),
				    g_value_get_object (value));
      break;
    case PROP_FORCE_ARMOR:
      dialog->force_armor = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_file_sign_dialog_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_file_sign_dialog_init (GpaFileSignDialog *dialog)
{
}


static GObject*
gpa_file_sign_dialog_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileSignDialog *dialog;
  GtkWidget *vboxSign;
  GtkWidget *frameMode;
  GtkWidget *vboxMode;
  GtkWidget *radio_sign_comp;
  GtkWidget *radio_sign;
  GtkWidget *radio_sign_sep;
  GtkWidget *checkerArmor;
  GtkWidget *vboxWho;
  GtkWidget *labelWho;
  GtkWidget *scrollerWho;
  GtkWidget *clistWho;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = GPA_FILE_SIGN_DIALOG (object);
  /* Initialize */

  /* Set up the dialog */
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  GTK_STOCK_OK, GTK_RESPONSE_OK,
			  _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Sign documents"));
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  vboxSign = GTK_DIALOG (dialog)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vboxSign), 5);

  frameMode = gtk_frame_new (_("Signing Mode"));
  gtk_container_set_border_width (GTK_CONTAINER (frameMode), 5);
  gtk_box_pack_start (GTK_BOX (vboxSign), frameMode, FALSE, FALSE, 0);

  vboxMode = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxMode), 5);
  gtk_container_add (GTK_CONTAINER (frameMode), vboxMode);

  radio_sign_comp = gtk_radio_button_new_with_mnemonic (NULL, _("si_gn and compress"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign_comp, FALSE, FALSE, 0);
  dialog->radio_comp = radio_sign_comp;

  radio_sign =
    gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radio_sign_comp), _("_cleartext signature"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign, FALSE, FALSE, 0);
  dialog->radio_sign = radio_sign;

  radio_sign_sep =
    gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radio_sign_comp), _("_detached signature"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign_sep, FALSE, FALSE, 0);
  dialog->radio_sep = radio_sign_sep;

  vboxWho = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxWho), 5);
  gtk_box_pack_start (GTK_BOX (vboxSign), vboxWho, TRUE, TRUE, 0);

  labelWho = gtk_label_new_with_mnemonic (_("Sign _as "));
  gtk_misc_set_alignment (GTK_MISC (labelWho), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxWho), labelWho, FALSE, TRUE, 0);

  scrollerWho = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrollerWho, 400, 200);
  gtk_box_pack_start (GTK_BOX (vboxWho), scrollerWho, TRUE, TRUE, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollerWho),
				       GTK_SHADOW_IN);

  clistWho = gpa_key_selector_new (TRUE);
  dialog->clist_who = clistWho;
  gtk_container_add (GTK_CONTAINER (scrollerWho), clistWho);
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelWho), clistWho);
			      

  checkerArmor = gtk_check_button_new_with_mnemonic (_("A_rmor"));
  gtk_container_set_border_width (GTK_CONTAINER (checkerArmor), 5);
  gtk_box_pack_start (GTK_BOX (vboxSign), checkerArmor, FALSE, FALSE, 0);
  dialog->check_armor = checkerArmor;
  if (dialog->force_armor)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->check_armor),
				    TRUE);
      gtk_widget_set_sensitive (dialog->check_armor, FALSE);
    }

  /* FIXME: Doesn't even work, as caller uses gtk_widget_show_all.  */
  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      gtk_widget_hide (dialog->radio_comp);
      gtk_widget_hide (dialog->check_armor);
    }
  else
    {
      gtk_widget_show (dialog->radio_comp);
      gtk_widget_show (dialog->check_armor);
    }

  return object;
}

static void
gpa_file_sign_dialog_class_init (GpaFileSignDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->constructor = gpa_file_sign_dialog_constructor;
  object_class->finalize = gpa_file_sign_dialog_finalize;
  object_class->set_property = gpa_file_sign_dialog_set_property;
  object_class->get_property = gpa_file_sign_dialog_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_WINDOW,
				   g_param_spec_object 
				   ("window", "Parent window",
				    "Parent window", GTK_TYPE_WIDGET,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_FORCE_ARMOR,
				   g_param_spec_boolean
				   ("force-armor", "Force armor",
				    "Force armor mode", FALSE,
				    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_file_sign_dialog_get_type (void)
{
  static GType sign_dialog_type = 0;
  
  if (!sign_dialog_type)
    {
      static const GTypeInfo sign_dialog_info =
	{
	  sizeof (GpaFileSignDialogClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_file_sign_dialog_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GpaFileSignDialog),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gpa_file_sign_dialog_init,
	};
      
      sign_dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
						    "GpaFileSignDialog",
						    &sign_dialog_info, 0);
    }
  
  return sign_dialog_type;
}

/* API */

GtkWidget *
gpa_file_sign_dialog_new (GtkWidget *parent, gboolean force_armor)
{
  GpaFileSignDialog *dialog;
  
  dialog = g_object_new (GPA_FILE_SIGN_DIALOG_TYPE,
			 "window", parent,
			 "force-armor", force_armor,
			 NULL);

  return GTK_WIDGET(dialog);
}

GList *
gpa_file_sign_dialog_signers (GpaFileSignDialog *dialog)
{
  return gpa_key_selector_get_selected_keys
    (GPA_KEY_SELECTOR(dialog->clist_who));
}

gboolean 
gpa_file_sign_dialog_get_armor (GpaFileSignDialog *dialog)
{
  return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->check_armor));
}

gpgme_sig_mode_t 
gpa_file_sign_dialog_get_sign_type (GpaFileSignDialog *dialog)
{
  gpgme_sig_mode_t sign_type = GPGME_SIG_MODE_NORMAL;
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_comp)))
    sign_type = GPGME_SIG_MODE_NORMAL;
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->radio_sign)))
    sign_type = GPGME_SIG_MODE_CLEAR;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dialog->radio_sep)))
    sign_type = GPGME_SIG_MODE_DETACH;

  return sign_type;
}
