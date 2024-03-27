
#include <vector>
#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <exception>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>


// Define your JPEG image buffer type (e.g., unsigned char*)
using ImageBuffer = std::vector<unsigned char>;

static std::vector<std::string> getAllFilesInFolder(const std::string& folderPath) {
    std::vector<std::string> filePaths;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                filePaths.push_back(entry.path().string());
            }
        }
    }
    catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
    catch (std::exception& e) {
        std::cerr << "General error: " << e.what() << '\n';
    }
    return filePaths;
}

static std::string getFileContents(const char* filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return(contents);
    }
    throw std::exception(__func__);
}

static std::vector<std::string> getAndLoadFiles(const std::string& folderPath)
{
    std::vector<std::string> frames;
    auto filesToLoad = getAllFilesInFolder(folderPath);
    for (const auto& file : filesToLoad)
    {
        std::cout << file << std::endl;
        frames.push_back(getFileContents(file.c_str()));
    }
    return frames;
}

int video_example(int argc, char* argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create GStreamer pipeline
    GstElement* pipeline = gst_pipeline_new("image-to-video-pipeline");
    GstElement* source = gst_element_factory_make("appsrc", "image-source");
    GstElement* parser = gst_element_factory_make("jpegparse", "jpeg-parser");
    GstElement* encoder = gst_element_factory_make("avenc_mpeg4", "video-encoder");
    GstElement* muxer = gst_element_factory_make("mp4mux", "mp4-muxer");
    GstElement* sink = gst_element_factory_make("filesink", "output-sink");

    // Check if elements are created successfully
    if (!pipeline || !source || !parser || !encoder || !muxer || !sink) {
        g_error("Failed to create elements.");
        return -1;
    }

    // Set properties for source element
    g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, nullptr);

    // Link elements together
    gst_bin_add_many(GST_BIN(pipeline), source, parser, encoder, muxer, sink, nullptr);
    gst_element_link_many(source, parser, encoder, muxer, sink, nullptr);

    // Set pipeline to playing state
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_error("Failed to start pipeline.");
        return -1;
    }

    // Example: Add your image buffers to the pipeline
    std::vector<std::string> imageBuffers = getAndLoadFiles("images_test");

    for (const auto& buffer : imageBuffers) {
        GstBuffer* gstBuffer = gst_buffer_new_allocate(nullptr, buffer.size(), nullptr);
        gst_buffer_fill(gstBuffer, 0, buffer.c_str(), buffer.size());
        gst_app_src_push_buffer(GST_APP_SRC(source), gstBuffer);
    }

    // Wait until EOS or error
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    gst_message_unref(msg);

    // Free resources
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}

// add to environmùent GST_DEBUG=4 for debug

int hello_world(int argc, char* argv[])
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

int main(int ac, char* av[])
{
    hello_world(ac, av);
}
