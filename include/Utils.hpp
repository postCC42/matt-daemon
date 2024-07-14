#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <csignal>
#include "TintinReporter.hpp"
#include "MattDaemon.hpp"
extern TintinReporter* global_logger;

class Utils {
public:
    static void signalHandler(int signal);
    static bool checkRootUser();
};

#endif // UTILS_HPP
