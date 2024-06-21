#include "Monitor.h"
#include <fstream>
#include <sstream>

void Monitor::addData(const std::string& monitor, TimeSeries& ts)
{
    std::scoped_lock lock(_mutex);
	_data[monitor].mergeInto(ts);
}

void Monitor::monitor()
{
	_monitorThread = std::thread([this]() {
		monitorThread();
		});
}

void Monitor::monitorThread()
{
    int i = 0;
    std::unique_lock<std::mutex> lock(_mutex, std::defer_lock);
    while (true) {
        lock.lock();
        if (_cv.wait_for(lock, std::chrono::seconds(180), [this] { return (bool)_shutdownRequested; })) {
            // Shutdown requested
            break;
        }
        lock.unlock();

        std::map<std::string, TimeSeries> local_data;

        {
            std::scoped_lock scopedLock(_mutex);
            local_data = _data;
        }

        // Plot the current data to a file
        std::stringstream filename;
        filename << "plot_" << i << ".gnuplot";
        plot(filename.str(), local_data);

        // Slice the data in the time series map to contain only the last 20 seconds
        {
            std::scoped_lock scopedLock(_mutex);
            for (auto& entry : _data) {
                auto& series = entry.second;
                auto endTime = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
                auto startTime = endTime - std::chrono::seconds(5);
                auto slicedSeries = series.slice(startTime, endTime);
                series = slicedSeries;
            }
        }

        i++;
    }
}

void Monitor::shutdown()
{
    {
        std::scoped_lock lock(_mutex);
        _shutdownRequested = true;
    }
    _cv.notify_all();
    _monitorThread.join();
}

void Monitor::plot(const std::string& filename, std::map<std::string, TimeSeries>& data)
{
    // Create and open the CSV file
    std::ofstream outFile(filename + ".csv");

    if (!outFile.is_open()) {
        return;
    }

    // Write the header row with titles for the CSV file
    for ( auto& entry : data) {
        outFile << "," << entry.first << "_Time" << "," << entry.first; // Write title for this series and timestamp
    }
    outFile << "\n";

    // Find the maximum number of samples across all time series
    size_t maxSamples = 0;
    for (auto& entry : data) {
        maxSamples = std::max(maxSamples, entry.second.getVector().size());
    }

    // Write the data for each time series to the CSV file
    for (size_t i = 0; i < maxSamples; ++i) {
        for ( auto& entry : data) {
            auto& series = entry.second;
            if (i < series.getVector().size()) {
                // Write the timestamp for this time series
                outFile << std::get<1>(series.getVector()[i]).time_since_epoch().count() << ",";
                // Write the value for this time series
                outFile << std::get<0>(series.getVector()[i]) << ",";
            }
            else {
                // Write empty values if there are no more data points for this time series
                outFile << ",,";
            }
        }
        outFile << "\n";
    }

    // Close the output file
    outFile.close();
}
