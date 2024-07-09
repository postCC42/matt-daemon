#include "Tintin_reporter.hpp"



Tintin_reporter::Tintin_reporter() :logfileName(LOGFILE_PATH) {
    initializeLogFile();
}

Tintin_reporter::Tintin_reporter(const std::string &default_log_filename) :logfileName(default_log_filename) {
    initializeLogFile();
}

Tintin_reporter::~Tintin_reporter() {
    if (logfile &&logfile->is_open()) {
       *logfile <<addTimestampAndLogLevel(LOGLEVEL_INFO, "Matt_Daemon is shutting down.") << std::endl;
       logfile->close();
        delete logfile;
    }
}

Tintin_reporter::Tintin_reporter(const Tintin_reporter &rhs) :logfileName(rhs.logfileName) {
    if (rhs.logfile && rhs.logfile->is_open()) {
       logfile = new std::ofstream(logfileName, std::ios::app);
    } else {
       logfile = nullptr;
    }
}

Tintin_reporter &Tintin_reporter::operator=(const Tintin_reporter &rhs) {
    if (this != &rhs) {
       logfileName = rhs.logfileName;
        if (logfile) {
            if (logfile->is_open()) {
               logfile->close();
            }
            delete logfile;
        }
        if (rhs.logfile && rhs.logfile->is_open()) {
           logfile = new std::ofstream(logfileName, std::ios::app);
        } else {
           logfile = nullptr;
        }
    }
    return *this;
}

void Tintin_reporter::log(int loglevel, const std::string &str) const {
    if (logfile &&logfile->is_open()) {
       *logfile << addTimestampAndLogLevel(loglevel, str) << std::endl;
    }
}

std::string Tintin_reporter::addTimestampAndLogLevel(int logLevel, const std::string &str) const {
    std::ostringstream entry;
    char timestring[26];
    std::time_t currentTime = std::time(nullptr);

    std::strftime(timestring, sizeof(timestring), "[%d/%m/%Y-%H:%M:%S]", std::localtime(&currentTime));

    // log level enum maybe not best solution
    std::string logLevelStr;
    switch (static_cast<LogLevel>(logLevel)) {
        case LOGLEVEL_LOG:
            logLevelStr = "[ LOG ]";
            break;
        case LOGLEVEL_INFO:
            logLevelStr = "[ INFO ]";
            break;
        case LOGLEVEL_WARN:
            logLevelStr = "[ WARN ]";
            break;
        case LOGLEVEL_ERROR:
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
    out << "Tintin_reporter: Log file: " << tintin.logfileName;
    return out;
}

void Tintin_reporter::initializeLogFile() {
    createLogDirectory();
    removeExistingLogFile();
    openLogFile();
    redirectStderrToLogFile();
    redirectStdoutToStderr();
    log(LOGLEVEL_INFO, "Matt_daemon: Started.");
    log(LOGLEVEL_INFO, "Matt_daemon: Creating server.");
}

void Tintin_reporter::createLogDirectory() {
    std::size_t found = logfileName.find_last_of("/");
    if (found != std::string::npos) {
        std::string logDir = logfileName.substr(0, found);
        int status = mkdir(logDir.c_str(), 0777);
        if (status != 0 && errno != EEXIST) {
            std::cerr << "Error creating log directory: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void Tintin_reporter::removeExistingLogFile() {
    if (access(logfileName.c_str(), F_OK) == 0) {
        if (unlink(logfileName.c_str()) != 0) {
            std::cerr << "Error deleting log file: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void Tintin_reporter::openLogFile() {
    logfile = new std::ofstream(logfileName, std::ios_base::app);
    if (!logfile || logfile->fail()) {
        std::cerr << "Error opening log file: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Tintin_reporter::redirectStderrToLogFile() {
    if (freopen(logfileName.c_str(), "a", stderr) == nullptr) {
        perror("Failed to redirect stderr");
        exit(EXIT_FAILURE);
    }
}

void Tintin_reporter::redirectStdoutToStderr() {
    if (dup2(fileno(stderr), STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout");
        exit(EXIT_FAILURE);
    }
}
