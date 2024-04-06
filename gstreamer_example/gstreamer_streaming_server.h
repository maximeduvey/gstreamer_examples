#pragma once

#include "gstreamer_client_receiver.h"

#include <gst/gst.h>
#include <string.h>
#include <string>
#include <iostream>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#define WIDTH 320
#define HEIGHT 240
#define BPP 3 // RGB


typedef struct _CustomData {
	gboolean is_live;
	GMainLoop* main_loop;
	GstElement* pipeline, * appsrc, * decoder, * converter, * encoder, * muxer, * appsink;
} CustomData;

class gstreamer_streaming_server
{
private:
	//GMainLoop* main_loop = NULL;
	//GstAppSrc* app_src;
	//GstAppSink* app_sink;

public:

	gstreamer_streaming_server(int argc, char* argv[], gstreamer_client_receiver *client);
	virtual ~gstreamer_streaming_server();


	void Init(int argc, char* argv[]);
	int StartPlaying();

	static std::string getImage();


	static void on_eos(GstAppSink* appsink, gpointer user_data);
	static GstFlowReturn on_new_data(GstElement* appsrc, guint size, gpointer user_data);
	static GstFlowReturn on_new_sample(GstAppSink* sink, gpointer data);
	static void on_stop_feed(GstElement* source);
	static GstFlowReturn on_pull_preroll(GstAppSink* sink, gpointer data);
	static GstFlowReturn on_new_buffer(GstAppSink* sink, gpointer data);


private:

	static gboolean stop_main_loop(gpointer data);

};
