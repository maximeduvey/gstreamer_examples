
#include "short_cutting_pipeline_ex3.h"

#include <gst/audio/audio.h>
#include <string.h>


#define CHUNK_SIZE 1024   /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 44100 /* Samples per second we are sending */


int short_cutting_pipeline_ex3::example_3_Dynamic_Hello_world(int argc, char* argv[]) {
    ex3_audio_video_CustomData data;
    GstBus* bus;
    GstMessage* msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    // uridecodebin will internally instantiate all the necessary elements (sources, demuxers and decoders) to turn a URI into raw audio and/or video streams.
    // It does half the work that playbin does. Since it contains demuxers, its source pads are not initially available and we will need to link to them on the fly.
    data.source = gst_element_factory_make("uridecodebin", "source");
    // audioconvert is useful for converting between different audio formats, making sure that this example will work on any platform,
    // since the format produced by the audio decoder might not be the same that the audio sink expects.
    data.convert = gst_element_factory_make("audioconvert", "convert");
    // audioresample is useful for converting between different audio sample rates, similarly making sure that this example will work on any platform,
    // since the audio sample rate produced by the audio decoder might not be one that the audio sink supports.
    data.resample = gst_element_factory_make("audioresample", "resample");
    //The autoaudiosink is the equivalent of autovideosink seen in the previous tutorial, for audio. It will render the audio stream to the audio card.
    data.sink = gst_element_factory_make("autoaudiosink", "audioSink");

    data.videoConvert = gst_element_factory_make("videoconvert", "videoConvert");
    data.videoSink = gst_element_factory_make("autovideosink", "videoSink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");

    if (!data.pipeline || !data.source || !data.convert || !data.resample || !data.sink || !data.videoConvert || !data.videoSink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline. Note that we are NOT linking the source at this
     * point. We will do it later. */
    // Here we link the elements converter, resample and sink, but we DO NOT link them with the source, since at this point it contains no source pads.
    // We just leave this branch (converter + sink) unlinked, until later on.
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convert, data.resample, data.videoConvert, data.videoSink, data.sink,  NULL);
    if (!gst_element_link_many(data.convert, data.resample, data.sink,  NULL)) {
        g_printerr("Audio Elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    if (!gst_element_link_many(data.videoConvert, data.videoSink, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    /* Set the URI to play */
    g_object_set(data.source, "uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);

    /* Connect to the pad-added signal */
    g_signal_connect(data.source, "pad-added", G_CALLBACK(short_cutting_pipeline_ex3::pad_added_handler), &data);

    /* Start playing */
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus(data.pipeline);
    do {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        /* Parse message */
        if (msg != NULL) {
            GError* err;
            gchar* debug_info;

            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                terminate = TRUE;
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                terminate = TRUE;
                break;
            case GST_MESSAGE_STATE_CHANGED:
                /* We are only interested in state-changed messages from the pipeline */
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)) {
                    GstState old_state, new_state, pending_state;
                    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                    g_print("Pipeline state changed from %s to %s:\n",
                        gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                }
                break;
            default:
                /* We should not reach here */
                g_printerr("Unexpected message received.\n");
                break;
            }
            gst_message_unref(msg);
        }
    } while (!terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}

void short_cutting_pipeline_ex3::pad_exit(GstCaps* new_pad_caps, GstPad* audio_sink_pad, GstPad* video_sink_pad)
{
    g_printerr("exiting pad, unrefing.\n");
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref(new_pad_caps);

    if (audio_sink_pad != NULL)
        gst_object_unref(audio_sink_pad);

    if (video_sink_pad != NULL)
        gst_object_unref(audio_sink_pad);

    exit(1);
}

int short_cutting_pipeline_ex3::link_pad_with_cap(GstPad* new_pad, GstPad* pad, const gchar* type)
{
    auto ret = gst_pad_link(new_pad, pad);
    if (GST_PAD_LINK_FAILED(ret) ) {
        g_print("Type is '%s' but link failed.\n", type);
        return -1;
    }
    else {
        g_print("Link succeeded (type '%s').\n", type);
    }
    return 0;
}

/* This function will be called by the pad-added signal */
void short_cutting_pipeline_ex3::pad_added_handler(GstElement* src, GstPad* new_pad, short_cutting_pipeline_ex3::ex3_audio_video_CustomData* data)
{
    
    GstPadLinkReturn ret;
    GstCaps* new_pad_caps = NULL;
    GstStructure* new_pad_struct = NULL;
    const gchar* new_pad_type = NULL;

    g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    GstPad* audio_sink_pad = gst_element_get_static_pad(data->convert, "sink");
    if (gst_pad_is_linked(audio_sink_pad)) {
        g_print("We are already linked. Ignoring.\n");
        pad_exit(new_pad_caps, audio_sink_pad, NULL);
    }

    GstPad* video_sink_pad = gst_element_get_static_pad(data->videoConvert, "sink");
    if (gst_pad_is_linked(audio_sink_pad)) {
        g_print("We are already linked. Ignoring.\n");
        pad_exit(new_pad_caps, audio_sink_pad, video_sink_pad);
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);


    if (g_str_has_prefix(new_pad_type, "audio/x-raw")) {
        g_print("Audio raw pad.\n");
        if (link_pad_with_cap(new_pad, audio_sink_pad, new_pad_type) == -1)
            pad_exit(new_pad_caps, audio_sink_pad, video_sink_pad);
    }
    else if (g_str_has_prefix(new_pad_type, "video/x-raw"))
    {
        g_print("Video raw pad.\n");
        if (link_pad_with_cap(new_pad, video_sink_pad, new_pad_type) == -1)
            pad_exit(new_pad_caps, audio_sink_pad, video_sink_pad);
    }
    else {
        g_print("Pad was not raw audio nor raw video.\n");
        pad_exit(new_pad_caps, audio_sink_pad, video_sink_pad);
    }
}


short_cutting_pipeline_ex3::short_cutting_pipeline_ex3()
{
}

short_cutting_pipeline_ex3::~short_cutting_pipeline_ex3()
{
}

int short_cutting_pipeline_ex3::example_4_timed_management(int argc, char* argv[]) {

    ex4_timed_CustomData data;
    GstStateChangeReturn ret;
    GstMessage* msg;
    GstBus* bus;

    data.playing = FALSE;
    data.terminate = FALSE;
    data.seek_enabled = FALSE;
    data.seek_done = FALSE;
    data.duration = GST_CLOCK_TIME_NONE;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    data.playbin = gst_element_factory_make("playbin", "playbin");

    if (!data.playbin) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Set the URI to play */
    g_object_set(data.playbin, "uri", "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);

    /* Start playing */
    ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(data.playbin);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus(data.playbin);
    do {
        msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND, (GstMessageType)(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION));

        /* Parse message */
        if (msg != NULL) {
            handle_message(&data, msg);
        }
        else {
            /* We got no message, this means the timeout expired */
            if (data.playing) {
                gint64 current = -1;

                /* Query the current position of the stream */
                if (!gst_element_query_position(data.playbin, GST_FORMAT_TIME, &current)) {
                    g_printerr("Could not query current position.\n");
                }

                /* If we didn't know it yet, query the stream duration */
                if (!GST_CLOCK_TIME_IS_VALID(data.duration)) {
                    if (!gst_element_query_duration(data.playbin, GST_FORMAT_TIME, &data.duration)) {
                        g_printerr("Could not query current duration.\n");
                    }
                }

                /* Print current position and total duration */
                g_print("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
                    GST_TIME_ARGS(current), GST_TIME_ARGS(data.duration));

                /* If seeking is enabled, we have not done it yet, and the time is right, seek */
                if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND) {
                    g_print("\nReached 10s, performing seek...\n");
                    gst_element_seek_simple(data.playbin, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 30 * GST_SECOND);
                    data.seek_done = TRUE;
                }
            }
        }
    } while (!data.terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(data.playbin, GST_STATE_NULL);
    gst_object_unref(data.playbin);
    return 0;
}

void short_cutting_pipeline_ex3::handle_message(ex4_timed_CustomData* data, GstMessage* msg) {
    GError* err;
    gchar* debug_info;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);
        data->terminate = TRUE;
        break;
    case GST_MESSAGE_EOS:
        g_print("\nEnd-Of-Stream reached.\n");
        data->terminate = TRUE;
        break;
    case GST_MESSAGE_DURATION:
        /* The duration has changed, mark the current one as invalid */
        data->duration = GST_CLOCK_TIME_NONE;
        break;
    case GST_MESSAGE_STATE_CHANGED: {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin)) {
            g_print("Pipeline state changed from %s to %s:\n",
                gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));

            /* Remember whether we are in the PLAYING state or not */
            data->playing = (new_state == GST_STATE_PLAYING);

            if (data->playing) {
                /* We just moved to PLAYING. Check if seeking is possible */
                GstQuery* query;
                gint64 start, end;
                query = gst_query_new_seeking(GST_FORMAT_TIME);
                if (gst_element_query(data->playbin, query)) {
                    gst_query_parse_seeking(query, NULL, &data->seek_enabled, &start, &end);
                    if (data->seek_enabled) {
                        g_print("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n",
                            GST_TIME_ARGS(start), GST_TIME_ARGS(end));
                    }
                    else {
                        g_print("Seeking is DISABLED for this stream.\n");
                    }
                }
                else {
                    g_printerr("Seeking query failed.");
                }
                gst_query_unref(query);
            }
        }
    } break;
    default:
        /* We should not reach here */
        g_printerr("Unexpected message received.\n");
        break;
    }
    gst_message_unref(msg);
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

/* Functions below print the Capabilities in a human-friendly format */
gboolean short_cutting_pipeline_ex3::print_field(GQuark field, const GValue* value, gpointer pfx) {
    gchar* str = gst_value_serialize(value);

    g_print("%s  %15s: %s\n", (gchar*)pfx, g_quark_to_string(field), str);
    g_free(str);
    return TRUE;
}

void short_cutting_pipeline_ex3::print_caps(const GstCaps* caps, const gchar* pfx) {
    guint i;

    g_return_if_fail(caps != NULL);

    if (gst_caps_is_any(caps)) {
        g_print("%sANY\n", pfx);
        return;
    }
    if (gst_caps_is_empty(caps)) {
        g_print("%sEMPTY\n", pfx);
        return;
    }

    for (i = 0; i < gst_caps_get_size(caps); i++) {
        GstStructure* structure = gst_caps_get_structure(caps, i);

        g_print("%s%s\n", pfx, gst_structure_get_name(structure));
        gst_structure_foreach(structure, print_field, (gpointer)pfx);
    }
}

/* Prints information about a Pad Template, including its Capabilities */
void short_cutting_pipeline_ex3::print_pad_templates_information(GstElementFactory* factory) {
    const GList* pads;
    GstStaticPadTemplate* padtemplate;

    g_print("Pad Templates for %s:\n", gst_element_factory_get_longname(factory));
    if (!gst_element_factory_get_num_pad_templates(factory)) {
        g_print("  none\n");
        return;
    }

    pads = gst_element_factory_get_static_pad_templates(factory);
    while (pads) {
        padtemplate = (GstStaticPadTemplate *)pads->data;
        pads = g_list_next(pads);

        if (padtemplate->direction == GST_PAD_SRC)
            g_print("  SRC template: '%s'\n", padtemplate->name_template);
        else if (padtemplate->direction == GST_PAD_SINK)
            g_print("  SINK template: '%s'\n", padtemplate->name_template);
        else
            g_print("  UNKNOWN!!! template: '%s'\n", padtemplate->name_template);

        if (padtemplate->presence == GST_PAD_ALWAYS)
            g_print("    Availability: Always\n");
        else if (padtemplate->presence == GST_PAD_SOMETIMES)
            g_print("    Availability: Sometimes\n");
        else if (padtemplate->presence == GST_PAD_REQUEST)
            g_print("    Availability: On request\n");
        else
            g_print("    Availability: UNKNOWN!!!\n");

        if (padtemplate->static_caps.string) {
            GstCaps* caps;
            g_print("    Capabilities:\n");
            caps = gst_static_caps_get(&padtemplate->static_caps);
            print_caps(caps, "      ");
            gst_caps_unref(caps);

        }

        g_print("\n");
    }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
void short_cutting_pipeline_ex3::print_pad_capabilities(GstElement* element, gchar* pad_name) {
    GstPad* pad = NULL;
    GstCaps* caps = NULL;

    /* Retrieve pad */
    pad = gst_element_get_static_pad(element, pad_name);
    if (!pad) {
        g_printerr("Could not retrieve pad '%s'\n", pad_name);
        return;
    }

    /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
    caps = gst_pad_get_current_caps(pad);
    if (!caps)
        caps = gst_pad_query_caps(pad, NULL);

    /* Print and free */
    g_print("Caps for the %s pad:\n", pad_name);
    print_caps(caps, "      ");
    gst_caps_unref(caps);
    gst_object_unref(pad);
}

int short_cutting_pipeline_ex3::example_6_show_cap(int argc, char* argv[]) {
    GstElement* pipeline, * source, * sink;
    GstElementFactory* source_factory, * sink_factory;
    GstBus* bus;
    GstMessage* msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the element factories */
    source_factory = gst_element_factory_find("audiotestsrc");
    sink_factory = gst_element_factory_find("autoaudiosink");
    if (!source_factory || !sink_factory) {
        g_printerr("Not all element factories could be created.\n");
        return -1;
    }

    /* Print information about the pad templates of these factories */
    print_pad_templates_information(source_factory);
    print_pad_templates_information(sink_factory);

    /* Ask the factories to instantiate actual elements */
    source = gst_element_factory_create(source_factory, "source");
    sink = gst_element_factory_create(sink_factory, "sink");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline || !source || !sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    if (gst_element_link(source, sink) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Print initial negotiated caps (in NULL state) */
    g_print("In NULL state:\n");
    print_pad_capabilities(sink, (gchar *)"sink");

    /* Start playing */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the playing state (check the bus for error messages).\n");
    }

    /* Wait until error, EOS or State Change */
    bus = gst_element_get_bus(pipeline);
    do {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED));

        /* Parse message */
        if (msg != NULL) {
            GError* err;
            gchar* debug_info;

            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                terminate = TRUE;
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
                terminate = TRUE;
                break;
            case GST_MESSAGE_STATE_CHANGED:
                /* We are only interested in state-changed messages from the pipeline */
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                    GstState old_state, new_state, pending_state;
                    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                    g_print("\nPipeline state changed from %s to %s:\n",
                        gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                    /* Print the current capabilities of the sink element */
                    print_pad_capabilities(sink, (gchar*)"sink");
                }
                break;
            default:
                /* We should not reach here because we only asked for ERRORs, EOS and STATE_CHANGED */
                g_printerr("Unexpected message received.\n");
                break;
            }
            gst_message_unref(msg);
        }
    } while (!terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(source_factory);
    gst_object_unref(sink_factory);
    return 0;
}

int short_cutting_pipeline_ex3::example_7_multithreading(int argc, char* argv[]) {
    GstElement* pipeline, * audio_source, * tee, * audio_queue, * audio_convert, * audio_resample, * audio_sink;
    GstElement* video_queue, * visual, * video_convert, * video_sink;
    GstBus* bus;
    GstMessage* msg;
    GstPad* tee_audio_pad, * tee_video_pad;
    GstPad* queue_audio_pad, * queue_video_pad;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    audio_source = gst_element_factory_make("audiotestsrc", "audio_source");
    tee = gst_element_factory_make("tee", "tee");

    audio_queue = gst_element_factory_make("queue", "audio_queue");
    audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
    audio_resample = gst_element_factory_make("audioresample", "audio_resample");
    audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");

    video_queue = gst_element_factory_make("queue", "video_queue");
    visual = gst_element_factory_make("wavescope", "visual");

    video_convert = gst_element_factory_make("videoconvert", "csp");
    video_sink = gst_element_factory_make("autovideosink", "video_sink");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline || !audio_source || !tee || !audio_queue || !audio_convert || !audio_resample || !audio_sink ||
        !video_queue || !visual || !video_convert || !video_sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Configure elements */
    g_object_set(audio_source, "freq", 215.0f, NULL);
    g_object_set(visual, "shader", 0, "style", 1, NULL);

    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many(GST_BIN(pipeline), audio_source, tee, audio_queue, audio_convert, audio_resample, audio_sink,
        video_queue, visual, video_convert, video_sink, NULL);
    if (gst_element_link_many(audio_source, tee, NULL) != TRUE ||
        gst_element_link_many(audio_queue, audio_convert, audio_resample, audio_sink, NULL) != TRUE ||
        gst_element_link_many(video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Manually link the Tee, which has "Request" pads */
    tee_audio_pad = gst_element_request_pad_simple(tee, "src_%u");
    g_print("Obtained request pad %s for audio branch.\n", gst_pad_get_name(tee_audio_pad));
    queue_audio_pad = gst_element_get_static_pad(audio_queue, "sink");
    tee_video_pad = gst_element_request_pad_simple(tee, "src_%u");
    g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(tee_video_pad));
    queue_video_pad = gst_element_get_static_pad(video_queue, "sink");
    if (gst_pad_link(tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
        gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
        g_printerr("Tee could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    gst_object_unref(queue_audio_pad);
    gst_object_unref(queue_video_pad);

    /* Start playing the pipeline */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Release the request pads from the Tee, and unref them */
    gst_element_release_request_pad(tee, tee_audio_pad);
    gst_element_release_request_pad(tee, tee_video_pad);
    gst_object_unref(tee_audio_pad);
    gst_object_unref(tee_video_pad);

    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);

    gst_object_unref(pipeline);
    return 0;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

static int count_nbr_frame;
static int count_nbr_push_data;

/* This method is called by the idle GSource in the mainloop, to feed CHUNK_SIZE bytes into appsrc.
 * The idle handler is added to the mainloop when appsrc requests us to start sending data (need-data signal)
 * and is removed when appsrc has enough data (enough-data signal).
 */
gboolean short_cutting_pipeline_ex3::ex8_push_data(ex8_shortcut_pipeline_CustomData* data) {
    
    ++count_nbr_push_data;
    GstBuffer* buffer;
    GstFlowReturn ret;
    int i;
    GstMapInfo map;
    gint16* raw;
    gint num_samples = CHUNK_SIZE / 2; /* Because each sample is 16 bits */
    gfloat freq;

    /* Create a new empty buffer */
    buffer = gst_buffer_new_and_alloc(CHUNK_SIZE);

    /* Set its timestamp and duration */
    GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->num_samples, GST_SECOND, SAMPLE_RATE);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(num_samples, GST_SECOND, SAMPLE_RATE);

    /* Generate some psychodelic waveforms */
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    raw = (gint16*)map.data;
    data->c += data->d;
    data->d -= data->c / 1000;
    freq = 1100 + 1000 * data->d;
    for (i = 0; i < num_samples; i++) {
        data->a += data->b;
        data->b -= data->a / freq;
        raw[i] = (gint16)(500 * data->a);
    }
    gst_buffer_unmap(buffer, &map);
    data->num_samples += num_samples;

    /* Push the buffer into the appsrc */
    g_signal_emit_by_name(data->app_source, "push-buffer", buffer, &ret);

    /* Free the buffer now that we are done with it */
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        /* We got some error, stop sending data */
        //g_print("ex8_push_data(FALSE)\n");
        return FALSE;
    }
    //g_print("ex8_push_data(TRUE)\n");
    return TRUE;
}

/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
void short_cutting_pipeline_ex3::ex8_start_feed(GstElement* source, guint size, ex8_shortcut_pipeline_CustomData* data) {
    if (data->sourceid == 0) {
        g_print("Start feeding %d - pushdata:%d\n", count_nbr_frame, count_nbr_push_data);
        count_nbr_push_data = 0;
        count_nbr_frame = 0;
        data->sourceid = g_idle_add((GSourceFunc)ex8_push_data, data);
    }
}

/* This callback triggers when appsrc has enough data and we can stop sending.
 * We remove the idle handler from the mainloop */
void short_cutting_pipeline_ex3::ex8_stop_feed(GstElement* source, ex8_shortcut_pipeline_CustomData* data) {
    if (data->sourceid != 0) {

        g_print("Stop feeding %d \n", count_nbr_frame);
        g_source_remove(data->sourceid);
        data->sourceid = 0;
    }
}

/* The appsink has received a buffer */
GstFlowReturn short_cutting_pipeline_ex3::ex8_new_sample(GstElement* sink, ex8_shortcut_pipeline_CustomData* data) {
    GstSample* sample;

    /* Retrieve the buffer */
    // g_print("New sample, retrieving buffer\n");
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (sample) {
        /* The only thing we do in this example is print a * to indicate a received buffer */
        g_print("*");
        gst_sample_unref(sample);
        ++count_nbr_frame;
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

/* This function is called when an error message is posted on the bus */
void short_cutting_pipeline_ex3::ex8_error_cb(GstBus* bus, GstMessage* msg, ex8_shortcut_pipeline_CustomData* data) {
    GError* err;
    gchar* debug_info;

    /* Print error details on the screen */
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    g_main_loop_quit(data->main_loop);
}

int short_cutting_pipeline_ex3::example_8_shortcutting_pipeline(int argc, char* argv[]) {
    count_nbr_frame = 0;
    count_nbr_push_data = 0;

    ex8_shortcut_pipeline_CustomData data;
    GstPad* tee_audio_pad, * tee_video_pad, * tee_app_pad;
    GstPad* queue_audio_pad, * queue_video_pad, * queue_app_pad;
    GstAudioInfo info;
    GstCaps* audio_caps;
    GstBus* bus;

    /* Initialize custom data structure */
    memset(&data, 0, sizeof(data));
    data.b = 1; /* For waveform generation */
    data.d = 1;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    data.app_source = gst_element_factory_make("appsrc", "audio_source");
    data.tee = gst_element_factory_make("tee", "tee");

    data.audio_queue = gst_element_factory_make("queue", "audio_queue");
    data.audio_convert1 = gst_element_factory_make("audioconvert", "audio_convert1");
    data.audio_resample = gst_element_factory_make("audioresample", "audio_resample");
    data.audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");
    data.audio_convert2 = gst_element_factory_make("audioconvert", "audio_convert2");

    data.video_queue = gst_element_factory_make("queue", "video_queue");
    data.visual = gst_element_factory_make("wavescope", "visual");
    data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
    data.video_sink = gst_element_factory_make("autovideosink", "video_sink");

    data.app_queue = gst_element_factory_make("queue", "app_queue");
    data.app_sink = gst_element_factory_make("appsink", "app_sink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");

    if (!data.pipeline || !data.app_source || !data.tee || !data.audio_queue || !data.audio_convert1 ||
        !data.audio_resample || !data.audio_sink || !data.video_queue || !data.audio_convert2 || !data.visual ||
        !data.video_convert || !data.video_sink || !data.app_queue || !data.app_sink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    /* Configure wavescope */
    g_object_set(data.visual, "shader", 0, "style", 0, NULL);

    /* Configure appsrc */
    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
    audio_caps = gst_audio_info_to_caps(&info);
    g_object_set(data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
    g_signal_connect(data.app_source, "need-data", G_CALLBACK(ex8_start_feed), &data);
    g_signal_connect(data.app_source, "enough-data", G_CALLBACK(ex8_stop_feed), &data);
    /* Configure appsink */
    g_object_set(data.app_sink, "emit-signals", TRUE, NULL);// "caps", audio_caps, 
    g_signal_connect(data.app_sink, "new-sample", G_CALLBACK(ex8_new_sample), &data);
    gst_caps_unref(audio_caps);

    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many(GST_BIN(data.pipeline), data.app_source, data.tee, data.audio_queue, data.audio_convert1, data.audio_resample,
        data.audio_sink, data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink, data.app_queue,
        data.app_sink, NULL);
    if (gst_element_link_many(data.app_source, data.tee, NULL) != TRUE ||
        gst_element_link_many(data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink, NULL) != TRUE ||
        gst_element_link_many(data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink, NULL) != TRUE ||
        gst_element_link_many(data.app_queue, data.app_sink, NULL) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    /* Manually link the Tee, which has "Request" pads */
    tee_audio_pad = gst_element_request_pad_simple(data.tee, "src_%u");
    g_print("Obtained request pad %s for audio branch.\n", gst_pad_get_name(tee_audio_pad));
    queue_audio_pad = gst_element_get_static_pad(data.audio_queue, "sink");
    tee_video_pad = gst_element_request_pad_simple(data.tee, "src_%u");
    g_print("Obtained request pad %s for video branch.\n", gst_pad_get_name(tee_video_pad));
    queue_video_pad = gst_element_get_static_pad(data.video_queue, "sink");
    tee_app_pad = gst_element_request_pad_simple(data.tee, "src_%u");
    g_print("Obtained request pad %s for app branch.\n", gst_pad_get_name(tee_app_pad));
    queue_app_pad = gst_element_get_static_pad(data.app_queue, "sink");
    if (gst_pad_link(tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
        gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ||
        gst_pad_link(tee_app_pad, queue_app_pad) != GST_PAD_LINK_OK) {
        g_printerr("Tee could not be linked\n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    gst_object_unref(queue_audio_pad);
    gst_object_unref(queue_video_pad);
    gst_object_unref(queue_app_pad);

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)ex8_error_cb, &data);
    gst_object_unref(bus);

    /* Start playing the pipeline */
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    /* Create a GLib Main Loop and set it to run */
    data.main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.main_loop);

    /* Release the request pads from the Tee, and unref them */
    gst_element_release_request_pad(data.tee, tee_audio_pad);
    gst_element_release_request_pad(data.tee, tee_video_pad);
    gst_element_release_request_pad(data.tee, tee_app_pad);
    gst_object_unref(tee_audio_pad);
    gst_object_unref(tee_video_pad);
    gst_object_unref(tee_app_pad);

    /* Free resources */
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

gboolean short_cutting_pipeline_ex3::ex3_push_data(ex3_short_cutting_CustomData* data) {
    GstBuffer* buffer;
    GstFlowReturn ret;
    int i;
    GstMapInfo map;
    gint16* raw;
    gint num_samples = CHUNK_SIZE / 2; /* Because each sample is 16 bits */
    gfloat freq;

    /* Create a new empty buffer */
    buffer = gst_buffer_new_and_alloc(CHUNK_SIZE);

    /* Set its timestamp and duration */
    GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->num_samples, GST_SECOND, SAMPLE_RATE);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(num_samples, GST_SECOND, SAMPLE_RATE);

    /* Generate some psychodelic waveforms */
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    raw = (gint16*)map.data;
    data->c += data->d;
    data->d -= data->c / 1000;
    freq = 1100 + 1000 * data->d;
    for (i = 0; i < num_samples; i++) {
        data->a += data->b;
        data->b -= data->a / freq;
        raw[i] = (gint16)(500 * data->a);
    }
    gst_buffer_unmap(buffer, &map);
    data->num_samples += num_samples;

    /* Push the buffer into the appsrc */
    g_signal_emit_by_name(data->app_source, "push-buffer", buffer, &ret);

    /* Free the buffer now that we are done with it */
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        /* We got some error, stop sending data */
        return FALSE;
    }

    return TRUE;
}

/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
void short_cutting_pipeline_ex3::ex3_start_feed(GstElement* source, guint size, ex3_short_cutting_CustomData* data) {
    if (data->sourceid == 0) {
        g_print("Start feeding\n");
        data->sourceid = g_idle_add((GSourceFunc)ex3_push_data, data);
    }
}

/* This callback triggers when appsrc has enough data and we can stop sending.
 * We remove the idle handler from the mainloop */
void short_cutting_pipeline_ex3::ex3_stop_feed(GstElement* source, ex3_short_cutting_CustomData* data) {
    if (data->sourceid != 0) {
        g_print("Stop feeding\n");
        g_source_remove(data->sourceid);
        data->sourceid = 0;
    }
}

/* This function is called when an error message is posted on the bus */
void short_cutting_pipeline_ex3::ex3_error_cb(GstBus* bus, GstMessage* msg, ex3_short_cutting_CustomData* data) {
    GError* err;
    gchar* debug_info;

    /* Print error details on the screen */
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&err);
    g_free(debug_info);

    g_main_loop_quit(data->main_loop);
}

/* This function is called when playbin has created the appsrc element, so we have
 * a chance to configure it. */
void short_cutting_pipeline_ex3::ex3_source_setup(GstElement* pipeline, GstElement* source, ex3_short_cutting_CustomData* data) {
    GstAudioInfo info;
    GstCaps* audio_caps;

    g_print("Source has been created. Configuring.\n");
    data->app_source = source;

    /* Configure appsrc */
    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
    audio_caps = gst_audio_info_to_caps(&info);
    g_object_set(source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
    g_signal_connect(source, "need-data", G_CALLBACK(ex3_start_feed), data);
    g_signal_connect(source, "enough-data", G_CALLBACK(ex3_stop_feed), data);
    gst_caps_unref(audio_caps);
}



int short_cutting_pipeline_ex3::example_3_short_cutting_the_pipeline(int argc, char* argv[])
{
    ex3_short_cutting_CustomData data;
    GstBus* bus;

    /* Initialize custom data structure */
    memset(&data, 0, sizeof(data));
    data.b = 1; /* For waveform generation */
    data.d = 1;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the playbin element */
    data.pipeline = gst_parse_launch("playbin uri=appsrc://", NULL);
    g_signal_connect(data.pipeline, "source-setup", G_CALLBACK(ex3_source_setup), &data);

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)ex3_error_cb, &data);
    gst_object_unref(bus);

    /* Start playing the pipeline */
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    /* Create a GLib Main Loop and set it to run */
    data.main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.main_loop);

    /* Free resources */
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    return 0;
}