#pragma once

#include "TimeSeries.h"
#include <mutex>
#include <map>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <thread>

class Monitor
{
public:
	void addData(const std::string& monitor, TimeSeries& ts);

	void monitor();
	void monitorThread();
	void shutdown();

	static void plot(const std::string& filename, std::map<std::string, TimeSeries>& data);

	typedef std::function<void(const std::string, TimeSeries&)> MonitorFunction;



private:
	std::mutex _mutex;
	std::map<std::string, TimeSeries> _data;
	std::atomic<bool> _shutdownRequested;
	std::thread _monitorThread;
	std::condition_variable _cv;
};

