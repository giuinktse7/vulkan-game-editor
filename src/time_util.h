#pragma once

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

class TimePoint
{
  public:
    using time_t = long long;
    TimePoint(std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now());

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

class ManualEvent
{
  public:
    explicit ManualEvent(bool signaled = false)
        : m_signaled(signaled) {}

    void signal()
    {
        {
            std::unique_lock lock(m_mutex);
            m_signaled = true;
        }
        m_cv.notify_all();
    }

    void wait()
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [&]() { return m_signaled != false; });
    }

    template <typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period> &t)
    {
        std::unique_lock lock(m_mutex);
        bool result = m_cv.wait_for(lock, t, [&]() { return m_signaled != false; });
        return result;
    }

    template <typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration> &t)
    {
        std::unique_lock lock(m_mutex);
        bool result = m_cv.wait_until(lock, t, [&]() { return m_signaled != false; });
        return result;
    }

    void reset()
    {
        std::unique_lock lock(m_mutex);
        m_signaled = false;
    }

  private:
    bool m_signaled = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class AutoEvent
{
  public:
    explicit AutoEvent(bool signaled = false)
        : m_signaled(signaled) {}

    void signal()
    {
        {
            std::unique_lock lock(m_mutex);
            m_signaled = true;
        }
        condition.notify_one();
    }

    void wait()
    {
        std::unique_lock lock(m_mutex);
        condition.wait(lock, [&]() { return m_signaled != false; });
        m_signaled = false;
    }

    template <typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period> &t)
    {
        std::unique_lock lock(m_mutex);
        bool result = condition.wait_for(lock, t, [&]() { return m_signaled != false; });
        if (result)
            m_signaled = false;
        return result;
    }

    template <typename Clock, typename Duration>
    bool waitUntil(const std::chrono::time_point<Clock, Duration> &t)
    {
        std::unique_lock lock(m_mutex);
        bool result = condition.wait_until(lock, t, [&]() { return m_signaled != false; });
        if (result)
            m_signaled = false;
        return result;
    }

  private:
    bool m_signaled = false;
    std::mutex m_mutex;
    std::condition_variable condition;
};

class Timer
{
  public:
    template <typename T>
    Timer(T &&tick)
        : tick(std::chrono::duration_cast<std::chrono::nanoseconds>(tick)), thread([this]() {
              assert(this->tick.count() > 0);
              auto start = std::chrono::high_resolution_clock::now();
              std::chrono::nanoseconds drift{0};
              while (!event.waitFor(this->tick - drift))
              {
                  ++ticks;
                  auto it = std::begin(events);
                  auto end = std::end(events);
                  while (it != end)
                  {
                      auto &event = *it;
                      ++event.elapsed;
                      if (event.elapsed == event.ticks)
                      {
                          auto remove = event.proc();
                          if (remove)
                          {
                              events.erase(it++);
                              continue;
                          }
                          else
                          {
                              event.elapsed = 0;
                          }
                      }
                      ++it;
                  }
                  auto now = std::chrono::high_resolution_clock::now();
                  auto realDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);
                  auto fakeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(this->tick * ticks);
                  drift = realDuration - fakeDuration;
              }
          })
    {
    }

    ~Timer()
    {
        event.signal();
        thread.join();
    }

    template <typename T, typename F, typename... Args>
    auto setTimeout(T &&timeout, F f, Args &&...args)
    {
        assert(std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count() >= tick.count());

        auto event = std::make_shared<ManualEvent>();

        auto procedure = [=]() {
            if (event->waitFor(std::chrono::seconds(0)))
                return true;

            f(args...);

            return true;
        };

        events.insert({EventContext::nextSequenceId++, procedure,
                       static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::nanoseconds>(timeout).count() / tick.count()), 0, event});
        return event;
    }

    template <typename T, typename F, typename... Args>
    auto setInterval(T &&interval, F f, Args &&...args)
    {
        assert(std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count() >= tick.count());

        auto event = std::make_shared<ManualEvent>();

        auto procedure = [=]() {
            if (event->waitFor(std::chrono::seconds(0)))
                return true;

            f(args...);

            return false;
        };

        events.insert({EventContext::nextSequenceId++, procedure,
                       static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count() / tick.count()), 0, event});
        return event;
    }

  private:
    std::chrono::nanoseconds tick;
    unsigned long long ticks = 0;
    ManualEvent event;
    std::thread thread;

    struct EventContext
    {
        bool operator<(const EventContext &rhs) const
        {
            return sequenceId < rhs.sequenceId;
        }
        static inline unsigned long long nextSequenceId = 0;
        unsigned long long sequenceId;
        std::function<bool(void)> proc;
        unsigned long long ticks;
        mutable unsigned long long elapsed;
        std::shared_ptr<ManualEvent> event;
    };

    using set = std::set<EventContext>;
    set events;
};
