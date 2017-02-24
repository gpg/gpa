/* cm-geldkarte.c  -  Widget to show information about a Geldkarte.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gpa.h"
#include "convert.h"

#include "cm-object.h"
#include "cm-geldkarte.h"




/* Identifiers for the entry fields.  */
enum
  {
    ENTRY_KBLZ,
    ENTRY_BANKTYPE,
    ENTRY_CARDNO,
    ENTRY_EXPIRES,
    ENTRY_VALIDFROM,
    ENTRY_COUNTRY,
    ENTRY_CURRENCY,
    ENTRY_ZKACHIPID,
    ENTRY_OSVERSION,
    ENTRY_BALANCE,
    ENTRY_MAXAMOUNT,
    ENTRY_MAXAMOUNT1,

    ENTRY_LAST
  };



/* Object's class definition.  */
struct _GpaCMGeldkarteClass
{
  GpaCMObjectClass parent_class;
};


/* Object definition.  */
struct _GpaCMGeldkarte
{
  GpaCMObject  parent_instance;

  GtkWidget *amount_frame;
  GtkWidget *general_frame;

  GtkWidget *entries[ENTRY_LAST];
};

/* The parent class.  */
static GObjectClass *parent_class;



/* Local prototypes */
static void gpa_cm_geldkarte_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

/* Clears the info contained in the card widget. */
static void
clear_card_data (GpaCMGeldkarte *card)
{
  int idx;

  for (idx=0; idx < ENTRY_LAST; idx++)
    gtk_label_set_text (GTK_LABEL (card->entries[idx]), "");
}


struct scd_getattr_parm
{
  GpaCMGeldkarte *card;  /* The object.  */
  const char *name;      /* Name of expected attribute.  */
  int entry_id;          /* The identifier for the entry.  */
  void (*updfnc) (GpaCMGeldkarte *card, int entry_id, const char *string);
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
          if (parm->updfnc)
            parm->updfnc (parm->card, entry_id, args);
          else
            gtk_label_set_text
              (GTK_LABEL (parm->card->entries[entry_id]), args);
        }
    }

  return 0;
}


/* Use the assuan machinery to load the bulk of the OpenPGP card data.  */
static void
reload_data (GpaCMGeldkarte *card, gpgme_ctx_t gpgagent)
{
  static struct {
    const char *name;
    int entry_id;
    void (*updfnc) (GpaCMGeldkarte *card, int entry_id,  const char *string);
  } attrtbl[] = {
    { "X-KBLZ",      ENTRY_KBLZ },
    { "X-BANKINFO",  ENTRY_BANKTYPE },
    { "X-CARDNO",    ENTRY_CARDNO },
    { "X-EXPIRES",   ENTRY_EXPIRES },
    { "X-VALIDFROM", ENTRY_VALIDFROM },
    { "X-COUNTRY",   ENTRY_COUNTRY },
    { "X-CURRENCY",  ENTRY_CURRENCY },
    { "X-ZKACHIPID", ENTRY_ZKACHIPID },
    { "X-OSVERSION", ENTRY_OSVERSION },
    { "X-BALANCE",   ENTRY_BALANCE },
    { "X-MAXAMOUNT", ENTRY_MAXAMOUNT },
    { "X-MAXAMOUNT1",ENTRY_MAXAMOUNT1 },
    { NULL }
  };
  int attridx;
  gpg_error_t err, operr;
  char command[100];
  struct scd_getattr_parm parm;

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
}



/* Helper for construct_data_widget.  */
static GtkWidget *
add_table_row (GtkWidget *table, int *rowidx, const char *labelstr)
{
  GtkWidget *widget;
  GtkWidget *label;

  widget = gtk_label_new (NULL);

  label = gtk_label_new (labelstr);
  gtk_label_set_width_chars  (GTK_LABEL (label), 22);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);

  gtk_misc_set_alignment (GTK_MISC (widget), 0, 0.5);
  gtk_label_set_selectable (GTK_LABEL (widget), TRUE);

  gtk_table_attach (GTK_TABLE (table), widget, 1, 2,
                    *rowidx, *rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0);

  ++*rowidx;

  return widget;
}


/* This function constructs the container holding all widgets making
   up this data widget.  It is called during instance creation.  */
static void
construct_data_widget (GpaCMGeldkarte *card)
{
  GtkWidget *amount_frame;
  GtkWidget *amount_table;
  GtkWidget *general_frame;
  GtkWidget *general_table;
  GtkWidget *label;
  int rowidx;

  /* Create frames and tables. */

  general_frame = card->general_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (general_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>General</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (general_frame), label);

  general_table = gtk_table_new (9, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (general_table), 10);

  amount_frame = card->amount_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (amount_frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>Amount</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (amount_frame), label);

  amount_table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (amount_table), 10);


  /* General frame.  */
  rowidx = 0;

  card->entries[ENTRY_CARDNO] = add_table_row
    (general_table, &rowidx, _("Card number: "));

  card->entries[ENTRY_KBLZ] = add_table_row
    (general_table, &rowidx, _("Short Bank Code number: "));

  card->entries[ENTRY_BANKTYPE] = add_table_row
    (general_table, &rowidx, _("Bank type: "));

  card->entries[ENTRY_VALIDFROM] = add_table_row
    (general_table, &rowidx, _("Card valid from: "));

  card->entries[ENTRY_EXPIRES] = add_table_row
    (general_table, &rowidx, _("Card expires: "));

  card->entries[ENTRY_COUNTRY] = add_table_row
    (general_table, &rowidx, _("Issuing country: "));

  card->entries[ENTRY_CURRENCY] = add_table_row
    (general_table, &rowidx, _("Currency: "));

  card->entries[ENTRY_ZKACHIPID] = add_table_row
    (general_table, &rowidx, _("ZKA chip Id: "));

  card->entries[ENTRY_OSVERSION] = add_table_row
    (general_table, &rowidx, _("Chip OS version: "));

  gtk_container_add (GTK_CONTAINER (general_frame), general_table);


  /* Amount frame.  */
  rowidx = 0;

  card->entries[ENTRY_BALANCE] = add_table_row
    (amount_table, &rowidx, _("Balance: "));

  card->entries[ENTRY_MAXAMOUNT] = add_table_row
    (amount_table, &rowidx, _("General limit: "));

  card->entries[ENTRY_MAXAMOUNT1] = add_table_row
    (amount_table, &rowidx, _("Transaction limit: "));

  gtk_container_add (GTK_CONTAINER (amount_frame), amount_table);


  /* Put all frames together.  */
  gtk_box_pack_start (GTK_BOX (card), amount_frame, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (card), general_frame, FALSE, TRUE, 0);
}



/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_cm_geldkarte_class_init (void *class_ptr, void *class_data)
{
  GpaCMGeldkarteClass *klass = class_ptr;

  parent_class = g_type_class_peek_parent (klass);

  G_OBJECT_CLASS (klass)->finalize = gpa_cm_geldkarte_finalize;
}


static void
gpa_cm_geldkarte_init (GTypeInstance *instance, void *class_ptr)
{
  GpaCMGeldkarte *card = GPA_CM_GELDKARTE (instance);

  construct_data_widget (card);

}


static void
gpa_cm_geldkarte_finalize (GObject *object)
{
/*   GpaCMGeldkarte *card = GPA_CM_GELDKARTE (object); */

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_cm_geldkarte_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCMGeldkarteClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_cm_geldkarte_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaCMGeldkarte),
	  0,    /* n_preallocs */
	  gpa_cm_geldkarte_init
	};

      this_type = g_type_register_static (GPA_CM_OBJECT_TYPE,
                                          "GpaCMGeldkarte",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_cm_geldkarte_new ()
{
  return GTK_WIDGET (g_object_new (GPA_CM_GELDKARTE_TYPE, NULL));
}


/* If WIDGET is of Type GpaCMGeldkarte do a data reload through the
   assuan connection.  */
void
gpa_cm_geldkarte_reload (GtkWidget *widget, gpgme_ctx_t gpgagent)
{
  if (GPA_IS_CM_GELDKARTE (widget))
    reload_data (GPA_CM_GELDKARTE (widget), gpgagent);
}
