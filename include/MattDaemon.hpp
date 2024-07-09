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
#include "Tintin_reporter.hpp"


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
        
    private:
        int serverSocket;
        int port;
        std::string lockFile;
        int maxClients;
        pid_t child_pid;

        void startChildAndLetParentExit();
        void runChildProcess();
        void createNewSessionAndMoveToRoot();
        void createLockFile();
        void setupServer();
        void handleClientConnection(int clientSocket);
        void deleteLockFileAndCloseSocket();
};

#endif // MATT_DAEMON_HPP
