#ifndef TIMING_H
#define TIMING_H

#include <chrono>

class Stopwatch
{
public:
    void Start()
    {
        m_start = std::chrono::high_resolution_clock::now();
    }
    void Stop()
    {
        m_stop = std::chrono::high_resolution_clock::now();
    }
    long ElapsedNanoseconds()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(m_stop - m_start).count();
    }
    double ElapsedMilliseconds()
    {
        long ns = ElapsedNanoseconds();
        return ns / 1e6;
    }
private:
    std::chrono::time_point<std::chrono::system_clock> m_start;
    std::chrono::time_point<std::chrono::system_clock> m_stop;
};

class Duration
{
public:
    Duration(long duration) :
        m_duration(duration)
    {}

    double Seconds()
    {
        return m_duration / 1000000000.0;
    }

    double Milliseconds()
    {
        return m_duration / 1000000.0;
    }

    long Nanoseconds()
    {
        return m_duration;
    }

private:
    long m_duration;
};

class Timestamp
{
public:
    Timestamp() :
        m_timestamp(std::chrono::high_resolution_clock::now())
    {}

    Timestamp(std::chrono::time_point<std::chrono::system_clock> timestamp) :
        m_timestamp(timestamp)
    {}

    static Timestamp now()
    {
        return { std::chrono::high_resolution_clock::now() };
    }

    Duration operator-(const Timestamp& other)
    {
        long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(m_timestamp - other.m_timestamp).count();
        return { ns };
    }
private:
    std::chrono::time_point<std::chrono::system_clock> m_timestamp;
};

#endif // TIMING_H
