#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>

std::mutex Logger::mtx_;
std::ofstream Logger::file_;
Logger::Level Logger::log_level_ = Logger::Level::INFO;
bool Logger::to_file_ = false;

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    file_.open(filename, std::ios::app);
    to_file_ = file_.is_open();
}

void Logger::setLevel(Level level) {
    log_level_ = level;
}

void Logger::log(Level level, const std::string& msg) {
    if (level < log_level_) return;
    std::string level_str;
    switch (level) {
        case Level::DEBUG: level_str = "DEBUG"; break;
        case Level::INFO:  level_str = "INFO";  break;
        case Level::WARN:  level_str = "WARN";  break;
        case Level::ERR:   level_str = "ERR";   break;
    }
    writeLog(level_str, msg);
}

void Logger::debug(const std::string& msg) { log(Level::DEBUG, msg); }
void Logger::info(const std::string& msg)  { log(Level::INFO, msg); }
void Logger::warn(const std::string& msg)  { log(Level::WARN, msg); }
void Logger::err(const std::string& msg)   { log(Level::ERR, msg); }

void Logger::writeLog(const std::string& level_str, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto now = std::chrono::system_clock::now();
    auto t_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t_c), "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    std::string log_line = oss.str() + " [" + level_str + "] " + msg + "\n";
    if (to_file_ && file_.is_open()) {
        file_ << log_line;
        file_.flush();
    } else {
        std::cout << log_line;
    }
} 