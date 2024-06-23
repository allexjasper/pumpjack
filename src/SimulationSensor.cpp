#include "SimulationSensor.h"
#include <boost/log/trivial.hpp>
#include <numbers>
//#include "windows.h"
#include <cmath>


SimulationSensor::SimulationSensor() : _shutdownRequested(false)
{
}

void SimulationSensor::readData(Queue& queue)
{
    _thread = std::thread([this, &queue]() {
        simulateThread(queue);
        });
}

void SimulationSensor::shutdown()
{
    _shutdownRequested = true;
    _thread.join();
}

void SimulationSensor::simulateThread(Sensor::Queue& queue)
{
    // Set the periodicity in seconds
    double p = 3;

    // Get the reference time before entering the loop
    auto reference_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

    auto prevTime = reference_time;

    while (!_shutdownRequested) {
        // Get the current absolute time
        auto current_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

        auto simTime = prevTime;
        while (simTime <= current_time)
        {
            // Calculate the sine wave value based on the elapsed time
            double value = 13.0 * std::sin(2.0 * std::numbers::pi * (double)std::chrono::duration_cast<std::chrono::milliseconds>(simTime - reference_time).count() / (p * 1e3));


            if (!queue.push({ value, simTime, 0 }))
            {
                BOOST_LOG_TRIVIAL(info) << "inbound queue overflow" << std::endl;
            }
            simTime += std::chrono::milliseconds(1);
        }

        prevTime = simTime;

       

        // Sleep for a short duration to control the loop speed
        //Sleep(5);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
