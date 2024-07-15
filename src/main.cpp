#include "MattDaemon.hpp"
#include "TintinReporter.hpp"
#include "Utils.hpp"
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (!Utils::checkRootUser()) {
        std::cerr << "Only root can run this program." << std::endl;
        return EXIT_FAILURE;
    }

    if (access(LOCKFILE_PATH, F_OK) == 0) {
        std::cerr << "Can't open " << LOCKFILE_PATH << ". Another instance of Matt_daemon is already running." << std::endl;
        TintinReporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Error file locked.");
        TintinReporter::getInstance().log(LOGLEVEL_ERROR, "Matt_daemon: Quitting.");
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
