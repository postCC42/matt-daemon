#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"
#include "Utils.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (!Utils::checkRootUser()) {
        std::cerr << "Only root can run this program." << std::endl;
        return EXIT_FAILURE;
    }

    if (access(LOCKFILE_PATH, F_OK) == 0) {
        std::cerr << "Can't open " << LOCKFILE_PATH << ". Another instance of Matt_daemon is already running." << std::endl;
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Error file locked.");
        Tintin_reporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Quitting.");
        return EXIT_FAILURE;
    }

    signal(SIGTERM, Utils::signalHandler);
    signal(SIGINT, Utils::signalHandler);
    signal(SIGQUIT, Utils::signalHandler);
    signal(SIGHUP, Utils::signalHandler);

    try {
        MattDaemon daemon;
        daemon.run();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
