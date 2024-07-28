#include <iostream>
#include <string>
#include <vector>
#include <thread>
//#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <chrono>
#include <mutex>
#include <fstream>
#include <optional>
#include <deque>
#include <boost/lexical_cast.hpp>
#include <tuple>
#include <boost/program_options.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/settings_parser.hpp>
//#include <windows.h>
#include <numbers>
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include "TimeSeries.h"
#include "Sensor.h"
#include "ReplaySensor.h"

#include "SimulationSensor.h"
#include "OilPumpMovementPredictor.h"
#include "WheelMovementPredictor.h"

#include "Monitor.h"
#include "UsbSensor.h"
//#include "TimeSeriesTest.h"
#include <memory>
#include "WheelSimulationSensor.h"
#include "OilPumpRenderer.h"
#include "WheelRenderer.h"
#include <SDL.h>




#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//using namespace boost::asio;
using namespace std;
namespace po = boost::program_options;
namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

bool plot_graph = false;


//size_t inboundBufferSize = 20000; // Modify as needed
//const size_t rotationsBufferSize = 50; // Modify as needed
//const size_t plotBufferSize = 30000; // Modify as needed

typedef std::chrono::time_point<std::chrono::steady_clock> Timestamp;
typedef std::tuple<float, Timestamp> Sample; // angle, time
TimeSeries  RawData;
typedef  std::deque<Sample>  RawDataDeueue;
RawDataDeueue inbound;

src::severity_logger< logging::trivial::severity_level > lg;

std::mutex lock_inbound;

Timestamp reference_time;

float magnetOffset = 0.0;
std::chrono::milliseconds clock_offset = std::chrono::milliseconds(0);

const int numSamples = 10; // Number of sample points
const double amplitudeRange = 17.0; // Desired amplitude range



typedef std::tuple<Timestamp, std::optional<float>, std::optional<float>> RenderPoint; // timestamp, angle raw, angle corrected
std::deque<RenderPoint> plot_data; // timestamp, angle raw, angle corrected
std::mutex lock_plot_data;


namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

// Initialize Boost::Log
class NullSink : public sinks::basic_sink_backend<sinks::concurrent_feeding>
{
public:
    void consume(logging::record_view const& rec) {}
};

void initLogging(bool doLog)
{
    if (doLog)
    {
        logging::add_file_log
        (
            keywords::file_name = "rotating_display_%N.log",
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::format = "[%TimeStamp%]: %Message%"
        );
    }
    else
    {
        typedef sinks::synchronous_sink<NullSink> sink_t;
        boost::shared_ptr<sink_t> null_sink = boost::make_shared<sink_t>();
        logging::core::get()->add_sink(null_sink);
    }

    logging::add_common_attributes();
}





Sensor* g_sensor;

int main(int argc, char* argv[]) {

    //TimeSeriesTest::testSliceOverlap();
   // TimeSeriesTest::testSlicePartialOverlap();
  //  TimeSeriesTest::testSliceNoOverlap();
  //  TimeSeriesTest::testCrossFadeOverlap();
 //   TimeSeriesTest::testCrossFadePartialOverlap();
  //  TimeSeriesTest::testCrossFadeNoOverlap();


    reference_time = std::chrono::steady_clock::now();

    float magnet_offset;
    int time_offset;  // Change to integer
    float scale;  // Change to integer
    bool fullscreen;
    bool simulate;
    bool replay;
    std::string replayFile;
    std::string videoFile;
    int zeroAnglePos;
    bool calibrationMode;
    bool wheelMode;
    bool doLog;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("magnet_offset,m", po::value<float>(&magnet_offset)->default_value(0.0), "Magnet Offset (float)")
        ("time_offset,t", po::value<int>(&time_offset)->default_value(0), "Time Offset (integer)")
        ("scale,s", po::value<float>(&scale)->default_value(1.0), "Scaling of video (float)")
        ("fullscreen,fs", po::value<bool>(&fullscreen)->default_value(true), "Fullscreen (bool)")
        ("simulate,s", po::value<bool>(&simulate)->default_value(false), "Simluate sensor data (bool)")
        ("replay,rp", po::value<bool>(&replay)->default_value(false), "Replay sensor data (bool)")
        ("replay_file,rp_file", po::value<std::string>(&replayFile)->default_value("unspecified"), "Replay sensor data file (string)")
        ("plot_graph,g", po::value<bool>(&plot_graph)->default_value(false), "Plot the debugging graph (bool)")
        ("video_file,vf", po::value<std::string>(&videoFile), "Video file (string)")
        ("zero_angle_pos,zap", po::value<int>(&zeroAnglePos)->default_value(0), "Video file (integer milliseconds)")
        ("calibration_mode,cm", po::value<bool>(&calibrationMode)->default_value(false), "Calibration mode (bool)")
        ("wheel_mode,wm", po::value<bool>(&wheelMode)->default_value(false), "Wheel mode (bool)")
        ("log,log", po::value<bool>(&doLog)->default_value(false), "Log (bool)");


   

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    initLogging(doLog);
   

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        BOOST_LOG_TRIVIAL(info) << "SDL could not initialize! SDL Error:\n" << SDL_GetError();
    }

    if (vm.count("magnet_offset")) {
        BOOST_LOG_TRIVIAL(trace) << "Magnet Offset: " << magnet_offset << std::endl;
    }

    if (vm.count("time_offset")) {
        BOOST_LOG_TRIVIAL(trace) << "Time Offset: " << time_offset << std::endl;
    }

    if (replay)
        g_sensor = new ReplaySensor(replayFile);

    if( !replay && simulate)
        if (wheelMode)
        {
            g_sensor = new WheelSimulationSensor();
        }
        else
        {
            g_sensor = new SimulationSensor();
        }

    if (!replay && !simulate)
        g_sensor = new UsbSensor(magnet_offset);

    //magnetOffset = magnet_offset;
    
    Sensor::Queue inbound_queue;
    g_sensor->readData(inbound_queue);

    if (calibrationMode)
    {
        while (true)
        {
            inbound_queue.consume_all([](const TimeSeries::Sample& sample)
                {
                    std::cout << "angle: " << std::get<0>(sample) << "\n";
                });
        }
    }


   
    Monitor monitor(doLog);

    // start sensor reading, prediction and rendering
    std::unique_ptr<AbstractMovementPredictor> predictor;
    std::unique_ptr<OpenGLRenderer> renderer;
    
   
    if (!wheelMode)
    {
        predictor = std::unique_ptr<AbstractMovementPredictor>(new OilPumpMovementPredictor(*g_sensor, std::chrono::milliseconds(1000), 
                                                                std::chrono::milliseconds(80), std::chrono::milliseconds(time_offset)));
        renderer = std::unique_ptr<OpenGLRenderer>(new OilPumpRenderer(videoFile, zeroAnglePos, fullscreen, scale, std::chrono::milliseconds(time_offset)));
    }
    else
    {
        renderer = std::unique_ptr<OpenGLRenderer>(new WheelRenderer(videoFile, fullscreen, scale, inbound_queue));
    }
    


    

    
   

    if (predictor)
    {
        predictor->predictMovement(inbound_queue, [&renderer](const TimeSeries& ts, std::chrono::milliseconds periodicity, TimeSeries::Timestamp lastPeriodBegin, std::chrono::milliseconds curRoationOffset, const std::string& overlay)
            {
                renderer->feedData(ts, periodicity, lastPeriodBegin, curRoationOffset, overlay);
            },
            [&monitor](const std::string& title, TimeSeries& ts) {
                monitor.addData(title, ts);
            });
    }
    monitor.monitor();
    renderer->render([&monitor](const std::string& title, TimeSeries& ts) {
        monitor.addData(title, ts);
        });

    

    //OpenGLRenderer::blockingMainLoop();
  
    

    renderer->shutdown();
    if(predictor)
        predictor->shutdown();
    monitor.shutdown();
    g_sensor->shutdown();

    return 0;
}
