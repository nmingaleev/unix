#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signalfd.h>
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

    sigset_t mask;
    sigset_t oldMask;

    struct signalfd_siginfo fdsi;
    ssize_t s;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    sigprocmask(SIG_BLOCK, &mask, &oldMask);

    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        error("ERROR signalfd");
    }

    char buffer[256];

    while (true) {
        fd_set readFdSet;
        FD_ZERO(&readFdSet);
        FD_SET(fd, &readFdSet);
        FD_SET(sfd, &readFdSet);

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

        maxFdId = max(maxFdId, sfd);

        int result = pselect(maxFdId+1, &readFdSet, nullptr, nullptr, &timeout, &mask);
        if (result > 0) {
            if (FD_ISSET(sfd, &readFdSet)) {
                s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));

                if (fdsi.ssi_signo == SIGINT) {
                    printf("SIGINT");
                    continue;
                }
            }

        } else {
            error("pselect error");
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