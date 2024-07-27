#pragma once
#include "OpenGLRenderer.h"
#include "Sensor.h"


class WheelRenderer :
    public OpenGLRenderer
{
public:
    WheelRenderer(const std::string& fileName, bool fullscreen, float scale, Sensor::Queue& inbound);

public:
    int findFrameToRender(std::optional<int> prevFrame, float angle,
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> refNow, TimeSeries& localTS);
    float findAngleToRender(std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> ts, TimeSeries& localTS);

    Sensor::Queue& _inbound;
};

