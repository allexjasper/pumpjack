#include "OilPumpRenderer.h"
#include <boost/log/trivial.hpp>

int positive_mod(int a, int b) {
    int result = a % b;
    if (result < 0) {
        result += b;
    }
    return result;
}


OilPumpRenderer::OilPumpRenderer(const std::string& fileName, int zeroAnglePos, bool fullscreen,
	float scale, std::chrono::milliseconds transmissionDelay) :
    OpenGLRenderer(fileName, fullscreen, scale),
    _zeroAnglePos(zeroAnglePos),
    _transmissionDelay(transmissionDelay)
{

}


float OilPumpRenderer::findAngleToRender(std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> ts, TimeSeries& localTS)
{
    int frame_to_render = 0;
    float angle = 0.0;
    auto ind = localTS.findIndex(ts + _transmissionDelay);
    if (ind)
    {
        angle = std::get<0>(localTS.getVector()[*ind]);

    }
    return angle;
}

int OilPumpRenderer::findFrameToRender(std::optional<int> prevFrame, float angle,
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> refNow, TimeSeries& localTS)
{
    refNow += _transmissionDelay;
    //BOOST_LOG_TRIVIAL(info) << "cur rotation offset: " << _curRoationOffset << std::endl;
    auto posInCurRotation = std::chrono::milliseconds((refNow - _lastPeriodBegin + _curRoationOffset).count() % (_periodicity.count() - _curRoationOffset.count()));
    //BOOST_LOG_TRIVIAL(info) << "posInCurRotation: " << posInCurRotation.count() << std::endl;
    auto time_per_frame = (float)_periodicity.count() / (float)_textures.size();
    float relative_pos = (float)posInCurRotation.count() / ((float)_periodicity.count() - (float)_curRoationOffset.count());
    int frameToRenderRawNotWrapped = relative_pos * (float)_textures.size() + _zeroAnglePos;
    int frameToRenderRaw = frameToRenderRawNotWrapped;

    // to speed adjustment to avoid skips
    int speedCompPrevFrame;
    if (prevFrame)
    {
        int speedCompPrevFrame = *prevFrame;
        if (std::abs(frameToRenderRawNotWrapped - *prevFrame) > _textures.size() / 2) // roll over case
        {
            frameToRenderRaw = frameToRenderRawNotWrapped;
            if (speedCompPrevFrame < frameToRenderRaw)
            {
                speedCompPrevFrame = *prevFrame + _textures.size();
            }
            else
            {
                frameToRenderRaw += _textures.size();
            }
        }

        if (std::abs(frameToRenderRaw - speedCompPrevFrame) * time_per_frame > 500) // skips bigger 500 ms are seeked
        {
            BOOST_LOG_TRIVIAL(info) << "seeking to due to large time gap: " << std::abs(frameToRenderRaw - speedCompPrevFrame) * time_per_frame << std::endl;
        }
        else
        {
            int maxSkip = (1000.0 / 40.0) / time_per_frame; // preserve at least 30 FPS
            frameToRenderRaw = std::min(std::max(*prevFrame + 3, frameToRenderRaw), *prevFrame + maxSkip);
        }

    }








    int frameToRender = positive_mod(frameToRenderRaw, _textures.size());;




    //if (frameToRenderFromMatch)
    //    frameToRender = *frameToRenderFromMatch;

    //BOOST_LOG_TRIVIAL(info) << "periodicity: " << _periodicity;
    //BOOST_LOG_TRIVIAL(info) << "pos in period: " << posInCurRotation << " rel pos: " << relative_pos;
    //BOOST_LOG_TRIVIAL(info) << "frame_to_render: " << frameToRender << " frame to render angel: " << std::get<1>(_textures[frameToRender]) << " rot angele: " << angle << " frame_as_per_time: " << frameToRenderRawNotWrapped << std::endl;
    if (prevFrame && frameToRender - *prevFrame > 10)
        BOOST_LOG_TRIVIAL(info) << "large frame skip from: " << *prevFrame << " to " << frameToRender << std::endl;
    prevFrame = frameToRender;
    return frameToRender;
  
}
