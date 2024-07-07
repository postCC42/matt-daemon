#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"

Tintin_reporter* global_logger = nullptr;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

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
