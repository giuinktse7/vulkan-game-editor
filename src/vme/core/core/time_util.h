#pragma once

#include <chrono>

class TimePoint
{
  private:
    static const std::string DEFAULT_SERIALIZE_FORMAT;
    static const std::string DEFAULT_PARSE_FORMAT;

  public:
    using time_t = long long;
    TimePoint(std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now());

    auto operator<=>(const TimePoint &other) const
    {
        return this->timePoint <=> other.timePoint;
    }

    static std::string toString(std::chrono::system_clock::time_point timestamp);

    static std::chrono::system_clock::time_point fromString(std::string raw, const std::string &format = DEFAULT_PARSE_FORMAT);

    static TimePoint now();
    static TimePoint sinceStart();
    static void setApplicationStartTimePoint();

    template <typename T>
    time_t timeSince(TimePoint other) const
    {
        return std::chrono::duration_cast<T>(this->timePoint - other.timePoint).count();
    }

    template <typename T>
    TimePoint addTime(time_t delta)
    {
        auto rawTime = std::chrono::time_point_cast<T>(this->timePoint).time_since_epoch().count() + delta;
        std::chrono::time_point<std::chrono::steady_clock> result(T{rawTime});

        return result;
    }

    template <typename T>
    TimePoint back(time_t delta)
    {
        return addTime<T>(-delta);
    }

    template <typename T>
    TimePoint forward(time_t delta)
    {
        return addTime<T>(delta);
    }

    TimePoint forwardMs(time_t delta)
    {
        return addTime<std::chrono::milliseconds>(delta);
    }

    time_t elapsedMillis() const;
    time_t elapsedMicros() const;
    time_t elapsedNanos() const;
    time_t elapsedMillis(TimePoint start) const;

  private:
    std::chrono::steady_clock::time_point timePoint;
};
