/* confdialog.c - GPGME based configuration dialog for GPA.
   Copyright (C) 2007, 2008 g10 Code GmbH

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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <gpgme.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "i18n.h"
#include "gpgmetools.h"
#include "gtktools.h"
#include "options.h"
#include "gpa.h"

/* Violation of GNOME standards: Cancel does not revert previous
   apply.  We do not auto-apply or syntax check after focus
   change.  */


/* Internal public interface.  */
static void hide_backend_config (void);


/* Some global variables.  */

/* Custom response IDs.  */
#define CUSTOM_RESPONSE_RESET 1


/* The configuration dialog.  */
static GtkWidget *dialog;

/* The notebook with one page for each component.  */
static gpgme_ctx_t dialog_ctx;

/* The notebook with one page for each component.  */
static GtkWidget *dialog_notebook;

/* The current configuration.  */
static gpgme_conf_comp_t dialog_conf;

/* If we modified something in the current tab.  */
static int dialog_tab_modified;

/* The expert level.  */
static gpgme_conf_level_t dialog_level;


/* We define the following behaviour for options:

   An option of alt-type NONE gets a check button, and if it has the
   LIST flag set, also a spin button for a repeat count.  The spin
   button is only active if the button is checked.

   An option of a different alt-type does not get a check box button
   but a combo box and a text entry field.  If the option does have a
   default or default description, the combo box contains an entry for
   "Use default value", which sets the text entry field to that
   default (and makes it insensitive?).  Otherwise the combo box
   contains an entry for "Option not active" which makes the entry
   field insensitive.  If the option has the OPTIONAl flag set, the
   combo box contains an entry for "Use default argument" which sets
   the text entry field to the no arg value or description, if any
   (and makes it insensitive?).  In any case, the combo box contains
   an entry for "Use custom value" which makes the text entry field
   sensitive and editable.

   Options with the LIST flag set could get a more sophisticated
   dialog where you have "Add" and "Remove" buttons, or a multi-line
   entry field.  Currently, having the LIST flag and OPTIONAL flag at
   the same time creates an ambiguity, and entering commas in values
   is not supported.  */
typedef enum
  {
    OPTION_SIMPLE,
    OPTION_SPIN,
    OPTION_ENTRY,
    OPTION_OPT_ENTRY
  } option_type_t;

typedef enum
  {
    COMBO_UNDEF = -1,
    COMBO_DEFAULT = 0,
    COMBO_CUSTOM = 1,
    COMBO_NO_ARG = 2
  } option_combo_t;


/* This structure combines an option with its GUI elements.  */
typedef struct option_widget_s
{
  /* The option.  */
  gpgme_conf_opt_t option;

  /* We remember the type of the option to make life easier on us.  */
  option_type_t type;

  /* The check button (or, in the case of COMBO being not NULL, the
     label) widget.  */
  GtkWidget *check;

  /* The entry or spin button widget.  */
  GtkWidget *widget;

  /* The current user setting in string representation.  This defaults
     to the last input by the user, followed by the current setting,
     followed by the default.  */
  char *saved_value;

  /* This helps to know about state changes.  */
  int old_combo_box_state;

  /* The combo box, if any.  Only used for OPTION_ENTRY and
     OPTION_OPT_ENTRY.  The combo has
     COMBO_DEFAULT = 0: Use default value or Do not use option.
     COMBO_CUSTOM = 1: Use custom value
     COMBO_NO_ARG = 2: Use default argument (for OPTION_OPT_ENTRY).  */
  GtkWidget *combo;
} *option_widget_t;


#if 0
/* Not needed anymore, as all stock actions work on all tabs.
   However, might come in handy at some point.  */

static gpgme_conf_comp_t
dialog_current_comp (void)
{
  gpgme_conf_comp_t comp = dialog_conf;
  int page = gtk_notebook_get_current_page (GTK_NOTEBOOK (dialog_notebook));
  const char *label;

  /* Iterate over all options in the current tab, and reset them.  */

  label = gtk_notebook_get_tab_label_text
    (GTK_NOTEBOOK (dialog_notebook),
     gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog_notebook), page));

  while (comp)
    {
      if (! strcmp (label, comp->description))
	break;
      comp = comp->next;
    }

  assert (comp);
  return comp;
}
#endif


/* Convert an argument value to a string representation.  Lists are
   converted to comma-separated values, empty strings in lists are
   surrounded by double-quotes.  */
static char *
arg_to_str (gpgme_conf_arg_t arg, gpgme_conf_type_t type)
{
  static char *result;
  char *new_result = NULL;

  if (result)
    {
      g_free (result);
      result = NULL;
    }

  while (arg)
    {
      if (result)
	{
	  new_result = g_strdup_printf ("%s,", result);
	  g_free (result);
	  result = new_result;
	}
      else
	result = g_strdup ("");

      if (!arg->no_arg)
        {
          switch (type)
            {
            case GPGME_CONF_NONE:
            case GPGME_CONF_UINT32:
              new_result = g_strdup_printf ("%s%u", result, arg->value.uint32);
              g_free (result);
              result = new_result;
              break;

            case GPGME_CONF_INT32:
              new_result = g_strdup_printf ("%s%i", result, arg->value.int32);
              g_free (result);
              result = new_result;
              break;

            case GPGME_CONF_STRING:
            case GPGME_CONF_PATHNAME:
            case GPGME_CONF_LDAP_SERVER:
              new_result = g_strdup_printf ("%s%s", result, arg->value.string);
              g_free (result);
              result = new_result;
              break;

            default:
              assert (!"Not supported.");
              break;
            }
        }
      arg = arg->next;
    }
  return result;
}


/* Extract the argument from a widget.  */
static gpgme_conf_arg_t
option_widget_to_arg (option_widget_t opt)
{
  gpgme_error_t err;
  gpgme_conf_opt_t option = opt->option;
  gpgme_conf_arg_t arg = NULL;

  if (opt->type == OPTION_SIMPLE || opt->type == OPTION_SPIN)
    {
      int active;
      unsigned int count = 1;

      active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (opt->check));
      if (!active)
	return NULL;

      if (opt->type == OPTION_SPIN)
	count = gtk_spin_button_get_value (GTK_SPIN_BUTTON (opt->widget));

      err = gpgme_conf_arg_new (&arg, option->alt_type, &count);
      if (err)
	gpa_gpgme_error (err);

      return arg;
    }
  else
    {
      int combo;
      gpgme_conf_arg_t *argp;
      char *val;
      char *valp;

      combo = gtk_combo_box_get_active (GTK_COMBO_BOX (opt->combo));
      if (combo == COMBO_DEFAULT)
	return NULL;

      if (combo == COMBO_NO_ARG)
	{
	  /* A single no-arg option.  */
	  err = gpgme_conf_arg_new (&arg, option->alt_type, NULL);
	  if (err)
	    gpa_gpgme_error (err);

	  return arg;
	}

      argp = &arg;

      val = g_strdup (gtk_entry_get_text (GTK_ENTRY (opt->widget)));
      valp = val;
      do
	{
	  while (*valp == ' ' || *valp == '\t')
	    valp++;

	  if (*valp == ',' || *valp == '\0')
	    {
	      if (option->flags & GPGME_CONF_OPTIONAL)
		err = gpgme_conf_arg_new (argp, option->alt_type, NULL);
	      else
		/* This seems a useful default to make things simpler
		   for the user, as the OPTIONAL flag is not much
		   used.  */
		err = gpgme_conf_arg_new (argp, option->alt_type, "");

	      if (err)
		gpa_gpgme_error (err);

	      argp = &(*argp)->next;

	      /* Skip comma.  */
	      if (*valp)
		valp++;
	    }
	  else
	    {
	      char *str = valp;
	      char *strend = valp;
	      int done = 0;
	      int in_quote = 0;

	      /* Remove quoting.  */
	      do
		{
		  if (in_quote)
		    {
		      if (*valp == '\0')
			done = 1;
		      else if (*valp == '"')
			{
			  in_quote = 0;
			  valp++;
			}
		      else
			*(strend++) = *(valp++);
		    }
		  else
		    {
		      if (*valp == ',' || *valp == '\0')
			done = 1;
		      else if (*valp == '"')
			{
			  in_quote = 1;
			  valp++;
			}
		      else
			*(strend++) = *(valp++);
		    }
		}
	      while (! done);

	      if (*valp == ',')
		valp++;

	      /* Find end of the string.  */
	      while (strend > str && (*strend == ' ' || *strend == '\t'))
		strend--;

	      *strend = '\0';

	      switch (option->alt_type)
		{
		case GPGME_CONF_NONE:
		case GPGME_CONF_UINT32:
		  {
		    unsigned int nr;
		    nr = strtoul (str, NULL, 0);
		    err = gpgme_conf_arg_new (argp, option->alt_type, &nr);
		    if (err)
		      gpa_gpgme_error (err);

		    argp = &(*argp)->next;
		  }
		  break;

		case GPGME_CONF_INT32:
		  {
		    int nr;
		    nr = strtoul (str, NULL, 0);
		    err = gpgme_conf_arg_new (argp, option->alt_type, &nr);
		    if (err)
		      gpa_gpgme_error (err);

		    argp = &(*argp)->next;
		  }
		  break;

		case GPGME_CONF_STRING:
		case GPGME_CONF_LDAP_SERVER:
		case GPGME_CONF_PATHNAME:
		  {
		    err = gpgme_conf_arg_new (argp, option->alt_type, str);
		    if (err)
		      gpa_gpgme_error (err);

		    argp = &(*argp)->next;
		  }
		  break;

		default:
		  assert (!"Not supported.");
		  break;
		}
	    }
	}
      while (*valp);

      g_free (val);
      return arg;
    }
}


/* Compare two arguments and returns true if they are equal.  */
static int
args_are_equal (gpgme_conf_arg_t arg1, gpgme_conf_arg_t arg2,
		gpgme_conf_type_t type)
{
  while (arg1 && arg2)
    {
      if ((!arg1->no_arg ^ !arg2->no_arg))
        return 0;
      if (!arg1->no_arg)
        {
          switch (type)
            {
            case GPGME_CONF_NONE:
            case GPGME_CONF_UINT32:
              if (arg1->value.uint32 != arg2->value.uint32)
                return 0;
              break;

            case GPGME_CONF_INT32:
              if (arg1->value.int32 != arg2->value.int32)
                return 0;
              break;

            case GPGME_CONF_STRING:
            case GPGME_CONF_LDAP_SERVER:
            case GPGME_CONF_PATHNAME:
              if (strcmp (arg1->value.string, arg2->value.string))
                return 0;
              break;

            default:
              assert (!"Not supported.");
              break;
            }
        }

      arg1 = arg1->next;
      arg2 = arg2->next;
    }
  if (arg1 || arg2)
    return 0;
  return 1;
}


/* Commit all the changes in component COMP.  */
static void
save_options (gpgme_conf_comp_t comp)
{
  gpgme_conf_opt_t option = comp->options;
  gpgme_error_t err;
  int changed;

  changed = 0;
  while (option)
    {
      option_widget_t opt = option->user_data;
      gpgme_conf_arg_t arg;

      /* Exclude group headers etc.  */
      if (!opt)
	{
	  option = option->next;
	  continue;
	}

      arg = option_widget_to_arg (opt);
      if (! args_are_equal (arg, option->value, option->alt_type))
	{
	  err = gpgme_conf_opt_change (option, 0, arg);
	  if (err)
	    gpa_gpgme_error (err);
	  changed++;

	  /* FIXME: Disable this.  */
	  g_debug ("Changing component %s, option %s",
		   comp->name, option->name);
	}

      option = option->next;
    }

  if (changed)
    {
      err = gpgme_op_conf_save (dialog_ctx, comp);
      if (err)
	gpa_gpgme_warning (err);
    }
}


static void
save_all_options (void)
{
  gpgme_conf_comp_t comp;

  /* Save all tabs.  */
  comp = dialog_conf;
  while (comp)
    {
      save_options (comp);
      comp = comp->next;
    }
}


/* Update user-changes to the option widget OPT.  */
static void
update_option (option_widget_t opt)
{
  gpgme_conf_opt_t option;

  option = opt->option;

  if (opt->type == OPTION_SPIN)
    {
      int active;

      active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (opt->check));

      gtk_widget_set_sensitive (opt->widget, active);
    }
  else if (opt->type == OPTION_ENTRY || opt->type == OPTION_OPT_ENTRY)
    {
      int combo;
      GtkWidget *entry = opt->widget;

      combo = gtk_combo_box_get_active (GTK_COMBO_BOX (opt->combo));

      if (opt->old_combo_box_state == COMBO_UNDEF)
	{
	  /* Initialization.  */
	  if (option->value)
	    {
	      /* A single no-arg argument can be represented directly
		 in the combo box.  Otherwise, we use a custom
		 argument and comma-separated lists.  */
	      if (option->value->no_arg && !option->value->next)
		combo = COMBO_NO_ARG;
	      else
		combo = COMBO_CUSTOM;
	    }
	  else
	    combo = COMBO_DEFAULT;
	  gtk_combo_box_set_active (GTK_COMBO_BOX (opt->combo), combo);
	}
      else if (combo == opt->old_combo_box_state)
	return;
      else if (opt->old_combo_box_state == COMBO_CUSTOM)
	{
	  if (opt->saved_value)
	    g_free (opt->saved_value);
	  opt->saved_value = g_strdup
	    (gtk_entry_get_text (GTK_ENTRY (opt->widget)));
	}

      if (combo == COMBO_DEFAULT)
	{
	  if (option->default_value)
	    gtk_entry_set_text (GTK_ENTRY (entry),
				arg_to_str (option->default_value,
					    option->alt_type));
	  else if (option->default_description)
	    gtk_entry_set_text (GTK_ENTRY (entry),
				option->default_description);
	  else
	    gtk_entry_set_text (GTK_ENTRY (entry), "");

	  gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);
	  gtk_widget_set_sensitive (entry, FALSE);
	}
      else if (combo == COMBO_NO_ARG)
	{
	  if (option->no_arg_value)
	    gtk_entry_set_text (GTK_ENTRY (entry),
				arg_to_str (option->no_arg_value,
					    option->alt_type));
	  else if (option->no_arg_description)
	    gtk_entry_set_text (GTK_ENTRY (entry),
				option->no_arg_description);
	  else
	    gtk_entry_set_text (GTK_ENTRY (entry), "");

	  gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);
	  gtk_widget_set_sensitive (entry, FALSE);
	  /* FIXME: Change focus.  */
	}
      else if (combo == COMBO_CUSTOM)
	{
	  if (opt->saved_value)
	    gtk_entry_set_text (GTK_ENTRY (entry), opt->saved_value);
	  else if (option->value)
	    gtk_entry_set_text (GTK_ENTRY (entry),
				arg_to_str (option->value,
					    option->alt_type));
	  else if (option->default_value)
	    {
	      gtk_entry_set_text (GTK_ENTRY (entry),
				  arg_to_str (option->default_value,
					      option->alt_type));
	      /* A default (rather than current) value is selected
		 initially.  */
	      gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	    }
	  else
	    gtk_entry_set_text (GTK_ENTRY (entry), "");

	  gtk_editable_set_editable (GTK_EDITABLE (entry), TRUE);
	  gtk_widget_set_sensitive (entry, TRUE);
	  /* FIXME: Change focus.  */
	}

      opt->old_combo_box_state = combo;
    }
}


/* Update the tab when it is modified (if modified is true) or reset
   it (else).  */
static void
update_modified (int modified)
{
  if (modified)
    {
      /* Run with every button click, so keep it short.  */
      if (!dialog_tab_modified)
	{
	  dialog_tab_modified = 1;
	  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					     GTK_RESPONSE_APPLY, TRUE);
	  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					     CUSTOM_RESPONSE_RESET, TRUE);
	}
    }
  else
    {
      /* This is also called at initialization time.  */

      dialog_tab_modified = 0;
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					 GTK_RESPONSE_APPLY, FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					     CUSTOM_RESPONSE_RESET, FALSE);
    }
}


static void
update_modified_cb (void *dummy)
{
  update_modified (1);
}


/* Update a toggled checkbox button.  */
static void
option_checkbutton_toggled (GtkToggleButton *button, gpointer data)
{
  option_widget_t opt = data;

  update_option (opt);
  update_modified (1);
}


/* Update a toggled combo box.  */
static void
option_combobox_changed (GtkComboBox *combo, gpointer data)
{
  option_widget_t opt = data;

  update_option (opt);
  update_modified (1);
}


static void create_dialog_tabs (void);


/* Handle stock response "apply".  */
static void
dialog_response_apply (GtkDialog *dummy)
{
  save_all_options ();

  /* Reload configuration.  */
  create_dialog_tabs ();
}


/* Soft reset which reuses the configuration at last load.  */
static void
reset_options (gpgme_conf_comp_t comp)
{
  gpgme_conf_opt_t option;

  option = comp->options;

  while (option)
    {
      option_widget_t opt = option->user_data;

      if (!opt)
	{
	  /* Exclude group headers etc.  */
	  option = option->next;
	  continue;
	}

      if (opt->type == OPTION_SIMPLE || opt->type == OPTION_SPIN)
	{
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (opt->check),
					!!option->value);
	}
      else if (opt->type == OPTION_ENTRY || opt->type == OPTION_OPT_ENTRY)
	{
	  /* Force re-initialization.  */
	  opt->old_combo_box_state = COMBO_UNDEF;
	  update_option (opt);
	}

    option = option->next;
  }
}


static void
reset_all_options (void)
{
  gpgme_conf_comp_t comp;

  /* Save all tabs.  */
  comp = dialog_conf;
  while (comp)
    {
      reset_options (comp);
      comp = comp->next;
    }
}


/* Handle stock response "reset".  */
static void
dialog_response_reset (GtkDialog *dummy)
{
#if 0
  /* Allow to use the reset button also as a refresh button.  Note:
     This is kind of awkward, because the reset button is only
     sensitive if local modifications exist.  Currently, we require a
     dialog cancel/reopen cycle or an expert level change for a "hard"
     reset.  */
  create_dialog_tabs ();
#else
  /* Soft reset, using last loaded configuration.  Much faster, and
     consistent with sensitivity of Reset button.  */
  reset_all_options ();
  update_modified (0);
#endif
}


/* Handle stock responses like OK, apply and cancel.  */
static int
dialog_response (GtkDialog *dlg, gint response, gpointer data)
{
  switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_YES:
      save_all_options ();
      hide_backend_config ();
      break;

    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_NO:
    case GTK_RESPONSE_DELETE_EVENT:
      hide_backend_config ();
      break;

    case GTK_RESPONSE_APPLY:
      dialog_response_apply (dlg);
      break;

    case CUSTOM_RESPONSE_RESET:
      dialog_response_reset (dlg);
      break;

    default:
      g_warning ("unhandled response: %i", response);
      /* Do whatever is the default.  */
      return FALSE;
    }

  /* Don't do anything.  We handled it all.  */
  return TRUE;
}


/* Determine the width of a checkbox.  We use this to align labels
   with and without checkboxes vertically.  This approach is not
   proper, as Gtk+ can not automatically adjust the padding when the
   theme switches, but it is simple.  */
static gint
get_checkbox_width (void)
{
  GtkWidget *checkbox;
  GtkRequisition checkbox_size;

  /* We do not save the result, as it may change with a theme change.
     In this case we will at least refresh eventually.  */

  checkbox = gtk_check_button_new ();
  gtk_widget_size_request (checkbox, &checkbox_size);
  gtk_widget_destroy (checkbox);

  return checkbox_size.width;
}


/* A callback to be used with gtk_container_foreach, which removes each
   widget unconditionally.  */
static void
remove_from_container (GtkWidget *widget, gpointer data)
{
  GtkWidget *container = data;

  gtk_container_remove (GTK_CONTAINER (container), widget);
}


/* Return true iff the component COMP has any displayed options.  */
static gboolean
comp_has_options (gpgme_conf_comp_t comp)
{
  gpgme_conf_opt_t option;
  gboolean has_options;

  /* Skip over all components that do not have any options.  This can
     happen for example with old installed versions of components, or
     if there are only options with a higher expert level.  */
  has_options = FALSE;
  option = comp->options;
  while (option)
    {
      if (option->level <= dialog_level)
	{
	  has_options = TRUE;
	  break;
	}
      option = option->next;
    }

  return has_options;
}


/* Return true iff the group GROUP has any displayed options.  If the
   result is FALSE, also set NEXT_GROUP to the start of the next
   group, or NULL if there is no more group.  */
static gboolean
group_has_options (gpgme_conf_opt_t option, gpgme_conf_opt_t *next_group)
{
  gboolean has_options;

  /* Skip the group header.  */
  option = option->next;

  has_options = FALSE;
  while (option && ! (option->flags & GPGME_CONF_GROUP))
    {
      if (option->level <= dialog_level)
	{
	  has_options = TRUE;
	  break;
	}
      option = option->next;
    }

  if (! has_options && next_group)
    *next_group = option;

  return has_options;
}


static void
create_dialog_tabs_2 (gpgme_conf_comp_t old_conf, gpgme_conf_comp_t new_conf)
{
  gpgme_conf_comp_t comp;
  gpgme_conf_comp_t comp_alt;
  int reset;
  int page_nr;
  gint checkbox_width = get_checkbox_width ();
  /* We keep size groups for the field elements across all tabs for
     visual consistency.  */
  GtkSizeGroup *size_group[2];

  size_group[0] = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  size_group[1] = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* In most cases, the new components are the same as the old
     components.  We catch this special case here, and if it is true,
     we reuse the existing tabs in the notebook below.  Otherwise, we
     reset everything.  */
  reset = 0;
  comp = old_conf;
  comp_alt = new_conf;
  while (comp && comp_alt)
    {
      /* A mismatch is found if either the description changes, or if
	 a component changes from no options to some options or vice
	 verse (as we suppress generating tabs for components without
	 options below).  */

      if (strcmp (comp->description, comp_alt->description)
	  || comp_has_options (comp) != comp_has_options (comp_alt))
	break;

      comp = comp->next;
      comp_alt = comp_alt->next;
    }
  if (comp || comp_alt)
    reset = 1;

  if (reset)
    {
      gtk_widget_hide (dialog_notebook);

      /* Remove the current tabs.  */
      gtk_container_foreach (GTK_CONTAINER (dialog_notebook),
			     &remove_from_container, dialog_notebook);
    }

  comp = new_conf;
  page_nr = 0;

  while (comp)
    {
      GtkWidget *label;
      GtkWidget *page;
      gpgme_conf_opt_t option;
      /* For each group in the component, we keep track of a frame,
	 and the table inside the frame.  */
      GtkWidget *frame = NULL;
      GtkWidget *frame_vbox = NULL;

      /* Skip over all components that do not have any options.  This
	 can happen for example with old installed versions of
	 components, or if there are only options with a higher expert
	 level.  */
      if (! comp_has_options (comp))
	{
	  comp = comp->next;
	  continue;
	}

      if (reset)
	{
	  char *description;
	  /* FIXME: Might need to put pages into scrolled panes if
	     there are too many.  */
	  page = gtk_vbox_new (FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (dialog_notebook), page);

	  description = xstrdup (comp->description);
	  percent_unescape (description, 0);
	  label = gtk_label_new (description);
	  xfree (description);
	  gtk_notebook_set_tab_label
	    (GTK_NOTEBOOK (dialog_notebook),
	     gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog_notebook),
					page_nr),
	     label);
	}
      else
	{
	  page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog_notebook),
					    page_nr);

	  gtk_container_foreach (GTK_CONTAINER (page),
				 &remove_from_container, page);
	}

      option = comp->options;
	  /* All ungrouped options come first.  We put them in a frame
	     titled "Main", just so we have a consistent layout.  */
      if (option && ! (option->flags & GPGME_CONF_GROUP))
	{
	  frame = gtk_frame_new (NULL);
	  gtk_box_pack_start (GTK_BOX (page), frame, TRUE, TRUE, 0);
	  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

	  label = gtk_label_new (_("<b>Main</b>"));
	  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

	  frame_vbox = gtk_vbox_new (FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (frame), frame_vbox);
	}

      while (option)
	{
	  if (option->flags & GPGME_CONF_GROUP)
	    {
	      char *name;
	      const char *title;

	      if (! group_has_options (option, &option))
		continue;

	      frame = gtk_frame_new (NULL);
	      gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
	      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

	      /* For i18n reasons we use the description.  It might be
                 better to add a new field to privide a localized
                 version of the Group name.  Maybe the argname can be
                 used for it.  AFAICS, we would only need to prefix
                 the description with the group name and gpgconf would
                 instantly privide that. */
	      title = (option->argname && *option->argname)?
			option->argname : option->description;
	      name = g_strdup_printf ("<b>%s</b>", title);
	      percent_unescape (name, 0);
	      label = gtk_label_new (name);
	      xfree (name);

	      gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	      gtk_frame_set_label_widget (GTK_FRAME (frame), label);

	      frame_vbox = gtk_vbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER (frame), frame_vbox);
	    }
	  else if (option->level <= dialog_level)
	    {
	      GtkWidget *vbox;
	      GtkWidget *hbox;
	      option_widget_t opt;
	      GtkWidget *widget;
	      GtkWidget *entry;

	      opt = g_new0 (struct option_widget_s, 1);
	      opt->option = option;
	      option->user_data = opt;

	      /* Add the entry fields for the option, depending on its
		 type and flags.  */
	      if (option->alt_type == GPGME_CONF_NONE
		  && !(option->flags & GPGME_CONF_LIST))
		opt->type = OPTION_SIMPLE;
	      else if (option->alt_type == GPGME_CONF_NONE
		       && (option->flags & GPGME_CONF_LIST))
		opt->type = OPTION_SPIN;
	      else if (option->flags & GPGME_CONF_OPTIONAL
		       && !(option->flags & GPGME_CONF_LIST))
		opt->type = OPTION_OPT_ENTRY;
	      else
		opt->type = OPTION_ENTRY;

	      /* We create a vbox for the option, in case we want to
		 support multi-hbox setups for options.  Currently we
		 only add one hbox to that vbox though.  */

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (frame_vbox), vbox, TRUE, TRUE, 0);

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	      /* Add the entry fields for the option, depending on its
		 type and flags.  */
	      if (opt->type == OPTION_SIMPLE)
		{
		  widget = gtk_check_button_new_with_label (option->name);
		  opt->check = widget;
		  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

		  /* Add the checkbox label to the size group.  */
		  gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group[0]),
					     widget);
		  if (option->value)
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
						  TRUE);
		}
	      else if (opt->type == OPTION_SPIN)
		{
		  widget = gtk_check_button_new_with_label (option->name);
		  opt->check = widget;
		  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

		  /* Add the checkbox label to the size group.  */
		  gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group[0]),
					     widget);
		  if (option->value)
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
						  TRUE);

		  /* For list options without arguments, we use a
		     simple spin button to capture the repeat
		     count.  */
		  widget = gtk_spin_button_new_with_range (1, 100, 1);
		  opt->widget = widget;

		  if (option->value)
		    gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
					       option->value->value.count);

		  g_signal_connect ((gpointer) widget, "value-changed",
				    G_CALLBACK (update_modified_cb), NULL);

		  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
		}
	      else
		{
		  GtkWidget *align;
		  guint pt, pb, pl, pr;

		  align = gtk_alignment_new (0, 0.5, 0, 0);
		  gtk_alignment_get_padding (GTK_ALIGNMENT (align),
					     &pt, &pb, &pl, &pr);
		  gtk_alignment_set_padding (GTK_ALIGNMENT (align), pt, pb,
					     pl + checkbox_width, pr);
		  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);

		  widget = gtk_label_new (option->name);
		  opt->check = widget;
		  gtk_container_add (GTK_CONTAINER (align), widget);

		  /* Add the checkbox label to the size group.  */
		  gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group[0]),
					     align);

		  widget = gtk_combo_box_new_text ();
		  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
		  gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group[1]),
					     widget);
		  opt->combo = widget;

		  entry = gtk_entry_new ();
		  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

		  opt->widget = entry;

		  g_signal_connect ((gpointer) entry, "changed",
				    G_CALLBACK (update_modified_cb), NULL);

		  if (option->flags & GPGME_CONF_DEFAULT
		      || option->flags & GPGME_CONF_DEFAULT_DESC)
		    gtk_combo_box_append_text
		      (GTK_COMBO_BOX (widget),
		       (option->flags & GPGME_CONF_LIST) ?
		       _("Use default values") : _("Use default value"));
		  else
		    gtk_combo_box_append_text
		      (GTK_COMBO_BOX (widget), _("Do not use option"));

		  gtk_combo_box_append_text
		    (GTK_COMBO_BOX (widget),
		     (option->flags & GPGME_CONF_LIST) ?
		     _("Use custom values") : _("Use custom value"));

		  if (opt->type == OPTION_OPT_ENTRY)
		    gtk_combo_box_append_text
		      (GTK_COMBO_BOX (widget), _("Use default argument"));
		}

	      /* Force update.  */
	      opt->old_combo_box_state = COMBO_UNDEF;
	      update_option (opt);

	      if (opt->combo)
		g_signal_connect ((gpointer) opt->combo, "changed",
				  G_CALLBACK (option_combobox_changed),
				  opt);
	      else
		g_signal_connect ((gpointer) opt->check, "toggled",
				  G_CALLBACK (option_checkbutton_toggled),
				  opt);

	      /* Grey out options which can not be changed at all.  */
	      if (option->flags & GPGME_CONF_NO_CHANGE)
		gtk_widget_set_sensitive (vbox, FALSE);

#if GTK_CHECK_VERSION (2, 12, 0)
	      /* Add a tooltip description.  */
	      if (option->description)
		{
		  char *description = xstrdup (option->description);

	          percent_unescape (description, 0);
		  gtk_widget_set_tooltip_text (vbox, description);
		  xfree (description);
		}
#endif
	    }
	  option = option->next;
	}
      page_nr++;
      comp = comp->next;
    }

  /* Release our references to the size groups.  */
  g_object_unref (size_group[0]);
  g_object_unref (size_group[1]);

  gtk_widget_show_all (dialog_notebook);

  update_modified (0);
}


static void
create_dialog_tabs (void)
{
  gpgme_error_t err;
  gpgme_conf_comp_t new_conf;
  int page;
  int nr_pages;
  char *current_tab = NULL;

  if (dialog_notebook)
    {
      /* Remember the current tab by its label.  */
      page = gtk_notebook_get_current_page (GTK_NOTEBOOK (dialog_notebook));
      if (page >= 0)
	current_tab = strdup (gtk_notebook_get_tab_label_text
			      (GTK_NOTEBOOK (dialog_notebook),
			       gtk_notebook_get_nth_page
			       (GTK_NOTEBOOK (dialog_notebook), page)));
    }

  err = gpgme_op_conf_load (dialog_ctx, &new_conf);
  if (err)
    {
      gpa_gpgme_warning (err);
      return;
    }

  create_dialog_tabs_2 (dialog_conf, new_conf);
  gpgme_conf_release (dialog_conf);
  dialog_conf = new_conf;

  if (current_tab)
    {
      /* Rediscover the current tab.  */
      page = 0;
      nr_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (dialog_notebook));
      while (page < nr_pages)
	{
	  const char *label = gtk_notebook_get_tab_label_text
	    (GTK_NOTEBOOK (dialog_notebook),
	     gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog_notebook), page));
	  if (! strcmp (label, current_tab))
	    break;
	  page++;
	}
      if (page < nr_pages)
	gtk_notebook_set_current_page (GTK_NOTEBOOK (dialog_notebook), page);

      free (current_tab);
    }
}


static void
dialog_level_chooser_cb (GtkComboBox *level_chooser, gpointer *data)
{
  gpgme_conf_level_t level;

  level = gtk_combo_box_get_active (GTK_COMBO_BOX (level_chooser));

  if (level == dialog_level)
    return;

  if (dialog_tab_modified)
    {
      GtkWidget *window;
      GtkWidget *hbox;
      GtkWidget *labelMessage;
      GtkWidget *pixmap;
      gint result;

      window = gtk_dialog_new_with_buttons
	(_("GPA Message"), (GtkWindow *) dialog, GTK_DIALOG_MODAL,
	 GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

      gtk_container_set_border_width (GTK_CONTAINER (window), 5);
      gtk_dialog_set_default_response (GTK_DIALOG (window),
				       GTK_RESPONSE_CANCEL);

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
      gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox), hbox);
      pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
					 GTK_ICON_SIZE_DIALOG);
      gtk_box_pack_start (GTK_BOX (hbox), pixmap, TRUE, FALSE, 10);
      labelMessage = gtk_label_new (_("There are unapplied changes by you. "
				      "Changing the expert setting will apply "
				      "those changes.  Do you want to "
				      "continue?"));
      gtk_label_set_line_wrap (GTK_LABEL (labelMessage), TRUE);
      gtk_box_pack_start (GTK_BOX (hbox), labelMessage, TRUE, FALSE, 10);

      gtk_widget_show_all (window);
      result = gtk_dialog_run (GTK_DIALOG (window));
      gtk_widget_destroy (window);

      if (result != GTK_RESPONSE_APPLY)
	{
	  gtk_combo_box_set_active (GTK_COMBO_BOX (level_chooser),
				    dialog_level);
	  return;
	}
    }

  save_all_options ();

  /* Note: We know intimately that this matches GPGME_CONF_BASIC,
     GPGME_CONF_ADVANCED and GPGME_CONF_BEGINNER.  */
  dialog_level = level;
  create_dialog_tabs ();
}


/* Return a new dialog widget.  */
static GtkDialog *
create_dialog (void)
{
  gpgme_error_t err;
  GtkWidget *dialog_vbox;
  GtkWidget *hbox;
  GtkWidget *level_chooser;
  GtkWidget *label;
  gint xpad;
  gint ypad;

  /* Check error.  */
  err = gpgme_new (&dialog_ctx);
  if (err)
    gpa_gpgme_error (err);

  dialog = gtk_dialog_new_with_buttons (_("Crypto Backend Configuration"),
					NULL /* transient parent */,
					0,
					NULL);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                          _("Reset"), CUSTOM_RESPONSE_RESET,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL );
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           CUSTOM_RESPONSE_RESET,
                                           GTK_RESPONSE_APPLY,
                                           -1);

  g_signal_connect ((gpointer) dialog, "response",
		    G_CALLBACK (dialog_response), NULL);

  dialog_vbox = GTK_DIALOG (dialog)->vbox;
  /*  gtk_box_set_spacing (GTK_CONTAINER (dialog_vbox), 5); */

  hbox = gtk_hbox_new (FALSE, 0);

  label = gtk_label_new (_("Configure the tools of the GnuPG system."));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  label = gtk_label_new (_("Level:"));
  gtk_misc_get_padding (GTK_MISC (label), &xpad, &ypad);
  xpad += 5;
  gtk_misc_set_padding (GTK_MISC (label), xpad, ypad);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  level_chooser = gtk_combo_box_new_text ();
  /* Note: We know intimately that this matches GPGME_CONF_BASIC,
     GPGME_CONF_ADVANCED and GPGME_CONF_BEGINNER.  */
  gtk_combo_box_append_text (GTK_COMBO_BOX (level_chooser), _("Basic"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (level_chooser), _("Advanced"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (level_chooser), _("Expert"));
  g_signal_connect ((gpointer) level_chooser, "changed",
		    G_CALLBACK (dialog_level_chooser_cb), NULL);

  gtk_box_pack_start (GTK_BOX (hbox), level_chooser, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), hbox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    dialog_level = GPGME_CONF_BASIC;
  else
    dialog_level = GPGME_CONF_ADVANCED;
  gtk_combo_box_set_active (GTK_COMBO_BOX (level_chooser), dialog_level);

  dialog_notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (dialog_notebook), 5);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), dialog_notebook, TRUE, TRUE, 0);

  /* This should also be run on show after hide.  */
  dialog_tab_modified = 0;
  create_dialog_tabs ();

  return GTK_DIALOG (dialog);
}


GtkWidget *
gpa_backend_config_dialog_new (void)
{
  assert (! dialog);

  create_dialog ();

  return dialog;
}


static void
hide_backend_config (void)
{
  gtk_widget_destroy (dialog);
  dialog = NULL;
  dialog_notebook = NULL;

  gpgme_conf_release (dialog_conf);
  dialog_conf = NULL;

  gpgme_release (dialog_ctx);
  dialog_ctx = NULL;
}



/* Load the value of option NAME of component CNAME from the backend.
   If none is configured, return NULL.  Caller must g_free the
   returned value.  */
char *
gpa_load_gpgconf_string (const char *cname, const char *name)
{
  gpg_error_t err;
  gpgme_ctx_t ctx;
  gpgme_conf_comp_t conf_list, conf;
  gpgme_conf_opt_t opt;
  char *retval = NULL;

  err = gpgme_new (&ctx);
  if (err)
    {
      gpa_gpgme_error (err);
      return NULL;
    }

  err = gpgme_op_conf_load (ctx, &conf_list);
  if (err)
    {
      gpa_gpgme_warning (err);
      gpgme_release (ctx);
      return NULL;
    }

  for (conf = conf_list; conf; conf = conf->next)
    {
      if ( !strcmp (conf->name, cname) )
        {
          for (opt = conf->options; opt; opt = opt->next)
            if ( !(opt->flags & GPGME_CONF_GROUP)
                 && !strcmp (opt->name, name))
              {
                if (opt->value && opt->alt_type == GPGME_CONF_STRING)
                  retval = g_strdup (opt->value->value.string);
                break;
              }
          break;
        }
    }

  gpgme_conf_release (conf_list);
  gpgme_release (ctx);
  return retval;
}


/* Set the option NAME in component "CNAME" to VALUE.  The option
   needs to be of type string. */
void
gpa_store_gpgconf_string (const char *cname,
                          const char *name, const char *value)
{
  gpg_error_t err;
  gpgme_ctx_t ctx;
  gpgme_conf_comp_t conf_list, conf;
  gpgme_conf_opt_t opt;
  gpgme_conf_arg_t arg;

  err = gpgme_conf_arg_new (&arg, GPGME_CONF_STRING, (char*)value);
  if (err)
    {
      gpa_gpgme_warning (err);
      return;
    }

  err = gpgme_new (&ctx);
  if (err)
    {
      gpa_gpgme_error (err);
      return;
    }

  err = gpgme_op_conf_load (ctx, &conf_list);
  if (err)
    {
      gpa_gpgme_error (err);
      gpgme_release (ctx);
      return;
    }

  for (conf = conf_list; conf; conf = conf->next)
    {
      if ( !strcmp (conf->name, cname) )
        {
          for (opt = conf->options; opt; opt = opt->next)
            if ( !(opt->flags & GPGME_CONF_GROUP)
                 && !strcmp (opt->name, name))
              {
                if (opt->alt_type == GPGME_CONF_STRING
                    && !args_are_equal (arg, opt->value, opt->alt_type))
                  {
                    err = gpgme_conf_opt_change (opt, 0, arg);
                    if (err)
                      gpa_gpgme_error (err);
                    else
                      {
                        err = gpgme_op_conf_save (ctx, conf);
                        if (err)
                          gpa_gpgme_error (err);
                      }
                  }
                break;
              }
          break;
        }
    }

  gpgme_conf_release (conf_list);
  gpgme_release (ctx);
}


/* Read the configured keyserver from the backend.  If none is
   configured, return NULL.  Caller must g_free the returned value.  */
char *
gpa_load_configured_keyserver (void)
{
#ifdef ENABLE_KEYSERVER_SUPPORT
  return gpa_load_gpgconf_string ("gpg", "keyserver");
#else
  return NULL;
#endif
}

/* Save the configured keyserver from the backend.  If none is
   configured, return NULL.  Caller must g_free the returned value.  */
void
gpa_store_configured_keyserver (const char *value)
{
#ifdef ENABLE_KEYSERVER_SUPPORT
  gpa_store_gpgconf_string ("gpg", "keyserver", value);
#endif
}


/* Ask the user whether to configure GnuPG to use a keyserver.  Return
   NULL if it could or shall not be configured or the name of the
   keyserver which needs to be g_freed.  */
char *
gpa_configure_keyserver (GtkWidget *parent)
{
#ifdef ENABLE_KEYSERVER_SUPPORT
  GtkWidget *msgbox;
  char *keyserver;

  msgbox = gtk_message_dialog_new
    (GTK_WINDOW(parent), GTK_DIALOG_MODAL,
     GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
     "%s\n\n%s",
     _("A keyserver has not been configured."),
     _("Configure backend to use a keyserver?"));
  gtk_dialog_add_buttons (GTK_DIALOG (msgbox),
                          _("_Yes"), GTK_RESPONSE_YES,
                          _("_No"), GTK_RESPONSE_NO, NULL);
  if (gtk_dialog_run (GTK_DIALOG (msgbox)) != GTK_RESPONSE_YES)
    {
      gtk_widget_destroy (msgbox);
      return NULL;
    }
  gtk_widget_destroy (msgbox);
  gpa_store_configured_keyserver ("hkp://keys.gnupg.net");
  keyserver = gpa_load_configured_keyserver ();
  if (!keyserver)
    {
      gpa_show_warn
        (parent, NULL, _("Configuring the backend to use a keyserver failed"));
      return NULL;
    }
  return keyserver;
#else
  (void)parent;
  return NULL
#endif
}
