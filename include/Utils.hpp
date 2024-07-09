#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <csignal>

class Utils {
public:
    static void signalHandler(int signal);
};

#endif // UTILS_HPP
