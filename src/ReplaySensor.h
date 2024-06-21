#pragma once
#include <atomic>
#include "Sensor.h"
#include "TimeSeries.h"
#include <thread>


class ReplaySensor : public Sensor
{
public:
	ReplaySensor(const std::string& fileName );
	virtual void readData(Queue& queue);
	virtual void shutdown();

private:
	void readFile();
	void replayThread(Sensor::Queue& queue);

private:
	std::atomic<bool> _shutdownRequested;
	std::string _fileName;
	std::vector<TimeSeries::Sample> _data;
	std::thread _replayThread;
};

