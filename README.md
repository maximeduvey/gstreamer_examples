Hello, this project is a simple github to display how to use different functionnalities of gstreamer.

It's a Visual studio 2022 C++ project using the msi https://gstreamer.freedesktop.org/download/ MSVC 64-bit (VS 2019, Release CRT) gstreamer.

In it's current form the code is not clean as it's a construction site that help me understand how gstreamer work.

The Main file is : gstreamer_example.cpp containing 
The main at the end of the file and a few custom made example (some not working) function,
 for example images_to_displayed_video_example that load jpg image and display a video

short_cutting_pipeline_ex3 is regrouping the differents gstreamer official example (some with the optionnal exercice)

gstreamer_streaming_server and gstreamer_client_receiver are a tentative to create a stream, encode it, "send" it, decode it and play it (not working for now)
The "server" is connected on it's "new-sample" to "send" to the client it's encoded data, but the client does not seem to be able to decode it and play it. 
the client is running on it's own thread.
 
