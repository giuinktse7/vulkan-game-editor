#include "time_util.h"

#include <date/date.h>

using namespace std::chrono;

const std::string TimePoint::DEFAULT_SERIALIZE_FORMAT = "{:%Y-%m-%dT%H:%M:%S}";
const std::string TimePoint::DEFAULT_PARSE_FORMAT = "%Y-%m-%dT%H:%M:%S";

TimePoint applicationStartTime;

TimePoint::TimePoint(steady_clock::time_point timePoint)
    : timePoint(timePoint) {}

TimePoint TimePoint::now()
{
    return TimePoint(steady_clock::now());
}

time_t TimePoint::elapsedMillis(TimePoint start) const
{
    return duration_cast<milliseconds>(this->timePoint - start.timePoint).count();
}

time_t TimePoint::elapsedMillis() const
{
    auto now = steady_clock::now();
    return duration_cast<milliseconds>(now - this->timePoint).count();
}

time_t TimePoint::elapsedMicros() const
{
    auto now = steady_clock::now();
    return duration_cast<microseconds>(now - this->timePoint).count();
}

time_t TimePoint::elapsedNanos() const
{
    auto now = steady_clock::now();
    return duration_cast<nanoseconds>(now - this->timePoint).count();
}

TimePoint TimePoint::sinceStart()
{
    return applicationStartTime;
}

void TimePoint::setApplicationStartTimePoint()
{
    applicationStartTime = TimePoint::now();
}

std::string TimePoint::toString(std::chrono::system_clock::time_point timestamp)
{
    return std::format("{:%Y-%m-%dT%H:%M:%S}", timestamp);
}

std::chrono::system_clock::time_point TimePoint::fromString(std::string raw, const std::string &format)
{
    using namespace std;
    using namespace date;

    auto now = system_clock::now();

    istringstream in{raw};
    date::sys_time<std::chrono::milliseconds> timePoint;

    in >> parse(format, timePoint);

    return std::chrono::time_point(timePoint);
}