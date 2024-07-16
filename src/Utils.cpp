#include "Utils.hpp"

void Utils::signalHandler(int signal) {
    Tintin_reporter::getInstance().log(LOGLEVEL_WARN, "Interrupt signal (" + std::to_string(signal) + ") received.");
    MattDaemon::getInstance().shutdownRequested = true;
    std::exit(signal);
}

bool Utils::checkRootUser() {
    return (geteuid() == 0);
}