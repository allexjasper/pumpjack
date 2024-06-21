#include "AbstractMovementPredictor.h"

#include <boost/log/trivial.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include "Monitor.h"


AbstractMovementPredictor::~AbstractMovementPredictor()
{

}




AbstractMovementPredictor::AbstractMovementPredictor(Sensor& sensor, std::chrono::milliseconds ms_to_predict,
    std::chrono::milliseconds ms_to_crossfade, std::chrono::milliseconds transmissionDelay) : _sensor(sensor),
    _shutdownRequested(false),
    _ms_to_predict(ms_to_predict),
    _ms_to_crossfade(ms_to_crossfade),
    _transmissionDelay(transmissionDelay)
{
}

void AbstractMovementPredictor::predictMovement(Sensor::Queue& inbound, ConsumeFunction consume, std::function<void(const std::string, TimeSeries&)> monitor)
{
    _predictThread = std::thread([this, &inbound, consume, monitor]() {
        predictMovementThread(inbound, consume, monitor);
        });

}


void AbstractMovementPredictor::predictMovementThread(Sensor::Queue& inbound, ConsumeFunction consume, std::function<void(const std::string, TimeSeries&)> monitor)
{
    TimeSeries inbound_ts;
    TimeSeries curPrediction;

    boost::circular_buffer<std::chrono::milliseconds> periodicity_buf(5);

    //std::chrono::milliseconds curBestOffset(0);
    //TimeSeries curTimeShifted;
    //TimeSeries curNewPredictionSlice;


    while (!_shutdownRequested)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto ts_pred_begin = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());


        // copy from queue into local time series
        inbound.consume_all([&inbound_ts](const TimeSeries::Sample& sample)
            {
                inbound_ts.add(sample);
            });
        monitor("raw", inbound_ts);
        inbound_ts.deduplicate();
        //assert(inbound_ts.checkConsistency());

        // resample to evently spaced 1 ms
        TimeSeries resampled_inbound = inbound_ts.resample();
        resampled_inbound.deduplicate();
        //assert(resampled_inbound.checkConsistency());

        // calc and check periodicity (after resampling for better accuracy)
        std::optional<std::chrono::milliseconds> periodicity;
        auto sinePeriodTuple = calcPeriodicity(resampled_inbound);//resampled_inbound.calcPeriodicitySine();
        if (sinePeriodTuple)
            periodicity = std::get<0>(*sinePeriodTuple);


        if (!periodicity)
        {
            BOOST_LOG_TRIVIAL(info) << "not enough data to predict" << std::endl;
            continue;
        }


        if (*periodicity < std::chrono::seconds(1) || *periodicity > std::chrono::seconds(30))
        {
            BOOST_LOG_TRIVIAL(info) << "periodicity out of range: " << periodicity->count() << std::endl;
        }
        else
        {
            periodicity_buf.push_back(*periodicity);
        }
        if (periodicity_buf.size() == 0)
            continue;

        // Calculate median
        std::vector<std::chrono::milliseconds> sorted_elements(periodicity_buf.begin(), periodicity_buf.end());
        std::sort(sorted_elements.begin(), sorted_elements.end());
        std::chrono::milliseconds median;
        size_t size = sorted_elements.size();
        auto median_period = sorted_elements[size / 2];






        // reduce original inbound buffer to 2.1 cylcles
        inbound_ts = inbound_ts.slice(std::get<1>(inbound_ts.getVector().back()) - std::chrono::milliseconds((int)(2.1 * median_period.count())), std::get<1>(inbound_ts.getVector().back()));
        //assert(inbound_ts.checkConsistency());

        auto resultExtend = extendOnPeriodicyity(resampled_inbound, median_period);
        TimeSeries newPredictionUnfiltered = std::get<0>(resultExtend);
        TimeSeries newPrediction = newPredictionUnfiltered; // newPredictionUnfiltered.filter(50, 2);
        //assert(newPrediction.checkConsistency());


        if (curPrediction.getVector().size() == 0)
        {
            curPrediction = newPrediction;
        }
        else
        {
            // in the renderer we are currently reading the data from time stamp Now() + transmissionDelay
            // hence until this point we want the old data, cross fade from that point on into the new data to avoid jumps

            auto refNow = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
            auto currentConsumerPos = refNow + _transmissionDelay;

            //BOOST_LOG_TRIVIAL(info) << "cur pred beg: " << std::chrono::steady_clock::to_time_t(std::get<1>(curPrediction.getVector().front();
            // keep the last 200 ms for potential overlaps. Can be increased for debugging
            // 
            //BOOST_LOG_TRIVIAL(info) << "cur pred beg: " << std::get<1>(curPrediction.getVector().front()) << " cur pred end: " << std::get<1>(curPrediction.getVector().back())
            //                        << "new pred beg: " << std::get<1>(newPrediction.getVector().front()) << " new pred end: " << std::get<1>(newPrediction.getVector().back()) << std::endl;

            //BOOST_LOG_TRIVIAL(info) << "cur pred duration: " << curPrediction.duration().count() << " new pred duration: " << newPrediction.duration().count();
            TimeSeries slicedOldPred = curPrediction.slice(refNow - std::chrono::milliseconds(200), currentConsumerPos + _ms_to_crossfade);
            TimeSeries slicedNewPred = newPrediction.slice(currentConsumerPos, currentConsumerPos + _ms_to_crossfade + _ms_to_predict);


            curPrediction = slicedOldPred.crossFade(slicedNewPred);

            //BOOST_LOG_TRIVIAL(info) << "cross faded beg: " << std::get<1>(curPrediction.getVector().front()) << " cross faded end: " << std::get<1>(curPrediction.getVector().back())   <<  std::endl;
            //BOOST_LOG_TRIVIAL(info) << "cross faded duration: " << curPrediction.duration().count();
            //assert(curPrediction.checkConsistency());
            //assert(old.checkConsistency());

            //monitor("old prediction", old);
            //monitor("new prediction", newPrediction);


            consume(curPrediction, median_period, std::get<1>(*sinePeriodTuple), std::get<1>(resultExtend), ""); // no overlay in non-calibration mode
        }

        auto ts_pred_end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());

        BOOST_LOG_TRIVIAL(info) << "prediction time: " << std::chrono::duration_cast<std::chrono::milliseconds>(ts_pred_end - ts_pred_begin).count() << std::endl;

    }
}

int fileNum = 0;

std::tuple<TimeSeries, std::chrono::milliseconds>  AbstractMovementPredictor::extendOnPeriodicyity(TimeSeries& ts, std::chrono::milliseconds periodicity)
{
    TimeSeries overlappingPreidction;
    TimeSeries crossFadedResult;

    const auto  correlationTimeSeriesLength = std::chrono::milliseconds(500);
    const auto  correlationSearchWindowSize = std::chrono::milliseconds(300);

    TimeSeries latestSamples = ts.slice(std::get<1>(ts.getVector().back()) - correlationTimeSeriesLength, std::get<1>(ts.getVector().back()));
    latestSamples.shift(-periodicity);

    auto bestOffset = ts.bestMatch(correlationSearchWindowSize, latestSamples);

    BOOST_LOG_TRIVIAL(info) << "current rotation offset: " << bestOffset.count() << std::endl;

#if 0
    std::string file_name = "match_" + boost::lexical_cast<std::string>(bestOffset.count()) + "_num_" + boost::lexical_cast<std::string>(fileNum++);
    std::map<std::string, TimeSeries> data;
    data["timeseries"] = ts.slice(std::get<1>(latestSamples.getVector().front()), std::get<1>(latestSamples.getVector().back()));
    data["latestSamples"] = latestSamples;
    Monitor::plot(file_name, data);
#endif


    auto sourceInd = ts.findIndex(std::get<1>(ts.getVector().back()) - _ms_to_crossfade - periodicity + bestOffset);
    if (!sourceInd)
    {
        BOOST_LOG_TRIVIAL(info) << "source index not found. TS too short" << std::endl;
        return { TimeSeries(), bestOffset };
    }

    if (periodicity <= _ms_to_predict + _ms_to_crossfade)
    {
        BOOST_LOG_TRIVIAL(info) << "periodicity must be longer than prediction + crossfade" << std::endl;
        return { TimeSeries(), bestOffset };
    }

    auto startInd = ts.findIndex(std::get<1>(ts.getVector().back()) - _ms_to_crossfade);
    auto predict_start_time = std::get<1>(ts.getVector()[*startInd]);
    for (int i = 0; i < (_ms_to_crossfade + _ms_to_predict).count(); i++) // predict cross fade ms of the already exisiiting data to allow for a smooth cross fade
    {
        overlappingPreidction.getVector().push_back({ std::get<0>(ts.getVector()[*sourceInd + i]), predict_start_time + std::chrono::milliseconds(i), 0 });
    }

    //assert(overlappingPreidction.checkConsistency());

    crossFadedResult = ts.crossFade(overlappingPreidction);
    //assert(crossFadedResult.checkConsistency());
    return { crossFadedResult, bestOffset };

}


void AbstractMovementPredictor::shutdown()
{
    _shutdownRequested = true;
    _predictThread.join();
}
