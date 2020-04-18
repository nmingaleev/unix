#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <set>
#include <csignal>
#include <cstring>

using namespace std;

// Output server

volatile sig_atomic_t wasSigHup = 0;

void handleSignal(int signum) {
    wasSigHup = 1;
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int fd, resultLength;

    struct sockaddr_in serverAddress{};

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        error("ERROR opening socket");
    }

    fcntl(fd, F_SETFL, O_NONBLOCK);

    int port = 5000;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR binding socket");
    }

    listen(fd, 3);

    set<int> clientSocketSet;
    clientSocketSet.clear();

    struct sigaction act{};
    memset(&act, 0, sizeof(act));
    act.sa_handler = handleSignal;

    sigset_t mask;
    sigset_t oldMask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    act.sa_mask = mask;

    sigaction(SIGINT, &act, nullptr);

    sigprocmask(SIG_BLOCK, &mask, &oldMask);

    char buffer[256];

    while (true) {
        fd_set readFdSet;
        FD_ZERO(&readFdSet);
        FD_SET(fd, &readFdSet);

        for (set<int>::iterator it = clientSocketSet.begin(); it != clientSocketSet.end(); it++) {
            FD_SET(*it, &readFdSet);
        }

        timespec timeout{};
        timeout.tv_sec = 60;
        timeout.tv_nsec = 0;

        if (clientSocketSet.empty()) {
            printf("No connections, 60 seconds left to connect\n");
        }

        int maxFdId = max(fd, *max_element(clientSocketSet.begin(), clientSocketSet.end()));

        if (pselect(maxFdId+1, &readFdSet, nullptr, nullptr, &timeout, &oldMask) <= 0){
            if (errno == EINTR) {
                if (wasSigHup == 1) {
                    printf("signal caught");
                    wasSigHup = 0;
                }
            } else {
                error("pselect error");
            }
        }

        if (FD_ISSET(fd, &readFdSet)) {
            int acceptFdSocket = accept(fd, nullptr, nullptr);

            if (acceptFdSocket < 0) {
                perror("ERROR: accept connection");
                exit(3);
            }

            printf("New connection!\n");

            fcntl(acceptFdSocket, F_SETFL, O_NONBLOCK);

            if (!clientSocketSet.empty()) {
                close(acceptFdSocket);
                printf("Connection closed 1 client at a time\n");
            } else {
                clientSocketSet.insert(acceptFdSocket);
            }
        }

        for (set<int>::iterator it = clientSocketSet.begin(); it != clientSocketSet.end() && !clientSocketSet.empty(); it++) {
            if (FD_ISSET(*it, &readFdSet)) {
                resultLength = recv(*it, buffer, sizeof(buffer), 0);

                if (resultLength <= 0) {
                    close(*it);
                    clientSocketSet.erase(*it);
                    continue;
                }

                printf("Client says: %s\n", buffer);

                send(*it, buffer, resultLength, 0);
            }
        }
    }

    return 0;
}