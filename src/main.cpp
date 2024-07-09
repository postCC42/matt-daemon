#include "MattDaemon.hpp"
#include "TintinReporter.hpp"
#include "Utils.hpp"

Tintin_reporter* global_logger = nullptr;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (!Utils::checkRootUser()) {
        std::cerr << "Only root can run this program." << std::endl;
        return EXIT_FAILURE;
    }
    if (access(LOCKFILE_PATH, F_OK) == 0) {
        std::cerr << "Can't open " << LOCKFILE_PATH << ". Another instance of Matt_daemon is already running." << std::endl;
        exit(EXIT_FAILURE);
    }
    // todo handle remotion of filelock when SIGKILL
    // probably need to use a file locking mechanism 
    // that is impermeabile to unexpected terminations
    signal(SIGTERM, Utils::signalHandler);
    signal(SIGINT, Utils::signalHandler);
    signal(SIGQUIT, Utils::signalHandler);
    try {
        global_logger = new Tintin_reporter();

        MattDaemon daemon;
        daemon.run();

        delete global_logger;
        global_logger = nullptr;

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        if (global_logger) {
            delete global_logger;
            global_logger = nullptr;
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
