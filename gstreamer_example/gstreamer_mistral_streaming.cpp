#include "gstreamer_mistral_streaming.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/app.h>

#include "Tools.hpp"

#include <vector>
#include <string>

static GstElement* pipeline, * source, * encoder, * sink, * encoder2;
static guint image_width = 640;
static guint image_height = 480;
static guint frames_encoded = 0;

static std::vector<std::string> loaded_images;

gstreamer_mistral_streaming::gstreamer_mistral_streaming()
{
}

gstreamer_mistral_streaming::~gstreamer_mistral_streaming()
{
}

static std::string getImage()
{
    return loaded_images[++frames_encoded % loaded_images.size()];
}

static GstFlowReturn new_buffer(GstAppSrc* appsrc, guint unused_size) {
    g_print("new_buffer() \n");
    std::string image_data = getImage();
    if (image_data.empty()) {
        return GST_FLOW_EOS;
    }

    GstBuffer* buffer = gst_buffer_new_allocate(NULL, image_data.size(), NULL);
    GstMapInfo mapInfo;
    if (gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE)) {
        memcpy(mapInfo.data, image_data.data(), image_data.size());
        gst_buffer_unmap(buffer, &mapInfo);
    }
    else {
        g_warning("Failed to map buffer for writing");
        gst_buffer_unref(buffer);
        return GST_FLOW_ERROR;
    }

    GST_BUFFER_DURATION(buffer) = GST_SECOND / 30; // Assuming 30 FPS
    GST_BUFFER_PTS(buffer) = GST_BUFFER_DURATION(buffer) * frames_encoded;

    return gst_app_src_push_buffer(appsrc, buffer);
}

static void on_new_buffer(GstElement* appsink, guint, GstBuffer* buffer) {
    g_print("on_new_buffer \n");
    GstMapInfo mapInfo;
    if (gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        gsize buffer_size = mapInfo.size;
        guint8* data = mapInfo.data;

        std::string message(reinterpret_cast<char*>(data), buffer_size);
        // Replace sendMessage with your actual function for sending data
        //your_send_function(message);

        gst_buffer_unmap(buffer, &mapInfo);
    }
    else {
        g_warning("Failed to map buffer");
    }
}

static void create_pipeline() {

    g_print("Init Pipeline\n");
    pipeline = gst_pipeline_new("image_encoder");

    source = gst_element_factory_make("appsrc", "image_source");
    encoder = gst_element_factory_make("jpegdec", "image_decoder");
    encoder2 = gst_element_factory_make("x264enc", "video_encoder");
    sink = gst_element_factory_make("appsink", "video_sink");

    g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, NULL);
    g_object_set(G_OBJECT(source), "caps", gst_caps_new_simple("image/jpeg",
        "width", G_TYPE_INT, image_width,
        "height", G_TYPE_INT, image_height,
        NULL), NULL);

    sink = gst_element_factory_make("appsink", "video_sink");
    g_object_set(G_OBJECT(sink), "emit-signals", TRUE, NULL);
    g_signal_connect(sink, "new-buffer", G_CALLBACK(on_new_buffer), NULL);


    g_print("adding linking \n");

    gst_bin_add_many(GST_BIN(pipeline), source, encoder, encoder2, sink, NULL);
    gst_element_link_many(source, encoder, encoder2, sink, NULL);

    g_signal_connect(source, "need-data", G_CALLBACK(new_buffer), NULL);
}

int gstreamer_mistral_streaming::start(int ac, char* av[])
{
    gst_init(&ac, &av);

    loaded_images = Tools::getAndLoadFiles("images_test/");

    g_print("Init \n");
    create_pipeline();
    g_print("after init \n");

    GstStateChangeReturn ret;
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // No need for the while loop, as the "need-data" signal will handle buffer pushing
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_print("ending \n");

    ret = gst_element_set_state(pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the null state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    gst_object_unref(pipeline);
    g_print("after clean \n");
    return 0;
}
