/* options.h - global option declarations.
   Copyright (C) 2002 Miguel Coca.
   Copyright (C) 2005, 2008 g10 Code GmbH.

   This file is part of GPA

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>. */

#include <config.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "options.h"
#include "gpa.h"
#include "gtktools.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "confdialog.h" /* gpa_read_configured_keyserver */

/* Internal API */
static void gpa_options_save_settings (GpaOptions *options);
static void gpa_options_read_settings (GpaOptions *options);

/* GObject type functions */

static void gpa_options_init (GpaOptions *options);
static void gpa_options_class_init (GpaOptionsClass *klass);
static void gpa_options_finalize (GObject *object);

enum
{
  CHANGED_UI_MODE,
  CHANGED_SHOW_ADVANCED_OPTIONS,
  CHANGED_DEFAULT_KEY,
  CHANGED_DEFAULT_KEYSERVER,
  CHANGED_BACKUP_GENERATED,
  CHANGED_VIEW,
  LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint signals [LAST_SIGNAL] = { 0 };

GType
gpa_options_get_type (void)
{
  static GType style_type = 0;

  if (!style_type)
    {
      static const GTypeInfo style_info =
      {
        sizeof (GpaOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaOptions),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_options_init,
      };

      style_type = g_type_register_static (G_TYPE_OBJECT,
					   "GpaOptions",
					   &style_info, 0);
    }

  return style_type;
}


static void
make_signal (int signo, GObjectClass *object_class,
             const char *signame, size_t func_off)
{
  g_assert (signo >= 0 && signo < LAST_SIGNAL);
  signals[signo] = g_signal_new (signame,
                                 G_TYPE_FROM_CLASS (object_class),
                                 G_SIGNAL_RUN_FIRST,
                                 func_off,
                                 NULL, NULL,
                                 g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);
}


static void
gpa_options_class_init (GpaOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_options_finalize;

  klass->changed_ui_mode = gpa_options_save_settings;
  klass->changed_show_advanced_options = gpa_options_save_settings;
  klass->changed_default_key = gpa_options_save_settings;
  klass->changed_default_keyserver = gpa_options_save_settings;
  klass->changed_backup_generated = gpa_options_save_settings;
  klass->changed_view = gpa_options_save_settings;

  /* Signals */
  make_signal (CHANGED_UI_MODE, object_class,
               "changed_ui_mode",
               G_STRUCT_OFFSET (GpaOptionsClass, changed_ui_mode));
  make_signal (CHANGED_SHOW_ADVANCED_OPTIONS, object_class,
               "changed_show_advanced_options",
               G_STRUCT_OFFSET (GpaOptionsClass,
                                changed_show_advanced_options));
  make_signal (CHANGED_DEFAULT_KEY, object_class,
                "changed_default_key",
                G_STRUCT_OFFSET (GpaOptionsClass, changed_default_key));
  make_signal (CHANGED_DEFAULT_KEYSERVER, object_class,
               "changed_default_keyserver",
               G_STRUCT_OFFSET (GpaOptionsClass, changed_default_keyserver));
  make_signal (CHANGED_VIEW, object_class,
               "changed_view",
               G_STRUCT_OFFSET (GpaOptionsClass, changed_view));
  make_signal (CHANGED_BACKUP_GENERATED, object_class,
               "changed_backup_generated",
               G_STRUCT_OFFSET (GpaOptionsClass, changed_backup_generated));
}

static void
gpa_options_init (GpaOptions *options)
{
  options->options_file = NULL;
  options->simplified_ui = TRUE;
  options->show_advanced_options = FALSE;
  options->backup_generated = FALSE;
  options->default_key = NULL;
  options->default_key_fpr = NULL;
  options->default_keyserver = NULL;
  options->detailed_view = FALSE;
}

static void
gpa_options_finalize (GObject *object)
{
  GpaOptions *options = GPA_OPTIONS (object);

  g_free (options->options_file);
  gpgme_key_unref (options->default_key);
  g_free (options->default_key_fpr);
  g_free (options->default_keyserver);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* API */

/* The object */
static GpaOptions *instance = NULL;

/* Create a new GpaOptions object, reading the options, reading the options
 * from the file given */
static GpaOptions *
gpa_options_new (void)
{
  GpaOptions *options;

  options = g_object_new (GPA_OPTIONS_TYPE, NULL);

  return options;
}

GpaOptions *
gpa_options_get_instance (void)
{
  if (!instance)
    {
      instance = gpa_options_new ();
    }
  return instance;
}


/* Choose the file the options should be read from and written to.
 */
void
gpa_options_set_file (GpaOptions *options, const gchar *filename)
{
  if (options->options_file)
    {
      g_free (options->options_file);
    }
  options->options_file = g_strdup (filename);
  gpa_options_read_settings (options);
}

const gchar *
gpa_options_get_file (GpaOptions *options)
{
  return options->options_file;
}

/* Set whether the ui should be in simplified mode */
void
gpa_options_set_simplified_ui (GpaOptions *options, gboolean value)
{
  int change = (!options->simplified_ui != !value);
  options->simplified_ui = value;
  if (change)
    g_signal_emit (options, signals[CHANGED_UI_MODE], 0);
}

gboolean
gpa_options_get_simplified_ui (GpaOptions *options)
{
  return options->simplified_ui;
}

/* Set whether the preference dialog shows the advanced options. */
void
gpa_options_set_show_advanced_options (GpaOptions *options, gboolean value)
{
  int change = (!options->show_advanced_options != !value);
  options->show_advanced_options = value;
  if (change)
    g_signal_emit (options, signals[CHANGED_SHOW_ADVANCED_OPTIONS], 0);
}

gboolean
gpa_options_get_show_advanced_options (GpaOptions *options)
{
  return options->show_advanced_options;
}

/* Choose the default key */
void
gpa_options_set_default_key (GpaOptions *options, gpgme_key_t key)
{
  if (options->default_key)
    {
      gpgme_key_unref (options->default_key);
      options->default_key = NULL;
    }
  if (key)
    {
      gpgme_key_ref (key);
      options->default_key = key;
      options->default_key_fpr = g_strdup (key->subkeys->fpr);
    }
  g_signal_emit (options, signals[CHANGED_DEFAULT_KEY], 0);
}


gpgme_key_t
gpa_options_get_default_key (GpaOptions *options)
{
  return options->default_key;
}


/* Return whether a default key is somehow known.  This is either the
   default key's fingerprint from gpa.conf or the default key from the
   options dialog.  */
gboolean
gpa_options_have_default_key (GpaOptions *options)
{
  return ((options->default_key_fpr && *options->default_key_fpr)
          || options->default_key);
}


/* Return the default key gpg would use, or at least a first
 * approximation. Currently this means the first secret key in the keyring.
 * If there's no secret key at all, return NULL
 */
static gpgme_key_t
determine_default_key (void)
{
  gpgme_key_t key = NULL;
  gpg_error_t err;
  gpgme_ctx_t ctx;

  ctx = gpa_gpgme_new ();
  if (!ctx)
    return NULL;

  err = gpgme_op_keylist_start (ctx, NULL, 1);
  if (err)
    {
      gpa_gpgme_error (err); 	/* Or should we use
				   gpa_gpgme_warning()?  -mo*/
    }
  else
    {
      err = gpgme_op_keylist_next (ctx, &key);
      if (!err)
	{
	  /* Got a new key. */
	  err = gpgme_op_keylist_end (ctx);
	  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
	    {
	      gpa_gpgme_warning (err);
	    }
	}
      else if (gpg_err_code (err) != GPG_ERR_EOF)
	{
	  gpa_gpgme_error (err); 	/* Or should we use
					   gpa_gpgme_warning()?  -mo*/
	}
    }

  gpgme_release (ctx);
  return key;
}

void
gpa_options_update_default_key (GpaOptions *options)
{
  gboolean update = FALSE;
  gpgme_key_t key = NULL;
  gpgme_ctx_t ctx = gpa_gpgme_new ();

  if (! options->default_key_fpr)
    update = TRUE;
  else if (gpg_err_code (gpgme_get_key (ctx, options->default_key_fpr,
					&key, TRUE)) == GPG_ERR_EOF)
    {
      gpa_window_error (_("The private key you selected as default is no "
                          "longer available.\n"
                          "GPA will try to choose a new default "
                          "key automatically."), NULL);
      update = TRUE;
    }
  else
    {
      /* Use the listed key if there is no previous one (at initialization).
       */
      if (!options->default_key)
	options->default_key = key;
      else
	gpgme_key_unref (key);
    }

  if (update)
    {
      key = determine_default_key ();
      gpa_options_set_default_key (options, key);
      if (key)
	gpgme_key_unref (key);
    }

  gpgme_release (ctx);
}

/* Specify the default keyserver */
void
gpa_options_set_default_keyserver (GpaOptions *options, const gchar *keyserver)
{
  if (is_gpg_version_at_least ("2.1.0"))
    return;

  if (options->default_keyserver)
    {
      g_free (options->default_keyserver);
    }
  options->default_keyserver = g_strdup (keyserver);
  g_signal_emit (options, signals[CHANGED_DEFAULT_KEYSERVER], 0);
}

const gchar *
gpa_options_get_default_keyserver (GpaOptions *options)
{
  return options->default_keyserver;
}

/* Remember whether using brief or detailed view */
void
gpa_options_set_detailed_view (GpaOptions *options, gboolean value)
{
  options->detailed_view = value;
  g_signal_emit (options, signals[CHANGED_VIEW], 0);
}

gboolean
gpa_options_get_detailed_view (GpaOptions *options)
{
  return options->detailed_view;
}


/* Remember whether the default key has already been backed up */
void
gpa_options_set_backup_generated (GpaOptions *options, gboolean value)
{
  options->backup_generated = value;
  g_signal_emit (options, signals[CHANGED_BACKUP_GENERATED], 0);
}

gboolean
gpa_options_get_backup_generated (GpaOptions *options)
{
  return options->backup_generated;
}

static void
gpa_options_save_settings (GpaOptions *options)
{
  FILE *options_file;

  g_assert(options->options_file != NULL);
  options_file = g_fopen (options->options_file, "w");
  if (!options_file)
    {
      g_warning("%s: %s", options->options_file, strerror (errno));
    }
  else
    {
      if (options->default_key)
        {
          fprintf (options_file, "default-key %s\n",
		   options->default_key->subkeys->fpr);
        }
      if (options->default_keyserver)
        {
          /* we do not write the keyserver anymore to gpa.conf.  */
/*           fprintf (options_file, "keyserver %s\n",  */
/*                    options->default_keyserver); */
        }
      if (options->backup_generated)
        {
          fprintf (options_file, "%s\n", "backup-generated");
        }
      if (!options->simplified_ui)
        {
          fprintf (options_file, "%s\n", "advanced-ui");
        }
      if (options->show_advanced_options)
        {
          fprintf (options_file, "%s\n", "show-advanced-options");
        }
      if (options->detailed_view)
        {
          fprintf (options_file, "%s\n", "detailed-view");
        }
      fclose (options_file);
    }

  /* Write the keyserver to the backend.  */
  if (options->default_keyserver && !is_gpg_version_at_least ("2.1.0"))
    gpa_store_configured_keyserver (options->default_keyserver);
}


static gboolean
read_next_word (FILE *file, char *buffer, int size)
{
  int i = 0;
  int c;

  buffer[0] = '\0';
  /* Skip leading whitespace */
  while ((c = getc (file)) != EOF && isspace (c))
    ;
  if (c == EOF)
    {
      return FALSE;
    }
  else
    {
      buffer[i++] = c;
      /* Read the word */
      while ((c = getc (file)) != EOF && !isspace (c) && i < (size-1))
        {
          buffer[i++] = c;
        }
      buffer[i] = '\0';

      return TRUE;
    }
}

typedef enum
 {
   PARSE_OPTIONS_STATE_START,
   PARSE_OPTIONS_STATE_HAVE_KEY,
   PARSE_OPTIONS_STATE_HAVE_KEYSERVER,
 } ParseOptionsState;

/* This MUST be called ONLY from gpa_options_new (). We don't emit any
 * signals when changing options here (it would overwrite the options file) */
static void
gpa_options_read_settings (GpaOptions *options)
{
  FILE *options_file;

  g_assert(options->options_file != NULL);
  options_file = g_fopen (options->options_file, "r");
  if (!options_file)
    {
      /* If the config file just doesn't exist, it's not an error */
      if (errno != ENOENT)
        {
          g_warning("%s: %s", options->options_file, strerror (errno));
        }
    }
  else
    {
      /* Parse the file */
      gchar next_word[100];
      ParseOptionsState state = PARSE_OPTIONS_STATE_START;

      /* There is no error checking here intentionally. This way we won't
       * complain too much about mixing GPA versions */
      while (read_next_word (options_file, next_word, sizeof (next_word)))
        {
          switch (state)
            {
            case PARSE_OPTIONS_STATE_START:
              if (g_str_equal (next_word, "default-key"))
                {
                  state = PARSE_OPTIONS_STATE_HAVE_KEY;
                }
              else if (g_str_equal (next_word, "keyserver"))
                {
                  state = PARSE_OPTIONS_STATE_HAVE_KEYSERVER;
                }
              else if (g_str_equal (next_word, "backup-generated"))
                {
                  options->backup_generated = TRUE;
                }
              else if (g_str_equal (next_word, "advanced-ui"))
                {
                  options->simplified_ui = FALSE;
                }
              else if (g_str_equal (next_word, "show-advanced-options"))
                {
                  options->show_advanced_options = TRUE;
                }
              else if (g_str_equal (next_word, "detailed-view"))
                {
                  options->detailed_view = TRUE;
                }
              break;
            case PARSE_OPTIONS_STATE_HAVE_KEY:
              options->default_key_fpr = g_strdup (next_word);
              state = PARSE_OPTIONS_STATE_START;
              break;
            case PARSE_OPTIONS_STATE_HAVE_KEYSERVER:
              /* We do not use the keyserver item from the gpa.conf anymore. */
              /* options->default_keyserver = g_strdup (next_word); */
              state = PARSE_OPTIONS_STATE_START;
              break;
            default:
              /* Can't happen */
              return;
            }
        }
      fclose (options_file);
    }

  /* Read the keyserver from the abckend.  */
  g_free (options->default_keyserver);
  options->default_keyserver = NULL;
  if (!is_gpg_version_at_least ("2.1.0"))
    options->default_keyserver = gpa_load_configured_keyserver ();
}
