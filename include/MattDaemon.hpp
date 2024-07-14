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
#include <poll.h>
#include <vector>
#include <algorithm>
#include "TintinReporter.hpp"

#define LOCKFILE_PATH "/var/lock/matt_daemon.lock"


class MattDaemon {
    friend class Utils;
    public:
        MattDaemon();
        MattDaemon(const MattDaemon& other);
        MattDaemon& operator=(const MattDaemon& other);
        MattDaemon(MattDaemon&& other) noexcept;
        MattDaemon& operator=(MattDaemon&& other) noexcept;
        ~MattDaemon();

        void run();
        void daemonize();
        static MattDaemon& getInstance();
        
    private:
        int serverSocket;
        int port;
        std::string lockFile;
        unsigned int maxClients;
        static MattDaemon* instance;
        fd_set readFds;
        std::vector<int> clientSockets;
        unsigned int connectionCount;

        void startChildAndLetParentExit();
        void runChildProcess();
        void createNewSessionAndMoveToRoot();
        void createLockFile();
        void setupServer();
        void readClientRequest(int clientSocket);
        void disconnectClient(int clientSocket);
        void deleteLockFileAndCloseSocket();
        bool checkIfLockFileExists();
};

#endif // MATT_DAEMON_HPP
