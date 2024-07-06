#include "MattDaemon.hpp"
#include "Utils.hpp"


MattDaemon::MattDaemon() 
    : serverSocket(-1),
      port(4242), 
      logFile("/var/log/matt_daemon/matt_daemon.log"),
      lockFile("/var/lock/matt_daemon.lock"), 
      maxClients(3), 
      child_pid(-1) {}

// Copy constructor
MattDaemon::MattDaemon(const MattDaemon& other)
    : serverSocket(other.serverSocket) {

}

// Copy assignment operator
MattDaemon& MattDaemon::operator=(const MattDaemon& other) {
    if (this != &other) {
        serverSocket = other.serverSocket;
    
    }
    return *this;
}

// Move constructor
MattDaemon::MattDaemon(MattDaemon&& other) noexcept
    : serverSocket(std::exchange(other.serverSocket, -1)) {
}

// Move assignment operator
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
    // todo set tintin logs
    daemonize();
    // todo setupServer();
    signal(SIGINT, Utils::signalHandler);
    signal(SIGQUIT, Utils::signalHandler);

    std::cout << "[INFO] - Matt_daemon: Entering Daemon mode." << std::endl;

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            perror("Accept failed");
            // Log the error to the daemon log
            std::ofstream logFileStream(logFile, std::ios_base::app);
            logFileStream << "[ERROR] - Matt_daemon: Accept failed: " << strerror(errno) << std::endl;
            logFileStream.close();
            continue;
        }
        std::thread(&MattDaemon::handleClientConnection, this, clientSocket).detach();
    }
}

void MattDaemon::daemonize() {
    startChildAndLetParentExit();
    createNewSessionAndMoveToRoot();
    createLockFile();

    std::cout << "[INFO] - Matt_daemon: Started." << std::endl;
    std::cout << "[INFO] - Matt_daemon: Creating server." << std::endl;
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

    setupLogFile();

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            perror("Accept failed");
            std::ofstream logFileStream(logFile, std::ios_base::app);
            logFileStream << "[ERROR] - Matt_daemon: Accept failed." << std::endl;
            logFileStream.close();
            continue;
        }

        std::cout << "[INFO] - Matt_daemon: Client connected." << std::endl;

        // handleClientConnection(clientSocket);

        // close(clientSocket);
    }
}

void MattDaemon::createNewSessionAndMoveToRoot(){

    if (setsid() < 0) {
        std::cerr << "Failed to create new session" << std::endl;
        exit(EXIT_FAILURE);
    } // new control session (detached from terminal session that created it)

    if (chdir("/") < 0) {
        std::cerr << "Failed to change directory to /" << std::endl;
        exit(EXIT_FAILURE);
    } // ensure that daemon process is not tied to any specific directory that might be unmounted or deleted during the daemon's lifetime
}

void MattDaemon::setupLogFile() {
    std::size_t found = logFile.find_last_of("/");
    if (found != std::string::npos) {
        std::string logDir = logFile.substr(0, found);
        int status = mkdir(logDir.c_str(), 0777); // 0777 allows full access to the directory
        if (status != 0 && errno != EEXIST) {
            std::cerr << "Error creating log directory: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Remove existing log file 
    if (access(logFile.c_str(), F_OK) == 0) {
        if (unlink(logFile.c_str()) != 0) {
            std::cerr << "Error deleting log file: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Create or open log file
    std::ofstream logFileStream(logFile, std::ios_base::app);
    if (!logFileStream) {
        std::cerr << "Error opening log file: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Redirect stderr to log file
    if (freopen(logFile.c_str(), "a", stderr) == nullptr) {
        perror("Failed to redirect stderr");
        _exit(EXIT_FAILURE);
    }

    // Redirect stdout to same file descriptor as stderr
    if (dup2(fileno(stderr), STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout");
        _exit(EXIT_FAILURE);
    }

    // Log startup message
    logFileStream << "[INFO] - Matt_daemon: Started." << std::endl;
    logFileStream << "[INFO] - Matt_daemon: Creating server." << std::endl;

    logFileStream.close();
}

// void MattDaemon::setupServer() {
//     serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverSocket < 0) {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     sockaddr_in serverAddr;
//     memset(&serverAddr, 0, sizeof(serverAddr));
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_addr.s_addr = INADDR_ANY;
//     serverAddr.sin_port = htons(port);

//     if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
//         perror("Bind failed");
//         exit(EXIT_FAILURE);
//     }

//     if (listen(serverSocket, maxClients) < 0) {
//         perror("Listen failed");
//         exit(EXIT_FAILURE);
//     }

//     std::ofstream logFileStream(logFile, std::ios_base::app);
//     logFileStream << "[INFO] - Matt_daemon: Server created." << std::endl;
//     logFileStream.close();

//     std::cout << "[INFO] - Matt_daemon: Server created." << std::endl;
// }

void MattDaemon::createLockFile() {
    std::ofstream lockFileStream(lockFile);
    if (!lockFileStream) {
        std::cerr << "Failed to create lock file: " << lockFile << std::endl;
        exit(EXIT_FAILURE);
    }
    lockFileStream.close();
}

// void MattDaemon::handleClientConnection(int clientSocket) {
//     char buffer[256];
//     memset(buffer, 0, sizeof(buffer));

//     while (true) {
//         int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
//         if (bytesRead < 0) {
//             perror("Read failed");
//             close(clientSocket);
//             return;
//         } else if (bytesRead == 0) {
//             // Client disconnected gracefully
//             std::cout << "[INFO] - Matt_daemon: Client disconnected." << std::endl;
//             close(clientSocket);
//             return;
//         }

//         buffer[bytesRead] = '\0';
//         std::string input(buffer);

//         std::ofstream logFileStream(logFile, std::ios_base::app);
//         if (!logFileStream) {
//             std::cerr << "Error opening log file: " << strerror(errno) << std::endl;
//             close(clientSocket);
//             return;
//         }
//         logFileStream << "[LOG] - Matt_daemon: User input: " << input << std::endl;
//         logFileStream.close();

//         // Process client input here

//         // Example: Check if the input is "quit\n"
//         if (input == "quit\n") {
//             std::ofstream quitLogFileStream(logFile, std::ios_base::app);
//             if (quitLogFileStream) {
//                 quitLogFileStream << "[INFO] - Matt_daemon: Received quit command." << std::endl;
//                 quitLogFileStream << "[INFO] - Matt_daemon: Quitting." << std::endl;
//                 quitLogFileStream.close();
//             }
//             close(clientSocket);
//             deleteLockFileAndCloseSocket();  // Cleanup function
//             exit(EXIT_SUCCESS);
//         }
//     }
// }


void MattDaemon::deleteLockFileAndCloseSocket() {
    if (serverSocket != -1) {
        close(serverSocket);
    }
    if (remove(lockFile.c_str()) != 0) {
        std::cerr << "Failed to delete lock file: " << lockFile << std::endl;
        // Log the error to the daemon log
        std::ofstream logFileStream(logFile, std::ios_base::app);
        logFileStream << "[ERROR] - Matt_daemon: Failed to delete lock file." << std::endl;
        logFileStream.close();
        exit(EXIT_FAILURE);
    }
    std::cout << "[INFO] - Matt_daemon: Cleanup done." << std::endl;
}