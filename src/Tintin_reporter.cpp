#include "Tintin_reporter.hpp"



Tintin_reporter::Tintin_reporter() : _logfileName(DFLT_LOGFILE_PATH) {
    setupLogFile();
}

Tintin_reporter::Tintin_reporter(const std::string &default_log_filename) : _logfileName(default_log_filename) {
    setupLogFile();
}

Tintin_reporter::~Tintin_reporter() {
    if (_logfile && _logfile->is_open()) {
        *_logfile << _buildLogEntry(LOGLVL_INFO, "Matt_Daemon is shutting down.") << std::endl;
        _logfile->close();
        delete _logfile;
    }
}

Tintin_reporter::Tintin_reporter(const Tintin_reporter &rhs) : _logfileName(rhs._logfileName) {
    if (rhs._logfile && rhs._logfile->is_open()) {
        _logfile = new std::ofstream(_logfileName, std::ios::app);
    } else {
        _logfile = nullptr;
    }
}

Tintin_reporter &Tintin_reporter::operator=(const Tintin_reporter &rhs) {
    if (this != &rhs) {
        _logfileName = rhs._logfileName;
        if (_logfile) {
            if (_logfile->is_open()) {
                _logfile->close();
            }
            delete _logfile;
        }
        if (rhs._logfile && rhs._logfile->is_open()) {
            _logfile = new std::ofstream(_logfileName, std::ios::app);
        } else {
            _logfile = nullptr;
        }
    }
    return *this;
}

void Tintin_reporter::log(int loglevel, const std::string &str) const {
    if (_logfile && _logfile->is_open()) {
        *_logfile << _buildLogEntry(loglevel, str) << std::endl;
    }
}

std::string Tintin_reporter::_buildLogEntry(int loglvl, const std::string &str) const {
    std::ostringstream entry;
    char timestring[26];
    std::time_t currentTime = std::time(nullptr);

    std::strftime(timestring, sizeof(timestring), "[%d/%m/%Y-%H:%M:%S]", std::localtime(&currentTime));

    // log level enum maybe not best solution
    std::string logLevelStr;
    switch (static_cast<LogLevel>(loglvl)) {
        case LOGLVL_LOG:
            logLevelStr = "[ LOG ]";
            break;
        case LOGLVL_INFO:
            logLevelStr = "[ INFO ]";
            break;
        case LOGLVL_WARN:
            logLevelStr = "[ WARN ]";
            break;
        case LOGLVL_ERROR:
            logLevelStr = "[ ERROR ]";
            break;
        default:
            logLevelStr = "[ UNKNOWN ]";
            break;
    }

    entry << timestring << " " << logLevelStr << " - " << str;
    return entry.str();
}

std::ostream &operator<<(std::ostream &out, const Tintin_reporter &tintin) {
    out << "Tintin_reporter: Log file: " << tintin._logfileName;
    return out;
}


void Tintin_reporter::setupLogFile() {
    std::size_t found = _logfileName.find_last_of("/");
    if (found != std::string::npos) {
        std::string logDir = _logfileName.substr(0, found);
        int status = mkdir(logDir.c_str(), 0777);
        if (status != 0 && errno != EEXIST) {
            std::cerr << "Error creating log directory: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (access(_logfileName.c_str(), F_OK) == 0) {
        if (unlink(_logfileName.c_str()) != 0) {
            std::cerr << "Error deleting log file: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    _logfile = new std::ofstream(_logfileName, std::ios_base::app);
    if (!_logfile || _logfile->fail()) {
        std::cerr << "Error opening log file: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (freopen(_logfileName.c_str(), "a", stderr) == nullptr) {
        perror("Failed to redirect stderr");
        _exit(EXIT_FAILURE);
    }

    if (dup2(fileno(stderr), STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout");
        _exit(EXIT_FAILURE);
    }

    log(LOGLVL_INFO, "Matt_daemon: Started.");
    log(LOGLVL_INFO, "Matt_daemon: Creating server.");
}
