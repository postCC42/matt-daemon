#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
#include "Utils.hpp"

MattDaemon* MattDaemon::instance = nullptr;
std::atomic<bool> shutdownRequested{false};


MattDaemon::MattDaemon()
    :   serverSocket(-1),
        port(4242), 
        lockFile("/var/lock/matt_daemon.lock"), 
        maxClients(3) {
    instance = this;
}

MattDaemon::MattDaemon(const MattDaemon& other)
    : serverSocket(other.serverSocket) {}

MattDaemon& MattDaemon::operator=(const MattDaemon& other) {
    if (this != &other) {
        serverSocket = other.serverSocket;
    }
    return *this;
}

MattDaemon::MattDaemon(MattDaemon&& other) noexcept
    : serverSocket(std::exchange(other.serverSocket, -1)) {}

MattDaemon& MattDaemon::operator=(MattDaemon&& other) noexcept {
    if (this != &other) {
        serverSocket = std::exchange(other.serverSocket, -1);
    }
    return *this;
}

MattDaemon::~MattDaemon() {
    deleteLockFileAndCloseSocket();
}

void MattDaemon::run() {
    createLockFile();
    // TODO: should we create the server inside the deamon?
    setupServer();
    daemonize();
    Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Entering Daemon mode.");

    while (!shutdownRequested) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            Tintin_reporter::getInstance().log(LOGLEVEL_ERROR,  "Accept failed: " + std::string(strerror(errno)));
            continue;
        }
        Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client connected.");
    }
    deleteLockFileAndCloseSocket();
    Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Shutdown complete.");
}

void MattDaemon::daemonize() {
    if (!checkIfLockFileExists()) {
        exit(EXIT_FAILURE);
    }
    startChildAndLetParentExit();
    createNewSessionAndMoveToRoot();
    Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Creating server.");
}

void MattDaemon::setupServer() {
    if (!checkIfLockFileExists()) {
        exit(EXIT_FAILURE);
    }
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "Socket creation failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // TODO: should we set socket options?
    int opt = 1;
    if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "setsockopt failed: " + std::string(strerror(errno)));
    }

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "bind failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, maxClients) < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "listen failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
    connectionCount = 1;

    Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Server socket n. " + std::to_string(serverSocket) + " bound and listening on port n. " + std::to_string(port));
}

void MattDaemon::createNewSessionAndMoveToRoot() {
    if (setsid() < 0) {
        std::cerr << "Failed to create new session" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (chdir("/") < 0) {
        std::cerr << "Failed to change directory to /" << std::endl;
        exit(EXIT_FAILURE);
    }
}



void MattDaemon::createLockFile() {
    lockFileDescriptor = open(lockFile.c_str(), O_CREAT | O_RDWR, 0666);
    if (lockFileDescriptor < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Failed to create lock file.");
        exit(EXIT_FAILURE);
    }

    if (flock(lockFileDescriptor, LOCK_EX | LOCK_NB) < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Failed to lock the lock file.");
        close(lockFileDescriptor);
        exit(EXIT_FAILURE);
    }

    std::string processName = "Matt_daemon\n";
    ssize_t bytesWritten = write(lockFileDescriptor, processName.c_str(), processName.size());
    if (bytesWritten != static_cast<ssize_t>(processName.size())) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Failed to write Matt_daemon to lock file.");
        close(lockFileDescriptor);
        exit(EXIT_FAILURE);
    }

    Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Lock file created and locked successfully: " + lockFile);
}


void MattDaemon::startChildAndLetParentExit() {
    pid_t child_pid = fork();
    if (child_pid < 0) {
        std::cerr << "Failed to fork process" << std::endl;
        throw std::runtime_error("Fork failure");
    } else if (child_pid == 0) {
        runChildProcess();
    } else {
        exit(EXIT_SUCCESS);
    }
}

void MattDaemon::runChildProcess() {
    umask(0); // Allow daemon to create files with maximum permissions

    // Set timeout for select
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (true) {
        FD_ZERO(&readFds);

        // Add server socket
        FD_SET(serverSocket, &readFds);
        int maxFd = serverSocket;

        // Add client sockets
        connectionCount = clientSockets.size();
        for (auto clientSocket : clientSockets) {
            if (clientSocket < 0) continue;
            FD_SET(clientSocket, &readFds);
            if (clientSocket > maxFd) {
                maxFd = clientSocket;
            }
        }

        int activity = select(maxFd + 1, &readFds, nullptr, nullptr, &timeout);
        if (activity < 0 && errno != EINTR) {
            Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "select failed: " + std::string(strerror(errno)));
            return;
        }

        if (FD_ISSET(serverSocket, &readFds)) {
            handleNewConnection();
        }

        for (auto clientSocket : clientSockets) {
            if (FD_ISSET(clientSocket, &readFds)) {
                readClientRequest(clientSocket);
            }
        }
    }
}

void MattDaemon::handleNewConnection() {
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "accept failed: " + std::string(strerror(errno)));
        return;
    }
    if (clientSockets.size() >= maxClients) {
        Tintin_reporter::getInstance().log(LOGLEVEL_WARN, "Matt_daemon: Max clients reached. Ignoring new connection.");
        sendDisconnectMessage(clientSocket);
        close(clientSocket);
        return;
    }

    Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client connected on socket n. " + std::to_string(clientSocket));
    clientSockets.push_back(clientSocket);
}

void MattDaemon::readClientRequest(const int clientSocket) {
    char buffer[256] = {};

    int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "read failed: " + std::string(strerror(errno)));
        disconnectClient(clientSocket);
        return;
    } else if (bytesRead == 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client disconnected.");
        disconnectClient(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    if (buffer[bytesRead - 1] == '\n') {
        buffer[bytesRead - 1] = '\0';
    }
    std::string input(buffer);

    Tintin_reporter::getInstance().log(LOGLEVEL_LOG, "Matt_daemon: User input [" + std::to_string(clientSocket) + "]: " + input);

    if (input == "quit") {
        Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Received quit command.");
        Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Quitting.");
        Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Matt_Daemon is shutting down.");
        disconnectAllClients();
        deleteLockFile();
        exit(EXIT_SUCCESS);
    }
}

void MattDaemon::disconnectAllClients() {
    for (auto clientSocket : clientSockets) {
        sendDisconnectMessage(clientSocket);
        close(clientSocket);
    }
    clientSockets.clear();
}

void MattDaemon::disconnectClient(int clientSocket) {
    sendDisconnectMessage(clientSocket);
    close(clientSocket);

    auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);

    if (it != clientSockets.end()) {
        clientSockets.erase(it);
    }
}

void MattDaemon::sendDisconnectMessage(int clientSocket) {
    const char* disconnectMessage = "Connection closed\n";
    ssize_t bytesSent = send(clientSocket, disconnectMessage, strlen(disconnectMessage), 0);
    if (bytesSent < 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "send failed: " + std::string(strerror(errno)));
    }
}

// todo close socket at exit check
void MattDaemon::deleteLockFile() {
    if (lockFileDescriptor >= 0) {
        flock(lockFileDescriptor, LOCK_UN);
        close(lockFileDescriptor);
        lockFileDescriptor = -1;
    }

    if (remove(lockFile.c_str()) == 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_INFO, "Lock file successfully removed.");
    }
}


void MattDaemon::deleteLockFileAndCloseSocket() {
    if (serverSocket != -1) {
        close(serverSocket);
    }
    deleteLockFile();
}


MattDaemon& MattDaemon::getInstance() {
    static MattDaemon instance; // This is a static local variable, ensures it's a singleton
    return instance;
}

bool MattDaemon::checkIfLockFileExists() {
    if (access(lockFile.c_str(), F_OK) != 0) {
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "Lock file not found. Can't proceed further");
        return false;
    }
    return true;
}
