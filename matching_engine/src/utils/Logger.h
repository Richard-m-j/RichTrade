#pragma once
#include <string>
#include <mutex>
#include <fstream>

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERR };

    static void setLogFile(const std::string& filename);
    static void setLevel(Level level);
    static void log(Level level, const std::string& msg);
    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void err(const std::string& msg);

private:
    static std::mutex mtx_;
    static std::ofstream file_;
    static Level log_level_;
    static bool to_file_;
    static void writeLog(const std::string& level_str, const std::string& msg);
}; 