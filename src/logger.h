#pragma once
#include "headers.h"

namespace vulkan_proto {
struct Logger {
    std::chrono::steady_clock::time_point m_startingTime;
    std::stringstream m_timeSS;
    std::ofstream m_fileStream;

    Logger(const char *fName) {
        m_startingTime = std::chrono::steady_clock::now();
        m_fileStream.open(fName, std::ios::out);
    }
    ~Logger() {}

    inline void formatTime() {
        m_timeSS.clear();
        m_timeSS.str(std::string());
        auto now = std::chrono::steady_clock::now();
        auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_startingTime);
        auto hours =
            std::chrono::duration_cast<std::chrono::hours>(milliseconds);
        milliseconds -=
            std::chrono::duration_cast<std::chrono::milliseconds>(hours);
        auto minutes =
            std::chrono::duration_cast<std::chrono::minutes>(milliseconds);
        milliseconds -=
            std::chrono::duration_cast<std::chrono::milliseconds>(minutes);
        auto seconds =
            std::chrono::duration_cast<std::chrono::seconds>(milliseconds);
        milliseconds -=
            std::chrono::duration_cast<std::chrono::milliseconds>(seconds);
        m_timeSS << "[";
        if (hours.count() < 10) {
            m_timeSS << "0";
        }
        m_timeSS << hours.count() << ":";

        if (minutes.count() < 10) {
            m_timeSS << "0";
        }
        m_timeSS << minutes.count() << ":";

        if (seconds.count() < 10) {
            m_timeSS << "0";
        }
        m_timeSS << seconds.count() << ":";

        if (milliseconds.count() < 100) {
            m_timeSS << "0";
        }
        if (milliseconds.count() < 10) {
            m_timeSS << "0";
        }
        m_timeSS << milliseconds.count() << "]";
    }

    void log(std::string msg) {
#ifndef NDEBUG
        formatTime();
        std::cout << m_timeSS.str().c_str() << " " << msg.c_str() << std::endl;
        if (m_fileStream.is_open()) {
            m_fileStream << m_timeSS.str().c_str() << " " << msg.c_str()
                         << std::endl;
        }
#endif
    }

    void flush() { m_fileStream.flush(); }
};
} // namespace vulkan_proto
