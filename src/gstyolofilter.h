#pragma once

#include "YoloObjectDetector.h"

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <string>
#include <memory>

G_BEGIN_DECLS

#define GST_TYPE_YOLOFILTER   (gst_yolofilter_get_type())
#define GST_YOLOFILTER(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_YOLOFILTER,GstYolofilter))
#define GST_YOLOFILTER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_YOLOFILTER,GstYolofilterClass))
#define GST_IS_YOLOFILTER(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_YOLOFILTER))
#define GST_IS_YOLOFILTER_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_YOLOFILTER))

typedef struct _GstYolofilter GstYolofilter;
typedef struct _GstYolofilterClass GstYolofilterClass;

struct _GstYolofilter {
    GstVideoFilter base_yolofilter{};

    std::unique_ptr<YoloObjectDetector> objectDetector{};

    bool draw{true};
    std::string configPath{};
    std::string weightsPath{};
    std::string classesPath{};
};

struct _GstYolofilterClass {
    GstVideoFilterClass base_yolofilter_class{};
};

GType gst_yolofilter_get_type(void);

G_END_DECLS
