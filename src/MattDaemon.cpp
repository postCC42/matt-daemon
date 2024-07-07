#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
#include "Utils.hpp"

extern Tintin_reporter *global_logger;

MattDaemon::MattDaemon() 
    : serverSocket(-1),
      port(4242), 
      logFile("/var/log/matt_daemon/matt_daemon.log"),
      lockFile("/var/lock/matt_daemon.lock"), 
      maxClients(3), 
      child_pid(-1) {}

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
    // todo check if root
    setupServer();
    daemonize();
    signal(SIGINT, Utils::signalHandler);
    signal(SIGQUIT, Utils::signalHandler);

    global_logger->log(LOGLVL_INFO, "Matt_daemon: Entering Daemon mode.");

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            global_logger->log(LOGLVL_ERROR, "Accept failed: " + std::string(strerror(errno)));
            continue;
        }
        global_logger->log(LOGLVL_INFO, "Matt_daemon: Client connected.");

        std::thread(&MattDaemon::handleClientConnection, this, clientSocket).detach();
    }
}

void MattDaemon::daemonize() {
    startChildAndLetParentExit();
    createNewSessionAndMoveToRoot();
    createLockFile();

    global_logger->log(LOGLVL_INFO, "Matt_daemon: Started.");
    global_logger->log(LOGLVL_INFO, "Matt_daemon: Creating server.");
}

void MattDaemon::startChildAndLetParentExit() {
    child_pid = fork();
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


    while (true) {
        std::cout << "Server socket: " << serverSocket << ", Port: " << port << std::endl;
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            perror("runChild Accept failed");
            global_logger->log(LOGLVL_ERROR, "run Child global Accept failed.");
            continue;
        }

        global_logger->log(LOGLVL_INFO, "Matt_daemon: Client connected.");

        handleClientConnection(clientSocket);

        close(clientSocket);
    }
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

void MattDaemon::setupServer() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        global_logger->log(LOGLVL_ERROR, "Socket creation failed.");
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server socket created: " << serverSocket << std::endl; // Debug statement

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, maxClients) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    global_logger->log(LOGLVL_INFO, "Matt_daemon: Server created.");
    std::cout << "Server socket bound and listening." << std::endl; // Debug statement
}


void MattDaemon::createLockFile() {
    std::ofstream lockFileStream(lockFile);
    if (!lockFileStream) {
        std::cerr << "Failed to create lock file: " << lockFile << std::endl;
        exit(EXIT_FAILURE);
    }
    lockFileStream.close();
}

void MattDaemon::handleClientConnection(int clientSocket) {
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    while (true) {
        int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead < 0) {
            perror("Read failed");
            close(clientSocket);
            return;
        } else if (bytesRead == 0) {
            global_logger->log(LOGLVL_INFO, "Matt_daemon: Client disconnected.");
            close(clientSocket);
            return;
        }

        buffer[bytesRead] = '\0';
        std::string input(buffer);

        global_logger->log(LOGLVL_LOG, "Matt_daemon: User input: " + input);

        if (input == "quit\n") {
            global_logger->log(LOGLVL_INFO, "Matt_daemon: Received quit command.");
            global_logger->log(LOGLVL_INFO, "Matt_daemon: Quitting.");
            close(clientSocket);
            deleteLockFileAndCloseSocket();
            exit(EXIT_SUCCESS);
        }
    }
}

void MattDaemon::deleteLockFileAndCloseSocket() {
    if (serverSocket != -1) {
        close(serverSocket);
    }
    if (remove(lockFile.c_str()) != 0) {
        std::cerr << "Failed to delete lock file: " << lockFile << std::endl;
        global_logger->log(LOGLVL_ERROR, "Failed to delete lock file.");
        exit(EXIT_FAILURE);
    }
    global_logger->log(LOGLVL_INFO, "Matt_daemon: Cleanup done.");
}
