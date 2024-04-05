
#pragma once

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <string.h>

class short_cutting_pipeline_ex3
{
public:
    /* Structure to contain all our information, so we can pass it to callbacks */
    typedef struct ex3_audio_video_CustomData {
        GstElement* pipeline;
        GstElement* source;
        GstElement* convert;
        GstElement* resample;
        GstElement* sink;
        GstElement* videoConvert;
        GstElement* videoSink;
    };

    /* Structure to contain all our information, so we can pass it around */
    typedef struct ex4_timed_CustomData{
        GstElement* playbin;  /* Our one and only element */
        gboolean playing;      /* Are we in the PLAYING state? */
        gboolean terminate;    /* Should we terminate execution? */
        gboolean seek_enabled; /* Is seeking enabled for this media? */
        gboolean seek_done;    /* Have we performed the seek already? */
        gint64 duration;       /* How long does this media last, in nanoseconds */
    };

    typedef struct ex8_shortcut_pipeline_CustomData {
        GstElement* pipeline, * app_source, * tee, * audio_queue, * audio_convert1, * audio_resample, * audio_sink, *jpgdec;
        GstElement* video_queue, * audio_convert2, * visual, * video_convert, * video_sink;
        GstElement* app_queue, * app_sink;
        guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
        gfloat a, b, c, d;     /* For waveform generation */
        guint sourceid;        /* To control the GSource */
        GMainLoop* main_loop;  /* GLib's Main Loop */
    };

    typedef struct ex3_short_cutting_CustomData {
        GstElement* pipeline;
        GstElement* app_source;

        guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
        gfloat a, b, c, d;     /* For waveform generation */

        guint sourceid;        /* To control the GSource */

        GMainLoop* main_loop;  /* GLib's Main Loop */
    };



	short_cutting_pipeline_ex3();
	~short_cutting_pipeline_ex3();

    /* Handler for the pad-added signal */
    static void pad_added_handler(GstElement* src, GstPad* pad, short_cutting_pipeline_ex3::ex3_audio_video_CustomData* data);
    static void pad_exit(GstCaps* new_pad_caps, GstPad* audio_sink_pad, GstPad* video_sink_pad);
    static int link_pad_with_cap(GstPad* new_pad, GstPad* pad, const gchar* type);
	int example_3_Dynamic_Hello_world(int argc, char* argv[]);

    int example_3_short_cutting_the_pipeline(int argc, char* argv[]);
    static void ex3_source_setup(GstElement* pipeline, GstElement* source, ex3_short_cutting_CustomData* data);
    static void ex3_error_cb(GstBus* bus, GstMessage* msg, ex3_short_cutting_CustomData* data);
    static void ex3_stop_feed(GstElement* source, ex3_short_cutting_CustomData* data);
    static void ex3_start_feed(GstElement* source, guint size, ex3_short_cutting_CustomData* data);
    static gboolean ex3_push_data(ex3_short_cutting_CustomData* data);


    int example_4_timed_management(int argc, char* argv[]);
    static void handle_message(ex4_timed_CustomData* data, GstMessage* msg);

    int example_6_show_cap(int argc, char* argv[]);
    static void print_pad_capabilities(GstElement* element, gchar* pad_name);
    static void print_caps(const GstCaps* caps, const gchar* pfx);
    static void print_pad_templates_information(GstElementFactory* factory);
    static gboolean print_field(GQuark field, const GValue* value, gpointer pfx);


    int example_7_multithreading(int argc, char* argv[]);

    static gboolean ex8_push_data(ex8_shortcut_pipeline_CustomData* data);
    static void ex8_start_feed(GstElement* source, guint size, ex8_shortcut_pipeline_CustomData* data);
    static void ex8_stop_feed(GstElement* source, ex8_shortcut_pipeline_CustomData* data);
    static GstFlowReturn ex8_new_sample(GstElement* sink, ex8_shortcut_pipeline_CustomData* data);
    static void ex8_error_cb(GstBus* bus, GstMessage* msg, ex8_shortcut_pipeline_CustomData* data);
    int example_8_shortcutting_pipeline(int argc, char* argv[]);
private:
};

