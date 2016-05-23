/*
 * purple
 *
 * Copyright (C) 2003 Christian Hammond <chipx86@gnupdate.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */
 
#include <libtcc.h>

#include "internal.h"
#include "debug.h"
#include "plugin.h"
#include "signals.h"
#include "version.h"

#define TCC_PLUGIN_ID "eionrobb-tcc-loader"


typedef struct {
	TCCState *state;
	gpointer mem;
} TccPluginPrivate;

typedef gboolean (*TccPluginInitFunc)(PurplePlugin *plugin);


static gboolean
probe_tcc_plugin(PurplePlugin *plugin)
{
	TCCState *state = NULL;
	TccPluginPrivate *priv = NULL;
	TccPluginInitFunc tcc_purple_init_plugin = NULL;
	gpointer memneeded = NULL;
	int memsize = -1;
	gboolean status;
	
	state = tcc_new();
	
	if(tcc_add_file(state, plugin->path) == -1) {
		purple_debug_error("tcc", "couldn't load file %s", plugin->path);
		
		tcc_delete(state);
		return FALSE;
	}

	/* copy code into memory */
	if((memsize = tcc_relocate(state, NULL)) < 0) {
		purple_debug_error("tcc", "couldn't work out how much memory is needed");

		tcc_delete(state);
		return FALSE;
	}
	memneeded = g_malloc0(memsize);
	if(tcc_relocate(state, memneeded) < 0) {
		purple_debug_error("tcc", "could not relocate plugin into memory");
		
		tcc_delete(state);
		g_free(memneeded);
		return FALSE;
	}
	
	tcc_purple_init_plugin = (TccPluginInitFunc) tcc_get_symbol(state, "purple_init_plugin");
	if (tcc_purple_init_plugin == NULL) {
		purple_debug_error("tcc", "no purple_init_plugin function found");
		
		tcc_delete(state);
		g_free(memneeded);
		return FALSE;
	}
	
	status = tcc_purple_init_plugin(plugin);
	if(status == FALSE) {
		tcc_delete(state);
		g_free(memneeded);
		return FALSE;
	}
	
	priv = g_new0(TccPluginPrivate, 1);
	priv->state = state;
	priv->mem = memneeded;
	plugin->info->extra_info = priv;

	return TRUE;
}

static gboolean
load_tcc_plugin(PurplePlugin *plugin)
{
	if (plugin->info->load != NULL) {
		return plugin->info->load(plugin);
	}
	
	return FALSE;
}

static gboolean
unload_tcc_plugin(PurplePlugin *plugin)
{
	purple_debug(PURPLE_DEBUG_INFO, "tcc", "Unloading tcc plugin\n");
	
	if (plugin->info->unload != NULL) {
		return plugin->info->unload(plugin);
	}

	return TRUE;
}

static void
destroy_tcc_plugin(PurplePlugin *plugin)
{
	if (plugin->info != NULL) {
		TccPluginPrivate *priv;
		
		if (plugin->info->destroy != NULL) {
			plugin->info->destroy(plugin);
		}
		
		priv = (TccPluginPrivate *)plugin->info->extra_info;

		if(priv->state)
			tcc_delete(priv->state);
		
		if(priv->mem)
			g_free(priv->mem);

		g_free(plugin->info->extra_info);
		plugin->info = NULL;
	}
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	//todo destroy all C plugins
	
	return FALSE;
}

static PurplePluginLoaderInfo loader_info =
{
	NULL,                                             /**< exts           */
	probe_tcc_plugin,                                 /**< probe          */
	load_tcc_plugin,                                  /**< load           */
	unload_tcc_plugin,                                /**< unload         */
	destroy_tcc_plugin,                               /**< destroy        */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_LOADER,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                          /**< priority       */

	TCC_PLUGIN_ID,                                    /**< id             */
	N_("TCC Plugin Loader"),                          /**< name           */
	DISPLAY_VERSION,                                  /**< version        */
	N_("Provides support for loading C plugins."),    /**< summary        */
	N_("Provides support for loading C plugins."),    /**< description    */
	"Eion Robb <eionrobb@gmail.com>",                 /**< author         */
	PURPLE_WEBSITE,                                   /**< homepage       */

	plugin_load,                                      /**< load           */
	plugin_unload,                                    /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	&loader_info,                                     /**< extra_info     */
	NULL,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
	loader_info.exts = g_list_append(loader_info.exts, "c");
}

PURPLE_INIT_PLUGIN(tcc, init_plugin, info)
