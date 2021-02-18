/*
 * Copyright (C) 2021 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gumdarwingrafter.h"

#include "gumdarwinmodule-priv.h"

#include <gio/gio.h>
#include <stdio.h>
#include <string.h>

typedef struct _GumGraftedLayout GumGraftedLayout;

enum
{
  PROP_0,
  PROP_PATH,
};

struct _GumDarwinGrafter
{
  GObject parent;

  gchar * path;
};

struct _GumGraftedLayout
{
  GumAddress linkedit_address;
  goffset linkedit_offset;
  gsize linkedit_size;
  gsize linkedit_shift;
};

static void gum_darwin_grafter_finalize (GObject * object);
static void gum_darwin_grafter_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gum_darwin_grafter_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);

G_DEFINE_TYPE (GumDarwinGrafter, gum_darwin_grafter, G_TYPE_OBJECT)

static void
gum_darwin_grafter_class_init (GumDarwinGrafterClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gum_darwin_grafter_finalize;
  object_class->get_property = gum_darwin_grafter_get_property;
  object_class->set_property = gum_darwin_grafter_set_property;

  g_object_class_install_property (object_class, PROP_PATH,
      g_param_spec_string ("path", "Path", "Path", NULL,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
gum_darwin_grafter_init (GumDarwinGrafter * self)
{
}

static void
gum_darwin_grafter_finalize (GObject * object)
{
  GumDarwinGrafter * self = GUM_DARWIN_GRAFTER (object);

  g_free (self->path);

  G_OBJECT_CLASS (gum_darwin_grafter_parent_class)->finalize (object);
}

static void
gum_darwin_grafter_get_property (GObject * object,
                                 guint property_id,
                                 GValue * value,
                                 GParamSpec * pspec)
{
  GumDarwinGrafter * self = GUM_DARWIN_GRAFTER (object);

  switch (property_id)
  {
    case PROP_PATH:
      g_value_set_string (value, self->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
gum_darwin_grafter_set_property (GObject * object,
                                 guint property_id,
                                 const GValue * value,
                                 GParamSpec * pspec)
{
  GumDarwinGrafter * self = GUM_DARWIN_GRAFTER (object);

  switch (property_id)
  {
    case PROP_PATH:
      g_free (self->path);
      self->path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

GumDarwinGrafter *
gum_darwin_grafter_new_from_file (const gchar * path)
{
  return g_object_new (GUM_TYPE_DARWIN_GRAFTER,
      "path", path,
      NULL);
}

gboolean
gum_darwin_grafter_graft (GumDarwinGrafter * self,
                          GError ** error)
{
  gboolean success = FALSE;
  GumDarwinModule * module;
  GumGraftedLayout layout;
  guint i;
  gconstpointer input;
  FILE * output = NULL;
  guint32 num_load_commands, size_of_load_commands;
  GumMachHeader64 header;
  gpointer commands = NULL;
  gpointer command;
  GumSegmentCommand64 * seg;
  GumSection64 * sect;
  gconstpointer end_of_load_commands;
  gsize gap_space_used;
  gconstpointer rest_of_gap;
  gpointer code = NULL;
  GumArm64Writer cw;

  module = gum_darwin_module_new_from_file (self->path, GUM_CPU_ARM64,
      GUM_PTRAUTH_INVALID, GUM_DARWIN_MODULE_FLAGS_NONE, error);
  if (module == NULL)
    goto beach;

  layout.linkedit_address = 0;
  layout.linkedit_offset = -1;
  layout.linkedit_shift = 0;
  for (i = 0; i != module->segments->len; i++)
  {
    const GumDarwinSegment * segment =
        &g_array_index (module->segments, GumDarwinSegment, i);

    if (strcmp (segment->name, "__LINKEDIT") == 0)
    {
      layout.linkedit_address = segment->vm_address;
      layout.linkedit_offset = segment->file_offset;
      layout.linkedit_size = segment->file_size;
    }
  }
  if (layout.linkedit_offset == -1)
    goto invalid_data;

  layout.linkedit_shift = 16384; /* TODO */

  input = module->image->data;
#if 1
  output = fopen (self->path, "wb");
#else
  {
    gchar * path = g_strconcat (self->path, ".grafted", NULL);
    output = fopen (path, "wb");
    g_free (path);
  }
#endif

  /* XXX: For now we assume matching endian and struct packing. */
  memcpy (&header, input, sizeof (GumMachHeader64));

  num_load_commands = header.ncmds;
  size_of_load_commands = header.sizeofcmds;

  header.ncmds++;
  header.sizeofcmds += sizeof (GumSegmentCommand64) + sizeof (GumSection64);
  fwrite (&header, sizeof (header), 1, output);

  commands = g_memdup ((const GumMachHeader64 *) input + 1, header.sizeofcmds);
  for (command = commands, i = 0; i != num_load_commands; i++)
  {
    GumLoadCommand * lc = command;

#define GUM_SHIFT(field) \
    field += layout.linkedit_shift
#define GUM_MAYBE_SHIFT(field) \
    if (field != 0) \
      field += layout.linkedit_shift

    switch (lc->cmd)
    {
      case GUM_LC_SEGMENT_64:
      {
        GumSegmentCommand64 * sc = command;

        if (sc->fileoff == layout.linkedit_offset)
        {
          GUM_SHIFT (sc->vmaddr);
          GUM_SHIFT (sc->fileoff);
        }

        break;
      }
      case GUM_LC_DYLD_INFO_ONLY:
      {
        GumDyldInfoCommand * ic = command;

        GUM_MAYBE_SHIFT (ic->rebase_off);
        GUM_MAYBE_SHIFT (ic->bind_off);
        GUM_MAYBE_SHIFT (ic->weak_bind_off);
        GUM_MAYBE_SHIFT (ic->lazy_bind_off);
        GUM_MAYBE_SHIFT (ic->export_off);

        break;
      }
      case GUM_LC_SYMTAB:
      {
        GumSymtabCommand * sc = command;

        GUM_SHIFT (sc->symoff);
        GUM_SHIFT (sc->stroff);

        break;
      }
      case GUM_LC_DYSYMTAB:
      {
        GumDysymtabCommand * dc = command;

        GUM_MAYBE_SHIFT (dc->tocoff);
        GUM_MAYBE_SHIFT (dc->modtaboff);
        GUM_MAYBE_SHIFT (dc->extrefsymoff);
        GUM_SHIFT (dc->indirectsymoff); /* XXX: Is it always specified? */
        GUM_MAYBE_SHIFT (dc->extreloff);
        GUM_MAYBE_SHIFT (dc->locreloff);

        break;
      }
      case GUM_LC_FUNCTION_STARTS:
      case GUM_LC_DATA_IN_CODE:
      {
        GumLinkeditDataCommand * dc = command;

        GUM_SHIFT (dc->dataoff);

        break;
      }
      default:
        break;
    }

    command = (guint8 *) command + lc->cmdsize;
  }

  seg = (GumSegmentCommand64 *) ((guint8 *) commands + size_of_load_commands);
  seg->cmd = GUM_LC_SEGMENT_64;
  seg->cmdsize = sizeof (GumSegmentCommand64) + sizeof (GumSection64);
  strcpy (seg->segname, "__FRIDA");
  seg->vmaddr = layout.linkedit_address;
  seg->vmsize = layout.linkedit_shift; /* TODO */
  seg->fileoff = layout.linkedit_offset;
  seg->filesize = layout.linkedit_shift; /* TODO */
  seg->maxprot = GUM_VM_PROT_READ | GUM_VM_PROT_EXECUTE;
  seg->initprot = GUM_VM_PROT_READ | GUM_VM_PROT_EXECUTE;
  seg->nsects = 1;
  seg->flags = 0;

  sect = (GumSection64 *) (seg + 1);
  strcpy (sect->sectname, "__text");
  strcpy (sect->segname, seg->segname);
  sect->addr = seg->vmaddr;
  sect->size = seg->vmsize;
  sect->offset = seg->fileoff;
  sect->align = 2;
  sect->reloff = 0;
  sect->nreloc = 0;
  sect->flags = GUM_S_ATTR_PURE_INSTRUCTIONS | GUM_S_ATTR_SOME_INSTRUCTIONS;
  sect->reserved1 = 0;
  sect->reserved2 = 0;
  sect->reserved3 = 0;

  fwrite (commands, header.sizeofcmds, 1, output);

  end_of_load_commands = (const guint8 *)
      ((const GumMachHeader64 *) input + 1) + size_of_load_commands;
  /* TODO: shift __TEXT if there's not enough space for our load commands. */
  gap_space_used = header.sizeofcmds - size_of_load_commands;
  rest_of_gap = (const guint8 *) end_of_load_commands + gap_space_used;
  fwrite (rest_of_gap, layout.linkedit_offset -
      ((const guint8 *) rest_of_gap - (const guint8 *) input), 1, output);

  code = g_malloc0 (layout.linkedit_shift); /* TODO */
  gum_arm64_writer_init (&cw, code);
  cw.pc = sect->addr;
  gum_arm64_writer_put_ldr_reg_address (&cw, ARM64_REG_X0, 42);
  gum_arm64_writer_put_ret (&cw);
  gum_arm64_writer_clear (&cw);
  fwrite (code, layout.linkedit_shift, 1, output);

  fwrite ((const guint8 *) input + layout.linkedit_offset,
      layout.linkedit_size, 1, output);

  success = TRUE;
  goto beach;

invalid_data:
  {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
        "Invalid Mach-O image");
    goto beach;
  }
beach:
  {
    g_free (code);
    g_free (commands);
    g_clear_pointer (&output, fclose);
    g_clear_object (&module);

    return success;
  }
}
