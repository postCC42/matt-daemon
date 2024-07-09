#include "Utils.hpp"

void Utils::signalHandler(int signal) {
    global_logger->log(LOGLEVEL_WARN, "Interrupt signal (" + std::to_string(signal) + ") received.");
    MattDaemon& daemon = MattDaemon::getInstance();
    daemon.deleteLockFileAndCloseSocket();
    if (global_logger) {
        delete global_logger;
        global_logger = nullptr;
    }
    std::exit(signal);
}

bool Utils::checkRootUser() {
    return (geteuid() == 0);
}