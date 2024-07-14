#include "TintinReporter.hpp"

TintinReporter::TintinReporter() :logfileName(LOGFILE_PATH) {
    initializeLogFile();
}

TintinReporter::~TintinReporter() {
    if (logfile &&logfile->is_open()) {
       *logfile <<addTimestampAndLogLevel(LOGLEVEL_INFO, "Matt_Daemon is shutting down.") << std::endl;
       logfile->close();
        delete logfile;
    }
}

TintinReporter::TintinReporter(const TintinReporter &rhs) :logfileName(rhs.logfileName) {
    if (rhs.logfile && rhs.logfile->is_open()) {
       logfile = new std::ofstream(logfileName, std::ios::app);
    } else {
       logfile = nullptr;
    }
}

TintinReporter &TintinReporter::operator=(const TintinReporter &rhs) {
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

void TintinReporter::log(int loglevel, const std::string &str) const {
    if (logfile &&logfile->is_open()) {
       *logfile << addTimestampAndLogLevel(loglevel, str) << std::endl;
    }
}

std::string TintinReporter::addTimestampAndLogLevel(int logLevel, const std::string &str) const {
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

std::ostream &operator<<(std::ostream &out, const TintinReporter &tintin) {
    out << "Tintin_reporter: Log file: " << tintin.logfileName;
    return out;
}

void TintinReporter::initializeLogFile() {
    createLogDirectory();
    removeExistingLogFile();
    openLogFile();
    redirectStderrToLogFile();
    redirectStdoutToStderr();
    log(LOGLEVEL_INFO, "Matt_daemon: Started.");
    log(LOGLEVEL_INFO, "Matt_daemon: Creating server.");
}

void TintinReporter::createLogDirectory() {
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

void TintinReporter::removeExistingLogFile() {
    if (access(logfileName.c_str(), F_OK) == 0) {
        if (unlink(logfileName.c_str()) != 0) {
            std::cerr << "Error deleting log file: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void TintinReporter::openLogFile() {
    logfile = new std::ofstream(logfileName, std::ios_base::app);
    if (!logfile || logfile->fail()) {
        std::cerr << "Error opening log file: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void TintinReporter::redirectStderrToLogFile() {
    if (freopen(logfileName.c_str(), "a", stderr) == nullptr) {
        perror("Failed to redirect stderr");
        exit(EXIT_FAILURE);
    }
}

void TintinReporter::redirectStdoutToStderr() {
    if (dup2(fileno(stderr), STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout");
        exit(EXIT_FAILURE);
    }
}
