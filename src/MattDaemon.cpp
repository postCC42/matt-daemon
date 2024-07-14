#include "MattDaemon.hpp"
#include "TintinReporter.hpp"
#include "Utils.hpp"

MattDaemon* MattDaemon::instance = nullptr;

MattDaemon::MattDaemon()
    :   serverSocket(-1),
        port(4242), 
        lockFile(LOCKFILE_PATH), 
        maxClients(3), 
        child_pid(-1) {
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
}

void MattDaemon::run() {
    createLockFile();
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

        std::thread(&MattDaemon::handleClientConnection, this, clientSocket).detach();
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
        // TODO: the maximum number of connection should be 3
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            perror("runChild Accept failed");
            TintinReporter::getInstance().log(LOGLEVEL_ERROR, "run Child global Accept failed.");
            continue;
        }

        TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client connected.");

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
    if (!checkIfLockFileExists()) {
        exit(EXIT_FAILURE);
    }
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        TintinReporter::getInstance().log(LOGLEVEL_ERROR, "Socket creation failed.");
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
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
    TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Server socket n. " + std::to_string(serverSocket) + " bound and listening on port n. " + std::to_string(port));
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
            TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Client disconnected.");
            close(clientSocket);
            return;
        }

        buffer[bytesRead] = '\0';
        std::string input(buffer);

        // TODO: there is a double new line for each log
        TintinReporter::getInstance().log(LOGLEVEL_LOG, "Matt_daemon: User input: " + input);

        if (input == "quit\n") {
            TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Received quit command.");
            TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_daemon: Quitting.");
            TintinReporter::getInstance().log(LOGLEVEL_INFO, "Matt_Daemon is shutting down.");
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
