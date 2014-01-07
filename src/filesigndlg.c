/* filesigndlg.c  -  The GNU Privacy Assistant
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <errno.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "gpakeyselector.h"

#include "filesigndlg.h"


/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_FORCE_ARMOR,
  PROP_ARMOR,
  PROP_FORCE_SIG_MODE,
  PROP_SIG_MODE
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
      g_value_set_boolean (value,
			   gpa_file_sign_dialog_get_force_armor (dialog));
      break;
    case PROP_ARMOR:
      g_value_set_boolean (value, gpa_file_sign_dialog_get_armor (dialog));
      break;
    case PROP_FORCE_SIG_MODE:
      g_value_set_boolean (value,
			   gpa_file_sign_dialog_get_force_sig_mode (dialog));
      break;
    case PROP_SIG_MODE:
      g_value_set_int (value, gpa_file_sign_dialog_get_sig_mode (dialog));
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
      gpa_file_sign_dialog_set_force_armor (dialog,
					    g_value_get_boolean (value));
      break;
    case PROP_ARMOR:
      gpa_file_sign_dialog_set_armor (dialog, g_value_get_boolean (value));
      break;
    case PROP_FORCE_SIG_MODE:
      gpa_file_sign_dialog_set_force_sig_mode (dialog,
					       g_value_get_boolean (value));
      break;
    case PROP_SIG_MODE:
      gpa_file_sign_dialog_set_sig_mode (dialog, g_value_get_int (value));
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
  GtkWidget *label;
  GtkWidget *frameMode;
  GtkWidget *vboxMode;
  GtkWidget *radio_sign_comp;
  GtkWidget *radio_sign;
  GtkWidget *radio_sign_sep;
  GtkWidget *checkerArmor;
  GtkWidget *frameWho;
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
			  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			  GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gpa_window_set_title (GTK_WINDOW (dialog), _("Sign documents"));
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  vboxSign = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vboxSign);

  frameWho = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frameWho), GTK_SHADOW_NONE);
  labelWho = gtk_label_new_with_mnemonic (_("<b>Sign _as</b>"));
  gtk_label_set_use_markup (GTK_LABEL (labelWho), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frameWho), labelWho);
  gtk_box_pack_start (GTK_BOX (vboxSign), frameWho, FALSE, FALSE, 0);

  vboxWho = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frameWho), vboxWho);

  scrollerWho = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrollerWho, 400, 200);
  gtk_box_pack_start (GTK_BOX (vboxWho), scrollerWho, TRUE, TRUE, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollerWho),
				       GTK_SHADOW_IN);

  clistWho = gpa_key_selector_new (TRUE, TRUE);
  dialog->clist_who = clistWho;
  gtk_container_add (GTK_CONTAINER (scrollerWho), clistWho);
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelWho), clistWho);

  frameMode = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frameMode), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>Signing Mode</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frameMode), label);
  gtk_box_pack_start (GTK_BOX (vboxSign), frameMode, FALSE, FALSE, 0);

  vboxMode = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frameMode), vboxMode);
  dialog->frame_mode = frameMode;

  radio_sign_comp = gtk_radio_button_new_with_mnemonic
    (NULL, _("Si_gn and compress"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign_comp, FALSE, FALSE, 0);
  dialog->radio_comp = radio_sign_comp;

  radio_sign =
    gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radio_sign_comp), _("Clear_text signature"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign, FALSE, FALSE, 0);
  dialog->radio_sign = radio_sign;
  /* FIXME: We hide the radio button here.  It still can be activated
     invisibly by setting the "sig-mode" property.  This is used by
     the clipboard code, which also hides the whole sig mode selection
     frame, so no harm done.  For the file sign mode, hiding the
     cleartext option is the right thing to do.  But eventually, all
     this should be freely configurable by the caller, instead of
     relying on such knowledge.  */
  gtk_widget_set_no_show_all (radio_sign, TRUE);
  gtk_widget_hide (radio_sign);

  radio_sign_sep =
    gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (radio_sign_comp), _("_Detached signature"));
  gtk_box_pack_start (GTK_BOX (vboxMode), radio_sign_sep, FALSE, FALSE, 0);
  dialog->radio_sep = radio_sign_sep;

  /* Allow for the frameMode to be hidden despite what show_all
     does.  */
  gtk_widget_show_all (frameMode);
  gtk_widget_set_no_show_all (frameMode, TRUE);

  checkerArmor = gtk_check_button_new_with_mnemonic (_("A_rmor"));
  gtk_box_pack_start (GTK_BOX (vboxSign), checkerArmor, FALSE, FALSE, 0);
  /* Take care of any child widgets there might be.  */
  gtk_widget_show_all (checkerArmor);
  gtk_widget_set_no_show_all (checkerArmor, TRUE);
  dialog->check_armor = checkerArmor;

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
				    G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
				   PROP_ARMOR,
				   g_param_spec_boolean
				   ("armor", "Armor mode",
				    "Armor mode", FALSE,
				    G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
				   PROP_FORCE_SIG_MODE,
				   g_param_spec_boolean
				   ("force-sig-mode", "Force signature mode",
				    "Force signature mode", FALSE,
				    G_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
				   PROP_SIG_MODE,
				   g_param_spec_int
				   ("sig-mode", "Signature mode",
				    "Signature mode", GPGME_SIG_MODE_NORMAL,
				    GPGME_SIG_MODE_CLEAR, GPGME_SIG_MODE_NORMAL,
				    G_PARAM_WRITABLE));
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
gpa_file_sign_dialog_new (GtkWidget *parent)
{
  GpaFileSignDialog *dialog;

  dialog = g_object_new (GPA_FILE_SIGN_DIALOG_TYPE,
			 "window", parent,
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
  g_return_val_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog), FALSE);
  g_return_val_if_fail (dialog->check_armor != NULL, FALSE);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->check_armor));
}


void
gpa_file_sign_dialog_set_armor (GpaFileSignDialog *dialog, gboolean armor)
{
  g_return_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog));
  g_return_if_fail (dialog->check_armor != NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->check_armor), armor);
}


gboolean
gpa_file_sign_dialog_get_force_armor (GpaFileSignDialog *dialog)
{
  g_return_val_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog), FALSE);

  return dialog->force_armor;
}


void
gpa_file_sign_dialog_set_force_armor (GpaFileSignDialog *dialog,
				      gboolean force_armor)
{
  g_return_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog));
  g_return_if_fail (dialog->check_armor != NULL);

  if (force_armor == dialog->force_armor)
    return;

  if (force_armor)
    gtk_widget_hide (dialog->check_armor);
  else
    gtk_widget_show (dialog->check_armor);

  dialog->force_armor = force_armor;
}


gpgme_sig_mode_t
gpa_file_sign_dialog_get_sig_mode (GpaFileSignDialog *dialog)
{
  gpgme_sig_mode_t sig_mode = GPGME_SIG_MODE_NORMAL;

  g_return_val_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog),
			GPGME_SIG_MODE_NORMAL);
  g_return_val_if_fail (dialog->frame_mode != NULL, GPGME_SIG_MODE_NORMAL);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_comp)))
    sig_mode = GPGME_SIG_MODE_NORMAL;
  else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->radio_sign)))
    sig_mode = GPGME_SIG_MODE_CLEAR;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(dialog->radio_sep)))
    sig_mode = GPGME_SIG_MODE_DETACH;

  return sig_mode;
}


void
gpa_file_sign_dialog_set_sig_mode (GpaFileSignDialog *dialog,
				   gpgme_sig_mode_t mode)
{
  GtkWidget *button = NULL;

  g_return_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog));
  g_return_if_fail (dialog->frame_mode != NULL);

  if (mode == GPGME_SIG_MODE_NORMAL)
    button = dialog->radio_comp;
  else if (mode == GPGME_SIG_MODE_CLEAR)
    button = dialog->radio_sign;
  else if (mode == GPGME_SIG_MODE_DETACH)
    button = dialog->radio_sep;

  if (button != NULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
}


gboolean
gpa_file_sign_dialog_get_force_sig_mode (GpaFileSignDialog *dialog)
{
  g_return_val_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog), FALSE);

  return dialog->force_sig_mode;
}


void
gpa_file_sign_dialog_set_force_sig_mode (GpaFileSignDialog *dialog,
				      gboolean force_sig_mode)
{
  g_return_if_fail (GPA_IS_FILE_SIGN_DIALOG (dialog));
  g_return_if_fail (dialog->frame_mode != NULL);

  if (force_sig_mode == dialog->force_sig_mode)
    return;

  if (force_sig_mode)
    gtk_widget_hide (dialog->frame_mode);
  else
    gtk_widget_show (dialog->frame_mode);
}
