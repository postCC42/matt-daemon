#include "Utils.hpp"
#include "Tintin_reporter.hpp"
extern Tintin_reporter* global_logger;



void Utils::signalHandler(int signal) {
    global_logger->log(LOGLEVEL_WARN, "Interrupt signal (" + std::to_string(signal) + ") received.");
    if (signal == SIGTERM) {
        if (global_logger) {
            delete global_logger;
            global_logger = nullptr;
        }
        if (std::remove("/var/lock/matt_daemon.lock") == 0) {
            global_logger->log(LOGLEVEL_WARN, "Lock file successfully removed");
         } else {
        std::perror("Lock file removing failed");
    }
        std::exit(signal);
    }
}