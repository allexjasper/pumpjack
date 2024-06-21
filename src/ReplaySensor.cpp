#include "ReplaySensor.h"
#include <iostream>
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <tuple>
#include <vector>
#include "TimeSeries.h"
#include <boost/log/trivial.hpp>


ReplaySensor::ReplaySensor(const std::string& fileName) : _shutdownRequested(false), _fileName(fileName)
{
}

void ReplaySensor::replayThread(Sensor::Queue& queue)
{
    TimeSeries::Timestamp start_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

    auto i = _data.begin();
    while(i < _data.end())
    {
        
        auto& sample = *i;
        float angle;
        int dummy;
        TimeSeries::Timestamp time_point;
        std::tie(angle, time_point, dummy) = sample;
        time_point = time_point - std::get<1>(_data[0]) + start_time;

        // Wait for 10 ms interval
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Rebase the timestamp
        TimeSeries::Timestamp curSlice = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

        // Replay all samples older than the current reference time within the 10 ms interval
        while (time_point <= curSlice && i < _data.end())
        {
            
            std::tie(angle, time_point, dummy) = *i;
            time_point = time_point - std::get<1>(_data[0]) + start_time;
            
            if (!queue.push(TimeSeries::Sample(angle, time_point, 0)))
            {
                BOOST_LOG_TRIVIAL(info) << "inbound queue overflow" << std::endl;
            }
                
            i++;        
        }
    }
}


void ReplaySensor::readFile()
{
    std::ifstream file(_fileName);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << _fileName << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(","));

        if (tokens.size() < 2) {
            std::cerr << "Error: Invalid line in file" << std::endl;
            continue; // skip this line
        }

        try {
            float angle = std::stof(tokens[1]); // Convert string to float
            long long milliseconds = std::stoll(tokens[0]); // Convert string to long long
            //TimeSeries::Timestamp time_point = std::chrono::steady_clock::time_point(std::chrono::milliseconds(milliseconds));
            TimeSeries::Timestamp time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>(std::chrono::milliseconds(milliseconds));

            _data.push_back(std::make_tuple(angle, time_point, 0)); // Store sample
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            continue; // skip this line
        }
    }
    file.close();
}


void ReplaySensor::readData(Sensor::Queue& queue)
{
    readFile();
    _replayThread = std::thread([this, &queue]() {
        replayThread(queue);
        });
    return;
}

void ReplaySensor::shutdown()
{
    _shutdownRequested = true;
    _replayThread.join();
}
