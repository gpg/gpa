/* gpacardreloadop.c - The GpaCardReloadOperation object.
 *	Copyright (C) 2008 g10 Code GmbH.
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
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

#include <gtk/gtk.h>
#include <gpgme.h>

#include "gpa.h"
#include "gtktools.h"
#include "gpaoperation.h"
#include "gpacardreloadop.h"


struct _GpaCardReloadOperation 
{
  GpaOperation parent;

  gpa_card_reload_cb_t card_reload_cb;
  void *card_reload_cb_opaque;
  gpgme_data_t gpgme_output;
};

struct _GpaCardReloadOperationClass 
{
  GpaOperationClass parent_class;
};


static GObjectClass *parent_class = NULL;

static void gpa_card_reload_operation_done_cb (GpaContext *context, 
					       gpg_error_t err,
					       GpaCardReloadOperation *op);

static void gpa_card_reload_operation_done_error_cb (GpaContext *context, 
						     gpg_error_t err,
						     GpaCardReloadOperation *op);

/* We do all the work in the idle loop.  */
static gboolean gpa_card_reload_operation_idle_cb (gpointer data);

/* GObject boilerplate */

/* Properties of our object.  */
enum
{
  PROP_0,
  PROP_CARD_RELOAD_CB,
  PROP_CARD_RELOAD_CB_OPAQUE
};

static void
gpa_card_reload_operation_get_property (GObject     *object,
					guint        prop_id,
					GValue      *value,
					GParamSpec  *pspec)
{
  GpaCardReloadOperation *op = GPA_CARD_RELOAD_OPERATION (object);

  switch (prop_id)
    {
    case PROP_CARD_RELOAD_CB:
      g_value_set_pointer (value, op->card_reload_cb);
      break;
    case PROP_CARD_RELOAD_CB_OPAQUE:
      g_value_set_pointer (value, op->card_reload_cb_opaque);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_card_reload_operation_set_property (GObject      *object,
					guint         prop_id,
					const GValue *value,
					GParamSpec   *pspec)
{
  GpaCardReloadOperation *op = GPA_CARD_RELOAD_OPERATION (object);
  gpa_card_reload_cb_t cb;

  switch (prop_id)
    {
    case PROP_CARD_RELOAD_CB:
      cb = (gpa_card_reload_cb_t) g_value_get_pointer (value);
      op->card_reload_cb = cb;
      break;
    case PROP_CARD_RELOAD_CB_OPAQUE:
      op->card_reload_cb_opaque = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_card_reload_operation_init (GpaCardReloadOperation *op)
{
  gpgme_data_t gpgme_output;
  gpg_error_t err;

  /* Create a new GPGME data handle into which the GPGME output during
     the card-list operation is written.  */
  gpgme_output = NULL;
  err = gpgme_data_new (&gpgme_output);
  if (err)
    gpa_gpgme_warning (err);

  op->card_reload_cb = NULL;
  op->card_reload_cb_opaque = NULL;
  op->gpgme_output = gpgme_output;
}

static void
gpa_card_reload_operation_finalize (GObject *object)
{
  /* GpaCardReloadOperation *op = GPA_CARD_RELOAD_OPERATION (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject*
gpa_card_reload_operation_constructor (GType type,
				       guint n_construct_properties,
				       GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaCardReloadOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_CARD_RELOAD_OPERATION (object);

  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_card_reload_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_card_reload_operation_done_cb), op);

  /* Begin working when we are back into the main loop */
  g_idle_add (gpa_card_reload_operation_idle_cb, op);

  return object;
}

static void
gpa_card_reload_operation_class_init (GpaCardReloadOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_card_reload_operation_constructor;
  object_class->finalize = gpa_card_reload_operation_finalize;
  object_class->set_property = gpa_card_reload_operation_set_property;
  object_class->get_property = gpa_card_reload_operation_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_CARD_RELOAD_CB,
				   g_param_spec_pointer 
				   ("card_reload_cb", "card_reload_cb",
				    "card_reload_cb",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_CARD_RELOAD_CB_OPAQUE,
				   g_param_spec_pointer 
				   ("card_reload_cb_opaque", "card_reload_cb_opaque",
				    "card_reload_cb_opaque",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_card_reload_operation_get_type (void)
{
  static GType operation_type = 0;
  
  if (!operation_type)
    {
      static const GTypeInfo operation_info =
      {
        sizeof (GpaCardReloadOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_card_reload_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaCardReloadOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_card_reload_operation_init,
      };
      
      operation_type = g_type_register_static (GPA_OPERATION_TYPE,
					       "GpaCardReloadOperation",
					       &operation_info, 0);
    }
  
  return operation_type;
}

/* API */

GpaCardReloadOperation *
gpa_card_reload_operation_new (GtkWidget *window, gpa_card_reload_cb_t cb, void *opaque)
{
  GpaCardReloadOperation *op;
  
  op = g_object_new (GPA_CARD_RELOAD_OPERATION_TYPE,
		     "window", window,
		     "card_reload_cb", cb,
		     "card_reload_cb_opaque", opaque,
		     NULL);

  return op;
}


/* Internal */

/* Called during idle loop; triggers the actual GPGME card-list
   operation.  */
static gboolean
gpa_card_reload_operation_idle_cb (gpointer data)
{
  GpaCardReloadOperation *op = data;
  gpg_error_t err;

  err = gpa_gpgme_card_edit_list_start (GPA_OPERATION(op)->context,
					op->gpgme_output);
  if (err)
    g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
  
  return FALSE;
}

/* Processes the line LINE, calling the callback CB with it's opaque
   argument for each data item. Modifies LINE. */
static void
process_line (char *line, gpa_card_reload_cb_t cb, void *opaque)
{
  char *field[5];
  int fields = 0;
  int idx;

  /* Note that !line indicates EOF but we have no use for it here.  */

  while (line && fields < DIM (field))
    {
      field[fields++] = line;
      line = strchr (line, ':');
      if (line)
	*line++ = 0;
    }

  for (idx=1; idx < fields; idx++)
    (*cb) (opaque, field[0], idx-1, field[idx]);
}


/* Processes the GPGME output contained in OUT, calling the callback
   CB with it's opaque argument OPAQUE for each data item. */
static void
process_gpgme_output (gpgme_data_t out, gpa_card_reload_cb_t cb, void *opaque)
{
  char *data;
  size_t data_length;
  FILE *stream;
  ssize_t ret;
  char *line;
  size_t line_length;

  /* FIXME: this function is NOT portable!!!! -mo */

  data = gpgme_data_release_and_get_mem (out, &data_length);
  stream = fmemopen (data, data_length, "r");
  if (!stream)
    {
      fprintf (stderr, "Ooops! Fatal error durign fmemopen (%s) occured. How to handle??\n",
	       strerror (errno));
      exit (1);
    }

  while (1)
    {
      line = NULL;
      line_length = 0;

      ret = getline (&line, &line_length, stream);
      if (ret == -1)
	{
	  if (ferror (stream))
	    {
	      fprintf (stderr, "Ooops! Fatal error during getline (%s) occured. How to handle??\n",
		       strerror (errno));
	      exit (1);
	    }
	  else
	    /* This must be EOF, no? */
	    break;
	}

      process_line (line, cb, opaque);
      free (line);
    }

  fclose (stream);
  gpgme_free (data);
}

static void
gpa_card_reload_operation_done_cb (GpaContext *context, 
				   gpg_error_t err,
				   GpaCardReloadOperation *op)
{
  if (!err)
    {
      /* Note: this releases op->gpgme_output! */
      process_gpgme_output (op->gpgme_output,
			    op->card_reload_cb,
			    op->card_reload_cb_opaque);
    }
  g_signal_emit_by_name (op, "completed", err);
}


static void
gpa_card_reload_operation_done_error_cb (GpaContext *context, 
					 gpg_error_t err,
					 GpaCardReloadOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
    default:
      gpa_gpgme_warning (err);
      break;
    }
}
