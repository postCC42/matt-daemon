#include "Utils.hpp"
#include <iostream>
#include <unistd.h>

void Utils::signalHandler(int sig_num) {
    std::cout << "Interrupt signal (" << sig_num << ") received.\n";
    std::remove("/var/lock/matt_daemon.lock");
    exit(sig_num);
}
