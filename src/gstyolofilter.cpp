#include "gstyolofilter.h"

#include "YoloObjectDetector.h"

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <cstdint>
#include <memory>

GST_DEBUG_CATEGORY_STATIC (gst_yolofilter_debug_category);
#define GST_CAT_DEFAULT gst_yolofilter_debug_category

/* prototypes */

int openCvInterface(int height, int width, uint8_t *i, uint8_t *o);

static void gst_yolofilter_set_property(GObject *object,
                                          guint property_id, const GValue *value, GParamSpec *pspec);

static void gst_yolofilter_get_property(GObject *object,
                                          guint property_id, GValue *value, GParamSpec *pspec);

static void gst_yolofilter_dispose(GObject *object);

static void gst_yolofilter_finalize(GObject *object);

static gboolean gst_yolofilter_start(GstBaseTransform *trans);

static gboolean gst_yolofilter_stop(GstBaseTransform *trans);

static gboolean gst_yolofilter_set_info(GstVideoFilter *filter, GstCaps *incaps,
                                          GstVideoInfo *in_info, GstCaps *outcaps, GstVideoInfo *out_info);

static GstFlowReturn gst_yolofilter_transform_frame(GstVideoFilter *filter,
                                                      GstVideoFrame *inframe, GstVideoFrame *outframe);

static GstFlowReturn gst_yolofilter_transform_frame_ip(GstVideoFilter *filter,
                                                         GstVideoFrame *frame);

enum {
    PROP_0,
    PROP_DRAW,
    PROP_CONFIG,
    PROP_WEIGHTS,
    PROP_CLASSES,
};

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstYolofilter, gst_yolofilter, GST_TYPE_VIDEO_FILTER,
                         GST_DEBUG_CATEGORY_INIT(gst_yolofilter_debug_category, "yolofilter", 0,
                                                 "debug category for yolofilter element"));

static void gst_yolofilter_class_init(GstYolofilterClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
    GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(VIDEO_SRC_CAPS)));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(VIDEO_SINK_CAPS)));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
        "yolofilter", "Generic", "Object Detector (YOLOv3) Video Filter", "Andrei Litvintsev");

    gobject_class->set_property = gst_yolofilter_set_property;
    gobject_class->get_property = gst_yolofilter_get_property;
    gobject_class->dispose = gst_yolofilter_dispose;
    gobject_class->finalize = gst_yolofilter_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR (gst_yolofilter_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_yolofilter_stop);
    video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_yolofilter_set_info);
    video_filter_class->transform_frame = GST_DEBUG_FUNCPTR (gst_yolofilter_transform_frame);
    video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_yolofilter_transform_frame_ip);

    auto* drawParamSpec = g_param_spec_boolean(
        "draw",
        "Draw",
        "Need to draw detections or not",
        true,
        static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );
    g_object_class_install_property(gobject_class, PROP_DRAW, drawParamSpec);

    auto* configParamSpec = g_param_spec_string(
        "cfg",
        "Config",
        "Config (model) file path",
        "",
        static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );
    g_object_class_install_property(gobject_class, PROP_CONFIG, configParamSpec);

    auto* weightsParamSpec = g_param_spec_string(
            "weights",
            "Weights",
            "Weights (model) file path",
            "",
            static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );
    g_object_class_install_property(gobject_class, PROP_WEIGHTS, weightsParamSpec);

    auto* classesParamSpec = g_param_spec_string(
            "classes",
            "Classes",
            "Classes (model) file path",
            "",
            static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );
    g_object_class_install_property(gobject_class, PROP_CLASSES, classesParamSpec);
}

static void gst_yolofilter_init(GstYolofilter *yolofilter) {
}

void gst_yolofilter_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (object);

    GST_DEBUG_OBJECT (yolofilter, "set_property");

    switch (property_id) {
        case PROP_DRAW:
            yolofilter->draw = g_value_get_boolean(value);
            break;
        case PROP_CONFIG:
            yolofilter->configPath = g_value_get_string(value);
            break;
        case PROP_WEIGHTS:
            yolofilter->weightsPath = g_value_get_string(value);
            break;
        case PROP_CLASSES:
            yolofilter->classesPath = g_value_get_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

void gst_yolofilter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (object);

    GST_DEBUG_OBJECT (yolofilter, "get_property");

    switch (property_id) {
        case PROP_DRAW:
            g_value_set_boolean(value, yolofilter->draw);
            break;
        case PROP_CONFIG:
            g_value_set_string(value, yolofilter->configPath.c_str());
            break;
        case PROP_WEIGHTS:
            g_value_set_string(value, yolofilter->weightsPath.c_str());
            break;
        case PROP_CLASSES:
            g_value_set_string(value, yolofilter->classesPath.c_str());
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

void gst_yolofilter_dispose(GObject *object) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (object);

    GST_DEBUG_OBJECT (yolofilter, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS (gst_yolofilter_parent_class)->dispose(object);
}

void gst_yolofilter_finalize(GObject *object) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (object);

    GST_DEBUG_OBJECT (yolofilter, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS (gst_yolofilter_parent_class)->finalize(object);
}

static gboolean gst_yolofilter_start(GstBaseTransform *trans) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (trans);

    GST_DEBUG_OBJECT (yolofilter, "start");

    yolofilter->objectDetector = std::make_unique<YoloObjectDetector>(
            yolofilter->configPath, yolofilter->weightsPath, yolofilter->classesPath, 0.2f, 0.4f);

    return TRUE;
}

static gboolean gst_yolofilter_stop(GstBaseTransform *trans) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (trans);

    GST_DEBUG_OBJECT (yolofilter, "stop");

    yolofilter->objectDetector.reset();

    return TRUE;
}

static gboolean gst_yolofilter_set_info(GstVideoFilter *filter, GstCaps *incaps,
                                          GstVideoInfo *in_info, GstCaps *outcaps, GstVideoInfo *out_info) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (filter);

    GST_DEBUG_OBJECT (yolofilter, "set_info");

    return TRUE;
}

/* transform */
static GstFlowReturn gst_yolofilter_transform_frame(GstVideoFilter *filter, GstVideoFrame *inframe,
                                                      GstVideoFrame *outframe) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (filter);

    guint8* inputFrameData{static_cast<guint8*>(GST_VIDEO_FRAME_PLANE_DATA (inframe, 0))};
    guint8* outputFrameData{static_cast<guint8*>(GST_VIDEO_FRAME_PLANE_DATA (outframe, 0))};
    gint width{GST_VIDEO_FRAME_COMP_WIDTH(inframe, 0)};
    gint height{GST_VIDEO_FRAME_COMP_HEIGHT(inframe, 0)};

    cv::Mat img{height, width, CV_8UC3, inputFrameData};
    cv::Mat dst{height, width, CV_8UC3, outputFrameData};

    if (!yolofilter->objectDetector)
        return GST_FLOW_ERROR;

    yolofilter->objectDetector->HandleFrame(img, yolofilter->draw).copyTo(dst);

    return GST_FLOW_OK;
}

static GstFlowReturn gst_yolofilter_transform_frame_ip(GstVideoFilter *filter, GstVideoFrame *frame) {
    GstYolofilter *yolofilter = GST_YOLOFILTER (filter);

    GST_DEBUG_OBJECT (yolofilter, "transform_frame_ip");

    return GST_FLOW_OK;
}

static gboolean plugin_init(GstPlugin *plugin) {
    /* FIXME Remember to set the rank if it's an element that is meant
       to be autoplugged by decodebin. */
    return gst_element_register(plugin, "yolofilter", GST_RANK_NONE,
                                GST_TYPE_YOLOFILTER);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "yolofilter"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "empty"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "empty"
#endif

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    yolofilter,
    "Object Detector (YOLOv3) Video Filter",
    plugin_init,
    VERSION,
    "MIT",
    PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)

