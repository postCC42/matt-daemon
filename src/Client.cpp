#include <ncurses.h>
#include <iostream>
#include <string>
#include <memory>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <curses.h>

// Custom deleter for unique_ptr to handle ncurses windows
struct WinDeleter {
    void operator()(WINDOW* win) const {
        if (win != nullptr) {
            delwin(win);
        }
    }
};

int setupSocket() {
    int sockfd = -1;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    // Define server address
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(4242);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Connection to the server failed\n";
        return -1;
    }
    return sockfd;
}

int main() {
    int sockfd = setupSocket();
    if (sockfd < 0) {
        return 1;
    }

    // Initialize ncurses
    initscr();
    cbreak();
    echo();

    if (has_colors() == FALSE) {
        endwin();
        std::cout << "Your terminal does not support color\n";
        close(sockfd);
        return 1;
    }
    start_color();

    // Create windows for input and output using smart pointers
    int height = 3, width = COLS - 2, starty = LINES - height, startx = 1;
    std::unique_ptr<WINDOW, WinDeleter> inputwin(newwin(height, width, starty, startx));
    std::unique_ptr<WINDOW, WinDeleter> outputwin(newwin(LINES - height - 2, width, 1, startx));

    box(inputwin.get(), 0, 0);
    box(outputwin.get(), 0, 0);
    mvwprintw(inputwin.get(), 1, 1, "Enter command: ");

    refresh();
    wrefresh(inputwin.get());
    wrefresh(outputwin.get());

    std::string str;
    char buffer[800];

    fd_set read_fds;
    struct timeval tv;
    int max_fd = sockfd + 1;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 ms

        wgetnstr(inputwin.get(), buffer, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        str = buffer;

        if (str == "exit") {
            break;
        }

        // Send data to server
        if (str.length() > 0) {
            int bytes = send(sockfd, str.c_str(), str.size(), 0);
            if (bytes < 0) {
                std::cerr << "Error in sending data to server\n";
                break;
            }
            wprintw(outputwin.get(), "You entered: %s\n", str.c_str());
            wrefresh(inputwin.get());
            wrefresh(outputwin.get());
        }

        // Read from the server
        int selectResult = select(max_fd, &read_fds, NULL, NULL, &tv);
        if (selectResult > 0) {
            if (FD_ISSET(sockfd, &read_fds)) {
                char recvBuffer[1024];
                ssize_t bytesRead = recv(sockfd, recvBuffer, sizeof(recvBuffer) - 1, 0);
                if (bytesRead > 0) {
                    recvBuffer[bytesRead] = '\0';
                    wprintw(outputwin.get(), "Server: %s\n", recvBuffer);
                    wrefresh(outputwin.get());
                } else if (bytesRead == 0) {
                    std::cerr << "Server closed the connection\n";
                    break;
                } else {
                    std::cerr << "Error in receiving data from server\n";
                    break;
                }
            }
        } else if (selectResult < 0) {
            std::cerr << "Error in select()\n";
            break;
        }

        werase(inputwin.get());
        box(inputwin.get(), 0, 0);
        mvwprintw(inputwin.get(), 1, 1, "Enter command: ");
        wrefresh(inputwin.get());
    }
    if (sockfd >= 0) {
        close(sockfd);
    }

    endwin();
}