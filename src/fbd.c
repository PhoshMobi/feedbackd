/*
 * Copyright (C) 2020 Purism SPC
 *               2023-2026 Phosh.mobi e.V.
 *
 * SPDX-License-Identifier: GPL-3.0+
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "fbd"

#include "fbd.h"
#include "fbd-feedback-manager.h"
#include "fbd-haptic-manager.h"
#include "lfb-names.h"
#include "lfb-gdbus.h"

#include <gio/gio.h>
#include <glib-unix.h>

static GMainLoop *loop;


static GDebugKey debug_keys[] =
{
  { .key = "force-haptic",
    .value = FBD_DEBUG_FLAG_FORCE_HAPTIC,
  },
};


static gboolean
quit_cb (gpointer user_data)
{
  g_info ("Caught signal, shutting down...");

  if (loop)
    g_idle_add ((GSourceFunc) g_main_loop_quit, loop);
  else
    exit (0);

  return FALSE;
}

static gboolean
reload_cb (gpointer user_data)
{
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default();

  g_return_val_if_fail (FBD_IS_FEEDBACK_MANAGER (manager), FALSE);

  g_debug ("Caught signal, reloading feedback theme...");
  fbd_feedback_manager_load_theme (manager);

  return TRUE;
}

static void
bus_acquired_cb (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  FbdFeedbackManager *manager = fbd_feedback_manager_get_default ();
  FbdHapticManager   *haptic_manager;

  g_assert (FBD_IS_FEEDBACK_MANAGER (manager));

  g_debug ("Bus acquired, exporting manager...");

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (manager),
                                    connection,
                                    FB_DBUS_PATH,
                                    NULL);

  haptic_manager = fbd_feedback_manager_get_haptic_manager (manager);
  if (haptic_manager) {
    g_debug ("Exporting haptic manager...");
    g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (haptic_manager),
                                      connection,
                                      FB_DBUS_PATH,
                                      NULL);
  }
}


static void
name_acquired_cb (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  g_debug ("Service name '%s' was acquired", name);
}

static void
name_lost_cb (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  /* Note that we're not allowing replacement, so once the name acquired, the
   * process won't lose it. */
  if (!name) {
    g_warning ("Could not get the session bus. Make sure "
               "the message bus daemon is running!");
  } else {
    if (connection)
      g_warning ("Could not acquire the '%s' service name", name);
    else
      g_debug ("DBus connection close");
  }

  g_main_loop_quit (loop);
}


int
main (int argc, char *argv[])
{
  g_autoptr (GError) err = NULL;
  gboolean opt_verbose = FALSE, opt_replace = FALSE;
  g_autoptr (GOptionContext) opt_context = NULL;
  g_autoptr (FbdFeedbackManager) manager = NULL;
  const char *debugenv;
  GOptionEntry options[] = {
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &opt_verbose,
      "Print debug information during command processing", NULL },
    { "replace", 'r', 0, G_OPTION_ARG_NONE, &opt_replace, "Replace a running instance", NULL },
    { NULL }
  };

  opt_context = g_option_context_new ("- A daemon to trigger event feedback");
  g_option_context_add_main_entries (opt_context, options, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_warning ("%s", err->message);
    g_clear_error (&err);
    return 1;
  }

  if (opt_verbose)
    g_log_writer_default_set_debug_domains ((const char *const[]){ "all", NULL });

  debugenv = g_getenv ("FEEDBACKD_DEBUG");
  fbd_debug_flags = g_parse_debug_string (debugenv,
                                          debug_keys,
                                          G_N_ELEMENTS (debug_keys));

  manager = fbd_feedback_manager_get_default ();
  fbd_feedback_manager_load_theme (manager);

  g_unix_signal_add (SIGTERM, quit_cb, NULL);
  g_unix_signal_add (SIGINT, quit_cb, NULL);
  g_unix_signal_add (SIGHUP, reload_cb, NULL);

  loop = g_main_loop_new (NULL, FALSE);

  g_bus_own_name (FB_DBUS_TYPE,
                  FB_DBUS_NAME,
                  G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                  (opt_replace ? G_BUS_NAME_OWNER_FLAGS_REPLACE : 0),
                  bus_acquired_cb,
                  name_acquired_cb,
                  name_lost_cb,
                  NULL,
                  NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}
