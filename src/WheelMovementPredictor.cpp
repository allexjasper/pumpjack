#include "WheelMovementPredictor.h"
#include <boost/log/trivial.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include "Monitor.h"



WheelMovementPredictor::WheelMovementPredictor(Sensor& sensor, std::chrono::milliseconds ms_to_predict,
    std::chrono::milliseconds ms_to_crossfade, std::chrono::milliseconds transmissionDelay) : AbstractMovementPredictor(sensor, ms_to_predict, ms_to_crossfade, transmissionDelay)
{
}

std::optional<std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp>> WheelMovementPredictor::calcPeriodicity(TimeSeries& ts)
{
    const auto& data = ts.getVector();

    TimeSeries::Timestamp first_transition;
    bool found_first_transition = false;
    TimeSeries::Timestamp second_transition;
    bool found_second_transition = false;

    if (data.size() < 2)
        return std::nullopt;

    const float UPPER_THRESHOLD = 359.0f;
    const float LOWER_THRESHOLD = 1.0f;

    for (auto it = data.rbegin() + 1; it != data.rend(); ++it) {
        float angle = std::get<0>(*(it -1));
        float prev_angle = std::get<0>(*(it));

        if (!found_first_transition && prev_angle > UPPER_THRESHOLD && angle < LOWER_THRESHOLD) {
            found_first_transition = true;
            first_transition = std::get<1>(*it);
        }
        else if (found_first_transition && prev_angle > UPPER_THRESHOLD && angle < LOWER_THRESHOLD) {
            found_second_transition = true;
            second_transition = std::get<1>(*it);
            break; // No need to iterate further
        }
    }

    if (!found_first_transition || !found_second_transition) {
        return std::nullopt; // Either transition not found
    }

    // Calculate the time difference between the two transitions
    std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp> ret = { std::chrono::duration_cast<std::chrono::milliseconds>(first_transition  - second_transition), first_transition };
    return ret;
}