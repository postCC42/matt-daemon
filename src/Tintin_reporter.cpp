#include "Tintin_reporter.hpp"

std::unique_ptr<Tintin_reporter> Tintin_reporter::instance = nullptr;

Tintin_reporter::Tintin_reporter() :logfileName(LOGFILE_PATH) {
    initializeLogFile();
}

Tintin_reporter::~Tintin_reporter() {
    if (logfile && logfile.is_open()) {
       logfile.close();
    }
}

Tintin_reporter::Tintin_reporter(Tintin_reporter &&other) noexcept : logfile(std::move(other.logfile)) {
    other.logfileName.clear();
}

Tintin_reporter & Tintin_reporter::operator=(Tintin_reporter &&other) noexcept {
    if (this != &other) {
        logfile = std::move(other.logfile);
        logfileName = std::move(other.logfileName);
        other.logfileName.clear();
    }
    return *this;
}

Tintin_reporter &Tintin_reporter::getInstance() {
    if (instance == nullptr) {
        instance.reset(new Tintin_reporter());
    }
    return *instance;
}

void Tintin_reporter::log(int loglevel, const std::string &str) {
    if (logfile && logfile.is_open()) {
       logfile << addTimestampAndLogLevel(loglevel, str) << std::endl;
    }
}

std::string Tintin_reporter::addTimestampAndLogLevel(int logLevel, const std::string &str) const {
    std::ostringstream entry;
    char timestring[26];
    std::time_t currentTime = std::time(nullptr);

    std::strftime(timestring, sizeof(timestring), "[%d/%m/%Y-%H:%M:%S]", std::localtime(&currentTime));

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

void Tintin_reporter::initializeLogFile() {
    createLogDirectory();
    openLogFile();
    log(LOGLEVEL_INFO, "Matt_daemon: Started.");
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

void Tintin_reporter::openLogFile() {
    logfile = std::ofstream(logfileName, std::ios_base::app);
    if (!logfile || logfile.fail()) {
        std::cerr << "Error opening log file: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}
