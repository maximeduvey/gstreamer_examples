#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

#include <fstream>

struct FrameData {
	guint8* data;
	gsize size;

	FrameData() : data(nullptr), size(0) {}
	FrameData(guint8* data, gsize size) : data(data), size(size) {}
	~FrameData() {
		delete[] data;
	}

	// Copy constructor
	FrameData(const FrameData& other) : data(new guint8[other.size]), size(other.size) {
		memcpy(data, other.data, size);
	}

	// Move constructor
	FrameData(FrameData&& other) noexcept : data(other.data), size(other.size) {
		other.data = nullptr;
		other.size = 0;
	}

	// Copy assignment operator
	FrameData& operator=(const FrameData& other) {
		if (this == &other) return *this;
		delete[] data;
		data = new guint8[other.size];
		size = other.size;
		memcpy(data, other.data, size);
		return *this;
	}

	// Move assignment operator
	FrameData& operator=(FrameData&& other) noexcept {
		if (this == &other) return *this;
		delete[] data;
		data = other.data;
		size = other.size;
		other.data = nullptr;
		other.size = 0;
		return *this;
	}
};


/// <summary>
/// This is not a really client as it's not reading from a socket, but it's just the logic of
/// HOW to reconstruct a displayeable stream from a GstMapInfo
/// </summary>
class gstreamer_client_receiver 
{
	struct CustomDataStruct {
		GstElement* pipeline, *appsrc, *decoder, *converter, *sink;
	};

public : 
	gstreamer_client_receiver();
	virtual ~gstreamer_client_receiver();


	int Init(int ac, char* av[]);
	int start();
	void push_data();
	bool check_error();

	bool _end = false;
	CustomDataStruct _data;

	struct FrameData {
		guint8* data;
		gsize size;
	};

	std::ifstream file;
	std::vector<char> buffer;
	std::streamsize size;

	inline void AddMapInfo(const GstMapInfo& gai) {
		std::lock_guard<std::mutex> lock(_mapInfoMutex);

		// Allocate new memory and copy buffer data
		guint8* newData = new guint8[gai.size];
		memcpy(newData, gai.data, gai.size);

		// Create a FrameData object and add it to the vector
		FrameData frameData(newData, gai.size);
		_receivedMapInfo.push_back(std::move(frameData)); // Use std::move for efficiency
	}

	inline FrameData GetLastMapInfo() {
		std::lock_guard<std::mutex> lock(_mapInfoMutex);
		auto ret = _receivedMapInfo.back();
		_receivedMapInfo.pop_back();
		return ret;
	}
	
	inline size_t GetSizeMapInfo() {
		std::lock_guard<std::mutex> lock(_mapInfoMutex);
		auto ret = _receivedMapInfo.size();
		return ret;
	}

	std::vector<FrameData> _receivedMapInfo;
	std::mutex _mapInfoMutex;
};