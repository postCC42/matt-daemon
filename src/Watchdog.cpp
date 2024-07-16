#include <iostream>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <ctime>
#include <cstring>
#include <chrono>
#include <thread>

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
    const int maxAttempts = 3;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        if (remove(LOCKFILE_PATH.c_str()) == 0) {
            logToFile("Lock file successfully removed by watchdog.");
            return; 
        } else {
            logToFile("Attempt " + std::to_string(attempt) + " failed to remove lock file.");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    logToFile("Failed to remove lock file after " + std::to_string(maxAttempts) + " attempts. It was probably no more there, removed safely by Matt Daemon");
}

void signalHandler(int signum) {
    logToFile("Watchdog received signal: " + std::to_string(signum));
    exit(signum);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS); // Parent process exits
    }

    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(027);
    
    if (chdir("/") < 0) { // Check if chdir succeeds
        exit(EXIT_FAILURE);
    }

    for (int fd = sysconf(_SC_OPEN_MAX) - 1; fd >= 0; fd--) {
        close(fd);
    }

    if (open("/dev/null", O_RDWR) < 0) { // Check if open succeeds
        exit(EXIT_FAILURE);
    }
    if (dup(0) < 0) { // Check if dup succeeds
        exit(EXIT_FAILURE);
    }
    if (dup(0) < 0) { // Check if dup succeeds
        exit(EXIT_FAILURE);
    }
}


int main() {
    if (geteuid() != 0) {
        std::cerr << "Only root can run this program." << std::endl;
        return EXIT_FAILURE;
    }
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    daemonize();

    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(20);

    logToFile("Waiting for lock file.");

    while (true) {
        std::ifstream lockFile(LOCKFILE_PATH);
        if (lockFile.is_open()) {
            logToFile("Lock file found.");
            lockFile.close();
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto now = std::chrono::steady_clock::now();
        if (now - start > timeout) {
            logToFile("Timeout reached while waiting for lock file. Exiting.");
            exit(0);
        }
    }

    std::ifstream lockFile(LOCKFILE_PATH);
    std::string processName;
    std::getline(lockFile, processName);
    lockFile.close();

    if (processName.empty()) {
        logToFile("Empty process name read from lock file. Removing lock file.");
        removeLockFile();
        exit(0);
    }

    logToFile("Process name read from lock file: " + processName);

    while (true) {
        bool running = isProcessRunning(processName);

        if (!running) {
            logToFile("Process not running. Removing lock file.");
            removeLockFile();
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    logToFile("Exiting.");
    exit(0);
}
