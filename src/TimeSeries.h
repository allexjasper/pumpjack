#pragma once

#include <vector>
#include <chrono>
#include <optional>
#include <tuple>

class TimeSeries
{
public:
	typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> Timestamp;
	typedef std::tuple<float, Timestamp, int> Sample; // angle, time
		
	TimeSeries resample() const;
	void add(Sample sample);
	std::vector<Sample>& getVector()
	{
		return _data;
	}

	std::optional<std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp>> calcPeriodicitySine() const;
	std::chrono::milliseconds duration() const;
	std::optional<size_t> findIndex(const Timestamp& timestamp) const;
	TimeSeries crossFade(TimeSeries& other);
	TimeSeries slice(const Timestamp& start, const Timestamp& end) const;
	void mergeInto(TimeSeries& other);
	float similarity( TimeSeries& other);
	void shift(std::chrono::milliseconds offset);
	bool checkConsistency();
	void deduplicate();
	std::chrono::milliseconds bestMatch(std::chrono::milliseconds windowExtension, TimeSeries& timeSeries);

private:
	std::vector<Sample> _data;

};

