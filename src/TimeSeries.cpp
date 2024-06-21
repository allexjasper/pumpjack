#include "TimeSeries.h"
#include <algorithm>
#include <cmath>


/*
* Iterates over the time range spanned by the original samples.
* For each millisecond, it interpolates the angle by finding the closest samples in the original data and performing linear interpolation.
* Adds the interpolated sample to the resampled time series.
* Returns the resampled time series.
*/
TimeSeries TimeSeries::resample() const {
    if (_data.empty())
        return *this;

    // Find the first and last timestamps
    auto first_time = std::get<1>(_data.front());
    auto last_time = std::get<1>(_data.back());

    // Calculate the total duration
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(last_time - first_time);

    // Create a new time series with evenly spaced samples
    TimeSeries resampled_series;
    for (auto time_offset = std::chrono::milliseconds(0); time_offset <= total_duration; time_offset += std::chrono::milliseconds(1)) {
        auto interpolated_time = first_time + time_offset;

        // Find the closest samples in the original data
        auto it_lower = std::lower_bound(_data.begin(), _data.end(), interpolated_time, [](const Sample& s, const Timestamp& t) {
            return std::get<1>(s) < t;
            });

        // If the time is before the first sample or after the last sample, skip
        if (it_lower == _data.begin() || it_lower == _data.end())
            continue;

        // Interpolate between the closest samples
        auto it_upper = it_lower--;
        auto time_lower = std::get<1>(*it_lower);
        auto time_upper = std::get<1>(*it_upper);
        auto angle_lower = std::get<0>(*it_lower);
        auto angle_upper = std::get<0>(*it_upper);
        auto duration_lower = std::chrono::duration_cast<std::chrono::milliseconds>(interpolated_time - time_lower);
        auto duration_upper = std::chrono::duration_cast<std::chrono::milliseconds>(time_upper - interpolated_time);

        // Check if duration_lower or duration_upper is zero to avoid division by zero
        if (duration_lower.count() == 0) {
            resampled_series.add(*it_lower);
            continue;
        }
        if (duration_upper.count() == 0) {
            resampled_series.add(*it_upper);
            continue;
        }

        // Handle rollover
        if (std::abs(angle_upper - angle_lower) > 180) {
            if (angle_lower > angle_upper) {
                angle_upper += 360;
            }
            else {
                angle_lower += 360;
            }
        }

        float interpolated_angle = (angle_lower * duration_upper.count() + angle_upper * duration_lower.count()) / (duration_lower.count() + duration_upper.count());

        // Normalize the interpolated angle to ensure it's within [0, 360)
        if (interpolated_angle >= 360) {
            interpolated_angle -= 360;
        }

        // Add the interpolated sample to the resampled series
        resampled_series.add(std::make_tuple(interpolated_angle, interpolated_time, std::get<2>(*it_lower)));
    }

    return resampled_series;
}



void TimeSeries::add(Sample sample)
{
	_data.push_back(sample);
}


/** 
* We iterate over the samples in reverse order using reverse iterators.
* We search for the first encounter of a negative angle followed by a positive angle.
* If found, we remember the timestamp of the positive angle.
* Then, we search for the second encounter of a negative angle after the positive angle.
* If found, we calculate the difference in time between the two encounters and return it.
* If any step fails, we return an empty optional.
*/

std::optional<std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp>> TimeSeries::calcPeriodicitySine() const {
    Timestamp first_transition;
    bool found_first_transition = false;
    Timestamp second_transition;
    bool found_second_transition = false;

    if (_data.size() < 2)
        return std::nullopt;

    for (auto it = _data.rbegin() + 1; it != _data.rend(); ++it) {
        float angle = std::get<0>(*it);
        float prev_angle = std::get<0>(*(it - 1));

        if (!found_first_transition && prev_angle <= 0 && angle > 0) {
            found_first_transition = true;
            first_transition = std::get<1>(*it);
        }
        else if (found_first_transition && prev_angle <= 0 && angle > 0) {
            found_second_transition = true;
            second_transition = std::get<1>(*it);
            break; // No need to iterate further
        }
    }

    if (!found_first_transition || !found_second_transition) {
        return std::nullopt; // Either transition not found
    }

    // Calculate the time difference between the two transitions
    std::tuple<std::chrono::milliseconds, TimeSeries::Timestamp> ret = { std::chrono::duration_cast<std::chrono::milliseconds>(first_transition - second_transition), first_transition };
    return ret;
}


std::chrono::milliseconds TimeSeries::duration() const {
    if (_data.empty()) {
        return std::chrono::milliseconds(0);
    }
    else {
        auto first_time = std::get<1>(_data.front());
        auto last_time = std::get<1>(_data.back());
        return std::chrono::duration_cast<std::chrono::milliseconds>(last_time - first_time);
    }
}

std::optional<size_t> TimeSeries::findIndex(const Timestamp& timestamp) const {
    auto lower_bound = std::lower_bound(_data.begin(), _data.end(), timestamp,
        [](const Sample& s, const Timestamp& t) {
            return std::get<1>(s) < t;
        });
    
    if (lower_bound != _data.end()) {
            return std::distance(_data.begin(), lower_bound);
    }
    else {
        return std::nullopt;
    }
}


/* cross fade other ts at the end of this ts from the point they overlap*/
TimeSeries TimeSeries::crossFade(TimeSeries& other) 
{
    TimeSeries result;
    //checkConsistency();
    //other.checkConsistency();

    if (other.getVector().size() < 2)
        return *this;

    // Find the index where the other TimeSeries starts in this TimeSeries
    auto index = findIndex(std::get<1>(other.getVector().front()));
    if (!index) {
        return *this;
    }

    // Calculate the number of elements to crossfade
    size_t num_elements_to_crossfade = std::min(_data.size() - *index, other.getVector().size());

    // Compute fade in and fade out weights
    std::vector<float> fadeInWeights(num_elements_to_crossfade);
    std::vector<float> fadeOutWeights(num_elements_to_crossfade);
    float step = 1.0f / (num_elements_to_crossfade - 1);
    for (size_t i = 0; i < num_elements_to_crossfade; ++i) {
        fadeInWeights[i] = i * step;
        fadeOutWeights[i] = 1.0f - i * step;
    }

    //copy in the first part of the TS
    std::copy(getVector().begin(), getVector().begin() + *index, std::back_inserter(result.getVector()));
    result.checkConsistency();

    // Perform crossfade element-wise
    for (size_t i = 0; i < num_elements_to_crossfade; ++i) {
        auto& sample = _data[*index + i];
        auto& otherSample = other.getVector()[i];
        float fadedAngle = std::get<0>(sample) * fadeOutWeights[i] + std::get<0>(otherSample) * fadeInWeights[i];
        result.add(std::make_tuple(fadedAngle, std::get<1>(sample), std::get<2>(sample)));
    }
    result.checkConsistency();

    size_t numElemToAppendAfterCF = other._data.size() - num_elements_to_crossfade; // total elements - num_elements_to_crossfade


    auto cpy = result;

    // Directly copy the remaining elements from other TimeSeries to result
    //std::copy(other.getVector().begin() + num_elements_to_crossfade +2, other.getVector().end(), std::back_inserter(result.getVector()));
    
    auto otherCpyfrom = other.findIndex(std::get<1>(getVector().back()));

    if (otherCpyfrom)
    {
        std::copy(other.getVector().begin() + *otherCpyfrom +1, other.getVector().end(), std::back_inserter(result.getVector()));
    }
    
    //result.checkConsistency();
    
    //result.deduplicate();
    //result.checkConsistency();

    return result;
}


TimeSeries TimeSeries::slice(const Timestamp& start, const Timestamp& end) const {
    TimeSeries result;

        // Find the start and end iterators of the slice using lower_bound and upper_bound
        auto startIt = std::lower_bound(_data.begin(), _data.end(), start,
            [](const Sample& s, const Timestamp& t) {
                return std::get<1>(s) < t;
            });

        // If startIt points beyond the end of _data and start is before the beginning of _data, move it to the beginning
        if (startIt == _data.end() && !_data.empty() && start < std::get<1>(_data.front())) {
            startIt = _data.begin();
        }

        auto endIt = std::upper_bound(_data.begin(), _data.end(), end,
            [](const Timestamp& t, const Sample& s) {
                return t < std::get<1>(s);
            });

        // Resize the result vector to the correct size
        result.getVector().resize(std::distance(startIt, endIt));

        // Copy the slice between the start and end iterators
        std::copy(startIt, endIt, result.getVector().begin());

        return result;

}



void TimeSeries::mergeInto(TimeSeries& other) {
    if (other.getVector().empty())
        return;


    // Find the index where the other TimeSeries starts in this TimeSeries
    auto index = std::lower_bound(_data.begin(), _data.end(), std::get<1>(other.getVector().front()),
        [](const Sample& s, const Timestamp& t) {
            return std::get<1>(s) < t;
        });

    // Create a temporary new time series
    TimeSeries tempSeries;

    // Copy data up to the lower bound
    std::copy(_data.begin(), index, std::back_inserter(tempSeries._data));

    // Add the full other time series
    std::copy(other.getVector().begin(), other.getVector().end(), std::back_inserter(tempSeries._data));

    // Perform self-assignment
    *this = tempSeries;
}



float TimeSeries::similarity(TimeSeries& other) 
{
    // Ensure both time series have the same length
    size_t min_length = std::min(_data.size(), other._data.size());

    // Compute the squared differences and sum them up
    float sum_of_squared_differences = 0.0f;

    for (size_t i = 0; i < min_length; ++i) {
        float diff = std::get<0>(_data[i]) - std::get<0>(other._data[i]);
        sum_of_squared_differences += diff * diff;
    }

    // Calculate the Euclidean distance (L2 norm)
    float euclidean_distance = std::sqrt(sum_of_squared_differences);

    // Return the similarity (inverse of distance)
    return 1.0f / (1.0f + euclidean_distance);
}

void  TimeSeries::shift(std::chrono::milliseconds offset) {
    for (auto& sample : _data) {
        std::get<1>(sample) += offset;
    }
}

bool TimeSeries::checkConsistency()
{
    bool isOrdered = true;
    bool duplicatesFound = false;

    // Check if the time series is ordered
    for (size_t i = 1; i < _data.size(); ++i)
    {
        if (std::get<1>(_data[i]) < std::get<1>(_data[i - 1]))
        {
            isOrdered = false;
            //std::cout << "Timestamp at index " << i << " is not ordered." << std::endl;
        }
    }

    // Check for duplicate timestamps
    
    auto duplicateIt = std::adjacent_find(_data.begin(), _data.end(), [](const auto& a, const auto& b) {
        return std::get<1>(a) == std::get<1>(b);
        });
    if (duplicateIt != _data.end()) {
        duplicatesFound = true;
        //std::cout << "Duplicate timestamp found: " << std::get<1>(*duplicateIt).time_since_epoch().count() << std::endl;
    }
    

    return isOrdered && !duplicatesFound; // Time series is consistent if ordered and no duplicates found
}

void TimeSeries::deduplicate() {
    auto it = _data.begin();
    while (it != _data.end()) {
        auto next_it = std::next(it);
        if (next_it != _data.end() && std::get<1>(*it) == std::get<1>(*next_it)) {
            // Duplicate timestamp found, remove it
            it = _data.erase(it);
        }
        else {
            ++it;
        }
    }
}


std::chrono::milliseconds TimeSeries::bestMatch(std::chrono::milliseconds windowExtension,  TimeSeries& timeSeries)
{
    float bestSimilarity = 0.0;
    std::chrono::milliseconds bestOffset = std::chrono::milliseconds(0);

    // Iterate through the window range
    TimeSeries timeShifted;
    for (auto i = std::chrono::milliseconds(-windowExtension); i < windowExtension; i += std::chrono::milliseconds(1)) {
        timeShifted = timeSeries;
        timeShifted.shift(i);

        // Slice the time series for comparison
        TimeSeries newPredictionSlice = slice(std::get<1>(timeShifted.getVector().front()), std::get<1>(timeShifted.getVector().back()));

        // Check if the size difference is within a threshold
        if (std::abs(static_cast<int>(newPredictionSlice.getVector().size() - timeShifted.getVector().size())) > 2)
            continue;

        // Calculate similarity
        float curSimilarity = newPredictionSlice.similarity(timeShifted);

        // Update best match if similarity is higher
        if (curSimilarity > bestSimilarity) {
            bestSimilarity = curSimilarity;
            bestOffset = i;
        }
    }

    return bestOffset;
}