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
        int maxClients;
        pid_t child_pid;
        static MattDaemon* instance;

        void startChildAndLetParentExit();
        void runChildProcess();
        void createNewSessionAndMoveToRoot();
        void createLockFile();
        void setupServer();
        void handleClientConnection(int clientSocket);
        void deleteLockFileAndCloseSocket();
        bool checkIfLockFileExists();
};

#endif // MATT_DAEMON_HPP
