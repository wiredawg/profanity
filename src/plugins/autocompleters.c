/*
 * autocompleters.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include <string.h>

#include <glib.h>

#include "tools/autocomplete.h"

static GHashTable *plugin_to_acs;

static void
_free_autocompleters(GHashTable *key_to_ac)
{
    g_hash_table_destroy(key_to_ac);
}

void
autocompleters_init(void)
{
    plugin_to_acs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_free_autocompleters);
}

void
autocompleters_add(const char *const plugin_name, const char *key, char **items)
{
    GHashTable *key_to_ac = g_hash_table_lookup(plugin_to_acs, plugin_name);
    if (key_to_ac) {
        if (g_hash_table_contains(key_to_ac, key)) {
            Autocomplete existing_ac = g_hash_table_lookup(key_to_ac, key);
            autocomplete_add_all(existing_ac, items);
        } else {
            Autocomplete new_ac = autocomplete_new();
            autocomplete_add_all(new_ac, items);
            g_hash_table_insert(key_to_ac, strdup(key), new_ac);
        }
    } else {
        key_to_ac = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)autocomplete_free);
        Autocomplete new_ac = autocomplete_new();
        autocomplete_add_all(new_ac, items);
        g_hash_table_insert(key_to_ac, strdup(key), new_ac);
        g_hash_table_insert(plugin_to_acs, strdup(plugin_name), key_to_ac);
    }
}

void
autocompleters_remove(const char *const plugin_name, const char *key, char **items)
{
    GHashTable *key_to_ac = g_hash_table_lookup(plugin_to_acs, plugin_name);
    if (!key_to_ac) {
        return;
    }

    if (!g_hash_table_contains(key_to_ac, key)) {
        return;
    }

    Autocomplete ac = g_hash_table_lookup(key_to_ac, key);
    autocomplete_remove_all(ac, items);
}

void
autocompleters_clear(const char *const plugin_name, const char *key)
{
    GHashTable *key_to_ac = g_hash_table_lookup(plugin_to_acs, plugin_name);
    if (!key_to_ac) {
        return;
    }

    if (!g_hash_table_contains(key_to_ac, key)) {
        return;
    }

    Autocomplete ac = g_hash_table_lookup(key_to_ac, key);
    autocomplete_clear(ac);
}

char *
autocompleters_complete(const char * const input)
{
    char *result = NULL;

    GList *ac_hashes = g_hash_table_get_values(plugin_to_acs);
    GList *curr_hash = ac_hashes;
    while (curr_hash) {
        GHashTable *key_to_ac = curr_hash->data;

        GList *keys = g_hash_table_get_keys(key_to_ac);
        GList *curr = keys;
        while (curr) {
            result = autocomplete_param_with_ac(input, curr->data, g_hash_table_lookup(key_to_ac, curr->data), TRUE);
            if (result) {
                g_list_free(keys);
                return result;
            }
            curr = g_list_next(curr);
        }
        g_list_free(keys);

        curr_hash = g_list_next(curr_hash);
    }
    g_list_free(ac_hashes);

    return NULL;
}

void
autocompleters_reset(void)
{
    GList *ac_hashes = g_hash_table_get_values(plugin_to_acs);
    GList *curr_hash = ac_hashes;
    while (curr_hash) {
        GList *acs = g_hash_table_get_values(curr_hash->data);
        GList *curr = acs;
        while (curr) {
            autocomplete_reset(curr->data);
            curr = g_list_next(curr);
        }

        g_list_free(acs);
        curr_hash = g_list_next(curr_hash);
    }

    g_list_free(ac_hashes);
}

void autocompleters_destroy(void)
{
    g_hash_table_destroy(plugin_to_acs);
}
