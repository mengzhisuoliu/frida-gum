/*
 * Copyright (C) 2021 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#ifndef __GUM_DARWIN_GRAFTER_H__
#define __GUM_DARWIN_GRAFTER_H__

#include <gum/gumdefs.h>

G_BEGIN_DECLS

#define GUM_TYPE_DARWIN_GRAFTER (gum_darwin_grafter_get_type ())
G_DECLARE_FINAL_TYPE (GumDarwinGrafter, gum_darwin_grafter, GUM, DARWIN_GRAFTER,
    GObject)

GUM_API GumDarwinGrafter * gum_darwin_grafter_new_from_file (const gchar * path,
    GError ** error);

GUM_API gboolean gum_darwin_grafter_graft (GumDarwinGrafter * self,
    GError ** error);

G_END_DECLS

#endif
