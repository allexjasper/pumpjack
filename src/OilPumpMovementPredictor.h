#pragma once
#include <atomic>
#include "Sensor.h"
#include <thread>
#include "AbstractMovementPredictor.h"
#include <tuple>

class OilPumpMovementPredictor : public AbstractMovementPredictor
{
public:
	OilPumpMovementPredictor(Sensor& sensor, std::chrono::milliseconds ms_to_predict, std::chrono::milliseconds ms_to_crossfade, std::chrono::milliseconds transmissionDelay);

protected:
	virtual std::optional<std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp>> calcPeriodicity(TimeSeries& ts);



};

