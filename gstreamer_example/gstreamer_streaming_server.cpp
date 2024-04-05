
#include "gstreamer_streaming_server.h"

#include "Tools.hpp"

static std::vector<std::string> loaded_images;
static guint frames_encoded;
static CustomData mdata;

gstreamer_streaming_server::gstreamer_streaming_server(int argc, char* argv[])
{
    Init(argc, argv);
}

gstreamer_streaming_server::~gstreamer_streaming_server()
{

}

GstFlowReturn gstreamer_streaming_server::on_new_buffer(GstAppSink* sink, gpointer data)
{
    g_print("on_new_buffer !!! \n");
    return GST_FLOW_OK;

}

void gstreamer_streaming_server::Init(int argc, char* argv[])
{
    gst_init(&argc, &argv);

    frames_encoded = 0;
    loaded_images = Tools::getAndLoadFiles("images_test/");

    mdata.pipeline = gst_pipeline_new("image-to-video-pipeline");
    mdata.appsrc = gst_element_factory_make("appsrc", "source");
    mdata.decoder = gst_element_factory_make("jpegdec", "decoder");
    mdata.converter = gst_element_factory_make("videoconvert", "converter");
    mdata.encoder = gst_element_factory_make("x264enc", "encoder");

    // le type de muxer est super important ! , c'est lui qui définie quand le "pull-sample" va etre appeler car il définie quand le fichier est "utilisable"
    // par exemple le format mp4 n'est utilisable que quand le fihcier est complet (quand il recoit EOS)
    // donc il ne peut pas etre streamer
    mdata.muxer = gst_element_factory_make("mpegtsmux", "muxer");
    mdata.appsink = gst_element_factory_make("appsink", "sink");

    if (!mdata.pipeline || !mdata.appsrc || !mdata.decoder || !mdata.converter || !mdata.encoder || !mdata.muxer || !mdata.appsink) {
        g_printerr("Not all elements could be created.\n");
        return;
    }

    GstCaps* caps = gst_caps_new_simple("image/jpeg",
        "width", G_TYPE_INT, WIDTH,
        "height", G_TYPE_INT, HEIGHT,
        "framerate", GST_TYPE_FRACTION, 2, 1,
        NULL);
    g_object_set(mdata.appsrc, "caps", caps, "format", GST_FORMAT_TIME, NULL);
    gst_caps_unref(caps);


    gst_bin_add_many(GST_BIN(mdata.pipeline), mdata.appsrc, mdata.decoder, mdata.converter, mdata.encoder, mdata.muxer, mdata.appsink, NULL);
    if (!gst_element_link_many(mdata.appsrc, mdata.decoder, mdata.converter, mdata.encoder, mdata.muxer, mdata.appsink, NULL))
    {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(mdata.pipeline);
        throw std::runtime_error("Could not link object.\n");
    }


    g_object_set(G_OBJECT(mdata.appsink), "emit-signals", TRUE, NULL);
    g_signal_connect(mdata.appsink, "new-sample", G_CALLBACK(gstreamer_streaming_server::on_new_sample), &mdata);

    g_signal_connect(mdata.appsrc, "need-data", G_CALLBACK(gstreamer_streaming_server::on_new_data), &mdata);
    g_signal_connect(mdata.appsrc, "enough-data", G_CALLBACK(on_stop_feed), NULL);

    g_signal_connect(mdata.appsink, "new-buffer", G_CALLBACK(on_new_buffer), NULL);

    g_print("Init done.\n");
}

std::string gstreamer_streaming_server::getImage()
{
    //g_print("getImage().\n");
    return loaded_images[frames_encoded % loaded_images.size()];
}

static int nbr_call_new_data = 0;
static int nbr_call_new_data_total = 0;

GstFlowReturn  gstreamer_streaming_server::on_new_data(GstElement* appsrc, guint size, gpointer user_data)
{
    std::string image_data = getImage();
    GstBuffer* buffer;
    GstMapInfo map;

    nbr_call_new_data_total += image_data.size();
    g_print("need_data(%d) :%d - total : %d \n", ++nbr_call_new_data, frames_encoded, nbr_call_new_data_total);

    buffer = gst_buffer_new_allocate(NULL, image_data.size(), NULL);
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    memcpy(map.data, image_data.data(), image_data.size());

    GstFlowReturn ret;
    g_signal_emit_by_name(mdata.appsrc, "push-buffer", buffer, &ret);
    if (ret != GST_FLOW_OK) {
        g_print("!!!!ERROR !!! \n");
    }

    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref(buffer);

    //gst_app_src_push_buffer(GST_APP_SRC(mdata.appsrc), buffer);

    frames_encoded++;
    return GST_FLOW_OK;
}

void gstreamer_streaming_server::on_stop_feed(GstElement* source)
{
    g_print("on_stop_feed()\n");

}


static int nbr_call_new_sample = 0;
static int nbr_call_new_sample_total = 0;

GstFlowReturn gstreamer_streaming_server::on_new_sample(GstAppSink* sink, gpointer data)
{
    g_print("non_new_sample(%d).\n", ++nbr_call_new_sample);
        GstSample* sample;
        g_signal_emit_by_name(sink, "pull-sample", &sample);
        if (sample) {
            GstBuffer* buffer = gst_sample_get_buffer(sample);
            GstMapInfo map;
            gst_buffer_map(buffer, &map, GST_MAP_READ);

            std::string buffer_str((char*)map.data, map.size);
            nbr_call_new_sample_total += map.size;
            g_print("non_new_sample sending: %d.\n", ++nbr_call_new_sample_total);
            
           // g_print("Buffer_ready_to_send sending.\n");
            //sendMessage(buffer_str);

            gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);
        }
        else {
            //g_print("sample was null.\n");
            return GST_FLOW_ERROR;
        }
    
    return GST_FLOW_OK;
}

void gstreamer_streaming_server::on_eos(GstAppSink* appsink, gpointer user_data)
{
    g_print("ON EOS-) \n");
    exit(1);
}

gboolean gstreamer_streaming_server::stop_main_loop(gpointer data) {
    GMainLoop* loop = (GMainLoop*)data;
    g_main_loop_quit(loop);
    g_print("stop_main_loop() Timeout reached, stopping main loop...\n");
    return G_SOURCE_REMOVE; // Do not call again
}

int gstreamer_streaming_server::StartPlaying()
{
    g_print("StartPlaying.\n");
    mdata.main_loop = g_main_loop_new(NULL, FALSE);
    if (!mdata.pipeline) {
        return -1;  // Initialization failed
    }

    // Set the pipeline to the PLAYING state
    gst_element_set_state(mdata.pipeline, GST_STATE_PLAYING);
    //g_timeout_add_seconds(10, (GSourceFunc)stop_main_loop, mdata.main_loop);

    g_print("main_loop.\n");
    // Create and run the main loop
    g_main_loop_run(mdata.main_loop);

    g_print("loop ended.\n");
    // Clean up on exit
    gst_element_set_state(mdata.pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(mdata.pipeline));
    g_main_loop_unref(mdata.main_loop);
}
