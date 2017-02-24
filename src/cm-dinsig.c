/* cm-dinsig.c  -  Widget to show information about a DINSIG card
 * Copyright (C) 2009 g10 Code GmbH
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

/* DINSIG is the old and still used standard for smartcards to create
   qualified signatures.  We provide this widget because modern German
   banking cards are prepared for that and due to the application
   precedence rules of Scdaemon this application will show up and not
   the Geldkarte which is also available on those cards.  */

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

#include "cm-object.h"
#include "cm-dinsig.h"




/* Identifiers for the entry fields.  */
enum
  {
    ENTRY_SERIALNO,

    ENTRY_LAST
  };



/* Object's class definition.  */
struct _GpaCMDinsigClass
{
  GpaCMObjectClass parent_class;
};


/* Object definition.  */
struct _GpaCMDinsig
{
  GpaCMObject  parent_instance;

  GtkWidget *warning_frame;  /* The frame used to display warnings etc.  */

  GtkWidget *entries[ENTRY_LAST];

  int  reloading;   /* Sentinel to avoid recursive reloads.  */
};

/* The parent class.  */
static GObjectClass *parent_class;



/* Local prototypes */
static void gpa_cm_dinsig_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

/* Clears the info contained in the card widget. */
static void
clear_card_data (GpaCMDinsig *card)
{
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    gtk_label_set_text (GTK_LABEL (card->entries[idx]), "");
}



struct scd_getattr_parm
{
  GpaCMDinsig *card;  /* The object.  */
  const char *name;   /* Name of expected attribute.  */
  int entry_id;       /* The identifier for the entry.  */
  void (*updfnc) (GpaCMDinsig *card, int entry_id, char *string);
};


static gpg_error_t
scd_getattr_cb (void *opaque, const char *status, const char *args)
{
  struct scd_getattr_parm *parm = opaque;
  int entry_id;

/*   g_debug ("STATUS_CB: status=`%s'  args=`%s'", status, args); */

  if (!strcmp (status, parm->name) )
    {
      entry_id = parm->entry_id;

      if (entry_id < ENTRY_LAST)
        {
          char *tmp = xstrdup (args);

          percent_unescape (tmp, 1);
          if (parm->updfnc)
            parm->updfnc (parm->card, entry_id, tmp);
          else if (GTK_IS_LABEL (parm->card->entries[entry_id]))
            gtk_label_set_text
              (GTK_LABEL (parm->card->entries[entry_id]), tmp);
          else
            gtk_entry_set_text
              (GTK_ENTRY (parm->card->entries[entry_id]), tmp);
          xfree (tmp);
        }
    }

  return 0;
}


/* Use the assuan machinery to load the bulk of the OpenPGP card data.  */
static void
reload_data (GpaCMDinsig *card)
{
  static struct {
    const char *name;
    int entry_id;
    void (*updfnc) (GpaCMDinsig *card, int entry_id, char *string);
  } attrtbl[] = {
    { "SERIALNO",    ENTRY_SERIALNO },
    { NULL }
  };
  int attridx;
  gpg_error_t err, operr;
  char command[100];
  struct scd_getattr_parm parm;
  gpgme_ctx_t gpgagent;

  gpgagent = GPA_CM_OBJECT (card)->agent_ctx;
  g_return_if_fail (gpgagent);

  card->reloading++;
  parm.card = card;
  for (attridx=0; attrtbl[attridx].name; attridx++)
    {
      parm.name     = attrtbl[attridx].name;
      parm.entry_id = attrtbl[attridx].entry_id;
      parm.updfnc   = attrtbl[attridx].updfnc;
      snprintf (command, sizeof command, "SCD GETATTR %s", parm.name);
      err = gpgme_op_assuan_transact_ext (gpgagent,
                                          command,
                                          NULL, NULL,
                                          NULL, NULL,
                                          scd_getattr_cb, &parm, &operr);
      if (!err)
        err = operr;

      if (err)
        {
          if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT)
            ; /* Lost the card.  */
          else
            {
              g_debug ("assuan command `%s' failed: %s <%s>\n",
                       command, gpg_strerror (err), gpg_strsource (err));
            }
          clear_card_data (card);
          break;
        }
    }
  card->reloading--;
}



/* Helper for construct_data_widget.  Returns the label widget. */
static GtkLabel *
add_table_row (GtkWidget *table, int *rowidx,
               const char *labelstr, GtkWidget *widget, GtkWidget *widget2,
               int readonly)
{
  GtkWidget *label;
  int is_label = GTK_IS_LABEL (widget);

  label = gtk_label_new (labelstr);
  gtk_label_set_width_chars  (GTK_LABEL (label), 22);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);

  if (is_label)
    gtk_misc_set_alignment (GTK_MISC (widget), 0, 0.5);

  if (readonly)
    {
      if (!is_label && GTK_IS_ENTRY (widget))
        {
          gtk_entry_set_has_frame (GTK_ENTRY (widget), FALSE);
          gtk_entry_set_editable (GTK_ENTRY (widget), FALSE);
        }
    }
  else
    {
      if (is_label)
        gtk_label_set_selectable (GTK_LABEL (widget), TRUE);
    }

  gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);
  if (widget2)
    gtk_table_attach (GTK_TABLE (table), widget2, 2, 3,
		      *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 5, 0);
  ++*rowidx;

  return GTK_LABEL (label);
}


/* This function constructs the container holding all widgets making
   up this data widget.  It is called during instance creation.  */
static void
construct_data_widget (GpaCMDinsig *card)
{
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *label;
  int rowidx;
  char *text;

  /* General frame.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>General</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  table = gtk_table_new (2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);
  rowidx = 0;

  card->entries[ENTRY_SERIALNO] = gtk_label_new (NULL);
  add_table_row (table, &rowidx, _("Serial number:"),
                 card->entries[ENTRY_SERIALNO], NULL, 0);

  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);

  /* Info frame.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  vbox = gtk_vbox_new (FALSE, 5);
  text = g_strdup_printf
    (_("There is not much information to display for a %s card.  "
       "You may want to use the application selector button to "
       "switch to another application available on this card."), "DINSIG");
  label = gtk_label_new (text);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_box_pack_start (GTK_BOX (card), frame, FALSE, TRUE, 0);

}



/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_cm_dinsig_class_init (void *class_ptr, void *class_data)
{
  GpaCMDinsigClass *klass = class_ptr;

  parent_class = g_type_class_peek_parent (klass);

  G_OBJECT_CLASS (klass)->finalize = gpa_cm_dinsig_finalize;
}


static void
gpa_cm_dinsig_init (GTypeInstance *instance, void *class_ptr)
{
  GpaCMDinsig *card = GPA_CM_DINSIG (instance);

  construct_data_widget (card);

}


static void
gpa_cm_dinsig_finalize (GObject *object)
{
/*   GpaCMDinsig *card = GPA_CM_DINSIG (object); */

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_cm_dinsig_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCMDinsigClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_cm_dinsig_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaCMDinsig),
	  0,    /* n_preallocs */
	  gpa_cm_dinsig_init
	};

      this_type = g_type_register_static (GPA_CM_OBJECT_TYPE,
                                          "GpaCMDinsig",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_cm_dinsig_new ()
{
  return GTK_WIDGET (g_object_new (GPA_CM_DINSIG_TYPE, NULL));
}


/* If WIDGET is of Type GpaCMDinsig do a data reload through the
   assuan connection.  */
void
gpa_cm_dinsig_reload (GtkWidget *widget, gpgme_ctx_t gpgagent)
{
  if (GPA_IS_CM_DINSIG (widget))
    {
      GPA_CM_OBJECT (widget)->agent_ctx = gpgagent;
      if (gpgagent)
        reload_data (GPA_CM_DINSIG (widget));
    }
}
