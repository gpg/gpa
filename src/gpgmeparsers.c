/* gpgmeparsers.c - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
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

#include <gpgme.h>
#include "gpa.h"
#include "gpgmeparsers.h"

/* Retrieve and parse the detailed results of an import operation */

struct parse_import_info_s
{
  GQueue *tag_stack;
  GpaImportInfo *info;
};

static
void parse_import_info_start (GMarkupParseContext *context,
			      const gchar         *element_name,
			      const gchar        **attribute_names,
			      const gchar        **attribute_values,
			      gpointer             user_data,
			      GError             **error)
{
  struct parse_import_info_s *data = user_data;
  g_queue_push_head (data->tag_stack, (gpointer) element_name);
}

static
void parse_import_info_end (GMarkupParseContext *context,
			    const gchar         *element_name,
			    gpointer             user_data,
			    GError             **error)
{
  struct parse_import_info_s *data = user_data;
  g_queue_pop_head (data->tag_stack);
}

static
void parse_import_info_text (GMarkupParseContext *context,
			     const gchar         *text,
			     gsize                text_len,  
			     gpointer             user_data,
			     GError             **error)
{
  struct parse_import_info_s *data = user_data;
  if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack), "keyid"))
    {
      data->info->keyids = g_list_append (data->info->keyids, g_strdup (text));
    }
  else
    {
      gint num;
      if (!sscanf (text, "%i", &num))
        {
          return;
        }
      if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack), "count"))
        {
          data->info->count = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack), 
                            "no_user_id"))
        {
          data->info->no_user_id = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "imported"))
        {
          data->info->imported = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack), 
                            "imported_rsa"))
        {
          data->info->imported_rsa = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "unchanged"))
        {
          data->info->unchanged = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack), 
                            "n_uids"))
        {
          data->info->n_uids = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "n_subk"))
        {
          data->info->n_subk = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "n_sigs"))
        {
          data->info->n_sigs = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "s_sigs"))
        {
          data->info->s_sigs = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "n_revoc"))
        {
          data->info->n_revoc = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "sec_read"))
        {
          data->info->sec_read = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "sec_imported"))
        {
          data->info->sec_imported = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "sec_dups"))
        {
          data->info->sec_dups = num;
        }
      else if (g_str_equal ((gchar*) g_queue_peek_head (data->tag_stack),
                            "skipped_new"))
        {
          data->info->skipped_new = num;
        }
    }
}

void gpa_parse_import_info (GpaImportInfo *info)
{
  GMarkupParser parser = 
    {
      parse_import_info_start,
      parse_import_info_end,
      parse_import_info_text,
      NULL, NULL
    };
  struct parse_import_info_s data = {g_queue_new(), info};
  char *import_info = gpgme_get_op_info (ctx, 0);
  GMarkupParseContext* context = g_markup_parse_context_new (&parser, 0,
							     &data, NULL);
  g_markup_parse_context_parse (context, import_info, strlen (import_info),
				NULL);
  g_markup_parse_context_free (context);
  free (import_info);
}
