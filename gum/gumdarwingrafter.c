/*
 * Copyright (C) 2021 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gumdarwingrafter.h"

struct _GumDarwinGrafter
{
  GObject parent;
};

G_DEFINE_TYPE (GumDarwinGrafter, gum_darwin_grafter, G_TYPE_OBJECT)

static void
gum_darwin_grafter_class_init (GumDarwinGrafterClass * klass)
{
}

static void
gum_darwin_grafter_init (GumDarwinGrafter * self)
{
}

GumDarwinGrafter *
gum_darwin_grafter_new_from_file (const gchar * path,
                                  GError ** error)
{
  GumDarwinGrafter * grafter;

  grafter = g_object_new (GUM_TYPE_DARWIN_GRAFTER, NULL);

  return grafter;
}

gboolean
gum_darwin_grafter_graft (GumDarwinGrafter * self,
                          GError ** error)
{
  return TRUE;
}
