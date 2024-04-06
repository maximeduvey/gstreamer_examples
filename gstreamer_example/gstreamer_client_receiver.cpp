
#include "gstreamer_client_receiver.h"


gstreamer_client_receiver::gstreamer_client_receiver()
{

}

gstreamer_client_receiver::~gstreamer_client_receiver()
{

}

void gstreamer_client_receiver::push_data()
{
    auto mapInfo = GetLastMapInfo();
    if (mapInfo.data != nullptr && mapInfo.size > 0) {
        GstBuffer* buffer = gst_buffer_new_allocate(NULL, mapInfo.size, NULL);
        gst_buffer_fill(buffer, 0, mapInfo.data, mapInfo.size);
        gst_app_src_push_buffer(GST_APP_SRC(_data.appsrc), buffer); // Takes ownership of the buffer
    }
    int ret;
    g_signal_emit_by_name(_data.appsrc, "push-buffer", buffer, &ret);

    //if (buffer.size() > 0) {
    //    /* Assuming your pipeline is set up to handle MPEG-TS H.264 data,
    //       directly push this buffer to the appsrc element */
    //    g_print("Loaded : %d .\n", size);
    //    GstBuffer* gstBuffer = gst_buffer_new_allocate(NULL, size, NULL);
    //    gst_buffer_fill(gstBuffer, 0, buffer.data(), size);
    //    gst_app_src_push_buffer(GST_APP_SRC(_data.appsrc), gstBuffer); // Takes ownership of the buffer
    //} 
    //else
    //{
    //    g_printerr("File read faile or empty %d \n", size);
    //}
}

static void on_pad_added(GstElement* src, GstPad* new_pad, GstElement* dest) {
    GstPad* sink_pad = gst_element_get_static_pad(dest, "sink");
    if (gst_pad_is_linked(sink_pad)) {
        g_object_unref(sink_pad);
        g_print("pad already linked.\n");
        return;
    }
    if (GST_PAD_LINK_SUCCESSFUL(gst_pad_link(new_pad, sink_pad))) {
        g_print("Demuxer pad linked.\n");
    }
    else {
        g_print("Failed to link demuxer pad.\n");
    }
    g_object_unref(sink_pad);
}

int gstreamer_client_receiver::Init(int ac, char *av[])
{
    g_print("Client init\n");
    gst_init(&ac, &av);

    // Create the elements
    _data.pipeline = gst_pipeline_new("video-display-pipeline");
    _data.appsrc = gst_element_factory_make("appsrc", "video-source");
    GstElement* demux = gst_element_factory_make("tsdemux", "ts-demuxer");
    _data.decoder = gst_element_factory_make("avdec_h264", "h264-decoder");
    //mdata.muxer = gst_element_factory_make("mpegtsmux", "muxer");
    _data.converter = gst_element_factory_make("videoconvert", "converter");
    _data.sink = gst_element_factory_make("autovideosink", "video-output");


    if (!_data.pipeline || !_data.appsrc || !demux || !_data.decoder || !_data.converter || !_data.sink) {
        std::cerr << "Not all elements could be created." << std::endl;
        return -1;
    }


    // Build the pipeline
    gst_bin_add_many(GST_BIN(_data.pipeline), _data.appsrc, demux, _data.decoder, _data.converter, _data.sink, NULL);
    
    if (!gst_element_link(_data.appsrc, demux)) {
        g_printerr("Failed to link appsrc to demux.\n");
    }
    
    if (!gst_element_link_many(_data.decoder, _data.converter, _data.sink, NULL)) {
        std::cerr << "Elements could not be linked." << std::endl;
        gst_object_unref(_data.pipeline);
        return -1;
    }

    gst_app_src_set_stream_type(GST_APP_SRC(_data.appsrc), GST_APP_STREAM_TYPE_STREAM);

    g_signal_connect(demux, "pad-added", G_CALLBACK(on_pad_added), _data.decoder);
    
    GstCaps* caps = gst_caps_new_simple("video/mpegts",
        "systemstream", G_TYPE_BOOLEAN, TRUE,
        "packetsize", G_TYPE_INT, 188,
        NULL);
  

    //"format", G_TYPE_STRING, "RGB",
    //"width", G_TYPE_INT, 240,
    //"height", G_TYPE_INT, 320,
    //"framerate", GST_TYPE_FRACTION, 2, 1,

    //GstCaps* caps = gst_caps_new_simple("video/x-h264",
    //    "stream-format", G_TYPE_STRING, "byte-stream",
    //    "alignment", G_TYPE_STRING, "nal",  // Try adding alignment if the stream is indeed nal aligned
    //    NULL);

    //GstCaps* caps = gst_caps_new_simple("video/x-h264",
    //    "stream-format", G_TYPE_STRING, "byte-stream",
    //    "alignment", G_TYPE_STRING, "au",
    //    "width", G_TYPE_INT, 1920,
    //    "height", G_TYPE_INT, 1080,
    //    "framerate", GST_TYPE_FRACTION, 30, 1,
    //    NULL);



    g_object_set(G_OBJECT(_data.appsrc), "caps", caps, "format", GST_FORMAT_TIME, NULL);
    // Set appsrc to treat the stream as live data
    g_object_set(G_OBJECT(_data.appsrc), "is-live", TRUE, NULL);
    
    gst_caps_unref(caps);


    g_print("Client inited\n");
}

bool gstreamer_client_receiver::check_error()
{
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_data.pipeline));
    gst_bus_add_watch(bus, [](GstBus* bus, GstMessage* msg, gpointer data) -> gboolean {
        GError* err;
        gchar* debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
            g_error_free(err);
            g_free(debug_info);
            break;
        default:
            break;
        }
        return TRUE;
    }, NULL);
    gst_object_unref(bus);
    return true;
}

int gstreamer_client_receiver::start()
{
    // Start playing
    gst_element_set_state(_data.pipeline, GST_STATE_PLAYING);
    check_error();

    if (true)
    {
        g_print("debug client \n");
        file = std::ifstream("test_video.ts", std::ios::binary | std::ios::ate);
        size = file.tellg();
        file.seekg(0, std::ios::beg);

        buffer = std::vector<char>(size);
        file.read(buffer.data(), size);
        push_data();

        auto main_loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(main_loop);

    }
    else {
        // Periodically push data into the pipeline
        bool displyed = false;
        while (_end != true)
        {
            if (GetSizeMapInfo() > 0)
            {
                push_data();
                if (displyed == false) {
                    g_print("Pushing data \n");
                    displyed = true;
                }
            }
            else
            {

                g_print("No data sleeping\n");
                displyed = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        // Clean up
        gst_element_set_state(_data.pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(_data.pipeline));
    }
    return 0;
}

