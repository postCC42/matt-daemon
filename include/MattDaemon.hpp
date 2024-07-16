#ifndef MATTDAEMON_HPP
#define MATTDAEMON_HPP

#include <iostream>
#include <fstream>
#include <signal.h>
#include <unistd.h> // close
#include <fcntl.h> // open
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h> // flock
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
#include <algorithm>
#include "Tintin_reporter.hpp"
#include <atomic>
#include <utility>

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
        int lockFileDescriptor;
        unsigned int maxClients;
        static MattDaemon* instance;
        fd_set readFds;
        std::vector<int> clientSockets;
        unsigned int connectionCount;
        std::atomic<bool> shutdownRequested{false};

        void startChildAndLetParentExit();
        void runChildProcess();
        void createNewSessionAndMoveToRoot();
        void setupServer();
        void handleNewConnection();
        void readClientRequest(int clientSocket);
        void disconnectAllClients();
        void disconnectClient(int clientSocket);
        static void sendDisconnectMessage(int clientSocket);
        bool checkIfLockFileExists();
        void createLockFile();
        void deleteLockFile();
        void deleteLockFileAndCloseSocket();
};

#endif // MATT_DAEMON_HPP
