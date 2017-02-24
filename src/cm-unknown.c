/* cm-unknown.c  -  Widget to show information about an unknown card
 * Copyright (C) 2009, 2011 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute and/or modify this part
 * of GPA under the terms of either
 *
 *   - the GNU Lesser General Public License as published by the Free
 *     Software Foundation; either version 3 of the License, or (at
 *     your option) any later version.
 *
 * or
 *
 *   - the GNU General Public License as published by the Free
 *     Software Foundation; either version 2 of the License, or (at
 *     your option) any later version.
 *
 * or both in parallel, as here.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* UNKNOWN is a dummy application used for, well, unknown cards.  It
   does only print the the ATR of the card.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gpa.h"
#include "gtktools.h"
#include "convert.h"
#include "membuf.h"

#include "cm-object.h"
#include "cm-unknown.h"





/* Object's class definition.  */
struct _GpaCMUnknownClass
{
  GpaCMObjectClass parent_class;
};


/* Object definition.  */
struct _GpaCMUnknown
{
  GpaCMObject  parent_instance;

  GtkWidget *label;

  int  reloading;   /* Sentinel to avoid recursive reloads.  */
};

/* The parent class.  */
static GObjectClass *parent_class;



/* Local prototypes */
static void gpa_cm_unknown_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

static gpg_error_t
scd_atr_data_cb (void *opaque, const void *data, size_t datalen)
{
  membuf_t *mb = opaque;

  put_membuf (mb, data, datalen);
  return 0;
}


/* Use the assuan machinery to read the ATR.  */
static void
reload_data (GpaCMUnknown *card)
{
  gpg_error_t err, operr;
  char command[100];
  gpgme_ctx_t gpgagent;
  membuf_t mb;
  char *buf;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);

  card->reloading++;

  init_membuf (&mb, 512);

  err = gpgme_op_assuan_transact_ext (gpgagent,
                                      "SCD APDU --dump-atr",
                                      scd_atr_data_cb, &mb,
                                      NULL, NULL, NULL, NULL, &operr);
  if (!err)
    err = operr;

  if (!err)
    {
      put_membuf (&mb, "", 1);
      buf = get_membuf (&mb, NULL);
      if (buf)
        {
          buf = g_strdup_printf ("\n%s\n%s",
                                 _("The ATR of the card is:"),
                                 buf);
          gtk_label_set_text (GTK_LABEL (card->label), buf);
          g_free (buf);
        }
      else
        gtk_label_set_text (GTK_LABEL (card->label), "");
    }
  else
    {
      g_free (get_membuf (&mb, NULL));

      if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT)
        ; /* Lost the card.  */
      else
        g_debug ("assuan command `%s' failed: %s <%s>\n",
                 command, gpg_strerror (err), gpg_strsource (err));
      gtk_label_set_text (GTK_LABEL (card->label), "");
    }
  card->reloading--;
}




/* This function constructs the container holding all widgets making
   up this data widget.  It is called during instance creation.  */
static void
construct_data_widget (GpaCMUnknown *card)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *label;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>Unknown Card</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  vbox = gtk_vbox_new (FALSE, 5);
  card->label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), card->label, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);
}



/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_cm_unknown_class_init (void *class_ptr, void *class_data)
{
  GpaCMUnknownClass *klass = class_ptr;

  parent_class = g_type_class_peek_parent (klass);

  G_OBJECT_CLASS (klass)->finalize = gpa_cm_unknown_finalize;
}


static void
gpa_cm_unknown_init (GTypeInstance *instance, void *class_ptr)
{
  GpaCMUnknown *card = GPA_CM_UNKNOWN (instance);

  construct_data_widget (card);

}


static void
gpa_cm_unknown_finalize (GObject *object)
{
/*   GpaCMUnknown *card = GPA_CM_UNKNOWN (object); */

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_cm_unknown_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCMUnknownClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_cm_unknown_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaCMUnknown),
	  0,    /* n_preallocs */
	  gpa_cm_unknown_init
	};

      this_type = g_type_register_static (GPA_CM_OBJECT_TYPE,
                                          "GpaCMUnknown",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_cm_unknown_new ()
{
  return GTK_WIDGET (g_object_new (GPA_CM_UNKNOWN_TYPE, NULL));
}


/* If WIDGET is of Type GpaCMUnknown do a data reload through the
   assuan connection.  */
void
gpa_cm_unknown_reload (GtkWidget *widget, gpgme_ctx_t gpgagent)
{
  if (GPA_IS_CM_UNKNOWN (widget))
    {
      GPA_CM_OBJECT (widget)->agent_ctx = gpgagent;
      if (gpgagent)
        reload_data (GPA_CM_UNKNOWN (widget));
    }
}
