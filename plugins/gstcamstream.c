/* GStreamer
 * Copyright (C) 2017 Linus Nielsen <linus@haxx.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstcamstream
 * @title: camstream
 *
 * The camstream element grabs raw RGB frames from the ARP FPGA and pushes them
 * to the stream.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 camstream ! videoconvert ! autovideosink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/video.h>

#include "gstcamstream.h"

GST_DEBUG_CATEGORY_STATIC (gst_camstream_debug_category);
#define GST_CAT_DEFAULT gst_camstream_debug_category

/* prototypes */
static void gst_camstream_dispose (GObject * object);
static void gst_camstream_finalize (GObject * object);

static gboolean gst_camstream_is_seekable (GstBaseSrc * src);
static gboolean gst_camstream_start (GstBaseSrc * src);
static gboolean gst_camstream_stop (GstBaseSrc * src);
static GstCaps *gst_camstream_get_caps (GstBaseSrc * src, GstCaps * filter);
static gboolean gst_camstream_set_caps (GstBaseSrc * src, GstCaps * caps);

static GstFlowReturn gst_camstream_create (GstPushSrc * src,
    GstBuffer ** buf);

static void gst_camstream_reset (GstCamstream * src);

#define WIDTH  640
#define HEIGHT 480

/* pad templates */

static GstStaticPadTemplate gst_camstream_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "video/x-raw, "
        "format = (string)RGB, "
        "width = (int)640, "
        "height = (int)480, "
        "framerate = (fraction)30/1")
    );

/* class initialization */
#define gst_camstream_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstCamstream, gst_camstream, GST_TYPE_PUSH_SRC,
    GST_DEBUG_CATEGORY_INIT (gst_camstream_debug_category, "camstream", 0,
        "debug category for camstream element"));

static void
gst_camstream_class_init (GstCamstreamClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstBaseSrcClass *gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *gstpushsrc_class = GST_PUSH_SRC_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "camstream", 0,
      "ARP external camera");

  gobject_class->dispose = gst_camstream_dispose;
  gobject_class->finalize = gst_camstream_finalize;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_camstream_src_template));

  gst_element_class_set_static_metadata (gstelement_class,
      "ARP external camera Video Source", "Source/Video",
      "ARP external camera video source", "Linus Nielsen <linus@haxx.se>");

  gstbasesrc_class->is_seekable = gst_camstream_is_seekable;
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_camstream_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_camstream_stop);
  gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_camstream_get_caps);
  gstbasesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_camstream_set_caps);

  gstpushsrc_class->create = GST_DEBUG_FUNCPTR (gst_camstream_create);

  /* Install GObject properties */
}

static void
gst_camstream_init (GstCamstream * src)
{
  /* set source as live (no preroll) */
  gst_base_src_set_live (GST_BASE_SRC (src), TRUE);

  /* override default of BYTES to operate in time mode */
//  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);

  gst_camstream_reset (src);
}

static void
gst_camstream_reset (GstCamstream * src)
{
  src->bufsize = WIDTH * HEIGHT * 3;
}

void
gst_camstream_dispose (GObject * object)
{
  GstCamstream *src;

  g_return_if_fail (GST_IS_CAMSTREAM (object));
  src = GST_CAMSTREAM (object);

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_camstream_parent_class)->dispose (object);
}

void
gst_camstream_finalize (GObject * object)
{
  GstCamstream *src;

  g_return_if_fail (GST_IS_CAMSTREAM (object));
  src = GST_CAMSTREAM (object);

  /* clean up object here */

  G_OBJECT_CLASS (gst_camstream_parent_class)->finalize (object);
}

static gboolean
gst_camstream_start (GstBaseSrc * bsrc)
{
  GstCamstream *src = GST_CAMSTREAM (bsrc);

  GST_INFO_OBJECT (src, "start");

  return TRUE;
}

static gboolean
gst_camstream_stop (GstBaseSrc * bsrc)
{
  GstCamstream *src = GST_CAMSTREAM (bsrc);

  GST_INFO_OBJECT (src, "stop");

  gst_camstream_reset (src);

  return TRUE;
}

static GstCaps *
gst_camstream_get_caps (GstBaseSrc * bsrc, GstCaps * filter)
{
  GstCamstream *src = GST_CAMSTREAM (bsrc);

  GST_INFO_OBJECT (src, "get_caps");

  return gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      "width", G_TYPE_INT, WIDTH,
      "height", G_TYPE_INT, HEIGHT,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      NULL);
}

static gboolean
gst_camstream_set_caps (GstBaseSrc * bsrc, GstCaps * caps)
{
  GstCamstream *src = GST_CAMSTREAM (bsrc);

  GST_INFO_OBJECT (src, "set_caps");

  return TRUE;
}

static GstFlowReturn
gst_camstream_create (GstPushSrc * psrc, GstBuffer ** buf)
{
  GstCamstream *src = GST_CAMSTREAM (psrc);
  GstMapInfo minfo;
  guint8 *image;
  int x, y;
  int r, g, b;

  int shift;

  GST_INFO_OBJECT (src, "create");

  r = g = b = 0;

  shift = src->n_frames % HEIGHT;

  /* TODO: use allocator */
  *buf = gst_buffer_new_and_alloc (WIDTH * HEIGHT * 3);

  /* Copy image to buffer from surface TODO: use orc_memcpy */
  gst_buffer_map (*buf, &minfo, GST_MAP_WRITE);

  image = minfo.data;

  for(y = 0;y < HEIGHT;y++) {
      r = (y / 2 + shift) % 256;
      g = (y / 2 + shift + HEIGHT/3) % 256;
      b = (y / 2 + shift + 2*(HEIGHT/3)) % 256;

      for(x = 0;x < WIDTH;x++) {
          image[y * WIDTH * 3 + x * 3] = r;
          image[y * WIDTH * 3 + x * 3 + 1] = g;
          image[y * WIDTH * 3 + x * 3 + 2] = b;
      }
  }

  gst_buffer_unmap (*buf, &minfo);

  g_usleep(1000000/30);

  src->n_frames++;

  return GST_FLOW_OK;
}

static gboolean
gst_camstream_is_seekable (GstBaseSrc * src)
{
  /* we're not seekable... */
  return FALSE;
}
