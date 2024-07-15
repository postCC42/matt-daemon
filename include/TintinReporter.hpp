#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <memory>

#define LOGFILE_PATH "/var/log/matt_daemon/matt_daemon.log"

enum LogLevel {
    LOGLEVEL_LOG,
    LOGLEVEL_INFO,
    LOGLEVEL_WARN,
    LOGLEVEL_ERROR
};

// SINGLETON PATTERN _ COPLIEN IS ADAPTED
class TintinReporter {
    public:
        ~TintinReporter();
        void log(int loglevel, const std::string &str);
        void initializeLogFile();
        static TintinReporter& getInstance();
        // Delete copy constructor and copy assignment operator
        TintinReporter(const TintinReporter &rhs) = delete;
        TintinReporter &operator=(const TintinReporter &rhs) = delete;

    private:
        // Private constructor to prevent instantiation
        TintinReporter();
        // Move constructor and move assignment operator
        TintinReporter(TintinReporter&& other) noexcept;
        TintinReporter& operator=(TintinReporter&& other) noexcept;

        std::ofstream logfile;
        std::string logfileName;
        std::string addTimestampAndLogLevel(int logLevel, const std::string &str) const;
        void createLogDirectory();
        void openLogFile();

        static std::unique_ptr<TintinReporter> instance;
};

#endif 
