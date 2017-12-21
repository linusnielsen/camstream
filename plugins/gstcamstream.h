/* GStreamer
 * Copyright (C) 2017 FIXME <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_CAMSTREAM_H_
#define _GST_CAMSTREAM_H_

#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

#define GST_TYPE_CAMSTREAM   (gst_camstream_get_type())
#define GST_CAMSTREAM(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CAMSTREAM,GstCamstream))
#define GST_CAMSTREAM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CAMSTREAM,GstCamstreamClass))
#define GST_IS_CAMSTREAM(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CAMSTREAM))
#define GST_IS_CAMSTREAM_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CAMSTREAM))

typedef struct _GstCamstream GstCamstream;
typedef struct _GstCamstreamClass GstCamstreamClass;

struct _GstCamstream
{
  GstPushSrc base_camstream;

  gint bufsize;
  gint64 n_frames;                      /* total frames sent */
};

struct _GstCamstreamClass
{
  GstPushSrcClass parent_class;

};

GType gst_camstream_get_type (void);

G_END_DECLS

#endif
