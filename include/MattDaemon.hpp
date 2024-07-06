#ifndef MATTDAEMON_HPP
#define MATTDAEMON_HPP

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

class MattDaemon {
public:
    MattDaemon();
    MattDaemon(const MattDaemon& other);
    MattDaemon& operator=(const MattDaemon& other);
    MattDaemon(MattDaemon&& other) noexcept;
    MattDaemon& operator=(MattDaemon&& other) noexcept;
    ~MattDaemon();

    void run();

    void daemonize();
    void setupServer();
    void deleteLockFileAndCloseSocket();
    void handleClientConnection(int clientSocket);
    int getServerSocket() const { return serverSocket; }





private:
    int serverSocket;
    int port;
    std::string logFile;
    std::string lockFile;
    int maxClients;
    pid_t child_pid;

    // static void signalHandler(int signum);
    void startChildAndLetParentExit();
    void runChildProcess();
    void createNewSessionAndMoveToRoot();
    void setupLogFile();
    void createLockFile();
};

#endif // MATTDAEMON_HPP
