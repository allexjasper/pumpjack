#pragma once

#include <atomic>
#include "sensor.h"
#include <thread>
#include <atomic>
#include "sensor.h"
#include <thread>
#include <tuple>

class AbstractMovementPredictor
{
public:
	typedef std::function<void(TimeSeries&, std::chrono::milliseconds, TimeSeries::Timestamp, std::chrono::milliseconds,  const std::string&) > ConsumeFunction;
	AbstractMovementPredictor(Sensor& sensor, std::chrono::milliseconds ms_to_predict,
		std::chrono::milliseconds ms_to_crossfade, std::chrono::milliseconds transmissionDelay);
	virtual ~AbstractMovementPredictor();
	virtual void predictMovement(Sensor::Queue& inbound, ConsumeFunction f, std::function<void(const std::string, TimeSeries&)> monitor);
	virtual void shutdown();

protected:
	virtual std::optional<std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp>> calcPeriodicity(TimeSeries& ts) = 0;

private:
	void predictMovementThread(Sensor::Queue& inbound, ConsumeFunction consume, std::function<void(const std::string, TimeSeries&)> monitor);
	std::tuple<TimeSeries, std::chrono::milliseconds> extendOnPeriodicyity(TimeSeries& ts, std::chrono::milliseconds periodicity);

protected:
	Sensor& _sensor;
	std::atomic<bool> _shutdownRequested;
	std::thread _predictThread;
	std::chrono::milliseconds _ms_to_predict;
	std::chrono::milliseconds _ms_to_crossfade;
	std::chrono::milliseconds _transmissionDelay;

};

