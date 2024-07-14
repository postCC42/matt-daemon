#include "MattDaemon.hpp"
#include "TintinReporter.hpp"
#include "Utils.hpp"

MattDaemon* MattDaemon::instance = nullptr;

MattDaemon::MattDaemon()
    :   serverSocket(-1),
        port(4242), 
        lockFile(LOCKFILE_PATH), 
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
    close(serverSocket);
}

void MattDaemon::run() {
    createLockFile();
    // TODO: should we create the server inside the deamon?
    setupServer();
    daemonize();
    TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Entering Daemon mode.");

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            TintinReporter::getInstance().log(LOGLEVEL_ERROR,  "Accept failed: " + std::string(strerror(errno)));
            continue;
        }
        TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client connected.");
    }
}

void MattDaemon::daemonize() {
    if (!checkIfLockFileExists()) {
        exit(EXIT_FAILURE);
    }
    startChildAndLetParentExit();
    createNewSessionAndMoveToRoot();
    TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Started.");
}

void MattDaemon::setupServer() {
    if (!checkIfLockFileExists()) {
        exit(EXIT_FAILURE);
    }
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        TintinReporter::getInstance().log(LOGLEVEL_ERROR, "Socket creation failed.");
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // TODO: should we set socket options?
    int opt = 1;
    if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
    }

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, maxClients) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    connectionCount = 1;

    TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Server socket n. " + std::to_string(serverSocket) + " bound and listening on port n. " + std::to_string(port));
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
    std::ofstream lockFileStream(lockFile);
    if (!lockFileStream) {
        TintinReporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Failed to create lock file.");
        exit(EXIT_FAILURE);
    } else {
        TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Lock file created successfully: " + lockFile);
    }
    lockFileStream.close();
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
            perror("select error");
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
        perror("accept failed");
        TintinReporter::getInstance().log(LOGLEVEL_ERROR, "run Child global Accept failed.");
        return;
    }
    if (clientSockets.size() >= maxClients) {
        TintinReporter::getInstance().log(LOGLEVEL_WARN, "Matt_daemon: Max clients reached. Ignoring new connection.");
        sendDisconnectMessage(clientSocket);
        close(clientSocket);
        return;
    }

    TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client connected on socket n. " + std::to_string(clientSocket));
    clientSockets.push_back(clientSocket);
}

void MattDaemon::readClientRequest(const int clientSocket) {
    char buffer[256] = {};

    // TODO: should use recv?
    int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        perror("Read failed");
        disconnectClient(clientSocket);
        return;
    } else if (bytesRead == 0) {
        TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client disconnected.");
        disconnectClient(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    if (buffer[bytesRead - 1] == '\n') {
        buffer[bytesRead - 1] = '\0';
    }
    std::string input(buffer);

    TintinReporter::getInstance().log(LOGLEVEL_LOG, "Matt_daemon: User input [" + std::to_string(clientSocket) + "]: " + input);

    if (input == "quit") {
        TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Received quit command.");
        TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Quitting.");
        TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_Daemon is shutting down.");
        disconnectAllClients();
        deleteLockFileAndCloseSocket();
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
        perror("send failed");
    }
}

void MattDaemon::deleteLockFileAndCloseSocket() {
    if (serverSocket != -1) {
        close(serverSocket);
    }
    if (access(lockFile.c_str(), F_OK) == 0) {
        if (remove(lockFile.c_str()) != 0) {
            std::cerr << "Failed to delete lock file: " << lockFile << std::endl;
            TintinReporter::getInstance().log(LOGLEVEL_ERROR, "Failed to delete lock file.");
        } else {
            TintinReporter::getInstance().log(LOGLEVEL_INFO, "Lock file successfully removed.");
        }
    } else {
        TintinReporter::getInstance().log(LOGLEVEL_WARN, "Lock file does not exist.");
    }
    TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Cleanup done.");
    delete instance;
}

MattDaemon& MattDaemon::getInstance() {
    static MattDaemon instance; // This is a static local variable, ensures it's a singleton
    return instance;
}

bool MattDaemon::checkIfLockFileExists() {
    if (access(lockFile.c_str(), F_OK) != 0) {
        TintinReporter::getInstance().log(LOGLEVEL_ERROR, "Lock file not found. Can't proceed further");
        return false;
    }
    return true;
}
