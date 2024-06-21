#include "TimeSeriesTest.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include "TimeSeries.h" // Include the TimeSeries header file


     void TimeSeriesTest::testSliceOverlap() {
        // Create a TimeSeries object
        TimeSeries ts;

        // Define fixed timestamps for sample data
        auto startTime = std::chrono::steady_clock::time_point(std::chrono::seconds(0));
        auto timeIncrement = std::chrono::seconds(1);

        // Add sample data to the time series with fixed timestamps
        for (int i = 1; i <= 5; ++i) {
            ts.add({ static_cast<float>(i), startTime + (i - 1) * timeIncrement });
        }

        // Slice the time series with overlap
        auto startSliceTime = startTime + timeIncrement / 2; // Overlaps with the first sample
        auto endSliceTime = startTime + 3 * timeIncrement; // Overlaps with the last sample
        auto slicedTs = ts.slice(startSliceTime, endSliceTime);

        // Assertion tests
        assert(slicedTs.getVector().size() == 4); // Check if the sliced time series has 4 samples
        assert(std::get<0>(slicedTs.getVector()[0]) == 1.0f); // Check the first sample after slicing
        assert(std::get<0>(slicedTs.getVector()[1]) == 2.0f); // Check the second sample after slicing
        assert(std::get<0>(slicedTs.getVector()[2]) == 3.0f); // Check the third sample after slicing
        //assert(std::get<0>(slicedTs.getVector()[3]) == 4.0f); // Check the fourth sample after slicing
    }

     void TimeSeriesTest::testSlicePartialOverlap() {
        // Create a TimeSeries object
        TimeSeries ts;

        // Define fixed timestamps for sample data
        auto startTime = std::chrono::steady_clock::time_point(std::chrono::seconds(3));
        auto timeIncrement = std::chrono::seconds(1);

        // Add sample data to the time series with fixed timestamps
        for (int i = 1; i <= 5; ++i) {
            ts.add({ static_cast<float>(i), startTime + (i - 1) * timeIncrement });
        }

        // Slice the time series with partial overlap in both directions
        auto startSliceTime = std::chrono::steady_clock::time_point(std::chrono::seconds(1)); 
        auto endSliceTime = std::chrono::steady_clock::time_point(std::chrono::seconds(4));
        auto slicedTs = ts.slice(startSliceTime, endSliceTime);

        // Assertion tests
        assert(slicedTs.getVector().size() == 2); // Check if the sliced time series has 4 samples
     
        assert(std::get<0>(slicedTs.getVector()[0]) == 1.0f); // Check the third sample after slicing
        assert(std::get<0>(slicedTs.getVector()[1]) == 2.0f); // Check the fourth sample after slicing
    }

     void TimeSeriesTest::testSliceNoOverlap() {
        // Create a TimeSeries object
        TimeSeries ts;

        // Define fixed timestamps for sample data
        auto startTime = std::chrono::steady_clock::time_point(std::chrono::seconds(10));
        auto timeIncrement = std::chrono::seconds(1);

        // Add sample data to the time series with fixed timestamps
        for (int i = 1; i <= 5; ++i) {
            ts.add({ static_cast<float>(i), startTime + (i - 1) * timeIncrement });
        }

        // Slice the time series with no overlap before and after
        auto startSliceTime = std::chrono::steady_clock::time_point(std::chrono::seconds(1));
        auto endSliceTime = std::chrono::steady_clock::time_point(std::chrono::seconds(4));
        auto slicedTs = ts.slice(startSliceTime, endSliceTime);

        // Assertion tests
        assert(slicedTs.getVector().size() == 0); 
    }


      void TimeSeriesTest::testCrossFadeOverlap() {
          // Create two TimeSeries objects
          TimeSeries ts1, ts2;

          // Define fixed timestamps for sample data
          auto startTime = std::chrono::steady_clock::time_point(std::chrono::seconds(0));
          auto timeIncrement = std::chrono::seconds(1);

          // Add sample data to the first time series with fixed timestamps
          for (int i = 1; i <= 10; ++i) {
              ts1.add({ 1.0, startTime + (i - 1) * timeIncrement });
          }

          // Add sample data to the second time series with fixed timestamps
          for (int i = 4; i <= 20; ++i) {
              ts2.add({ 10.0, startTime + (i - 1) * timeIncrement });
          }

          // Perform crossFade between the two time series
          auto resultTs = ts1.crossFade(ts2);

          // Assertion tests
          auto& resultVector = resultTs.getVector();
          assert(resultVector.size() == 18); // Check if the result time series has 8 samples
     }

     void TimeSeriesTest::testCrossFadePartialOverlap() {
         // Create two TimeSeries objects
         TimeSeries ts1, ts2;

         // Define fixed timestamps for sample data
         auto startTime = std::chrono::steady_clock::time_point(std::chrono::seconds(0));
         auto timeIncrement = std::chrono::seconds(1);

         // Add sample data to the first time series with fixed timestamps
         for (int i = 1; i <= 5; ++i) {
             ts1.add({ static_cast<float>(i), startTime + (i - 1) * timeIncrement });
         }

         // Add sample data to the second time series with fixed timestamps
         for (int i = 6; i <= 10; ++i) {
             ts2.add({ static_cast<float>(i), startTime + (i - 1) * timeIncrement });
         }

         // Perform crossFade between the two time series
         auto resultTs = ts1.crossFade(ts2);

         // Assertion tests
         auto& resultVector = resultTs.getVector();
         assert(resultVector.size() == 10); // Check if the result time series has 10 samples
         assert(std::get<0>(resultVector[4]) == 6.0f); // Check the first sample after crossFade
         assert(std::get<0>(resultVector[5]) == 7.0f); // Check the second sample after crossFade
         assert(std::get<0>(resultVector[6]) == 8.0f); // Check the third sample after crossFade
         assert(std::get<0>(resultVector[7]) == 9.0f); // Check the fourth sample after crossFade
         assert(std::get<0>(resultVector[8]) == 10.0f); // Check the fifth sample after crossFade
     }

      void TimeSeriesTest::testCrossFadeNoOverlap() {
          // Create two TimeSeries objects
          TimeSeries ts1, ts2;

          // Define fixed timestamps for sample data
          auto startTime = std::chrono::steady_clock::time_point(std::chrono::seconds(0));
          auto timeIncrement = std::chrono::seconds(1);

          // Add sample data to the first time series with fixed timestamps
          for (int i = 1; i <= 5; ++i) {
              ts1.add({ static_cast<float>(i), startTime + (i - 1) * timeIncrement });
          }

          // Add sample data to the second time series with fixed timestamps
          for (int i = 6; i <= 10; ++i) {
              ts2.add({ static_cast<float>(i), startTime + (i - 1) * timeIncrement });
          }

          // Perform crossFade between the two time series
          auto resultTs = ts1.crossFade(ts2);

          // Assertion tests
          auto& resultVector = resultTs.getVector();
          assert(resultVector.size() == 10); // Check if the result time series has 10 samples
          assert(std::get<0>(resultVector[0]) == 1.0f); // Check the first sample after crossFade
          assert(std::get<0>(resultVector[1]) == 2.0f); // Check the second sample after crossFade
          assert(std::get<0>(resultVector[2]) == 3.0f); // Check the third sample after crossFade
          assert(std::get<0>(resultVector[3]) == 4.0f); // Check the fourth sample after crossFade
          assert(std::get<0>(resultVector[4]) == 5.0f); // Check the fifth sample after crossFade
          assert(std::get<0>(resultVector[5]) == 6.0f); // Check the sixth sample after crossFade
          assert(std::get<0>(resultVector[6]) == 7.0f); // Check the seventh sample after crossFade
          assert(std::get<0>(resultVector[7]) == 8.0f); // Check the eighth sample after crossFade
          assert(std::get<0>(resultVector[8]) == 9.0f); // Check the ninth sample after crossFade
          assert(std::get<0>(resultVector[9]) == 10.0f); // Check the tenth sample after crossFade
     }

