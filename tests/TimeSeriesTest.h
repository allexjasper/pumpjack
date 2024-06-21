#pragma once
#include <iostream>
#include <cassert>
#include <chrono>
#include "TimeSeries.h" // Include the TimeSeries header file

class TimeSeriesTest {
public:
    static void testSliceOverlap();
    static void testSlicePartialOverlap();
    static void testSliceNoOverlap();
    static void testCrossFadeOverlap();
    static void testCrossFadePartialOverlap();
    static void testCrossFadeNoOverlap();
};
