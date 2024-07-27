#pragma once
#include "OpenGLRenderer.h"
class OilPumpRenderer :
    public OpenGLRenderer
{
public:
    OilPumpRenderer(const std::string& fileName, int zeroAnglePos, bool fullscreen, float scale, std::chrono::milliseconds transmissionDelay);
    int findFrameToRender(std::optional<int> prevFrame, float angle,
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> refNow, TimeSeries& localTS);
    float findAngleToRender(std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> ts, TimeSeries& localTS);

public:
    int _zeroAnglePos;
    std::chrono::milliseconds _transmissionDelay;
};

