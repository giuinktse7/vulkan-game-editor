#include "time_point.h"

using namespace std::chrono;

TimePoint::TimePoint(steady_clock::time_point timePoint)
    : timePoint(timePoint)
{
}

TimePoint TimePoint::now()
{
    return TimePoint(steady_clock::now());
}

time_t TimePoint::elapsedMillis(TimePoint start)
{
    return duration_cast<milliseconds>(this->timePoint - start.timePoint).count();
}

time_t TimePoint::elapsedMillis()
{
    auto now = steady_clock::now();
    return duration_cast<milliseconds>(now - this->timePoint).count();
}
