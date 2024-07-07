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

#define DFLT_LOGFILE_PATH "/var/log/matt_daemon/matt_daemon.log"

// Define your logging levels if not already defined
enum LogLevel {
    LOGLVL_LOG,
    LOGLVL_INFO,
    LOGLVL_WARN,
    LOGLVL_ERROR
};

class Tintin_reporter {
public:
    Tintin_reporter();
    explicit Tintin_reporter(const std::string &default_log_filename);
    ~Tintin_reporter();
    Tintin_reporter(const Tintin_reporter &rhs);
    Tintin_reporter &operator=(const Tintin_reporter &rhs);

    void log(int loglevel, const std::string &str) const;

    friend std::ostream &operator<<(std::ostream &out, const Tintin_reporter &tintin);

private:
    std::ofstream *_logfile;
    std::string _logfileName;

    std::string _buildLogEntry(int loglvl, const std::string &str) const;
    void setupLogFile();
};

#endif // TINTIN_REPORTER_HPP
