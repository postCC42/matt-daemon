#include "Utils.hpp"

void Utils::signalHandler(int signal) {
    TintinReporter::getInstance().log(LOGLEVEL_WARN, "Interrupt signal (" + std::to_string(signal) + ") received.");
    MattDaemon::getInstance().shutdownRequested = true;
    // MattDaemon& daemon = MattDaemon::getInstance();
    // daemon.deleteLockFileAndCloseSocket();
    // TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_Daemon is shutting down.");
    std::exit(signal);
}

bool Utils::checkRootUser() {
    return (geteuid() == 0);
}