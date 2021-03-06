/*
 * Copyright (C) 2009-2020 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "fakeeventsink.h"

static void gum_fake_event_sink_iface_init (gpointer g_iface,
    gpointer iface_data);
static void gum_fake_event_sink_finalize (GObject * obj);
static GumEventType gum_fake_event_sink_query_mask (GumEventSink * sink);
static void gum_fake_event_sink_process (GumEventSink * sink,
    const GumEvent * event, GumCpuContext * cpu_context);

G_DEFINE_TYPE_EXTENDED (GumFakeEventSink,
                        gum_fake_event_sink,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (GUM_TYPE_EVENT_SINK,
                                               gum_fake_event_sink_iface_init))

static void
gum_fake_event_sink_class_init (GumFakeEventSinkClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gum_fake_event_sink_finalize;
}

static void
gum_fake_event_sink_iface_init (gpointer g_iface,
                                gpointer iface_data)
{
  GumEventSinkInterface * iface = g_iface;

  iface->query_mask = gum_fake_event_sink_query_mask;
  iface->process = gum_fake_event_sink_process;
}

static void
gum_fake_event_sink_init (GumFakeEventSink * self)
{
  self->events = g_array_sized_new (FALSE, FALSE, sizeof (GumEvent), 16384);
}

static void
gum_fake_event_sink_finalize (GObject * obj)
{
  GumFakeEventSink * self = GUM_FAKE_EVENT_SINK (obj);

  g_array_free (self->events, TRUE);

  G_OBJECT_CLASS (gum_fake_event_sink_parent_class)->finalize (obj);
}

GumEventSink *
gum_fake_event_sink_new (void)
{
  GumFakeEventSink * sink;

  sink = g_object_new (GUM_TYPE_FAKE_EVENT_SINK, NULL);

  return GUM_EVENT_SINK (sink);
}

void
gum_fake_event_sink_reset (GumFakeEventSink * self)
{
  self->mask = 0;
  g_array_set_size (self->events, 0);
}

const GumCallEvent *
gum_fake_event_sink_get_nth_event_as_call (GumFakeEventSink * self,
                                           guint n)
{
  const GumEvent * ev;

  ev = &g_array_index (self->events, GumEvent, n);
  g_assert_cmpint (ev->type, ==, GUM_CALL);
  return &ev->call;
}

const GumRetEvent *
gum_fake_event_sink_get_nth_event_as_ret (GumFakeEventSink * self,
                                          guint n)
{
  const GumEvent * ev;

  ev = &g_array_index (self->events, GumEvent, n);
  g_assert_cmpint (ev->type, ==, GUM_RET);
  return &ev->ret;
}

const GumExecEvent *
gum_fake_event_sink_get_nth_event_as_exec (GumFakeEventSink * self,
                                           guint n)
{
  const GumEvent * ev;

  ev = &g_array_index (self->events, GumEvent, n);
  g_assert_cmpint (ev->type, ==, GUM_EXEC);
  return &ev->exec;
}

void
gum_fake_event_sink_dump (GumFakeEventSink * self)
{
  guint i;

  g_print ("%u events\n", self->events->len);

  for (i = 0; i < self->events->len; i++)
  {
    GumEvent * ev = &g_array_index (self->events, GumEvent, i);

    switch (ev->type)
    {
      case GUM_EXEC:
        g_print ("GUM_EXEC at %p\n", ev->exec.location);
        break;
      case GUM_CALL:
        g_print ("GUM_CALL at %p, target=%p\n", ev->call.location,
            ev->call.target);
        break;
      case GUM_RET:
        g_print ("GUM_RET at %p, target=%p\n", ev->ret.location,
            ev->ret.target);
        break;
      default:
        g_print ("UNKNOWN EVENT\n");
        break;
    }
  }
}

static GumEventType
gum_fake_event_sink_query_mask (GumEventSink * sink)
{
  GumFakeEventSink * self = GUM_FAKE_EVENT_SINK (sink);

  return self->mask;
}

static void
gum_fake_event_sink_process (GumEventSink * sink,
                             const GumEvent * event,
                             GumCpuContext * cpu_context)
{
  GumFakeEventSink * self = GUM_FAKE_EVENT_SINK (sink);

  g_array_append_val (self->events, *event);
}
