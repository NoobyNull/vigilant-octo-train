#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace dw {

struct TimingEntry {
    std::string name;
    double duration_ms;
};

class StartupTimer {
public:
    void begin(const std::string& name) {
        m_current_name = name;
        m_start_time = std::chrono::high_resolution_clock::now();
    }

    void end() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - m_start_time);
        m_entries.push_back({m_current_name, duration.count()});

        // Log immediately during startup
        printf("STARTUP: %s took %.2f ms\n", m_current_name.c_str(), duration.count());
    }

    void printReport() const {
        printf("\n=== STARTUP TIMING REPORT ===\n");
        double total = 0.0;
        for (const auto& entry : m_entries) {
            printf("%s: %.2f ms\n", entry.name.c_str(), entry.duration_ms);
            total += entry.duration_ms;
        }
        printf("TOTAL: %.2f ms\n", total);
        printf("=============================\n\n");
    }

private:
    std::string m_current_name;
    std::chrono::high_resolution_clock::time_point m_start_time;
    std::vector<TimingEntry> m_entries;
};

// RAII timing helper
class ScopedTimer {
public:
    ScopedTimer(StartupTimer& timer, const std::string& name) : m_timer(timer) {
        m_timer.begin(name);
    }
    ~ScopedTimer() {
        m_timer.end();
    }
private:
    StartupTimer& m_timer;
};

#define TIME_STARTUP(timer, name) ScopedTimer _timer(timer, name)

} // namespace dw