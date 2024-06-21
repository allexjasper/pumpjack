#pragma once

#include "TimeSeries.h"
#include <functional>
#include <chrono>
#include <string>

class Renderer
{
public:
	
	virtual ~Renderer()
	{

	}
	virtual void feedData(const TimeSeries& ts, std::chrono::milliseconds periodicity, TimeSeries::Timestamp, std::chrono::milliseconds, const std::string& overlay) = 0;
	virtual void render(std::function<void(const std::string, TimeSeries&)> monitor) = 0;
	virtual void shutdown() = 0;
};

