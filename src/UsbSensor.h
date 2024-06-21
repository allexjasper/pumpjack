#pragma once
#include "Sensor.h"
#include <thread>

class UsbSensor :
    public Sensor
{
public:
	UsbSensor(float magnetOffset);
	virtual void readData(Queue& queue);
	virtual void shutdown();

private:
	void readThread(Sensor::Queue& queue);

private:
	std::atomic<bool> _shutdownRequested;
	std::vector<TimeSeries::Sample> _data;
	std::thread _thread;
	float _magnetOffset;
};