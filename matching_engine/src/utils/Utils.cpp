#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Utils {
    std::string getCurrentTimestamp() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        auto us = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(6) << us.count() << "Z";
        return oss.str();
    }

    std::string toUpper(const std::string& str) {
        std::string out = str;
        std::transform(out.begin(), out.end(), out.begin(), ::toupper);
        return out;
    }

    std::string toLower(const std::string& str) {
        std::string out = str;
        std::transform(out.begin(), out.end(), out.begin(), ::tolower);
        return out;
    }
} 