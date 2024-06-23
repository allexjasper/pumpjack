#include "WheelSimulationSensor.h"

#include <boost/log/trivial.hpp>
#include <numbers>
//#include "windows.h"
#include <cmath>


WheelSimulationSensor::WheelSimulationSensor() : _shutdownRequested(false)
{
}

void WheelSimulationSensor::readData(Queue& queue)
{
    _thread = std::thread([this, &queue]() {
        simulateThread(queue);
        });
}

void WheelSimulationSensor::shutdown()
{
    _shutdownRequested = true;
    _thread.join();
}



void WheelSimulationSensor::simulateThread(Sensor::Queue& queue) {
    // Set the periodicity in milliseconds for one rotation
    auto p = std::chrono::milliseconds(6000); // 4 seconds for one rotation

    // Get the reference time before entering the loop
    auto reference_time = std::chrono::steady_clock::now();

    while (!_shutdownRequested) {
        // Get the current absolute time
        auto current_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

        // Calculate the elapsed time since the start in milliseconds
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - reference_time);

        // Calculate the angle based on elapsed time with 1/100 degree resolution
        double elapsed_seconds = static_cast<double>(elapsed_time.count()) / 1000.0;
        double period_seconds = static_cast<double>(p.count()) / 1000.0;
        double angle = fmod(36000.0 * (elapsed_seconds / period_seconds), 36000.0);

        // Push the angle to the queue with current time
        if (!queue.push({ static_cast<float>(angle / 100.0), current_time, 0 })) {
            BOOST_LOG_TRIVIAL(info) << "Inbound queue overflow" << std::endl;
        }

        // Sleep for a short duration to control the loop speed
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}