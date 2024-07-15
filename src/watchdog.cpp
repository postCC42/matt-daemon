#include <iostream>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string>
#include <sstream>
#include <ctime>
#include <cstring>
#include <syslog.h>
#include <cstdlib>

const std::string LOCKFILE_PATH = "/var/lock/matt_daemon.lock";
const std::string LOGFILE_PATH = "/var/log/matt_daemon/watchdog.log";

void logToFile(const std::string& message) {
    std::ofstream logFile(LOGFILE_PATH, std::ios_base::app);
    if (logFile.is_open()) {
        time_t now = time(0);
        char* dt = ctime(&now);
        dt[strlen(dt) - 1] = '\0'; 
        logFile << "[" << dt << "] " << message << std::endl;
    } else {
        std::cerr << "Failed to open log file: " << LOGFILE_PATH << std::endl;
    }
}

bool isProcessRunning(const std::string& processName) {
    std::string command = "ps aux | grep " + processName + " | grep -v grep";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    char buffer[128];
    bool found = false;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        found = true; 
    }
    pclose(pipe);
    
    return found; 
}

void removeLockFile() {
    if (remove(LOCKFILE_PATH.c_str()) == 0) {
        logToFile("Lock file successfully removed by watchdog.");
    } else {
        logToFile("Failed to remove lock file by watchdog.");
    }
}

void signalHandler(int signum) {
    logToFile("Watchdog received signal: " + std::to_string(signum));
    exit(signum);
}

int main() {
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    sleep(10); 

    while (true) {
        logToFile("Checking for lock file.");

        std::ifstream lockFile(LOCKFILE_PATH);
        if (!lockFile.is_open()) {
            logToFile("Lock file not found.");
            sleep(1);
            continue;
        }

        std::string processName;
        std::getline(lockFile, processName);
        lockFile.close();

        if (processName.empty()) {
            logToFile("Empty process name read from lock file.");
            removeLockFile();
            break;
        }

        logToFile("Process name read from lock file: " + processName);

        bool running = isProcessRunning(processName);
        logToFile("isProcessRunning returned: " + std::to_string(running));

        if (!running) {
            logToFile("Process not running. Removing lock file.");
            removeLockFile();
            break;
        }

        sleep(1);
    }

    logToFile("Exiting.");
    return 0;
}
