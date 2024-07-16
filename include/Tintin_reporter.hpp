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
class Tintin_reporter {
    public:
        ~Tintin_reporter();
        void log(int loglevel, const std::string &str);
        void initializeLogFile();
        static Tintin_reporter& getInstance();
        // Delete copy constructor and copy assignment operator
        Tintin_reporter(const Tintin_reporter &rhs) = delete;
        Tintin_reporter &operator=(const Tintin_reporter &rhs) = delete;

    private:
        // Private constructor to prevent instantiation
        Tintin_reporter();
        // Move constructor and move assignment operator
        Tintin_reporter(Tintin_reporter&& other) noexcept;
        Tintin_reporter& operator=(Tintin_reporter&& other) noexcept;

        std::ofstream logfile;
        std::string logfileName;
        std::string addTimestampAndLogLevel(int logLevel, const std::string &str) const;
        void createLogDirectory();
        void openLogFile();

        static std::unique_ptr<Tintin_reporter> instance;
};

#endif 
