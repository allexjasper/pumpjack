#pragma once
#include "Sensor.h"
#include <thread>

class WheelSimulationSensor :
    public Sensor
{
public:
	WheelSimulationSensor();
	virtual void readData(Queue& queue);
	virtual void shutdown();

private:
	void simulateThread(Sensor::Queue& queue);

private:
	std::atomic<bool> _shutdownRequested;
	std::vector<TimeSeries::Sample> _data;
	std::thread _thread;
};

