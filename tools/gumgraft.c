/*
 * Copyright (C) 2021 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include <gum/gum.h>

int
main (int argc,
      char * argv[])
{
  const gchar * path;
  GumDarwinGrafter * grafter;
  GError * error;

#ifdef HAVE_FRIDA_GLIB
  glib_init ();
#endif

  if (argc != 2)
  {
    g_printerr ("Usage: %s <path/to/binary>\n", argv[0]);
    return 1;
  }

  path = argv[1];

  error = NULL;
  grafter = gum_darwin_grafter_new_from_file (path, &error);
  if (error != NULL)
  {
    g_printerr ("%s\n", error->message);
    return 2;
  }

  gum_darwin_grafter_graft (grafter, &error);
  if (error != NULL)
  {
    g_printerr ("%s\n", error->message);
    return 3;
  }

  return 0;
}
