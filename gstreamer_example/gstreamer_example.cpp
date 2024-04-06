
#include <vector>
#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <exception>
#include <cstdio>

#include <chrono>
#include <iostream>
#include <thread>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include "gstreamer_streaming_server.h"
#include "gstreamer_client_receiver.h"
#include "short_cutting_pipeline_ex3.h"

#include "Tools.hpp"

#define FPS_VALUE 30

int reproduced_command_line_jpg_to_video(int ac, char* av[])
{
        gst_init(&ac, &av);

        GstElement* pipeline, * source, * decoder, * convert, * queue1, * encoder, * queue2, * muxer, * sink;

        pipeline = gst_pipeline_new("image-to-video-pipeline");
        source = gst_element_factory_make("multifilesrc", "source");
        decoder = gst_element_factory_make("jpegdec", "decoder");
        convert = gst_element_factory_make("videoconvert", "convert");
        queue1 = gst_element_factory_make("queue", "queue1");
        encoder = gst_element_factory_make("x264enc", "encoder");
        queue2 = gst_element_factory_make("queue", "queue2");
        muxer = gst_element_factory_make("mp4mux", "muxer");
        sink = gst_element_factory_make("filesink", "sink");

        if (!pipeline || !source || !decoder || !convert || !queue1 || !encoder || !queue2 || !muxer || !sink) {
            g_error("Failed to create elements.");
            return -1;
        }

        // Set properties
        g_object_set(source, "location", "images_test/%d.jpg", "index", 1, NULL);
        g_object_set(sink, "location", "images_test/vs_image.mp4", NULL);

        // Add elements to pipeline and link them
        gst_bin_add_many(GST_BIN(pipeline), source, decoder, convert, queue1, encoder, queue2, muxer, sink, NULL);
        if (!gst_element_link_many(source, decoder, convert, queue1, encoder, queue2, muxer, sink, NULL)) {
            g_error("Elements could not be linked.");
            gst_object_unref(pipeline);
            return -1;
        }

        // Start the pipeline
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        // Wait for EOS or error
        GstBus* bus = gst_element_get_bus(pipeline);
        GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (msg != NULL) {
            gst_message_unref(msg);
        }

        gst_object_unref(bus);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);

        return 0;
    
}

#include <chrono>
#include <iostream>
#include <thread>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
int test_file_sink(int argc, char* argv[])
{
    GstElement* pipeline, * source, * sink;
    GMainLoop* loop;

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create elements
    source = gst_element_factory_make("videotestsrc", "source");
    sink = gst_element_factory_make("filesink", "sink");

    if (!source || !sink) {
        printf("Not all elements could be created.\n");
        return -1;
    }

    // Set filesink properties
    g_object_set(sink, "location", "test_output.raw", NULL);

    // Create the empty pipeline
    pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline) {
        printf("Pipeline could not be created.\n");
        return -1;
    }

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    if (gst_element_link(source, sink) != TRUE) {
        printf("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_print("start playing\n");
    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_print("after playing\n");
    // Wait until error or EOS
    loop = g_main_loop_new(NULL, FALSE);

    g_print("after mainloop\n");
    g_main_loop_run(loop);

    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
static void on_decoder_pad_added(GstElement* element, GstPad* pad, gpointer data) {
    GstPad* sinkpad;
    GstElement* sink = (GstElement*)data;

    /* We can now link this pad with the appsink sink pad */
    g_print("Dynamic pad created, linking uridecodebin/sink\n");

    sinkpad = gst_element_get_static_pad(sink, "sink");

    GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
    if (GST_PAD_LINK_FAILED(ret)) {
        g_printerr("Pad link failed.\n");
    }
    else {
        g_print("Link succeeded.\n");
    }

    gst_object_unref(sinkpad);
}

/* This is the bus call, checking for errors or the EOS message */
static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data) {
    GMainLoop* loop = (GMainLoop*)data;

    switch (GST_MESSAGE_TYPE(msg)) {

    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(loop);
        break;

    case GST_MESSAGE_ERROR: {
        gchar* debug;
        GError* error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        g_printerr("Error: %s\n", error->message);
        g_error_free(error);

        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

static gboolean stop_main_loop(gpointer data) {
    GMainLoop* loop = (GMainLoop*)data;
    g_main_loop_quit(loop);
    g_print("Timeout reached, stopping main loop...\n");
    return G_SOURCE_REMOVE; // Do not call again
}

int test_app_sink(int argc, char* argv[])
{
    GMainLoop* loop;
    GstElement* pipeline, * source, * sink;
    GstBus* bus;
    guint bus_watch_id;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create main event loop */
    loop = g_main_loop_new(NULL, FALSE);

    /* Create gstreamer elements */
    pipeline = gst_pipeline_new("video-player");
    source = gst_element_factory_make("uridecodebin", "network-source");
    sink = gst_element_factory_make("appsink", "video-output");

    if (!pipeline || !source || !sink) {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

    /* Set up the pipeline */

    // Set the URI to the Sintel trailer
    g_object_set(G_OBJECT(source), "uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);

    /* we add all elements into the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);

    /* uridecodebin will automatically link itself when it has determined the stream type */
    g_signal_connect(source, "pad-added", G_CALLBACK(on_decoder_pad_added), sink);

    /* Set up the bus */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_timeout_add_seconds(10, (GSourceFunc)stop_main_loop, loop);

    /* Iterate */
    g_print("Running...\n");
    g_main_loop_run(loop);

    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;
}

static std::vector<std::string> images;
static int image_pos = 0;

static void need_data_for_video_file(GstElement* appsrc, guint unused_size, gpointer user_data) {
    static gboolean white = FALSE;
    GstBuffer* buffer;
    GstFlowReturn ret;
    GstMapInfo map;

    // Only proceed if we have image data
    int pos = ((++image_pos) % images.size());
    printf("Need_data %d - %d \n", image_pos, images.size());

    if (!images[pos].empty())
    {
        printf("adding image\n");
        buffer = gst_buffer_new_allocate(NULL, images[pos].size(), NULL);
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        memcpy(map.data, images[pos].data(), images[pos].size());
        gst_buffer_unmap(buffer, &map);

        GST_BUFFER_PTS(buffer) = 33333333 * image_pos; // Timestamps in nanoseconds for a 30fps stream
        GST_BUFFER_DURATION(buffer) = 33333333; // Duration of each frame in nanoseconds for a 30fps stream


        // Push the buffer into appsrc
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        gst_buffer_unref(buffer);

        if (ret != GST_FLOW_OK) {
            // Something went wrong, stop pushing
            exit(1);
            // g_main_loop_quit(main_loop);
        }
    }
    if (pos > 20) {
        printf("ending stream\n");
        g_signal_emit_by_name(appsrc, "end-of-stream", &ret);
    }
}
int images_to_video_example(int argc, char* argv[]) {

    GstElement* pipeline, * appsrc, * convert, * sink, * decoder, * encoder, * mux;
    printf("loading images :) \n");
    images = Tools::getAndLoadFiles("images_test/");
    
    printf("loaded %d \n", images.size());

    gst_init(&argc, &argv);

    pipeline =  gst_pipeline_new("pipeline");
    appsrc =    gst_element_factory_make("appsrc", "source");
    convert =   gst_element_factory_make("videoconvert", "convert");
    decoder =   gst_element_factory_make("jpegdec", "decoder");

    encoder = gst_element_factory_make("avenc_mpeg4", "encoder");
    mux = gst_element_factory_make("mp4mux", "mux");

    printf("running\n");
    sink = gst_element_factory_make("filesink", "sink");

    sink =      gst_element_factory_make("autovideosink", "sink");


    if (!pipeline || !appsrc || !convert || !sink || !decoder) {
    //if (!pipeline || !appsrc || !convert || !sink || !decoder || !encoder || !mux) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }
    //!
    g_object_set(sink, "location", "output_vid.mp4", NULL);

    // Configure appsrc
    GstCaps* caps = gst_caps_new_simple("image/jpeg",
        "format", G_TYPE_STRING, "RGB",
        "width", G_TYPE_INT, WIDTH,
        "height", G_TYPE_INT, HEIGHT,
        "framerate", GST_TYPE_FRACTION, 2, 1,
        NULL);
    g_object_set(appsrc, "caps", caps, "format", GST_FORMAT_TIME, NULL);
    gst_caps_unref(caps);

    g_signal_connect(appsrc, "need-data", G_CALLBACK(need_data_for_video_file), NULL);

    gst_bin_add_many(GST_BIN(pipeline), appsrc, convert, sink, NULL);
    if (!gst_element_link_many(appsrc, convert, sink, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    printf("starting loop\n");
    // Wait until error or EOS
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    GMainLoop* main_loop = g_main_loop_new(NULL, FALSE);
    printf("running\n");
    g_main_loop_run(main_loop);

    printf("done\n");
    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_main_loop_unref(main_loop);

    return 0;
}
static void need_data(GstElement* appsrc, guint unused_size, gpointer user_data) {
    static gboolean white = FALSE;
    GstBuffer* buffer;
    GstFlowReturn ret;
    GstMapInfo map;

    // Only proceed if we have image data
    int pos = ((++image_pos) % images.size());
    g_print("Need_data %d - %d \n", image_pos, images.size());
    if (!images[pos].empty()) {
        buffer = gst_buffer_new_allocate(NULL, images[pos].size(), NULL);
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        memcpy(map.data, images[pos].data(), images[pos].size());
        gst_buffer_unmap(buffer, &map);

        // Push the buffer into appsrc
        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        gst_buffer_unref(buffer);

        if (ret != GST_FLOW_OK) {
            // Something went wrong, stop pushing
            exit(1);
            // g_main_loop_quit(main_loop);
        }
    }
}
int images_to_displayed_video_example(int argc, char* argv[]) {

    GstElement* pipeline, * appsrc, * convert, * sink, * decoder;

    images = Tools::getAndLoadFiles("images_test/");

    gst_init(&argc, &argv);

    pipeline = gst_pipeline_new("pipeline");
    appsrc = gst_element_factory_make("appsrc", "source");
    convert = gst_element_factory_make("videoconvert", "convert");
    sink = gst_element_factory_make("autovideosink", "sink");
    decoder = gst_element_factory_make("jpegdec", "decoder");


    if (!pipeline || !appsrc || !convert || !sink || !decoder) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    // Configure appsrc
    GstCaps* caps = gst_caps_new_simple("image/jpeg",
        "format", G_TYPE_STRING, "RGB",
        "width", G_TYPE_INT, WIDTH,
        "height", G_TYPE_INT, HEIGHT,
        "framerate", GST_TYPE_FRACTION, 2, 1,
        NULL);
    g_object_set(appsrc, "caps", caps, "format", GST_FORMAT_TIME, NULL);
    gst_caps_unref(caps);

    g_signal_connect(appsrc, "need-data", G_CALLBACK(need_data), NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), appsrc, decoder, convert, sink, NULL);
    gst_element_link_many(appsrc, decoder, convert, sink, NULL);

    //gst_bin_add_many(GST_BIN(pipeline), appsrc, convert, sink, NULL);
    //gst_element_link_many(appsrc, decoder, sink, NULL);

    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS
    GMainLoop* main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_main_loop_unref(main_loop);

    return 0;
}

static void on_pad_added(GstElement* element, GstPad* pad, gpointer data) {
    GstPad* sinkpad;
    GstElement* decoder = (GstElement*)data;

    sinkpad = gst_element_get_static_pad(decoder, "sink");
    gst_pad_link(pad, sinkpad);
    gst_object_unref(sinkpad);
}

int example_2_gstreamer_concepts_functionnal(int ac, char*av[])
{
    GstElement* pipeline, * source, * sink, *filter;
    GstBus* bus;
    GstMessage* msg;
    GstStateChangeReturn ret;

    /* Initialize GStreamer */
    gst_init(&ac, &av);

    /* Create the elements */
    // is a source element (it produces data), which creates a test video pattern.
    // This element is useful for debugging purposes (and tutorials) and is not usually found in real applications.
    source = gst_element_factory_make("videotestsrc", "source");
    // is a sink element (it consumes data), which displays on a window the images it receives. There exist several video sinks, depending on the operating system,
    //  with a varying range of capabilities.
    // autovideosink automatically selects and instantiates the best one, so you do not have to worry with the details, and your code is more platform-independent.
    sink = gst_element_factory_make("autovideosink", "sink");

    filter = gst_element_factory_make("vertigotv", "vertigo_effect");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline || !source || !sink || !filter) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);
    //if (gst_element_link(source, sink) != TRUE) {
    if (gst_element_link_many(source, filter, sink, NULL) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Modify the source's properties */
    g_object_set(source, "pattern", 0, NULL);

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Parse message */
    if (msg != NULL) {
        GError* err;
        gchar* debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n",
                GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n",
                debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
        default:
            /* We should not reach here because we only asked for ERRORs and EOS */
            g_printerr("Unexpected message received.\n");
            break;
        }
        gst_message_unref(msg);
    }

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

// add to environmùent GST_DEBUG=4 for debug
int example_1_hello_world_functionnal(int argc, char* argv[])
{
    GstElement* pipeline;
    GstBus* bus;
    GstMessage* msg;
    gst_init(&argc, &argv);
    pipeline = gst_element_factory_make("playbin", "pipeline");
    if (!pipeline) {
        g_error("Failed to create playbin element.");
        return -1;
    }
    g_object_set(G_OBJECT(pipeline), "uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* See next tutorial for proper error message handling/parsing */
    if (msg != nullptr && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        g_error("An error occurred! Re-run with the GST_DEBUG=*:WARN environment "
            "variable set for more details.");
    }

    /* Free resources */
    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

//5,*FACTORY*:4,*mpeg4*:5,*pipeline*:5
int main(int ac, char* av[])
{
    try {
     //example_1_hello_world_functionnal(ac, av);
     //example_2_gstreamer_concepts_functionnal(ac, av);

    short_cutting_pipeline_ex3 examples;
    //examples.example_3_short_cutting_the_pipeline(ac, av);
    // examples.example_3_Dynamic_Hello_world(ac, av);
    // examples.example_4_timed_management(ac, av);
    // examples.example_6_show_cap(ac, av);
    // examples.example_7_multithreading(ac, av);
    
    // examples.example_8_shortcutting_pipeline(ac, av);

    //images_to_displayed_video_example(ac, av);
    //images_to_video_example(ac, av);

    
    //test_file_sink(ac, av);

    //test_app_sink(ac, av);

    //images_to_video_example(ac, av);
    
   // reproduced_command_line_jpg_to_video(ac, av);

    //test.start_example_4(ac, av);


	 gstreamer_client_receiver* client = new gstreamer_client_receiver();
	 client->Init(ac, av);
     //client->start();

	 gstreamer_streaming_server streaming(ac, av, client);
     std::thread(&gstreamer_client_receiver::start, client).detach();
	 streaming.StartPlaying();


    }
    catch (...) {
        std::cout << "Unknow Error \n" << std::endl;
    }
}
