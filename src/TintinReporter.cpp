#include "TintinReporter.hpp"

std::unique_ptr<TintinReporter> TintinReporter::instance = nullptr;

TintinReporter::TintinReporter() :logfileName(LOGFILE_PATH) {
    initializeLogFile();
}

TintinReporter::~TintinReporter() {
    if (logfile && logfile.is_open()) {
       logfile.close();
    }
}

TintinReporter::TintinReporter(TintinReporter &&other) noexcept : logfile(std::move(other.logfile)) {
    other.logfileName.clear();
}

TintinReporter & TintinReporter::operator=(TintinReporter &&other) noexcept {
    if (this != &other) {
        logfile = std::move(other.logfile);
        logfileName = std::move(other.logfileName);
        other.logfileName.clear();
    }
    return *this;
}

TintinReporter &TintinReporter::getInstance() {
    if (instance == nullptr) {
        instance.reset(new TintinReporter());
    }
    return *instance;
}

void TintinReporter::log(int loglevel, const std::string &str) {
    if (logfile && logfile.is_open()) {
       logfile << addTimestampAndLogLevel(loglevel, str) << std::endl;
    }
}

std::string TintinReporter::addTimestampAndLogLevel(int logLevel, const std::string &str) const {
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

void TintinReporter::initializeLogFile() {
    createLogDirectory();
    openLogFile();
    redirectStderrToLogFile();
    redirectStdoutToStderr();
    log(LOGLEVEL_INFO, "Matt_daemon: Started.");
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

void TintinReporter::openLogFile() {
    logfile = std::ofstream(logfileName, std::ios_base::app);
    if (!logfile || logfile.fail()) {
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
