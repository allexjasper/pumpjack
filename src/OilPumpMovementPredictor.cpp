#include "OilPumpMovementPredictor.h"
#include <boost/log/trivial.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include "Monitor.h"



OilPumpMovementPredictor::OilPumpMovementPredictor(Sensor& sensor, std::chrono::milliseconds ms_to_predict, 
                                     std::chrono::milliseconds ms_to_crossfade, std::chrono::milliseconds transmissionDelay) : 
                            AbstractMovementPredictor(sensor, ms_to_predict, ms_to_crossfade,  transmissionDelay)
{
}



std::optional<std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp>> OilPumpMovementPredictor::calcPeriodicity(TimeSeries& ts)
{

    return ts.calcPeriodicitySine();
}
