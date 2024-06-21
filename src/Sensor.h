#pragma once

#include <boost/lockfree/spsc_queue.hpp>
#include "TimeSeries.h"

class Sensor
{
public:
	typedef boost::lockfree::spsc_queue<TimeSeries::Sample, boost::lockfree::capacity<500>> Queue;
	virtual void readData(Queue& queue) = 0;
	virtual void shutdown() = 0;
};

