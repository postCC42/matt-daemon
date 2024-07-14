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

class TintinReporter {
    public:
        TintinReporter();
        ~TintinReporter();
        TintinReporter(const TintinReporter &rhs);
        TintinReporter &operator=(const TintinReporter &rhs);
        void log(int loglevel, const std::string &str) const;
        friend std::ostream &operator<<(std::ostream &out, const TintinReporter &tintin);

        static TintinReporter& getInstance();
        void initializeLogFile();

    private:
        std::ofstream *logfile;
        std::string logfileName;
        std::string addTimestampAndLogLevel(int logLevel, const std::string &str) const;
        void createLogDirectory();
        void removeExistingLogFile();
        void openLogFile();
        void redirectStderrToLogFile();
        void redirectStdoutToStderr();

        static std::unique_ptr<TintinReporter> instance;
};

#endif 
