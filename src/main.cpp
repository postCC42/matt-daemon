#include "MattDaemon.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    try {
        MattDaemon daemon;
        daemon.run();

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
