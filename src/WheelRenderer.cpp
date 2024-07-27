#include "WheelRenderer.h"

WheelRenderer::WheelRenderer(const std::string& fileName, bool fullscreen, float scale, Sensor::Queue& inbound) : _inbound(inbound),
OpenGLRenderer(fileName, fullscreen, scale)
{

}



float WheelRenderer::findAngleToRender(std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> ts, TimeSeries& localTS)
{
    // copy from queue into local time series
    _inbound.consume_all([&localTS](const TimeSeries::Sample& sample)
        {
            localTS.add(sample);
        });
    float angle = 0.0;

    if (!localTS.getVector().empty())
        angle = std::get<0>(localTS.getVector().back());

    return angle;
}


int WheelRenderer::findFrameToRender(std::optional<int> prevFrame, float angle,
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> refNow, TimeSeries& localTS)
{
    if (prevFrame)
    {
        int frameToRender = (*prevFrame + 1) % _textures.size();
        //BOOST_LOG_TRIVIAL(info) << "frame_to_render: " << frameToRender << std::endl;
        return frameToRender;
    }
    else
    {
        return 0;
    }
    
}